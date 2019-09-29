//
//  MotionPathDrawContext.cpp
//  MotionPath
//
//  Created by Daniele Federico on 30/11/14.
//
//

#include "MotionPathDrawContext.h"

#include "GlobalSettings.h"
#include "ContextUtils.h"

extern MotionPathManager mpManager;

MPxContext* MotionPathDrawContextCmd::makeObj()
{
	return new MotionPathDrawContext();
}

void* MotionPathDrawContextCmd::creator()
{
	return new MotionPathDrawContextCmd;
}

MotionPathDrawContext::MotionPathDrawContext()
{
}

void MotionPathDrawContext::toolOnSetup( MEvent& event )
{
    this->selectedMotionPathPtr = NULL;
    this->currentMode = kNoneMode;
    this->selectedKeyId = -1;
    
	// set the help text in the maya help boxs
	setHelpString("Left-Click key frame then drag to draw path; CTRL-Left-Click key frame then drag to draw proximity stroke; Middle-Click in the viewport to add a keyframe at the current time.");
}

void MotionPathDrawContext::toolOffCleanup()
{
    if (selectedMotionPathPtr)
    {
        M3dView view = M3dView::active3dView();
		view.refresh();
    }
}

void MotionPathDrawContext::drawStroke()
{
    if (strokePoints.length() < 2)
        return;
    
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i < strokePoints.length(); ++i)
        glVertex2f(strokePoints[i].x, strokePoints[i].y);
    glEnd();
}

void MotionPathDrawContext::drawStrokeNew(MHWRender::MUIDrawManager& drawMgr)
{
    if (strokePoints.length() < 2)
        return;
    
    drawMgr.beginDrawable();
    drawMgr.setLineWidth(2.0f);
    drawMgr.setColor(MColor(1.0, 1.0, 1.0));
    for (int i = 1; i < strokePoints.length(); ++i)
        drawMgr.line2d(strokePoints[i-1], strokePoints[i]);
    
    drawMgr.endDrawable();
    
}

