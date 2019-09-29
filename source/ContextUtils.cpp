//
//  ContextUtils.cpp
//  MotionPath
//
//  Created by Daniele Federico on 29/11/14.
//
//

#include "ContextUtils.h"
#include "Keyframe.h"

#include <maya/MPoint.h>
#include <maya/MIntArray.h>

bool contextUtils::worldCameraSpaceToWorldSpace(MVector &position, M3dView &view, const double time, const MMatrix &inverseCameraMatrix, MotionPathManager &mpManager)
{
    CameraCache * cachePtr = mpManager.getCameraCachePtrFromView(view);
    if (!cachePtr)
        return false;
    cachePtr->ensureMatricesAtTime(time);
    position = position * inverseCameraMatrix * cachePtr->matrixCache[time].inverse();
    return true;
}

bool contextUtils::worldCameraSpaceToWorldSpace(MPoint &position, M3dView &view, const double time, const MMatrix &inverseCameraMatrix, MotionPathManager &mpManager)
{
    CameraCache * cachePtr = mpManager.getCameraCachePtrFromView(view);
    if (!cachePtr)
        return false;
    cachePtr->ensureMatricesAtTime(time);
    position = position * inverseCameraMatrix * cachePtr->matrixCache[time].inverse();
    return true;
}

MVector contextUtils::getWorldPositionFromProjPoint(const MVector &pointToMove, const double initialX, const double initialY, const double currentX, const double currentY, const M3dView &view, const MVector &cameraPosition)
{
    MPoint startPoint, endPoint;
	MVector worldVectorStart, worldVectorEnd;
    
	view.viewToWorld(initialX, initialY, startPoint, worldVectorStart);
	view.viewToWorld(currentX, currentY, endPoint, worldVectorEnd);
    
	double distanceToCamera = (pointToMove - cameraPosition).length();
    
	startPoint += worldVectorStart * distanceToCamera;
	endPoint += worldVectorEnd * distanceToCamera;
    
    return (endPoint - startPoint) + pointToMove;
}

int contextUtils::processCurveHits(M3dView &view, CameraCache *cachePtr, MotionPathManager &mpManager)
{
    GLuint glSelectionBuffer[256];
    view.beginSelect(glSelectionBuffer, 256);
    view.initNames();
    
    mpManager.drawCurvesForSelection(view, cachePtr);
    
    GLint hits = view.endSelect();
    if (hits == 0)  return -1;
    
	GLuint* ptr = (GLuint *) glSelectionBuffer;
    ptr += (hits - 1) * 4 + 3;
	return *ptr;
}

int contextUtils::processCurveHits(const short mx, const short my, const MMatrix &cameraMatrix, M3dView &view, CameraCache *cachePtr, MotionPathManager &mpManager)
{
	for (unsigned int i = 0; i < mpManager.getMotionPathsCount(); ++i)
	{
		MotionPath* mpPtr = mpManager.getMotionPathPtr(i);
		if (!mpPtr)
			continue;

		std::vector<std::pair<int, MVector>> vec;
		
		MIntArray keys;
		contextUtils::processKeyFrameHits(mx, my, mpPtr, view, cameraMatrix, cachePtr, keys);
		if (keys.length() > 0)
			return i;

		int selectedKeyId, selectedTangent;
		contextUtils::processTangentHits(mx, my, mpPtr, view, cameraMatrix, cachePtr, selectedKeyId, selectedTangent);
		if (selectedTangent != -1)
			return i;

		double time;
		if (contextUtils::processFramesHits(mx, my, mpPtr, view, cameraMatrix, cachePtr, time))
			return i;
	}
	return -1;
}

