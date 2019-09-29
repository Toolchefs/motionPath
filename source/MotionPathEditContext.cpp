
#include "MotionPathEditContext.h"
#include "GlobalSettings.h"
#include "ContextUtils.h"

extern MotionPathManager mpManager;

MotionPathEditContextCmd::MotionPathEditContextCmd()
{}

MPxContext* MotionPathEditContextCmd::makeObj()
{
	return new MotionPathEditContext();
}

void* MotionPathEditContextCmd::creator()
{
	return new MotionPathEditContextCmd;
}

MotionPathEditContext::MotionPathEditContext():
	MPxContext(),
	ctxMenuWidget(nullptr)
{
	this->selectedMotionPathPtr = NULL;
    this->currentMode = kNoneMode;
    
    setTitleString ("MotionPath Edit");
}

MotionPathEditContext::~MotionPathEditContext()
{
	if (ctxMenuWidget)
	{
		delete ctxMenuWidget;
	}
}

void MotionPathEditContext::toolOffCleanup()
{
	M3dView view = M3dView::active3dView();
	view.refresh(true, true);

	if (ctxMenuWidget)
	{
		delete ctxMenuWidget;
		ctxMenuWidget = nullptr;
	}

    for (int i = 0; i < mpManager.getMotionPathsCount(); ++i)
    {
        MotionPath *motionPath = mpManager.getMotionPathPtr(i);
        if (motionPath)
            motionPath->deselectAllKeys();
    }
    
    M3dView::active3dView().refresh(true, true);
    
    MPxContext::toolOffCleanup();
}

void MotionPathEditContext::toolOnSetup( MEvent& event )
{
    this->selectedMotionPathPtr = NULL;
    this->currentMode = kNoneMode;
    this->alongPreferredAxis = false;
    this->prefEditAxis = -1;
    this->startedRecording = false;
    
	// set the help text in the maya help boxs
	setHelpString("Left-Click: Select/Move; Shift+Left-Click: Add to selection; CTRL+Left-Click: Toggle selection; CTRL+Left-Click-Drag: Move Selection on the XY plane; CTRL+Middle-Click-Drag: Move Along Y Axis; Right-Click on path/frame/key: show menu");
    
    M3dView view = M3dView::active3dView();
	view.refresh(true, true);

	if (!ctxMenuWidget)
		ctxMenuWidget = new ContextMenuWidget(view.widget());

	MPxContext::toolOnSetup(event);
	if (view.widget())
		view.widget()->setFocus();
}

void MotionPathEditContext::modifySelection(const MDoubleArray &selectedTimes, const bool ctrl, const bool shift)
{
    mpManager.storePreviousKeySelection();
    for (int i = 0; i < selectedTimes.length(); ++i)
    {
        bool thisShift = i != 0 || shift;
        
        if (ctrl)
        {
            if (selectedMotionPathPtr->isKeyAtTimeSelected(selectedTimes[i]))
                selectedMotionPathPtr->deselectKeyAtTime(selectedTimes[i]);
            else
                selectedMotionPathPtr->selectKeyAtTime(selectedTimes[i]);
        }
        else if (thisShift)
        {
            selectedMotionPathPtr->selectKeyAtTime(selectedTimes[i]);
        }
        else
        {
            if (selectedMotionPathPtr->isKeyAtTimeSelected(selectedTimes[i]))
                return;
            
            for (int j=0; j < mpManager.getMotionPathsCount(); ++j)
                mpManager.getMotionPathPtr(j)->deselectAllKeys();
            selectedMotionPathPtr->selectKeyAtTime(selectedTimes[i]);
        }
    }
    MGlobal::executeCommand("tcMotionPathCmd -keySelectionChanged", true, true);
}