bool MotionPathDrawContext::doPressCommon(MEvent &event, const bool old)
{
    event.getPosition(initialX, initialY);
    activeView = M3dView::active3dView();
    
    if (!GlobalSettings::showKeyFrames)
        return false;
    
    MDagPath camera;
    activeView.getCamera(camera);
    MMatrix cameraMatrix = camera.inclusiveMatrix();
    cameraPosition.x = cameraMatrix(3, 0);
    cameraPosition.y = cameraMatrix(3, 1),
    cameraPosition.z = cameraMatrix(3, 2);
    
    inverseCameraMatrix = cameraMatrix.inverse();
    
    if (event.mouseButton() == MEvent::kMiddleMouse)
    {
        selectedMotionPathPtr = mpManager.getMotionPathPtr(0);
        if (selectedMotionPathPtr)
        {
            selectedMotionPathPtr->setSelectedFromTool(true);
            
            currentMode = kClickAddWorld;
            selectedTime = MAnimControl::currentTime().as(MTime::uiUnit());
            
            //get closest frame to the currentTime
            double minTimeBoundary = 999999999;
            double maxTimeBoundary = 999999999;
            
            selectedMotionPathPtr->getBoundariesForTime(selectedTime, &minTimeBoundary, &maxTimeBoundary);
            double keyTime;
            if (minTimeBoundary != 999999999)
                keyTime = minTimeBoundary;
            else if (maxTimeBoundary != 999999999)
                keyTime = maxTimeBoundary;
            else
                keyTime = selectedTime;
            
            keyWorldPosition = selectedMotionPathPtr->getWorldPositionAtTime(keyTime);
            activeView.worldToView(keyWorldPosition, initialX, initialY);
            
            short int thisX, thisY;
            event.getPosition(thisX, thisY);
            
            MVector newPosition = contextUtils::getWorldPositionFromProjPoint(keyWorldPosition, initialX, initialY, thisX, thisY, activeView, cameraPosition);
            
            if (GlobalSettings::motionPathDrawMode == GlobalSettings::kCameraSpace)
            {
                if (!contextUtils::worldCameraSpaceToWorldSpace(newPosition, activeView, selectedTime, inverseCameraMatrix, mpManager))
                    return false;
            }
            
            mpManager.startAnimUndoRecording();
            
            selectedMotionPathPtr->addKeyFrameAtTime(selectedTime, mpManager.getAnimCurveChangePtr(), &newPosition);
            
            activeView.refresh();
            
            return true;
        }
    }

    
	CameraCache * cachePtr = mpManager.MotionPathManager::getCameraCachePtrFromView(activeView);
	int selectedCurveId;
	if (!old)
		selectedCurveId = contextUtils::processCurveHits(initialX, initialY, GlobalSettings::cameraMatrix, activeView, cachePtr, mpManager);
	else
		selectedCurveId = contextUtils::processCurveHits(activeView, cachePtr, mpManager);

    if (selectedCurveId != -1)
    {
        selectedMotionPathPtr = mpManager.getMotionPathPtr(selectedCurveId);
        if (selectedMotionPathPtr)
        {
            selectedMotionPathPtr->setSelectedFromTool(true);
                
            MIntArray ids;
			if (!old)
				contextUtils::processKeyFrameHits(initialX, initialY, selectedMotionPathPtr, activeView, GlobalSettings::cameraMatrix, cachePtr, ids);
			else
				contextUtils::processKeyFrameHits(selectedMotionPathPtr, activeView, cachePtr, ids);
            if (ids.length() > 0)
            {
                selectedKeyId = ids[ids.length() - 1];
                    
                keyWorldPosition = MVector::zero;
                selectedTime = selectedMotionPathPtr->getTimeFromKeyId(selectedKeyId);
                selectedMotionPathPtr->getKeyWorldPosition(selectedTime, keyWorldPosition);
                selectedMotionPathPtr->selectKeyAtTime(selectedTime);
                    
                mpManager.startAnimUndoRecording();
                    
                if (event.isModifierControl())
                {
                    currentMode = kStroke;
                        
                    strokePoints.clear();
                    strokePoints.append(MVector(initialX, initialY, 0));
                }
                else
                {
                    currentMode = kDraw;

                    selectedMotionPathPtr->setIsDrawing(true);
                    selectedMotionPathPtr->setEndrawingTime(selectedTime);
                        
                    maxTime = MAnimControl::maxTime().as(MTime::uiUnit());
                       
                    steppedTime = selectedTime;
                        
                    selectedMotionPathPtr->deleteAllKeyFramesAfterTime(selectedTime, mpManager.getAnimCurveChangePtr());
                        
                    ////USA UN MPOINT!!! NON UN MVECTOR!
                        
                    MVector position = keyWorldPosition;
                    if (GlobalSettings::motionPathDrawMode == GlobalSettings::kCameraSpace)
                    {
                        MPoint worldPos = position;
                        if (!contextUtils::worldCameraSpaceToWorldSpace(worldPos, activeView, selectedTime, inverseCameraMatrix, mpManager))
                            return false;
                        position = worldPos;
                    }
                        
                    selectedMotionPathPtr->addKeyFrameAtTime(selectedTime, mpManager.getAnimCurveChangePtr(), &position);

                    initialClock = clock();
                }
                    
                activeView.refresh();
                    
                return true;
            }
        }
    }
    else
    {
        contextUtils::refreshSelectionMethod(event, listAdjustment);
        
        if (old)
            fsDrawn = false;
    }
    
    return false;
}

MStatus MotionPathDrawContext::doPress(MEvent &event)
{
    return doPressCommon(event, true) ? MStatus::kSuccess: MStatus::kFailure;
}

MStatus MotionPathDrawContext::doPress(MEvent & event, MHWRender::MUIDrawManager& drawMgr, const MHWRender::MFrameContext& context)
{
    return doPressCommon(event, false) ? MStatus::kSuccess: MStatus::kFailure;
}