void contextUtils::processKeyFrameHits(const short mx, const short my, MotionPath* motionPathPtr, M3dView &view, const MMatrix &cameraMatrix, CameraCache *cachePtr, MIntArray &selectedKeys)
{
	KeyframeMap *km = motionPathPtr->keyFramesCachePtr();
	int key = -1;

	double kfs = GlobalSettings::frameSize * 1.5 / 2;
	kfs = kfs * kfs;
	for (KeyframeMapIterator it = km->begin(); it != km->end(); ++it)
	{
		Keyframe &k = it->second;
		short x, y;
		view.worldToView(k.worldPosition, x, y);

		double distance = (mx - x) * (mx - x) + (my - y) * (my - y);
		if (distance < kfs)
			key = it->second.id;
	}

	if (key != -1)
		selectedKeys.append(key);
}

void contextUtils::processTangentHits(const short mx, const short my, MotionPath* motionPathPtr, M3dView &view, const MMatrix &cameraMatrix, CameraCache *cachePtr, int &selectedKeyId, int &selectedTangent)
{
	KeyframeMap *km = motionPathPtr->keyFramesCachePtr();

	double tfs = GlobalSettings::frameSize / 2;
	tfs = tfs * tfs;

	selectedTangent = -1;

	for (KeyframeMapIterator it = km->begin(); it != km->end(); ++it)
	{
		Keyframe &k = it->second;
		short x, y;
		view.worldToView(k.inTangentWorldFromCurve, x, y);

		double distance = (mx - x) * (mx - x) + (my - y) * (my - y);
		if  (distance < tfs)
		{
			selectedKeyId = k.id;
			selectedTangent = (int)Keyframe::kInTangent;
			return;
		}

		view.worldToView(k.outTangentWorldFromCurve, x, y);
		distance = (mx - x) * (mx - x) + (my - y) * (my - y);
		if (distance < tfs)
		{
			selectedKeyId = k.id;
			selectedTangent = (int)Keyframe::kOutTangent;
			return;
		}
	}
}

bool contextUtils::processFramesHits(const short mx, const short my, MotionPath* motionPathPtr, M3dView &view, const MMatrix &cameraMatrix, CameraCache *cachePtr, double &time)
{
	double fs = GlobalSettings::frameSize / 2;
	fs = fs * fs;

	std::vector<std::pair<int, MVector>> vec;
	motionPathPtr->getFramePositions(vec);
	for (unsigned int i = 0; i < vec.size(); ++i)
	{
		MVector& pos = vec[i].second;

		short x, y;
		view.worldToView(MPoint(pos.x, pos.y, pos.z), x, y);

		double distance = (mx - x) * (mx - x) + (my - y) * (my - y);
		if (distance < fs)
		{
			time = vec[i].first;
			return true;
		}
	}

	return false;
}

void contextUtils::processTangentHits(MotionPath* motionPathPtr, M3dView &view, CameraCache *cachePtr, int &selectedKeyId, int &selectedTangent)
{
    GLuint glSelectionBuffer[256];
    GLint hits;
    
    view.beginSelect(glSelectionBuffer, 256);
    view.initNames();
    motionPathPtr->drawTangentsForSelection(view, cachePtr);
    
    hits = view.endSelect();
    
    if (hits == 0)
    {
        selectedTangent = -1;
        return;
    }
    
	GLuint* ptr = (GLuint *) glSelectionBuffer;
    ptr += (hits - 1) * 5 + 3;
    selectedKeyId = *ptr;
    ++ptr;
    selectedTangent = *ptr;
}

void contextUtils::processKeyFrameHits(MotionPath* motionPathPtr, M3dView &view, CameraCache *cachePtr, MIntArray &selectedKeys)
{
    GLuint glSelectionBuffer[256];
    GLint hits;
    
    view.beginSelect(glSelectionBuffer, 256);
    view.initNames();
    motionPathPtr->drawKeysForSelection(view, cachePtr);
    hits = view.endSelect();
    
    if (hits == 0)
        return;
    
    GLuint* ptr = (GLuint *) glSelectionBuffer;
    ptr += (hits - 1) * 4 + 1;
    GLuint minZ = *ptr;
    ptr += 2;
    selectedKeys.append(*ptr);
    
    for (int i = 0; i < hits - 1; ++i)
    {
        if (minZ == glSelectionBuffer[i * 4 + 1])
            selectedKeys.append(glSelectionBuffer[i * 4 + 3]);
    }
}