bool MotionPathEditContext::doPressCommon(MEvent &event, const bool old)
{
    selectedMotionPathPtr = NULL;
    startedRecording = false;
    
    event.getPosition(initialX, initialY);
    activeView = M3dView::active3dView();
    
    if (!GlobalSettings::showKeyFrames)
        return false;
    
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
            MDagPath camera;
            activeView.getCamera(camera);
			MMatrix cameraMatrix = camera.inclusiveMatrix();
            cameraPosition.x = cameraMatrix(3, 0);
            cameraPosition.y = cameraMatrix(3, 1),
            cameraPosition.z = cameraMatrix(3, 2);
            
            inverseCameraMatrix = cameraMatrix.inverse();
            
            selectedMotionPathPtr->setSelectedFromTool(true);
            
            MIntArray selectedKeys;

			if (!old)
				contextUtils::processKeyFrameHits(initialX, initialY, selectedMotionPathPtr, activeView, GlobalSettings::cameraMatrix, cachePtr, selectedKeys);
			else
				contextUtils::processKeyFrameHits(selectedMotionPathPtr, activeView, cachePtr, selectedKeys);

            if (selectedKeys.length() == 0)
            {
                int selectedTangent = -1;
                if (GlobalSettings::showTangents)
                {
                    int selectedKeyId;

					if (!old)
						contextUtils::processTangentHits(initialX, initialY, selectedMotionPathPtr, activeView, GlobalSettings::cameraMatrix, cachePtr, selectedKeyId, selectedTangent);
					else
						contextUtils::processTangentHits(selectedMotionPathPtr, activeView, cachePtr, selectedKeyId, selectedTangent);

                    //move tangent
                    if (selectedTangent != -1)
                    {                        
                        currentMode = kTangentEditMode;
                        selectedTangentId = selectedTangent;
                        tangentWorldPosition = MVector::zero;
                        
                        lastSelectedTime = selectedMotionPathPtr->getTimeFromKeyId(selectedKeyId);
                        
                        selectedMotionPathPtr->getTangentHandleWorldPosition(lastSelectedTime, (Keyframe::Tangent)selectedTangentId, tangentWorldPosition);
                        lastWorldPosition = tangentWorldPosition;
                        
                        selectedMotionPathPtr->getKeyWorldPosition(lastSelectedTime, keyWorldPosition);
                    }
                }
            }
            else
            {
                if(event.mouseButton() == MEvent::kMiddleMouse)
                {
                    //move along the major axis
                    alongPreferredAxis = true;
                    currentMode = kFrameEditMode;
                    prefEditAxis = event.isModifierControl() ? 1: 0;
                }
                else
                    currentMode = kFrameEditMode;
                
                keyWorldPosition = MVector::zero;
                
                MDoubleArray times;
                for (int i=0; i < selectedKeys.length(); ++i)
                {
                    lastSelectedTime = selectedMotionPathPtr->getTimeFromKeyId(selectedKeys[i]);
                    times.append(lastSelectedTime);
                }
                
                modifySelection(times, event.isModifierControl(), event.isModifierShift());
                
                selectedMotionPathPtr->getKeyWorldPosition(lastSelectedTime, keyWorldPosition);
                lastWorldPosition = keyWorldPosition;
            }
        }
    }
    else
    {
        contextUtils::refreshSelectionMethod(event, listAdjustment);
        
        if (old)
            fsDrawn = false;
    }
    
    return true;
}

MStatus MotionPathEditContext::doPress(MEvent &event)
{
    return doPressCommon(event, true)? MStatus::kSuccess: MStatus::kFailure;
}

MStatus MotionPathEditContext::doPress(MEvent & event, MHWRender::MUIDrawManager& drawMgr, const MHWRender::MFrameContext& context)
{
    return doPressCommon(event, false) ? MStatus::kSuccess: MStatus::kFailure;
}