bool MotionPathDrawContext::doDragCommon(MEvent &event, const bool old)
{
    if (selectedMotionPathPtr)
    {
        short int thisX, thisY;
		event.getPosition(thisX, thisY);
        
        if (currentMode == kClickAddWorld)
        {
            MVector newPosition = contextUtils::getWorldPositionFromProjPoint(keyWorldPosition, initialX, initialY, thisX, thisY, activeView, cameraPosition);
            if (GlobalSettings::motionPathDrawMode == GlobalSettings::kCameraSpace)
            {
                if (!contextUtils::worldCameraSpaceToWorldSpace(newPosition, activeView, selectedTime, inverseCameraMatrix, mpManager))
                    return false;
            }
            
			selectedMotionPathPtr->setFrameWorldPosition(newPosition, selectedTime, mpManager.getAnimCurveChangePtr());
        }
        else if (currentMode == kDraw)
        {
            clock_t thisClock = clock();
            
            double diff = thisClock - initialClock;
            if (diff/CLOCKS_PER_SEC > GlobalSettings::drawTimeInterval)
            {
                steppedTime += GlobalSettings::drawFrameInterval;
                if (steppedTime > maxTime)
                    MGlobal::displayWarning("MotionPathDrawContext: Drawing outside of timeline frame range, not showing path, just key frames.");
                
                MVector newPosition = contextUtils::getWorldPositionFromProjPoint(keyWorldPosition, initialX, initialY, thisX, thisY, activeView, cameraPosition);
                if (GlobalSettings::motionPathDrawMode == GlobalSettings::kCameraSpace)
                {
                    MPoint worldPos = newPosition;
                    if (!contextUtils::worldCameraSpaceToWorldSpace(worldPos, activeView, steppedTime, inverseCameraMatrix, mpManager))
                        return false;
                    newPosition = worldPos;
                }
                
                selectedMotionPathPtr->addKeyFrameAtTime(steppedTime, mpManager.getAnimCurveChangePtr(), &newPosition);
                
                selectedMotionPathPtr->setEndrawingTime(steppedTime);

                initialClock = thisClock;
            }
        }
        
        activeView.refresh(false, true);
    }
    else
        return false;
    
    return true;
}

MStatus MotionPathDrawContext::doDrag(MEvent &event)
{
    event.getPosition( finalX, finalY );
    
    if (selectedMotionPathPtr)
    {
        if (currentMode == kStroke)
        {
            activeView.beginXorDrawing(true, true, 2.0f, M3dView::kStippleNone);
            
            drawStroke();
            
            MVector v(finalX, finalY, 0);
            if ((v - strokePoints[strokePoints.length()-1]).length() > 20)
                strokePoints.append(v);
            
            drawStroke();
            
            activeView.endXorDrawing();
            
            return MStatus::kSuccess;
        }
        else
            return doDragCommon(event, true) ? MStatus::kSuccess: MStatus::kFailure;
    }
    else
    {
        activeView.beginXorDrawing();
        // Redraw the marquee at its old position to erase it.
        if (fsDrawn)
            contextUtils::drawMarqueeGL(initialX, initialY, finalX, finalY);
        
        fsDrawn = true;
        
        // Draw the marquee at its new position.
        contextUtils::drawMarqueeGL(initialX, initialY, finalX, finalY);
        
        activeView.endXorDrawing();
    }
    return MStatus::kSuccess;
}

MStatus MotionPathDrawContext::doDrag(MEvent & event, MHWRender::MUIDrawManager& drawMgr, const MHWRender::MFrameContext& context)
{
    if (selectedMotionPathPtr)
    {
        if (currentMode == kStroke)
        {
            short int thisX, thisY;
            event.getPosition(thisX, thisY);
            
            MVector v(thisX, thisY, 0);
            if ((v - strokePoints[strokePoints.length()-1]).length() > 20)
                strokePoints.append(v);

            drawStrokeNew(drawMgr);
            
            return MStatus::kSuccess;
        }
        else
            return doDragCommon(event, false) ? MStatus::kSuccess: MStatus::kFailure;
    }
    else
    {
        //  Get the marquee's new end position.
        event.getPosition( finalX, finalY );
        // Draw the marquee at its new position.
        contextUtils::drawMarquee(drawMgr, initialX, initialY, finalX, finalY);
        
    }
    return MStatus::kSuccess;
}

MVector MotionPathDrawContext::getkeyScreenPosition(const double time)
{
    MVector pos;
    selectedMotionPathPtr->getKeyWorldPosition(time, pos);
    short thisX, thisY;
    activeView.worldToView(pos, thisX, thisY);
    return MVector(thisX, thisY, 0);
}

int MotionPathDrawContext::getStrokeDirection(MVector directionalVector, const MDoubleArray &keys, const int selectedIndex)
{
    MVector pos = getkeyScreenPosition(keys[selectedIndex]);
    MVector pp = selectedIndex == 0 ? MVector(0,0,0): getkeyScreenPosition(keys[selectedIndex-1]) - pos;
    MVector ap = selectedIndex == keys.length() - 1 ? MVector(0,0,0): getkeyScreenPosition(keys[selectedIndex+1]) - pos;
    pp.normalize();
    ap.normalize();
    
    double dot1 = pp * directionalVector;
    double dot2 = ap * directionalVector;
    
    if (dot1 == 0 && dot2 < 0)
        return 0;
    
    if (dot2 == 0 && dot1 < 0)
        return 0;
    
    return dot1 > dot2 ? -1: 1;
}