bool contextUtils::processFramesHits(MotionPath* motionPathPtr, M3dView &view, CameraCache *cachePtr, double &time)
{
    GLuint glSelectionBuffer[256];
    GLint hits;
    
    view.beginSelect(glSelectionBuffer, 256);
    view.initNames();
    motionPathPtr->drawFramesForSelection(view, cachePtr);
    
    hits = view.endSelect();
    
    if (hits == 0)
        return false;
    
    GLuint* ptr = (GLuint *) glSelectionBuffer;
    ptr += (hits - 1) * 4 + 3;
    time = *ptr;
	return true;
}

void contextUtils::drawMarqueeGL(short initialX, short initialY, short finalX, short finalY)
{
    glBegin( GL_LINE_LOOP );
    glVertex2i( initialX, initialY );
    glVertex2i( finalX, initialY );
    glVertex2i( finalX, finalY );
    glVertex2i( initialX, finalY );
    glEnd();
}

void contextUtils::drawMarquee(MHWRender::MUIDrawManager& drawMgr, short initialX, short initialY, short finalX, short finalY)
{
    drawMgr.beginDrawable();
    
    drawMgr.line2d( MPoint( initialX, initialY), MPoint(finalX, initialY) );
    drawMgr.line2d( MPoint( finalX, initialY), MPoint(finalX, finalY) );
    drawMgr.line2d( MPoint( finalX, finalY), MPoint(initialX, finalY) );
    drawMgr.line2d( MPoint( initialX, finalY), MPoint(initialX, initialY) );
    
    drawMgr.endDrawable();
}


void contextUtils::applySelection(short initialX, short initialY, short finalX, short finalY, const MGlobal::ListAdjustment &listAdjustment)
{
    MSelectionList          incomingList, marqueeList;
    
    // Save the state of the current selections.  The "selectFromSceen"
    // below will alter the active list, and we have to be able to put
    // it back.
    MGlobal::getActiveSelectionList(incomingList);
    
    // If we have a zero dimension box, just do a point pick
    //
    if ( abs(initialX - finalX) < 2 && abs(initialY - finalY) < 2 ) {
        // This will check to see if the active view is in wireframe or not.
        MGlobal::SelectionMethod selectionMethod = MGlobal::selectionMethod();
        MGlobal::selectFromScreen( initialX, initialY, listAdjustment, selectionMethod );
    } else {
        // The Maya select tool goes to wireframe select when doing a marquee, so
        // we will copy that behaviour.
        // Select all the objects or components within the marquee.
        MGlobal::selectFromScreen( initialX, initialY, finalX, finalY,
                                  listAdjustment,
                                  MGlobal::kWireframeSelectMethod );
    }
    
    // Get the list of selected items
    MGlobal::getActiveSelectionList(marqueeList);
    
    //we need to set back the previous selection here or the undo for the tcMotionPathCmd won't work
    MGlobal::setActiveSelectionList(incomingList);
    
    //building args and running cmd
    MString cmd("tcMotionPathCmd -selectionChanged ");
    for (int i = 0; i < marqueeList.length(); ++i)
    {
        MDagPath d;
        marqueeList.getDagPath(i, d);
        if (d.isValid())
            cmd += d.fullPathName() + " ";
    }
    MGlobal::executeCommand(cmd, true, true);
}

void contextUtils::refreshSelectionMethod(MEvent &event, MGlobal::ListAdjustment &listAdjustment)
{
    if (event.isModifierShift() || event.isModifierControl() ) {
        if ( event.isModifierShift() ) {
            if ( event.isModifierControl() ) {
                // both shift and control pressed, merge new selections
                listAdjustment = MGlobal::kAddToList;
            }
            else
            {
                // shift only, xor new selections with previous ones
                listAdjustment = MGlobal::kXORWithList;
            }
        }
        else if ( event.isModifierControl() ) {
            // control only, remove new selections from the previous list
            listAdjustment = MGlobal::kRemoveFromList;
        }
    }
    else
        listAdjustment = MGlobal::kReplaceList;
}