void MotionPathEditContext::doDragCommon(MEvent &event, const bool old)
{
    if (!startedRecording && (currentMode == kFrameEditMode || currentMode == kTangentEditMode || currentMode == kShiftKeyMode))
    {
        mpManager.startAnimUndoRecording();
        startedRecording = true;
    }
    
    short int thisX, thisY;
    event.getPosition(thisX, thisY);
    M3dView view = M3dView::active3dView();
    
    if (currentMode == kFrameEditMode)
    {
        
        MVector newPosition = contextUtils::getWorldPositionFromProjPoint(keyWorldPosition, initialX, initialY, thisX, thisY, view, cameraPosition);
         
        //if control is pressed with find the axis with the maximum value and we move the selected keyframes only on that axis
        if (alongPreferredAxis)
        {
            if (prefEditAxis == 0)
                newPosition[1] = keyWorldPosition[1];
            else
            {
                newPosition[0] = keyWorldPosition[0];
                newPosition[2] = keyWorldPosition[2];
            }
        }
        
        MVector offset = newPosition - lastWorldPosition;
        CameraCache * cachePtr = mpManager.MotionPathManager::getCameraCachePtrFromView(activeView);
        if (GlobalSettings::motionPathDrawMode == GlobalSettings::kCameraSpace)
        {
            if (!cachePtr)
                return;
            offset = offset * inverseCameraMatrix;
        }
    
        for (int i = 0; i < mpManager.getMotionPathsCount(); i++)
        {
            MotionPath *motionPath = mpManager.getMotionPathPtr(i);
            if (motionPath)
            {
                MDoubleArray selectedTimes = motionPath->getSelectedKeys();
                for (int j = 0; j < selectedTimes.length(); j++)
                    motionPath->offsetWorldPosition(GlobalSettings::motionPathDrawMode == GlobalSettings::kWorldSpace ? offset : offset * cachePtr->matrixCache[selectedTimes[j]].inverse(), selectedTimes[j], mpManager.getAnimCurveChangePtr());
            }
        }
        
        lastWorldPosition = newPosition;
    }
    else if (currentMode == kTangentEditMode)
    {
        MVector newPosition = contextUtils::getWorldPositionFromProjPoint(tangentWorldPosition, initialX, initialY, thisX, thisY, view, cameraPosition);
        
        MMatrix toWorldMatrix;
        if (GlobalSettings::motionPathDrawMode == GlobalSettings::kCameraSpace)
        {
            CameraCache * cachePtr = mpManager.MotionPathManager::getCameraCachePtrFromView(activeView);
            if (!cachePtr)
                return;
            //newPosition = MVector(MPoint(newPosition) * inverseCameraMatrix * cachePtr->matrixCache[lastSelectedTime].inverse());
            toWorldMatrix = inverseCameraMatrix * cachePtr->matrixCache[lastSelectedTime].inverse();
        }
        else
            toWorldMatrix.setToIdentity();
        
        selectedMotionPathPtr->setTangentWorldPosition(newPosition, lastSelectedTime, (Keyframe::Tangent)selectedTangentId, toWorldMatrix, mpManager.getAnimCurveChangePtr());
        
    }
    
    view.refresh(true, true);
}

MStatus MotionPathEditContext::doDrag(MEvent &event)
{
    if (selectedMotionPathPtr)
    {
        doDragCommon(event, true);
    }
    else
    {
        activeView.beginXorDrawing();
        // Redraw the marquee at its old position to erase it.
        if (fsDrawn)
            contextUtils::drawMarqueeGL(initialX, initialY, finalX, finalY);

        fsDrawn = true;

        event.getPosition( finalX, finalY );
        // Draw the marquee at its new position.
        contextUtils::drawMarqueeGL(initialX, initialY, finalX, finalY);

        activeView.endXorDrawing();
    }
    
    return MStatus::kSuccess;
}

MStatus MotionPathEditContext::doDrag(MEvent & event, MHWRender::MUIDrawManager& drawMgr, const MHWRender::MFrameContext& context)
{
    if (selectedMotionPathPtr)
    {
        doDragCommon(event, true);
    }
    else
    {
        //  Get the marquee's new end position.
        event.getPosition( finalX, finalY );
        // Draw the marquee at its new position.
        contextUtils::drawMarquee(drawMgr, initialX, initialY, finalX, finalY);
    }
    return MS::kSuccess;
}

void MotionPathEditContext::doReleaseCommon(MEvent &event, const bool old)
{
    if (selectedMotionPathPtr)
    {
        if(startedRecording && (currentMode == kFrameEditMode || currentMode == kTangentEditMode || currentMode == kShiftKeyMode))
            mpManager.stopDGAndAnimUndoRecording();
        
        selectedMotionPathPtr->setSelectedFromTool(false);
        selectedMotionPathPtr = NULL;
        currentMode = kNoneMode;
        
        alongPreferredAxis = false;
        prefEditAxis = -1;
        
        M3dView view = M3dView::active3dView();
		view.refresh(true, true);
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
}

MStatus MotionPathEditContext::doRelease(MEvent &event)
{
    doReleaseCommon(event, true);
    return MStatus::kSuccess;
}

MStatus MotionPathEditContext::doRelease(MEvent & event, MHWRender::MUIDrawManager& drawMgr, const MHWRender::MFrameContext& context)
{
    doReleaseCommon(event, false);
    return MS::kSuccess;
}