MVector MotionPathDrawContext::getClosestPointOnPolyLine(const MVector &q)
{
    const int count = strokePoints.length();
    double finalT = 0;
    int index = 0;

    MVector b = strokePoints[0], dbq = b - q;
    double dist = (dbq.x*dbq.x + dbq.y*dbq.y);
    for (size_t i = 1; i < count; ++i)
    {
        const MVector a = b, daq = dbq;

        b = strokePoints[i];
        dbq = b - q;
        
        const MVector dab = a - b;
        const double inv_sqrlen = 1./(dab.x*dab.x + dab.y*dab.y),
                              t = (dab.x*daq.x + dab.y*daq.y) * inv_sqrlen;
        if (t<0.)
            continue;
        double current_dist;
        if (t<=1.)
            current_dist = (dab.x*dbq.y - dab.y*dbq.x) * (dab.x*dbq.y - dab.y*dbq.x) * inv_sqrlen;
        else//t>1.
            current_dist = (dbq.x*dbq.x + dbq.y*dbq.y);
        
        if (current_dist<dist)
        {
            dist = current_dist;
            finalT = t > 1 ? 1: t;
            index = i;
        }
    }
    
    if (finalT == 0 && index == 0)
        return strokePoints[0];
    else
        return strokePoints[index] * finalT + strokePoints[index-1] * (1-finalT);
}

MVector MotionPathDrawContext::getSpreadPointOnPolyLine(const int i, const int pointSize, const double strokeLenght, const std::vector<double> &segmentLenghts)
{
    if (i == pointSize-1)
        return strokePoints[strokePoints.length() - 1];
    else
    {
        //we do +1 as the first key is not evaluated, and with pointSize there no -1
        float tl = (((float)i+1) / ((float)pointSize)) * strokeLenght;
        
        int currentSegmentIndex = 0;
        double currentSegmentLenght = 0;
        for (int j = 0; j < strokePoints.length() - 1; ++j)
        {
            if (tl > currentSegmentLenght && tl < currentSegmentLenght + segmentLenghts[j])
            {
                currentSegmentIndex = j;
                break;
            }
            else
                currentSegmentLenght += segmentLenghts[j];
        }

        float t = (tl - currentSegmentLenght) / (segmentLenghts[currentSegmentIndex]);
        return strokePoints[currentSegmentIndex+1] * t + strokePoints[currentSegmentIndex] * (1 - t);
    }

}

bool MotionPathDrawContext::doReleaseCommon(MEvent &event, const bool old)
{
    if (selectedMotionPathPtr)
    {
        if (currentMode == kStroke)
        {
            int strokeNum = strokePoints.length() - 1;
            if (strokeNum > 1)
            {
                //get the direction vector
                MVector directionalVector;
                for (int i = 1; i < strokePoints.length(); ++i)
                {
                    MVector pos = strokePoints[i];
                    directionalVector += strokePoints[i] - strokePoints[0];
                }
                directionalVector /= strokeNum;
                directionalVector.normalize();
                
                //get the key frame going into that direction, just the two left and right of this keyframe
                MDoubleArray keys = selectedMotionPathPtr->getKeys();
                
                int selectedIndex = 0;
                for (; selectedIndex < keys.length(); ++selectedIndex)
                    if (keys[selectedIndex] == selectedTime)
                        break;
                
                int direction = getStrokeDirection(directionalVector, keys, selectedIndex);
                if (direction != 0)
                {
                    //  go back or forward in time until the distance of the points is not greatest from the previous distance
                    
                    std::vector<StrokeCache> cache, tempCache;
                    
                    int MAX_SKIPPED = 5, skipped = 0;
                    
                    MVector lastStrokePos = strokePoints[strokeNum];
                    double distance = (lastStrokePos - getkeyScreenPosition(keys[selectedIndex])).length();
                    for (int i = selectedIndex + direction; true; i += direction)
                    {
                        MVector pos = getkeyScreenPosition(keys[i]);
                        double thisDistance = (lastStrokePos - pos).length();
                        
                        if (thisDistance > distance)
                        {
                            skipped++;
                            if (skipped > MAX_SKIPPED)
                                break;
                            
                            if (i == 0 || i == keys.length() - 1)
                                break;
                            
                            StrokeCache c;
                            c.originalScreenPosition = pos;
                            c.time = keys[i];
                            selectedMotionPathPtr->getKeyWorldPosition(keys[i], pos);
                            c.originalWorldPosition = pos;
                            tempCache.push_back(c);
                            continue;
                        }
                        
                        skipped = 0;
                        if (tempCache.size() > 0)
                        {
                            cache.insert(cache.end(), tempCache.begin(), tempCache.end());
                            tempCache.clear();
                        }
                        
                        distance = thisDistance;
                        
                        StrokeCache c;
                        c.originalScreenPosition = pos;
                        c.time = keys[i];
                        selectedMotionPathPtr->getKeyWorldPosition(keys[i], pos);
                        c.originalWorldPosition = pos;
                        cache.push_back(c);
                        
                        if (i == 0 || i == keys.length() - 1)
                            break;
                    }
                    
                    int pointSize = cache.size();
                    if (pointSize > 0)
                    {
                        //we delete the key frames so maya will recalculate the tangents when adding the keyframes back
                        for (int i = cache.size() - 1; i > -1 ; --i)
                            selectedMotionPathPtr->deleteKeyFrameAtTime(cache[i].time, mpManager.getAnimCurveChangePtr(), false);
                        
                        //get the stroke lenght
                        double strokeLenght = 0;
                        std::vector<double> segmentLenghts;
                        segmentLenghts.resize(strokeNum);
                        for (int i = 1; i < strokeNum; ++i)
                        {
                            segmentLenghts[i-1] = (strokePoints[i] - strokePoints[i - 1]).length();
                            strokeLenght += segmentLenghts[i-1];
                        }
                        
                        // match each key using the right mode (closest or spread)
                        for (int i = 0; i < pointSize ; ++i)
                        {
                            if (GlobalSettings::strokeMode == 0) //closest
                                cache[i].screenPosition = getClosestPointOnPolyLine(cache[i].originalScreenPosition);
                            else // spread
                                cache[i].screenPosition = getSpreadPointOnPolyLine(i, pointSize, strokeLenght, segmentLenghts);

                            MVector newPosition = contextUtils::getWorldPositionFromProjPoint(cache[i].originalWorldPosition, cache[i].originalScreenPosition.x, cache[i].originalScreenPosition.y, cache[i].screenPosition.x, cache[i].screenPosition.y, activeView, cameraPosition);
                            
                            if (GlobalSettings::motionPathDrawMode == GlobalSettings::kCameraSpace)
                            {
                                MPoint worldPos = newPosition;
                                if (!contextUtils::worldCameraSpaceToWorldSpace(worldPos, activeView, cache[i].time, inverseCameraMatrix, mpManager))
                                    return false;
                                newPosition = worldPos;
                            }
                            
                            selectedMotionPathPtr->addKeyFrameAtTime(cache[i].time, mpManager.getAnimCurveChangePtr(), &newPosition, false);
                        }
                    }
                }
            }
        }
        
        if (currentMode != kNoneMode)
            mpManager.stopDGAndAnimUndoRecording();
        
        selectedMotionPathPtr->deselectAllKeys();
        selectedMotionPathPtr->setSelectedFromTool(false);
        selectedMotionPathPtr->setIsDrawing(false);
        
		activeView.refresh();
        
        selectedMotionPathPtr = NULL;
        currentMode = kNoneMode;
    }
    else
    {
        event.getPosition( finalX, finalY );
        
        if (fsDrawn && old)
        {
            activeView.beginXorDrawing();
            contextUtils::drawMarqueeGL(initialX, initialY, finalX, finalY);
            activeView.endXorDrawing();
        }
        
        contextUtils::applySelection(initialX, initialY, finalX, finalY, listAdjustment);
    }
    
    return 1;
}

MStatus MotionPathDrawContext::doRelease(MEvent &event)
{
    return doReleaseCommon(event, true) ? MStatus::kSuccess: MStatus::kFailure;
}

MStatus MotionPathDrawContext::doRelease(MEvent & event, MHWRender::MUIDrawManager& drawMgr, const MHWRender::MFrameContext& context)
{
    return doReleaseCommon(event, false) ? MStatus::kSuccess: MStatus::kFailure;
}
