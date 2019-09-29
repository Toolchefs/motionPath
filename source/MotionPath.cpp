//
//  MotionPath.cpp
//  MotionPath
//
//  Created by Daniele Federico on 11/11/14.
//
//

#define TANGENT_TIME_DELTA 0.01

#include <QtWidgets/QApplication> 

#include "MotionPathManager.h"
#include "GlobalSettings.h"
#include "MotionPath.h"
#include "animCurveUtils.h"
#include "Vp2DrawUtils.h"

#include <maya/MPlugArray.h>
#include <maya/MAnimUtil.h>
#include <maya/MFnTransform.h>
#include <maya/MEulerRotation.h>
#include <maya/MPxTransformationMatrix.h>


extern MotionPathManager mpManager;

MotionPath::MotionPath(const MObject &object)
{
    thisObject = object;
    
    MFnDependencyNode depNodFn(object);
    txPlug = depNodFn.findPlug("translateX");
    tyPlug = depNodFn.findPlug("translateY");
    tzPlug = depNodFn.findPlug("translateZ");
    
    rxPlug = depNodFn.findPlug("rotateX");
    ryPlug = depNodFn.findPlug("rotateY");
    rzPlug = depNodFn.findPlug("rotateZ");
    
    rpxPlug = depNodFn.findPlug("rotatePivotX");
    rpyPlug = depNodFn.findPlug("rotatePivotY");
    rpzPlug = depNodFn.findPlug("rotatePivotZ");
    
    rptxPlug = depNodFn.findPlug("rotatePivotTranslateX");
    rptyPlug = depNodFn.findPlug("rotatePivotTranslateY");
    rptzPlug = depNodFn.findPlug("rotatePivotTranslateZ");

    isDrawing = false;
    
    startTime = 0;
    endTime = 0;
    displayStartTime = 0;
    displayEndTime = 0;
    startTimeCached = 0;
    endTimeCached = 0;
    selectedFromTool = false;
    colorMultiplier = 1.0;
    
    isWeighted = false;
    
    constrained = isConstrained(object);
	findParentMatrixPlug(object, constrained, pMatrixPlug);
    
    selectedKeyTimes.clear();
    
    cacheDone = false;
    worldSpaceCallbackCalled = false;
    
    setTimeRange(GlobalSettings::startTime, GlobalSettings::endTime);
}

MotionPath::~MotionPath()
{
    removeWorldMartrixCallback();
}

void MotionPath::addWorldMatrixCallback()
{
    MStatus status;
    MDagPath dp;
    MDagPath::getAPathTo(thisObject, dp);
    worldMatrixCallbackId = MDagMessage::addWorldMatrixModifiedCallback(dp, worldMatrixChangedCallback,(void *) this, &status);
}

void MotionPath::removeWorldMartrixCallback()
{
    if (worldMatrixCallbackId)
        MMessage::removeCallback(worldMatrixCallbackId);
    worldMatrixCallbackId = 0;
}

void MotionPath::cacheParentMatrixRange()
{
    MTime currentTime = MAnimControl::currentTime();
    double currentFrame = currentTime.as(MTime::uiUnit());
    double startFrame = currentFrame - GlobalSettings::framesBack;
    double endFrame = currentFrame + GlobalSettings::framesFront;
    
    if(startFrame < GlobalSettings::startTime)	startFrame = GlobalSettings::startTime;
    if(endFrame > GlobalSettings::endTime) 	endFrame = GlobalSettings::endTime;
    for (double i = startFrame; i <= endFrame; ++i)
        ensureParentAndPivotMatrixAtTime(i);
}

void MotionPath::cacheParentMatrixRangeForWorldCallback(MObject &transformNode)
{
    MFnDependencyNode depNodFn(transformNode);
    MPlug txP = depNodFn.findPlug("translateX");
    MPlug tyP = depNodFn.findPlug("translateY");
    MPlug tzP = depNodFn.findPlug("translateZ");
    
    MPlug rxP = depNodFn.findPlug("rotateX");
    MPlug ryP = depNodFn.findPlug("rotateY");
    MPlug rzP = depNodFn.findPlug("rotateZ");
    
    MStatus txStatus, tyStatus, tzStatus, rxStatus, ryStatus, rzStatus, sxStatus, syStatus, szStatus;
    MFnAnimCurve cTX(txP, &txStatus);
    MFnAnimCurve cTY(tyP, &tyStatus);
    MFnAnimCurve cTZ(tzP, &tzStatus);
    MFnAnimCurve cRX(rxP, &rxStatus);
    MFnAnimCurve cRY(ryP, &ryStatus);
    MFnAnimCurve cRZ(rzP, &rzStatus);
    
    MTime currentTime = MAnimControl::currentTime();
    
    double newTXValue, newTYValue, newTZValue, newRXValue, newRYValue, newRZValue;
    double oldTXValue, oldTYValue, oldTZValue, oldRXValue, oldRYValue, oldRZValue;
    int newKeyTX, newKeyTY, newKeyTZ, newKeyRX, newKeyRY, newKeyRZ;
    int oldKeyTX, oldKeyTY, oldKeyTZ, oldKeyRX, oldKeyRY, oldKeyRZ;
    bool txUpdated, tyUpdated, tzUpdated, rxUpdated, ryUpdated, rzUpdated;
    if (txStatus != MS::kNotFound)
        txUpdated = animCurveUtils::updateCurve(txP, cTX, currentTime, oldTXValue, newTXValue, newKeyTX, oldKeyTX);
    if (tyStatus != MS::kNotFound)
        tyUpdated = animCurveUtils::updateCurve(tyP, cTY, currentTime, oldTYValue, newTYValue, newKeyTY, oldKeyTY);
    if (tzStatus != MS::kNotFound)
        tzUpdated = animCurveUtils::updateCurve(tzP, cTZ, currentTime, oldTZValue, newTZValue, newKeyTZ, oldKeyTZ);
    if (rxStatus != MS::kNotFound)
        rxUpdated = animCurveUtils::updateCurve(rxP, cRX, currentTime, oldRXValue, newRXValue, newKeyRX, oldKeyRX);
    if (ryStatus != MS::kNotFound)
        ryUpdated = animCurveUtils::updateCurve(ryP, cRY, currentTime, oldRYValue, newRYValue, newKeyRY, oldKeyRY);
    if (rzStatus != MS::kNotFound)
        rzUpdated = animCurveUtils::updateCurve(rzP, cRZ, currentTime, oldRZValue, newRZValue, newKeyRZ, oldKeyRZ);
    
    cacheParentMatrixRange();
    
    if (txUpdated && txStatus != MS::kNotFound)
    {
        animCurveUtils::restoreCurve(cTX, currentTime, oldTXValue, newKeyTX, oldKeyTX);
        txP.setValue(newTXValue);
    }
    if (tyUpdated && tyStatus != MS::kNotFound)
    {
        animCurveUtils::restoreCurve(cTY, currentTime, oldTYValue, newKeyTY, oldKeyTY);
        tyP.setValue(newTYValue);
    }
    if (tzUpdated && tzStatus != MS::kNotFound )
    {
        animCurveUtils::restoreCurve(cTZ, currentTime, oldTZValue, newKeyTZ, oldKeyTZ);
        tzP.setValue(newTZValue);
    }
    
    if (rxUpdated && rxStatus != MS::kNotFound)
    {
        animCurveUtils::restoreCurve(cRX, currentTime, oldRXValue, newKeyRX, oldKeyRX);
        rxP.setValue(newRXValue);
    }
    if (ryUpdated && ryStatus != MS::kNotFound)
    {
        animCurveUtils::restoreCurve(cRY, currentTime, oldRYValue, newKeyRY, oldKeyRY);
        ryP.setValue(newRYValue);
    }
    if (rzUpdated && rzStatus != MS::kNotFound )
    {
        animCurveUtils::restoreCurve(cRZ, currentTime, oldRZValue, newKeyRZ, oldKeyRZ);
        rzP.setValue(newRZValue);
    }
    
}

bool MotionPath::getWorldSpaceCallbackCalled()
{
    return worldSpaceCallbackCalled;
}

void MotionPath::setWorldSpaceCallbackCalled(const bool value, const MObject &ancestorNode)
{
    tempAncestorNode = ancestorNode;
    worldSpaceCallbackCalled = value;
}

void MotionPath::worldMatrixChangedCallback(MObject& transformNode, MDagMessage::MatrixModifiedFlags& modified, void* data)
{
    if (GlobalSettings::lockedMode && GlobalSettings::lockedModeInteractive)
    {
        bool autokey = MAnimControl::autoKeyMode();
        
        //check if the camera has the needed data
        if (MAnimControl::isPlaying() && autokey)
            return;
        
        #if MAYA_API_VERSION > 201500
        //check if the camera has the needed data
        if (MAnimControl::isScrubbing() && autokey)
            return;
        #endif
        
        MotionPath *mPath  = (MotionPath *) data;
        //if this object is the only one selected we don't refresh the parent matrices
        if (MGlobal::isSelected(mPath->object()))
        {
            MSelectionList selList;
            MGlobal::getActiveSelectionList(selList);
            if (selList.length() == 1)
                return;
        }
        
        mPath->setWorldSpaceCallbackCalled(true, transformNode);
    }
}

bool MotionPath::hasAnimationLayers(const MObject &object)
{
    MFnDependencyNode depNodFn(object);
    MPlug txPlug = depNodFn.findPlug("translateX");
    MPlug tyPlug = depNodFn.findPlug("translateY");
    MPlug tzPlug = depNodFn.findPlug("translateZ");
    
    MString type("kAnimLayer");
    
    MPlugArray pArray;
    txPlug.connectedTo(pArray, false, true);
    if (pArray.length() > 0)
    {
        for (int i = 0; i < pArray.length(); ++i)
        {
            if (type == pArray[i].node().apiTypeStr())
                return true;
        }
    }
    
    tyPlug.connectedTo(pArray, false, true);
    if (pArray.length() > 0)
    {
        for (int i = 0; i < pArray.length(); ++i)
        {
            if (type == pArray[i].node().apiTypeStr())
                return true;
        }
    }
    
    tzPlug.connectedTo(pArray, false, true);
    if (pArray.length() > 0)
    {
        for (int i = 0; i < pArray.length(); ++i)
        {
            if (type == pArray[i].node().apiTypeStr())
                return true;
        }
    }
    
    return false;
}

bool isPlugConstrained(const MPlug &plug)
{
    const char* types[] = {"kAnimCurveTimeToAngular", "kAnimCurveTimeToDistance", "kAnimCurveTimeToTime",
        "kAnimCurveTimeToUnitless"};
    MStringArray anTypes(types, 4);
    
    MPlugArray pArray;
    plug.connectedTo(pArray, true, false);
    if (pArray.length() == 0)
        return false;
    
    for(int j = 0; j < anTypes.length(); j++)
        if(anTypes[j] == pArray[0].node().apiTypeStr())
            return false;

    return true;
}

bool MotionPath::isConstrained(const MFnDagNode &dagNodeFn)
{
    MPlugArray tsArray;
    tsArray.append(txPlug);
    tsArray.append(tyPlug);
    tsArray.append(tzPlug);
    
    for (int i =0; i < tsArray.length(); ++i)
    {
        if (isPlugConstrained(tsArray[i]))
            return true;
    }
    
    return false;
}

void MotionPath::clearParentMatrixCache()
{
    pMatrixCache.clear();
}

void MotionPath::findParentMatrixPlug(const MObject &transform, const bool constrained, MPlug &matrixPlug)
{
	MFnDagNode dagNodeFn(transform);
	MPlug parentMatrixPlugs = dagNodeFn.findPlug(constrained ? "worldMatrix" : "parentMatrix");
	parentMatrixPlugs.evaluateNumElements();
	matrixPlug = parentMatrixPlugs[0];
}

bool MotionPath::isCurveTypeAnimatable(MFnAnimCurve::AnimCurveType type)
{
    return type == MFnAnimCurve::kAnimCurveTA || type == MFnAnimCurve::kAnimCurveTL || type == MFnAnimCurve::kAnimCurveTT || type == MFnAnimCurve::kAnimCurveTU;
}

void MotionPath::setTimeRange(double startTime, double endTime)
{
    this->startTime = startTime;
    this->endTime = endTime;

    cacheDone = false;
}

void MotionPath::setDisplayTimeRange(double start, double end)
{
	displayStartTime = start;
	if(displayStartTime < startTimeCached)
		displayStartTime = startTimeCached;
    
	displayEndTime = end;
	if(displayEndTime > endTimeCached)
		displayEndTime = endTimeCached;

}

MMatrix MotionPath::getMatrixFromPlug(const MPlug &matrixPlug, const MTime &t)
{
	MDGContext context(t);
	MObject val;
	matrixPlug.getValue(val, context);
	return MFnMatrixData(val).matrix();
}

void MotionPath::growParentAndPivotMatrixCache(double time, double expansion)
{
	double evalTimeBefore = time - expansion;
	if(evalTimeBefore < (time - GlobalSettings::framesBack))
		evalTimeBefore = time - GlobalSettings::framesBack;
    
	double evalTimeAfter = time + expansion;
	if(evalTimeAfter > (time + GlobalSettings::framesFront))
		evalTimeAfter = time + GlobalSettings::framesFront;

	if(evalTimeBefore >= this->startTime)
	{
        ensureParentAndPivotMatrixAtTime(evalTimeBefore);
		startTimeCached = evalTimeBefore;
	}
    
	if(evalTimeAfter <= this->endTime)
	{
        ensureParentAndPivotMatrixAtTime(evalTimeAfter);
		endTimeCached = evalTimeAfter;
	}
    
	if(startTimeCached == startTime && endTimeCached == endTime)
		cacheDone = true;
}

void MotionPath::drawKeyFrames(CameraCache *cachePtr, MMatrix &currentCameraMatrix, MHWRender::MUIDrawManager* drawManager, const MHWRender::MFrameContext* frameContext)
{
    int portWidth = GlobalSettings::portWidth;
    int portHeight = GlobalSettings::portHeight;
    
    if (GlobalSettings::motionPathDrawMode == GlobalSettings::kCameraSpace)
    {
        portWidth = cachePtr->portWidth;
        portHeight = cachePtr->portHeight;
    }
    
	if (drawManager)
		VP2DrawUtils::drawKeyFramePoints(keyframesCache, GlobalSettings::frameSize * 1.5, colorMultiplier, portWidth, portHeight, GlobalSettings::showRotationKeyFrames, currentCameraMatrix, drawManager, frameContext);
	else
		drawUtils::drawKeyFramePoints(keyframesCache, GlobalSettings::frameSize * 1.5, colorMultiplier, portWidth, portHeight, GlobalSettings::showRotationKeyFrames);
}

void MotionPath::drawFrames(CameraCache* cachePtr, const MMatrix &currentCameraMatrix, M3dView &view, MHWRender::MUIDrawManager* drawManager, const MHWRender::MFrameContext* frameContext)
{
    MColor curveColor = isWeighted ? GlobalSettings::weightedPathColor : GlobalSettings::pathColor;

    if(this->selectedFromTool)  curveColor *= 1.3;

    curveColor *= colorMultiplier;
    
    ensureParentAndPivotMatrixAtTime(displayStartTime);
    
    MVector previousWorldPos = multPosByParentMatrix(getPos(displayStartTime), pMatrixCache[displayStartTime]);
    if (GlobalSettings::motionPathDrawMode == GlobalSettings::kCameraSpace)
    {
        cachePtr->ensureMatricesAtTime(displayStartTime);
        previousWorldPos = MPoint(previousWorldPos) * cachePtr->matrixCache[displayStartTime] * currentCameraMatrix;
    }
    
	for(double i = displayStartTime + 1.0; i <= displayEndTime; i += 1.0)
	{
        ensureParentAndPivotMatrixAtTime(i);
        
		MVector worldPos = multPosByParentMatrix(getPos(i), pMatrixCache[i]);
        if (GlobalSettings::motionPathDrawMode == GlobalSettings::kCameraSpace)
        {
            cachePtr->ensureMatricesAtTime(i);
            worldPos = MPoint(worldPos) * cachePtr->matrixCache[i] * currentCameraMatrix;
        }
        
        if (GlobalSettings::showPath)
        {
            double factor = 1;
            if (GlobalSettings::alternatingFrames)
                factor = int(i) % 2 == 1 ? 1.4 : 0.6;
                
			if (drawManager)
				VP2DrawUtils::drawLineWithColor(previousWorldPos, worldPos, GlobalSettings::pathSize, curveColor * factor, currentCameraMatrix, drawManager, frameContext);
			else
				drawUtils::drawLineWithColor(previousWorldPos, worldPos, GlobalSettings::pathSize, curveColor * factor);
        }

		if (drawManager)
			VP2DrawUtils::drawPointWithColor(previousWorldPos, GlobalSettings::pathSize * 2, curveColor, currentCameraMatrix, drawManager, frameContext);
		else
			drawUtils::drawPointWithColor(previousWorldPos, GlobalSettings::pathSize, curveColor);
        previousWorldPos = worldPos;
        
		if (i == displayEndTime)
		{
			if (drawManager)
				VP2DrawUtils::drawPointWithColor(worldPos, GlobalSettings::pathSize * 2, curveColor, currentCameraMatrix, drawManager, frameContext);
			else
				drawUtils::drawPointWithColor(worldPos, GlobalSettings::pathSize, curveColor);
		}
	}
}

void MotionPath::expandKeyFramesCache(const MFnAnimCurve &curve, const Keyframe::Axis &axisName, bool isTranslate)
{
    int numKeys = curve.numKeys();
    
    double endTime = isDrawing ? endDrawingTime : displayEndTime;
    
	for(int i = 0; i < numKeys; i++)
	{
		MTime keyTime = curve.time(i);
		double keyTimeVal = keyTime.as(MTime::uiUnit());
        
		if(keyTimeVal >= displayStartTime)
		{
			if(keyTimeVal <= endTime)
			{
                //we don't want to add a keyframe if only rotation keyframes are present at this time
                if (!isTranslate && keyframesCache.find(keyTimeVal) == keyframesCache.end())
                    continue;
                
				Keyframe* keyframePtr = &keyframesCache[keyTimeVal];
                
                if (isTranslate)
                {
                    keyframePtr->time = keyTimeVal;
                
                    keyframePtr->setTangent(i, curve, axisName, Keyframe::kInTangent);
                    keyframePtr->setTangent(i, curve, axisName, Keyframe::kOutTangent);
                
                    keyframePtr->setKeyId(i, axisName);
                
                    if(keyframePtr->tangentsLocked)
                        keyframePtr->tangentsLocked = curve.tangentsLocked(i);
                }
                else
                    keyframePtr->setRotKeyId(i, axisName);
            }
            else
                break;
        }
    }
}

MVector MotionPath::getPos(double time)
{
	MVector pos(0.0, 0.0, 0.0);
	if(this->constrained == false)
	{
		MTime evalTime(time, MTime::uiUnit());
		MDGContext context(evalTime);
        
		pos.x = txPlug.asDouble(context);
		pos.y = tyPlug.asDouble(context);
		pos.z = tzPlug.asDouble(context);
	}
    
	return pos;
}

MVector MotionPath::multPosByParentMatrix(const MVector &vec, const MMatrix &mat)
{   
	MVector multipliedVec;
    multipliedVec = vec * mat;
    multipliedVec.x += mat[3][0];
    multipliedVec.y += mat[3][1];
    multipliedVec.z += mat[3][2];   
	return multipliedVec;
}

bool MotionPath::showTangent(const double time, const int firstId, const double firstTime, const int secondId, const double secondTime)
{
    return !((firstId == -1 && secondId == -1) || (time == firstTime && time == secondTime));
}

void MotionPath::setShowInOutTangents(const MFnAnimCurve &curveTX, const MFnAnimCurve &curveTY, const MFnAnimCurve &curveTZ)
{
    //if none of these are animated we don't want to proceed as this would also mean adding a keyframe to the frames cache
    if (curveTX.numKeys() == 0 && curveTY.numKeys() == 0 && curveTZ.numKeys() == 0)
        return;
        
    double minTimeX = curveTX.time(0).as(MTime::uiUnit());
    double maxTimeX = curveTX.time(curveTX.numKeys() - 1).as(MTime::uiUnit());
    double minTimeY = curveTY.time(0).as(MTime::uiUnit());
    double maxTimeY = curveTY.time(curveTY.numKeys() - 1).as(MTime::uiUnit());
    double minTimeZ = curveTZ.time(0).as(MTime::uiUnit());
    double maxTimeZ = curveTZ.time(curveTZ.numKeys() - 1).as(MTime::uiUnit());

    Keyframe *keyFramePtr;
    if (minTimeX >= displayStartTime && minTimeX <= displayEndTime)
    {
        keyFramePtr = &keyframesCache[minTimeX];
        keyFramePtr->showInTangent = showTangent(minTimeX, keyFramePtr->yKeyId, minTimeY, keyFramePtr->zKeyId, minTimeZ);
    }
    
    if (minTimeY >= displayStartTime && minTimeY <= displayEndTime)
    {
        keyFramePtr = &keyframesCache[minTimeY];
        keyFramePtr->showInTangent = showTangent(minTimeY, keyFramePtr->xKeyId, minTimeX, keyFramePtr->zKeyId, minTimeZ);
    }
    
    if (minTimeZ >= displayStartTime && minTimeZ <= displayEndTime)
    {
        keyFramePtr = &keyframesCache[minTimeZ];
        keyFramePtr->showInTangent = showTangent(minTimeZ, keyFramePtr->xKeyId, minTimeX, keyFramePtr->yKeyId, minTimeY);
    }
    
    if (maxTimeX >= displayStartTime && maxTimeX <= displayEndTime)
    {
        keyFramePtr = &keyframesCache[maxTimeX];
        keyFramePtr->showOutTangent = showTangent(maxTimeX, keyFramePtr->yKeyId, maxTimeY, keyFramePtr->zKeyId, maxTimeZ);
    }
    
    if (maxTimeY >= displayStartTime && maxTimeY <= displayEndTime)
    {
        keyFramePtr = &keyframesCache[maxTimeY];
        keyFramePtr->showOutTangent = showTangent(maxTimeY, keyFramePtr->xKeyId, maxTimeX, keyFramePtr->zKeyId, maxTimeZ);
    }
    
    if (maxTimeZ >= displayStartTime && maxTimeZ <= displayEndTime)
    {
        keyFramePtr = &keyframesCache[maxTimeZ];
        keyFramePtr->showOutTangent = showTangent(maxTimeZ, keyFramePtr->xKeyId, maxTimeX, keyFramePtr->yKeyId, maxTimeY);
    }
}

MVector MotionPath::getVectorFromPlugs(const MTime &evalTime, const MPlug &x, const MPlug &y, const MPlug &z)
{
    MDGContext context(evalTime);
    
    MVector pos;
    pos.x = x.asDouble(context);
    pos.y = y.asDouble(context);
    pos.z = z.asDouble(context);
    return pos;
}

MMatrix MotionPath::getPMatrixAtTime(const MTime &evalTime)
{
    MMatrix m = getMatrixFromPlug(pMatrixPlug, evalTime);

    if (GlobalSettings::usePivots)
    {
        MVector piv = getVectorFromPlugs(evalTime, rpxPlug, rpyPlug, rpzPlug);
		MMatrix pivotMtx;
        pivotMtx[3][0] = piv.x;
        pivotMtx[3][1] = piv.y;
        pivotMtx[3][2] = piv.z;
		m = pivotMtx * m;
        
        piv = getVectorFromPlugs(evalTime, rptxPlug, rptyPlug, rptzPlug);
        pivotMtx[3][0] = piv.x;
        pivotMtx[3][1] = piv.y;
        pivotMtx[3][2] = piv.z;
        m = pivotMtx * m;
    }
    
    return m;
}

void MotionPath::ensureParentAndPivotMatrixAtTime(const double time)
{
    if(pMatrixCache.find(time) == pMatrixCache.end())
    {
        MTime evalTime(time, MTime::uiUnit());
        pMatrixCache[time] = getPMatrixAtTime(evalTime);
    }
}

void MotionPath::cacheKeyFrames(const MFnAnimCurve &curveTX, const MFnAnimCurve &curveTY, const MFnAnimCurve &curveTZ, const MFnAnimCurve &curveRX, const MFnAnimCurve &curveRY, const MFnAnimCurve &curveRZ, CameraCache* cachePtr, const MMatrix &currentCameraMatrix)
{
    if (isCurveTypeAnimatable(curveTX.animCurveType()))
        expandKeyFramesCache(curveTX, Keyframe::kAxisX, true);

    if (isCurveTypeAnimatable(curveTY.animCurveType()))
        expandKeyFramesCache(curveTY, Keyframe::kAxisY, true);
    
    if (isCurveTypeAnimatable(curveTZ.animCurveType()))
        expandKeyFramesCache(curveTZ, Keyframe::kAxisZ, true);
    
    if (GlobalSettings::showRotationKeyFrames)
    {
        if (isCurveTypeAnimatable(curveRX.animCurveType()))
            expandKeyFramesCache(curveRX, Keyframe::kAxisX, false);
        
        if (isCurveTypeAnimatable(curveRY.animCurveType()))
            expandKeyFramesCache(curveRY, Keyframe::kAxisY, false);
        
        if (isCurveTypeAnimatable(curveRZ.animCurveType()))
            expandKeyFramesCache(curveRZ, Keyframe::kAxisZ, false);
    }
    
    setShowInOutTangents(curveTX, curveTY, curveTZ);
    
	int i = 0;
	for(KeyframeMapIterator keyIt = keyframesCache.begin(); keyIt != keyframesCache.end(); keyIt++)
	{
		Keyframe* key = &keyIt->second;
		key->id = i;
        
        if (selectedKeyTimes.find(key->time) != selectedKeyTimes.end())
            key->selectedFromTool = true;
        
        //OVERRIDE: if we are drawing, we don't show the tangents to make perfomances faster
        if (isDrawing)
        {
            key->showInTangent = false;
            key->showOutTangent = false;
        }
        
        ensureParentAndPivotMatrixAtTime(key->time);
        
        key->position = getPos(key->time);
        key->worldPosition = multPosByParentMatrix(key->position, pMatrixCache[key->time]);
        if (GlobalSettings::motionPathDrawMode == GlobalSettings::kCameraSpace)
        {
            cachePtr->ensureMatricesAtTime(key->time);
            key->worldPosition = MPoint(key->worldPosition) * cachePtr->matrixCache[key->time] * currentCameraMatrix;
        }
        
		key->inTangentWorld = multPosByParentMatrix((-key->inTangent) + key->position, pMatrixCache[key->time]);
		key->outTangentWorld = multPosByParentMatrix(key->outTangent + key->position, pMatrixCache[key->time]);
        
        if (key->showInTangent)
        {
            if (isWeighted)
                key->inTangentWorldFromCurve = key->inTangentWorld;
            else
            {
                double prevTime = key->time - TANGENT_TIME_DELTA;
                ensureParentAndPivotMatrixAtTime(prevTime);
                
                MVector inWorldPosition;
                if (GlobalSettings::motionPathDrawMode == GlobalSettings::kWorldSpace)
                    inWorldPosition = multPosByParentMatrix(getPos(prevTime), pMatrixCache[prevTime]) - key->worldPosition;
                else
                {
                    cachePtr->ensureMatricesAtTime(prevTime, true);
                    inWorldPosition = MVector(MPoint(multPosByParentMatrix(getPos(prevTime), pMatrixCache[prevTime])) * cachePtr->matrixCache[prevTime] * currentCameraMatrix) - key->worldPosition;
                }
                
                inWorldPosition.normalize();
                key->inTangentWorldFromCurve = inWorldPosition * key->inTangent.length() + key->worldPosition;
            }
        }
    
        if (key->showOutTangent)
        {
            if (isWeighted)
                 key->outTangentWorldFromCurve = key->outTangentWorld;
            else
            {
                double afterTime = key->time + TANGENT_TIME_DELTA;
                ensureParentAndPivotMatrixAtTime(afterTime);
                
                MVector outWorldPosition = multPosByParentMatrix(getPos(afterTime), pMatrixCache[afterTime]) - key->worldPosition;
                if (GlobalSettings::motionPathDrawMode == GlobalSettings::kWorldSpace)
                    outWorldPosition = multPosByParentMatrix(getPos(afterTime), pMatrixCache[afterTime]) - key->worldPosition;
                else
                {
                    cachePtr->ensureMatricesAtTime(afterTime, true);
                    outWorldPosition = MVector(MPoint(multPosByParentMatrix(getPos(afterTime), pMatrixCache[afterTime])) * cachePtr->matrixCache[afterTime] * currentCameraMatrix) - key->worldPosition;
                }
                
                outWorldPosition.normalize();
                key->outTangentWorldFromCurve = outWorldPosition * key->outTangent.length() + key->worldPosition;
            }
        }
        
		i += 1;
	}
}

void MotionPath::drawTangents(M3dView &view, MMatrix& currentCameraMatrix, MHWRender::MUIDrawManager* drawManager, const MHWRender::MFrameContext* frameContext)
{
    if (isWeighted && GlobalSettings::motionPathDrawMode == GlobalSettings::kCameraSpace)
        return;
    
    //if the user is navigating we don't refresh the tangents
    if ((QApplication::mouseButtons() != Qt::NoButton) && (QApplication::keyboardModifiers() == Qt::AltModifier))
        return;
    
	MColor tangentColor;
	for(KeyframeMapIterator keyIt = keyframesCache.begin(); keyIt != keyframesCache.end(); keyIt++)
	{
		Keyframe* key = &keyIt->second;
        if (isWeighted)
            tangentColor = GlobalSettings::weightedPathTangentColor;
        else
            tangentColor = key->tangentsLocked ? GlobalSettings::tangentColor : GlobalSettings::brokenTangentColor;
        
        if (key->showInTangent)
        {
			if (drawManager)
			{
				VP2DrawUtils::drawLineWithColor(key->worldPosition, key->inTangentWorldFromCurve, 1.0, tangentColor, currentCameraMatrix, drawManager, frameContext);
				VP2DrawUtils::drawPointWithColor(key->inTangentWorldFromCurve, GlobalSettings::frameSize, tangentColor, currentCameraMatrix, drawManager, frameContext);
			}
			else
			{
				drawUtils::drawLineWithColor(key->worldPosition, key->inTangentWorldFromCurve, 1.0, tangentColor);
				drawUtils::drawPointWithColor(key->inTangentWorldFromCurve, GlobalSettings::frameSize, tangentColor);
			}
        }
        
        if (key->showOutTangent)
		{
			if (drawManager)
			{
				VP2DrawUtils::drawLineWithColor(key->worldPosition, key->outTangentWorldFromCurve, 1.0, tangentColor, currentCameraMatrix, drawManager, frameContext);
				VP2DrawUtils::drawPointWithColor(key->outTangentWorldFromCurve, GlobalSettings::frameSize, tangentColor, currentCameraMatrix, drawManager, frameContext);
			}
			else
			{
				drawUtils::drawLineWithColor(key->worldPosition, key->outTangentWorldFromCurve, 1.0, tangentColor);
				drawUtils::drawPointWithColor(key->outTangentWorldFromCurve, GlobalSettings::frameSize, tangentColor);
			}
        }
	}
}

void MotionPath::drawFrameLabels(M3dView &view, CameraCache* cachePtr, const MMatrix &currentCameraMatrix, MHWRender::MUIDrawManager* drawManager, const MHWRender::MFrameContext* frameContext)
{
    MColor labelColor = GlobalSettings::frameLabelColor;
    if(this->selectedFromTool)  labelColor *= 1.3;
    
	for(double i = displayStartTime; i <= displayEndTime; i += 1.0)
	{
        bool hasKey = false;
        if (GlobalSettings::showKeyFrames)
            hasKey = keyframesCache.find(i) != keyframesCache.end();
        if (hasKey && !GlobalSettings::showKeyFrameNumbers)
            continue;
        else if (!hasKey && !GlobalSettings::showFrameNumbers)
            continue;
        
   		MVector worldPos = multPosByParentMatrix(getPos(i), pMatrixCache[i]);
        if (GlobalSettings::motionPathDrawMode == GlobalSettings::kCameraSpace)
        {
            cachePtr->ensureMatricesAtTime(i);
            worldPos = MPoint(worldPos) * cachePtr->matrixCache[i] * currentCameraMatrix;
        }
        
        double offset = hasKey ? 1.1 : 0.8;

		if (drawManager)
			VP2DrawUtils::drawFrameLabel(i, worldPos, view, offset, labelColor, currentCameraMatrix, drawManager, frameContext);
		else
			drawUtils::drawFrameLabel(i, worldPos, view, offset, labelColor, currentCameraMatrix);
	}
}

void MotionPath::drawCurrentFrame(CameraCache* cachePtr, const MMatrix &currentCameraMatrix, M3dView &view, MHWRender::MUIDrawManager* drawManager, const MHWRender::MFrameContext* frameContext)
{
	MColor frameColor = GlobalSettings::currentFrameColor;
	if(this->selectedFromTool)		frameColor *= 1.3;
    
	MTime currentTime = MAnimControl::currentTime();
	double currentTimeValue = currentTime.as(MTime::uiUnit());
    
    ensureParentAndPivotMatrixAtTime(currentTimeValue);
    
    MVector worldPos = multPosByParentMatrix(getPos(currentTimeValue), this->pMatrixCache[currentTimeValue]);
    if (GlobalSettings::motionPathDrawMode == GlobalSettings::kCameraSpace)
    {
        cachePtr->ensureMatricesAtTime(currentTimeValue);
        worldPos = MPoint(worldPos) * cachePtr->matrixCache[currentTimeValue] * currentCameraMatrix;
    } 
    
	if (drawManager) 
		VP2DrawUtils::drawPointWithColor(worldPos, GlobalSettings::frameSize * 2.2, frameColor, currentCameraMatrix, drawManager, frameContext);
	else
		drawUtils::drawPointWithColor(worldPos, GlobalSettings::frameSize * 2.2, frameColor);
}

void MotionPath::drawPath(M3dView &view, CameraCache* cachePtr, const MMatrix &currentCameraMatrix, const bool selecting, MHWRender::MUIDrawManager* drawManager, const MHWRender::MFrameContext* frameContext)
{    
    drawFrames(cachePtr, GlobalSettings::cameraMatrix, view, drawManager, frameContext);
    
    if (!selecting)
    {
        drawCurrentFrame(cachePtr, GlobalSettings::cameraMatrix, view, drawManager, frameContext);
    
        if (GlobalSettings::showKeyFrameNumbers || GlobalSettings::showFrameNumbers)
            drawFrameLabels(view, cachePtr, GlobalSettings::cameraMatrix, drawManager, frameContext);
    }
    
    if (GlobalSettings::showKeyFrames && keyframesCache.size() > 0)
    {
        if (GlobalSettings::showTangents)
            drawTangents(view, GlobalSettings::cameraMatrix, drawManager, frameContext);
        
        // we want the keyframes to appear on the top of everything
        drawKeyFrames(cachePtr, GlobalSettings::cameraMatrix, drawManager, frameContext);
    }
}

void MotionPath::draw(M3dView &view, CameraCache* cachePtr, MHWRender::MUIDrawManager* drawManager, const MHWRender::MFrameContext* frameContext)
{
    MTime currentTime = MAnimControl::currentTime();
    
    MStatus xStatus, yStatus, zStatus;
	MFnAnimCurve curveX(txPlug, &xStatus);
	MFnAnimCurve curveY(tyPlug, &yStatus);
	MFnAnimCurve curveZ(tzPlug, &zStatus);
    MFnAnimCurve curveRotX(rxPlug);
	MFnAnimCurve curveRotY(ryPlug);
	MFnAnimCurve curveRotZ(rzPlug);
    
    //storing values to keep the curve appear like it was edited in real time (won't happen if autoKeyFrame is off)
    double newXValue, newYValue, newZValue;
    double oldXValue, oldYValue, oldZValue;
    int newKeyX, newKeyY, newKeyZ;
    int oldKeyX, oldKeyY, oldKeyZ;
    bool xUpdated=false, yUpdated=false, zUpdated=false;
    
    //Refreshing the parent matrix cache if we need to do so
    if (GlobalSettings::lockedMode && GlobalSettings::lockedModeInteractive && getWorldSpaceCallbackCalled())
    {
        if (QApplication::mouseButtons() != Qt::LeftButton)
        {
            clearParentMatrixCache();
            cacheParentMatrixRangeForWorldCallback(tempAncestorNode);
            setWorldSpaceCallbackCalled(false, MObject());
        }
    }
    
    MMatrix currentCameraMatrix;
    if (GlobalSettings::motionPathDrawMode == GlobalSettings::kCameraSpace)
    {
        double currentTime = MAnimControl::currentTime().as(MTime::uiUnit());
        currentCameraMatrix = cachePtr->matrixCache[currentTime].inverse();
    }
    
    if (!constrained)
    {
        xUpdated = animCurveUtils::updateCurve(txPlug, curveX, currentTime, oldXValue, newXValue, newKeyX, oldKeyX);
        yUpdated = animCurveUtils::updateCurve(tyPlug, curveY, currentTime, oldYValue, newYValue, newKeyY, oldKeyY);
        zUpdated = animCurveUtils::updateCurve(tzPlug, curveZ, currentTime, oldZValue, newZValue, newKeyZ, oldKeyZ);
        
        isWeighted = curveX.isWeighted() || curveY.isWeighted() || curveZ.isWeighted();
        
        keyframesCache.clear();

        cacheKeyFrames(curveX, curveY, curveZ, curveRotX, curveRotY, curveRotZ, cachePtr, currentCameraMatrix);
    }
    
    drawPath(view, cachePtr, currentCameraMatrix, false, drawManager, frameContext);
     
    //restoring the previous values if a keyframe was not actually set by the user
    if (xUpdated && xStatus != MS::kNotFound)
    {
        animCurveUtils::restoreCurve(curveX, currentTime, oldXValue, newKeyX, oldKeyX);
        this->txPlug.setValue(newXValue);
    }
    if (yUpdated && yStatus != MS::kNotFound)
    {
        animCurveUtils::restoreCurve(curveY, currentTime, oldYValue, newKeyY, oldKeyY);
        this->tyPlug.setValue(newYValue);
    }
    if (zUpdated && zStatus != MS::kNotFound )
    {
        animCurveUtils::restoreCurve(curveZ, currentTime, oldZValue, newKeyZ, oldKeyZ);
        this->tzPlug.setValue(newZValue);
    }
    

}

double MotionPath::getTimeFromKeyId(const int id)
{
	for(KeyframeMapIterator keyIt = keyframesCache.begin(); keyIt != keyframesCache.end(); keyIt++)
	{
		Keyframe* key = &keyIt->second;
		if(key->id == id)
			return key->time;
	}
    
	return 0.0;
}

int MotionPath::getNumKeyFrames()
{
    return keyframesCache.size();
}

void MotionPath::getBoundariesForTime(const double time, double *minBoundary, double *maxBoundary)
{
    bool minFound = false;
    bool maxFound = false;
    double min, max;
    
    for(KeyframeMapIterator keyIt = keyframesCache.begin(); keyIt != keyframesCache.end(); keyIt++)
	{
		Keyframe* key = &keyIt->second;
        if (key->time == time)
            continue;
        
        if (time - key->time > 0 && (!minFound || key->time > min))
        {
            min = key->time;
            minFound = true;
        }
        
        if (key->time - time > 0 && (!maxFound || key->time < max))
        {
            max = key->time;
            maxFound = true;
        }
	}
    
    if (minFound) *minBoundary = min;
    if (maxFound) *maxBoundary = max;
    
}

void MotionPath::deleteKeyFramesAfterTime(const double time, MFnAnimCurve &curve, MAnimCurveChange *change)
{
    for (int i = curve.numKeys() - 1; i >= 0; --i)
    {
        MTime mtime = curve.time(i);
        if (mtime.as(MTime::uiUnit()) > time)
            curve.remove(i, change);
    }
}

void MotionPath::deleteKeyFramesBetweenTimes(const double startTime, const double endTime, MFnAnimCurve &curve, MAnimCurveChange *change)
{
    for (int i = curve.numKeys() - 1; i >= 0; --i)
    {
        double t = curve.time(i).as(MTime::uiUnit());
        if (t > startTime && t < endTime)
            curve.remove(i, change);
    }
}

void MotionPath::deleteAllKeyFramesAfterTime(const double time, MAnimCurveChange *change)
{
    MFnAnimCurve curveX(txPlug);
	MFnAnimCurve curveY(tyPlug);
	MFnAnimCurve curveZ(tzPlug);
    
    deleteKeyFramesAfterTime(time, curveX, change);
    deleteKeyFramesAfterTime(time, curveY, change);
    deleteKeyFramesAfterTime(time, curveZ, change);
}

void MotionPath::getKeyWorldPosition(const double keyTime, MVector &keyWorldPosition)
{
    KeyframeMapIterator keyIt = keyframesCache.find(keyTime);
	if(keyIt != keyframesCache.end())
	{
		Keyframe* key = &keyIt->second;
        keyWorldPosition = key->worldPosition;
       
	}
}

void MotionPath::deleteKeyFrameWithId(const int id, MAnimCurveChange *change)
{
    MFnAnimCurve curveX(txPlug);
	MFnAnimCurve curveY(tyPlug);
	MFnAnimCurve curveZ(tzPlug);
    
    for(KeyframeMapIterator keyIt = keyframesCache.begin(); keyIt != keyframesCache.end(); keyIt++)
	{
		Keyframe* key = &keyIt->second;
		if(key->id == id)
        {
            if (key->xKeyId != -1)
                curveX.remove(key->xKeyId, change);
            if (key->yKeyId != -1)
                curveY.remove(key->yKeyId, change);
            if (key->zKeyId != -1)
                curveZ.remove(key->zKeyId, change);
            return;
        }
	}
}

void MotionPath::deleteKeyFrameAtTime(const double time, MAnimCurveChange *change, const bool useCache)
{
    MFnAnimCurve curveX(txPlug);
    MFnAnimCurve curveY(tyPlug);
    MFnAnimCurve curveZ(tzPlug);
    
    if (!useCache)
    {
        MTime mtime(time, MTime::uiUnit());
        unsigned int xKeyID, yKeyID, zKeyID;
        
        if (curveX.find(mtime, xKeyID))
            curveX.remove(xKeyID, change);
        
        if (curveY.find(mtime, yKeyID))
            curveY.remove(yKeyID, change);
        
        if (curveZ.find(mtime, zKeyID))
            curveZ.remove(zKeyID, change);
        
        return;
    }
    
    KeyframeMapIterator keyIt = keyframesCache.find(time);
	if(keyIt != keyframesCache.end())
	{
		Keyframe* key = &keyIt->second;
        
        if (key->xKeyId != -1)
            curveX.remove(key->xKeyId, change);
        if (key->yKeyId != -1)
            curveY.remove(key->yKeyId, change);
        if (key->zKeyId != -1)
            curveZ.remove(key->zKeyId, change);
	}
}

void MotionPath::addKeyFrameAtTime(const double time, MAnimCurveChange *change, MVector *position, const bool useCache)
{
    MFnAnimCurve curveX(txPlug);
	MFnAnimCurve curveY(tyPlug);
	MFnAnimCurve curveZ(tzPlug);
    
    MVector pos;
    if (position == NULL)
        pos = getPos(time);
    else
    {
        ensureParentAndPivotMatrixAtTime(time);
        pos = multPosByParentMatrix(*position, pMatrixCache[time].inverse());
    }
    
    MTime mtime(time, MTime::uiUnit());
    KeyframeMapIterator keyIt = keyframesCache.find(time);
	if(keyIt == keyframesCache.end() || !useCache)
    {
        curveX.addKeyframe(mtime, pos.x, change);
        curveY.addKeyframe(mtime, pos.y, change);
        curveZ.addKeyframe(mtime, pos.z, change);
    }
    else
    {
        Keyframe* key = &keyIt->second;
        if (key->xKeyId != -1)
            curveX.setValue(key->xKeyId, pos.x, change);
        else
            curveX.addKeyframe(mtime, pos.x, change);
        if (key->yKeyId != -1)
            curveY.setValue(key->yKeyId, pos.y, change);
        else
            curveY.addKeyframe(mtime, pos.y, change);
        if (key->zKeyId != -1)
            curveZ.setValue(key->zKeyId, pos.z, change);
        else
            curveZ.addKeyframe(mtime, pos.z, change);
    }
}

void MotionPath::setFrameWorldPosition(const MVector &position, const double time, MAnimCurveChange *change)
{
    KeyframeMapIterator keyIt = keyframesCache.find(time);
	if(keyIt == keyframesCache.end())
        return;
    
	Keyframe* key = &keyIt->second;
    
    ensureParentAndPivotMatrixAtTime(time);
	MVector lPos = multPosByParentMatrix(position, pMatrixCache[time].inverse());
    
	MFnAnimCurve curveX(txPlug);
	MFnAnimCurve curveY(tyPlug);
	MFnAnimCurve curveZ(tzPlug);
    
    if (key->xKeyId != -1)
        curveX.setValue(key->xKeyId, lPos.x, change);
    if (key->yKeyId != -1)
        curveY.setValue(key->yKeyId, lPos.y, change);
    if (key->zKeyId != -1)
        curveZ.setValue(key->zKeyId, lPos.z, change);
}

void MotionPath::offsetWorldPosition(const MVector &offset, const double time, MAnimCurveChange *change)
{
    KeyframeMapIterator keyIt = keyframesCache.find(time);
	if(keyIt == keyframesCache.end())
        return;
    
	Keyframe* key = &keyIt->second;
    
    ensureParentAndPivotMatrixAtTime(time);
    MVector lOffset = offset * pMatrixCache[time].inverse();
    
    MFnAnimCurve curveX(txPlug);
	MFnAnimCurve curveY(tyPlug);
	MFnAnimCurve curveZ(tzPlug);

    MTime mtime(time, MTime::uiUnit());
    
    double val;
    if (key->xKeyId != -1)
    {
        val = curveX.evaluate(mtime);
        curveX.setValue(key->xKeyId, val + offset.x, change);
    }
    if (key->yKeyId != -1)
    {
        val = curveY.evaluate(mtime);
        curveY.setValue(key->yKeyId, val + offset.y, change);
    }
    if (key->zKeyId != -1)
    {
        val = curveZ.evaluate(mtime);
        curveZ.setValue(key->zKeyId, val + offset.z, change);
    }
}

void MotionPath::copyKeyFrameFromToOnCurve(MFnAnimCurve &curve, int keyId, double value, double time, MAnimCurveChange *change)
{
    double inW, outW;
    MAngle inAngle, outAngle;

    curve.getTangent(keyId, inAngle, inW, true);
    curve.getTangent(keyId, outAngle, outW, false);
    
    bool tangentsLocked = curve.tangentsLocked(keyId);
    bool weightLocked = curve.weightsLocked(keyId);
    
    MFnAnimCurve::TangentType tin = curve.inTangentType(keyId);
    MFnAnimCurve::TangentType tout = curve.outTangentType(keyId);
 
    curve.remove(keyId, change);
    
    MTime mtime;
    mtime.setValue(time);
    int newKeyId = curve.addKey(mtime, value, tin, tout, change);
    
    curve.setTangentsLocked(newKeyId, tangentsLocked, change);
    curve.setWeightsLocked(newKeyId, weightLocked, change);
    
    curve.setTangent(newKeyId, inAngle, inW, true, change);
    if (!tangentsLocked)
        curve.setTangent(newKeyId, outAngle, outW, false, change);
}

void MotionPath::copyKeyFrameFromTo(const double from, const double to, const MVector &cachedPosition, MAnimCurveChange *change)
{
    KeyframeMapIterator keyIt = keyframesCache.find(from);
	if(keyIt == keyframesCache.end())
        return;
    
    Keyframe* key = &keyIt->second;
    
    if (key->xKeyId != -1)
    {
        MFnAnimCurve curveX(txPlug);
        copyKeyFrameFromToOnCurve(curveX, key->xKeyId, cachedPosition.x, to, change);
    }
    if (key->yKeyId != -1)
    {
      	MFnAnimCurve curveY(tyPlug);
        copyKeyFrameFromToOnCurve(curveY, key->yKeyId, cachedPosition.y, to, change);
    }
    if (key->zKeyId != -1)
    {
       	MFnAnimCurve curveZ(tzPlug);
        copyKeyFrameFromToOnCurve(curveZ, key->zKeyId, cachedPosition.z, to, change);
    }
}


void MotionPath::setTangentWorldPosition(const MVector &position, const double time, Keyframe::Tangent tangentId, const MMatrix &toWorldMatrix, MAnimCurveChange *change)
{

    KeyframeMapIterator keyIt = keyframesCache.find(time);
	if(keyIt == keyframesCache.end())
        return;
    
	Keyframe* key = &keyIt->second;
    
    MVector localPosition;
    
    if (isWeighted)
    {
        localPosition = (position - key->worldPosition) * pMatrixCache[time].inverse();
    }
    else
    {
        MVector tangentPos;
        if (tangentId == Keyframe::kInTangent)
            tangentPos = key->inTangentWorldFromCurve;
        else
            tangentPos = key->outTangentWorldFromCurve;
        
        MVector vec1 = position - key->worldPosition;
        MVector vec2 = tangentPos - key->worldPosition;
        
        double lenMultiplier = vec1.length() / vec2.length();
        vec1.normalize(); vec2.normalize();
        
        MQuaternion rotation = vec2.rotateTo(vec1);
        
        MVector tangentVector;
        if (tangentId == Keyframe::kInTangent)
            tangentVector = key->inTangentWorld - MVector(MPoint(key->worldPosition) * toWorldMatrix);
        else
            tangentVector = key->outTangentWorld - MVector(MPoint(key->worldPosition) * toWorldMatrix);
        
        localPosition = tangentVector.rotateBy(rotation) * pMatrixCache[time].inverse();
        localPosition *= lenMultiplier;
    }
    
    MTime mtime(time, MTime::uiUnit());
    
    MFnAnimCurve cx(txPlug);
    MFnAnimCurve cy(tyPlug);
    MFnAnimCurve cz(tzPlug);
    
    setTangentValue(localPosition.x, key->xKeyId, cx, tangentId, mtime, change);
    setTangentValue(localPosition.y, key->yKeyId, cy, tangentId, mtime, change);
    setTangentValue(localPosition.z, key->zKeyId, cz, tangentId, mtime, change);
}

void MotionPath::setTangentValue(float value, int key, MFnAnimCurve &curve, Keyframe::Tangent tangentId, const MTime &time, MAnimCurveChange *change)
{
	unsigned int index;
	if(curve.numKeys() <= 1 || !curve.find(time, index))
		return;
    
	if(tangentId == Keyframe::kInTangent)
		value = -value;
    
	if(!curve.isWeighted())
	{
		MAngle tmpAngle; double w;
		curve.getTangent(key, tmpAngle, w, (bool)tangentId);
        
		MAngle angle(atan(value*w));
		curve.setTangent(key, angle, w, (bool)tangentId, change);
	}
	else
	{

#if defined(MAYA2018)
		MFnAnimCurve::TangentValue x, y;
#else
		float x, y;
#endif
		curve.getTangent(key, x, y, (bool)tangentId);
        
        // x is constant
        MTime convert(1.0, MTime::kSeconds);
		x *= (float) convert.as(MTime::uiUnit());
        
        y = value * 3.0;
        
		curve.setTangent(key, x, y, (bool)tangentId, change);
	}
}

void MotionPath::getTangentHandleWorldPosition(const double keyTime, const Keyframe::Tangent &tangentName, MVector &tangentWorldPosition)
{
    KeyframeMapIterator keyIt = keyframesCache.find(keyTime);
	if(keyIt != keyframesCache.end())
	{
		Keyframe* key = &keyIt->second;

        if(tangentName == Keyframe::kInTangent)
            tangentWorldPosition = key->inTangentWorldFromCurve;
        else
            tangentWorldPosition = key->outTangentWorldFromCurve;
	}
}

void MotionPath::drawTangentsForSelection(M3dView &view, CameraCache *cachePtr)
{
    for(KeyframeMapIterator keyIt = keyframesCache.begin(); keyIt != keyframesCache.end(); keyIt++)
    {
        Keyframe* key = &keyIt->second;
        
        view.pushName(key->id);
        
        if (key->showInTangent)
        {
            view.pushName((int)Keyframe::kInTangent);
            drawUtils::drawPoint(key->inTangentWorldFromCurve, GlobalSettings::frameSize);
            view.popName();
        }
        
        if (key->showOutTangent)
        {
            view.pushName((int)Keyframe::kOutTangent);
            drawUtils::drawPoint(key->outTangentWorldFromCurve, GlobalSettings::frameSize);
            view.popName();
        }
        
        view.popName();
    }
}

MVector MotionPath::getWorldPositionAtTime(const double time)
{
    ensureParentAndPivotMatrixAtTime(time);
    return multPosByParentMatrix(getPos(time), pMatrixCache[time]);
}

void MotionPath::drawKeysForSelection(M3dView &view, CameraCache* cachePtr)
{

    for(KeyframeMapIterator keyIt = keyframesCache.begin(); keyIt != keyframesCache.end(); keyIt++)
    {
        Keyframe* key = &keyIt->second;
        
        view.pushName(key->id);
        drawUtils::drawPoint(key->worldPosition, GlobalSettings::frameSize * 1.2);
        view.popName();
    }

}

void MotionPath::drawFramesForSelection(M3dView &view, CameraCache* cachePtr)
{
    MVector pos;
    for(double i = displayStartTime; i <= displayEndTime; i += 1.0)
    {
        ensureParentAndPivotMatrixAtTime(i);
        
        view.pushName(static_cast<int>(i));
        pos = multPosByParentMatrix(getPos(i), pMatrixCache[i]);
        drawUtils::drawPoint(pos, GlobalSettings::frameSize);
        view.popName();
    }

}

void MotionPath::getFramePositions(std::vector<std::pair<int, MVector>> &vec)
{
	for (double i = displayStartTime; i <= displayEndTime; i += 1.0)
	{
		ensureParentAndPivotMatrixAtTime(i);
		vec.push_back(std::pair<int, MVector>(i, multPosByParentMatrix(getPos(i), pMatrixCache[i])));
	}
}

void MotionPath::drawCurvesForSelection(M3dView &view, CameraCache* cachePtr)
{
    MMatrix currentCameraMatrix;
    if (GlobalSettings::motionPathDrawMode == GlobalSettings::kCameraSpace)
    {
        double currentTime = MAnimControl::currentTime().as(MTime::uiUnit());
        currentCameraMatrix = cachePtr->matrixCache[currentTime].inverse();
    }
    
    drawPath(view, cachePtr, currentCameraMatrix, true);
}

int MotionPath::getMinTime(MFnAnimCurve &curveX, MFnAnimCurve &curveY, MFnAnimCurve &curveZ)
{
    double minTimeX = curveX.time(0).as(MTime::uiUnit());
    double minTimeY = curveY.time(0).as(MTime::uiUnit());
    double minTimeZ = curveZ.time(0).as(MTime::uiUnit());

    if (minTimeX < minTimeY && minTimeX < minTimeZ)
        return static_cast<int>(minTimeX);
    else if (minTimeY < minTimeZ)
        return static_cast<int>(minTimeY);
    else
        return static_cast<int>(minTimeZ);
}

int MotionPath::getMaxTime(MFnAnimCurve &curveX, MFnAnimCurve &curveY, MFnAnimCurve &curveZ)
{
    double maxTimeX = curveX.time(curveX.numKeys() - 1).as(MTime::uiUnit());
    double maxTimeY = curveY.time(curveY.numKeys() - 1).as(MTime::uiUnit());
    double maxTimeZ = curveZ.time(curveZ.numKeys() - 1).as(MTime::uiUnit());
    
    if (maxTimeX > maxTimeY && maxTimeX > maxTimeZ)
        return static_cast<int>(maxTimeX);
    else if (maxTimeY > maxTimeZ)
        return static_cast<int>(maxTimeY);
    else
        return static_cast<int>(maxTimeZ);
}

void MotionPath::expandeBufferPathKeyFrames(MFnAnimCurve &curve, std::map<double, MVector> &keyFrames)
{
    for (int i = 0; i < curve.numKeys(); ++i)
    {
        double time = curve.time(i).as(MTime::uiUnit());
        MVector *vec = &keyFrames[time];
        *vec = MVector::zero;
    }
}

BufferPath MotionPath::createBufferPath()
{
    BufferPath bp;
    
    //stora tutto il range e poi storati i keyframes se ce ne sono e stop
    
    std::vector<MVector> frames;
    frames.clear();
    if (constrained)
    {
        frames.reserve(int(GlobalSettings::endTime - GlobalSettings::startTime) + 1);
        for (double i = GlobalSettings::startTime; i <= GlobalSettings::endTime; ++i)
        {
            ensureParentAndPivotMatrixAtTime(i);
            frames.push_back(MVector(pMatrixCache[i](3, 0), pMatrixCache[i](3, 1), pMatrixCache[i](3, 2)));
        }
        
        bp.setMinTime(GlobalSettings::startTime);
    }
    else
    {
        std::map<double, MVector> keyFrames;
        MStatus xStatus, yStatus, zStatus;
        MFnAnimCurve curveTX(txPlug, &xStatus), curveTY(tyPlug, &yStatus), curveTZ(tzPlug, &zStatus);
        
        int minTime = getMinTime(curveTX, curveTY, curveTZ);
        int maxTime = getMaxTime(curveTX, curveTY, curveTZ);
        
        if (minTime > GlobalSettings::startTime)
            minTime = static_cast<int>(GlobalSettings::startTime);
        
        if (maxTime < GlobalSettings::endTime)
            maxTime = static_cast<int>(GlobalSettings::endTime);
        
        frames.reserve(maxTime - minTime + 1);
        for (double i = minTime; i <= maxTime; ++i)
        {
            ensureParentAndPivotMatrixAtTime(i);
            MTime mtime(i, MTime::uiUnit());
            
            float x = xStatus == MS::kNotFound ? txPlug.asDouble() :curveTX.evaluate(mtime);
            float y = yStatus == MS::kNotFound ? tyPlug.asDouble() :curveTY.evaluate(mtime);
            float z = zStatus == MS::kNotFound ? tzPlug.asDouble() :curveTZ.evaluate(mtime);
            
            MVector vec(x, y, z);
            frames.push_back(multPosByParentMatrix(vec, pMatrixCache[i]));
        }
        
        // parse each curve and add keyframes
        keyFrames.clear();
        expandeBufferPathKeyFrames(curveTX, keyFrames);
        expandeBufferPathKeyFrames(curveTY, keyFrames);
        expandeBufferPathKeyFrames(curveTZ, keyFrames);
        
        for(BPKeyframeIterator keyIt = keyFrames.begin(); keyIt != keyFrames.end(); ++keyIt)
        {
            double time = keyIt->first;
            ensureParentAndPivotMatrixAtTime(time);
            keyIt->second = multPosByParentMatrix(getPos(time), pMatrixCache[time]);
        }
        
        bp.setKeyFrames(keyFrames);
        bp.setMinTime(minTime);
    }
    
    bp.setFrames(frames);
    
    return bp;
}

MDoubleArray MotionPath::getSelectedKeys()
{
    MDoubleArray a;
    if (selectedKeyTimes.size() == 0)
        return a;
    
    std::vector<double> times(selectedKeyTimes.size());
    std::copy(selectedKeyTimes.begin(), selectedKeyTimes.end(), times.begin());
    std::sort (times.begin(), times.end());

    for(int i = 0; i < times.size(); ++i)
        a.append(times[i]);
    return a;
}

MDoubleArray MotionPath::getKeys()
{
    MDoubleArray a;
    
    std::vector<double> times;
    times.reserve(keyframesCache.size());
    for (KeyframeMapIterator it=keyframesCache.begin() ; it != keyframesCache.end(); ++it )
        times.push_back(it->first);
    std::sort (times.begin(), times.end());
    
    for(int i = 0; i < times.size(); ++i)
        a.append(times[i]);
    return a;
}

void setExtraKeyFramesForStoringTangentsForClipboard(MFnAnimCurve &curve, KeyCopy &kc, const bool hasKey, const bool boundaryKey, const bool initialKey, const MTime &currentTime, MIntArray &keysToDelete)
{
    unsigned int keyID;
    bool hasOther = false;
    double value;
    
    if (!hasKey)
    {
        value = curve.evaluate(currentTime);
        curve.addKey(currentTime, value);
    }
    
    curve.find(currentTime, keyID);
    
    MTime otherTime = currentTime;
    if (keyID == curve.numKeys() - 1 && initialKey)
    {
        //this mess here is to take care of all possible combinations of where the actual key is inside the range of keys (this will be happening only of boundary keys)
        //int mult = boundaryKey && ((keyID == 0 && initialKey) || (keyID == curve.numKeys() - 1 && !initialKey)) ? -1: 1;
        //int mult = (keyID == 0 && initialKey) ? 1: -1;
        
        hasOther = true;
        otherTime += 1;
    }
    else if (keyID == 0 && !initialKey)
    {
        hasOther = true;
        otherTime -= 1;
    }
        
    if (hasOther)
    {
        value = curve.evaluate(otherTime);
        curve.addKey(otherTime, value);
        curve.find(otherTime, keyID);
        keysToDelete.append(keyID);
    }
    
    //we do this here, the keyID might have changed after inserting the other key
    if (!hasKey)
    {
        curve.find(currentTime, keyID);
        keysToDelete.append(keyID);
    }
}

void cleanExtraKeysForClipboard(MFnAnimCurve &curve, const MIntArray &keys)
{
    //keys will be 2 maximum
    int size = keys.length();
    if (size > 0 && size < 3)
    {
        if (size == 1)
            curve.remove(keys[0]);
        else if (keys[1] > keys[0])
        {
            curve.remove(keys[1]);
            curve.remove(keys[0]);
        }
        else
        {
            curve.remove(keys[0]);
            curve.remove(keys[1]);
        }
    }
}

double getTangentValueForClipboard(const MFnAnimCurve &curve, const int keyID, bool inTangent)
{
	if(!curve.isWeighted())
	{
		MAngle angle;
		double w1;
		curve.getTangent(keyID, angle, w1, inTangent);
		return tan(angle.asRadians()) * w1;
	}
	else
	{
#if defined(MAYA2018)
		MFnAnimCurve::TangentValue x, y;
#else
		float x, y;
#endif
        curve.getTangent(keyID, x, y, inTangent);
		return y / 3.0; // divide by the number of curves
	}
}

MVector evaluateTangentForClipboard(const MFnAnimCurve &cx, MFnAnimCurve &cy, MFnAnimCurve &cz, const int xKeyID, const int yKeyID, const int zKeyID, const bool inTangent)
{
    MVector tangent(0,0,0);
    if (xKeyID != -1)
        tangent.x = getTangentValueForClipboard(cx, xKeyID, inTangent);
    if (yKeyID != -1)
        tangent.y = getTangentValueForClipboard(cy, yKeyID, inTangent);
    if (zKeyID != -1)
        tangent.z = getTangentValueForClipboard(cz, zKeyID, inTangent);
    return tangent;
}

bool isCurveBoundaryKey(const MFnAnimCurve &curve, const MTime &time)
{
    unsigned int keyID;
    if (!curve.find(time, keyID))
        return false;
    return keyID == 0 || keyID == curve.numKeys() - 1;
}

void restoreTangents(const MFnAnimCurve &fnSource, MFnAnimCurve &fnDest)
{
    for (unsigned int index = 0; index < fnSource.numKeys(); ++index)
    {
        MTime time = fnSource.time(index);
        unsigned int keyID;
        if (fnDest.find(time, keyID))
        {
#if defined(MAYA2018)
			MFnAnimCurve::TangentValue inXTangentValue, inYTangentValue, outXTangentValue, outYTangentValue;
#else
			float inXTangentValue, inYTangentValue, outXTangentValue, outYTangentValue;
#endif
            
            fnSource.getTangent(index, inXTangentValue, inYTangentValue, true);
            fnSource.getTangent(index, outXTangentValue, outYTangentValue, false);
            
            fnDest.setTangentsLocked(keyID, false);
            fnDest.setWeightsLocked(keyID, false);
            fnDest.setTangent(keyID, inXTangentValue, inYTangentValue, true, NULL, false);
            fnDest.setTangent(keyID, outXTangentValue, outYTangentValue, false, NULL, false);
            fnDest.setTangentsLocked(keyID, fnSource.tangentsLocked(index));
            fnDest.setWeightsLocked(keyID, fnSource.weightsLocked(index));
        }
    }
}

void copyKeys(MFnAnimCurve &fnSource, MFnAnimCurve &fnDest)
{
	for (unsigned int index = 0; index < fnSource.numKeys(); ++index)
    {
        MTime time = fnSource.time(index);
        double val = fnSource.value(index);
        
        MFnAnimCurve::TangentType inTType = fnSource.inTangentType(index);
        MFnAnimCurve::TangentType outTType = fnSource.outTangentType(index);
        
#if defined(MAYA2018)
		MFnAnimCurve::TangentValue inXTangentValue, inYTangentValue, outXTangentValue, outYTangentValue;
#else
		float inXTangentValue, inYTangentValue, outXTangentValue, outYTangentValue;
#endif
        fnSource.getTangent(index, inXTangentValue, inYTangentValue, true);
        fnSource.getTangent(index, outXTangentValue, outYTangentValue, false);
        
        fnDest.addKey(time, val, inTType,  outTType);
    }
    
    restoreTangents(fnSource, fnDest);
}

void createTempCurves(MFnAnimCurve &source, MFnAnimCurve &temp1, MFnAnimCurve &temp2, MDGModifier &modifier)
{
    temp1.create(source.animCurveType(), &modifier);
    temp1.setIsWeighted(source.isWeighted());
    copyKeys(source, temp1);
    
    temp2.create(source.animCurveType(), &modifier);
    temp2.setIsWeighted(source.isWeighted());
}

void MotionPath::storeSelectedKeysInClipboard()
{
    MStatus xStatus, yStatus, zStatus;
    MFnAnimCurve curveX(txPlug, &xStatus);
	MFnAnimCurve curveY(tyPlug, &yStatus);
	MFnAnimCurve curveZ(tzPlug, &zStatus);
    
    std::vector<double> times(selectedKeyTimes.size());
    std::copy(selectedKeyTimes.begin(), selectedKeyTimes.end(), times.begin());
    std::sort (times.begin(), times.end());
    
    KeyClipboard &clipboard = KeyClipboard::getClipboard();
    clipboard.clearClipboard();
    clipboard.setClipboardSize(times.size());

    clipboard.setXWeighted(curveX.isWeighted());
    clipboard.setYWeighted(curveY.isWeighted());
    clipboard.setZWeighted(curveZ.isWeighted());
    
    //here we copy the xyz curves so we have their original tangents values stored. We'll use them at the end of this function
    // to restore the tangents on them
    MFnAnimCurve tempCurveX, tempCurveY, tempCurveZ;
    MFnAnimCurve tempCurveXAfterkeyAdd, tempCurveYAfterkeyAdd, tempCurveZAfterkeyAdd;
    MDGModifier modifier;
    if (xStatus != MS::kNotFound)
        createTempCurves(curveX, tempCurveX, tempCurveXAfterkeyAdd, modifier);

    if (yStatus != MS::kNotFound)
        createTempCurves(curveY, tempCurveY, tempCurveYAfterkeyAdd, modifier);
        
    if (zStatus != MS::kNotFound)
        createTempCurves(curveZ, tempCurveZ, tempCurveZAfterkeyAdd, modifier);
    
    /*
     In this function we create keys necessary for the correct evaluation of the key tangents.
     At the end of the function we then delete them
     */
    
    MIntArray xKeys, yKeys, zKeys;
    
    unsigned int tsize = times.size();
    for (int i = 0; i < tsize; ++i)
    {
        KeyframeMapIterator keyIt = keyframesCache.find(times[i]);
        if(keyIt != keyframesCache.end())
        {
            Keyframe* key = &keyIt->second;
            
            KeyCopy kc;
            kc.deltaTime = times[i] - times[0];
            //this is accurate
            kc.worldPos = key->worldPosition;
            
            //setting up tangents
            bool boundaryKey = i == 0 || i == tsize - 1;
            MTime currentTime(times[i], MTime::uiUnit());
            
            if (boundaryKey)
            {
                setExtraKeyFramesForStoringTangentsForClipboard(curveX, kc, key->xKeyId != -1, boundaryKey, i==0,currentTime, xKeys);
                setExtraKeyFramesForStoringTangentsForClipboard(curveY, kc, key->yKeyId != -1, boundaryKey, i==0, currentTime, yKeys);
                setExtraKeyFramesForStoringTangentsForClipboard(curveZ, kc, key->zKeyId != -1, boundaryKey, i==0, currentTime, zKeys);
            }
            
            kc.hasKeyX = key->xKeyId != -1 || boundaryKey;
            kc.hasKeyY = key->yKeyId != -1 || boundaryKey;
            kc.hasKeyZ = key->zKeyId != -1 || boundaryKey;
            
            clipboard.addKey(kc);
        }
    }
    
    // here we store temporary curves at this point
    if (xStatus != MS::kNotFound)
        copyKeys(curveX, tempCurveXAfterkeyAdd);

    if (yStatus != MS::kNotFound)
        copyKeys(curveY, tempCurveYAfterkeyAdd);

    if (zStatus != MS::kNotFound)
        copyKeys(curveZ, tempCurveZAfterkeyAdd);
    
    // While storing the tangent values in the clipboard we have to make sure the curve tangents are always restored to their original state
    int size = clipboard.getSize();
    for (int i = 0; i < size; ++i)
    {
        KeyCopy *kc = clipboard.keyCopyAt(i);
        if (kc == NULL) continue;
        
        KeyframeMapIterator keyIt = keyframesCache.find(times[0] + kc->deltaTime);
        if(keyIt != keyframesCache.end())
        {
            Keyframe* key = &keyIt->second;
            
            MTime currentTime(times[0] + kc->deltaTime, MTime::uiUnit());
            unsigned int xKeyID = -1, yKeyID = -1, zKeyID = -1;
            if (kc->hasKeyX)
                curveX.find(currentTime, xKeyID);
            if (kc->hasKeyY)
                curveY.find(currentTime, yKeyID);
            if (kc->hasKeyZ)
                curveZ.find(currentTime, zKeyID);
            
            //WE NEED TO STORE THE TANGENT BOTH IN NON WEIGHTED MODE AND IN WEIGHTED MODE
            //THIS IS BECAUSE WE DO NOT KNOW IF THE THE DESTINATION CURVE IS WEIGHTED OR NOT
            
            //storing the weighted tangent
            curveX.setIsWeighted(true);
            curveY.setIsWeighted(true);
            curveY.setIsWeighted(true);
            MVector inTangent = evaluateTangentForClipboard(curveX, curveY, curveZ, xKeyID, yKeyID, zKeyID, true);
            MVector outTangent = evaluateTangentForClipboard(curveX, curveY, curveZ, xKeyID, yKeyID, zKeyID, false);
            kc->inWeightedWorldTangent = multPosByParentMatrix(-inTangent + key->position, pMatrixCache[key->time]);
            kc->outWeightedWorldTangent = multPosByParentMatrix(outTangent + key->position, pMatrixCache[key->time]);
            
            //storing the non weighted tangent
            curveX.setIsWeighted(false);
            curveY.setIsWeighted(false);
            curveY.setIsWeighted(false);
            inTangent = evaluateTangentForClipboard(curveX, curveY, curveZ, xKeyID, yKeyID, zKeyID, true);
            outTangent = evaluateTangentForClipboard(curveX, curveY, curveZ, xKeyID, yKeyID, zKeyID, false);
            kc->inWorldTangent = multPosByParentMatrix(-inTangent + key->position, pMatrixCache[key->time]);
            kc->outWorldTangent = multPosByParentMatrix(outTangent + key->position, pMatrixCache[key->time]);
            
            //setting back the curves to their original states and restoring their values in case they are weighted
            curveX.setIsWeighted(clipboard.isXWeighed());
            curveY.setIsWeighted(clipboard.isYWeighed());
            curveZ.setIsWeighted(clipboard.isZWeighed());
            
            //restoring tangents as we might have screwed them up when changing the weight state of the curves
            restoreTangents(tempCurveXAfterkeyAdd, curveX);
            restoreTangents(tempCurveYAfterkeyAdd, curveY);
            restoreTangents(tempCurveZAfterkeyAdd, curveZ);
             
            if (kc->hasKeyX)
                kc->copyKeyTangentStatus(curveX, xKeyID, Keyframe::kAxisX);

            if (kc->hasKeyY)
                kc->copyKeyTangentStatus(curveY, yKeyID, Keyframe::kAxisY);

            if (kc->hasKeyZ)
                kc->copyKeyTangentStatus(curveZ, zKeyID, Keyframe::kAxisZ);
            
            //restoring tangents as we might have screwed them up when changing the weight state of the curves
            restoreTangents(tempCurveXAfterkeyAdd, curveX);
            restoreTangents(tempCurveYAfterkeyAdd, curveY);
            restoreTangents(tempCurveZAfterkeyAdd, curveZ);
        }
    }
    
    // cleaning the extra keys created in the first for loop
    cleanExtraKeysForClipboard(curveX, xKeys);
    cleanExtraKeysForClipboard(curveY, yKeys);
    cleanExtraKeysForClipboard(curveZ, zKeys);
    
    //restoring the tangents as they were at the beginning
    restoreTangents(tempCurveX, curveX);
    restoreTangents(tempCurveY, curveY);
    restoreTangents(tempCurveZ, curveZ);
    
    //deleting the temp curves
    modifier.undoIt();
}

bool breakTangentsForKeyCopy(const MFnAnimCurve &curve, const double time, const bool lastKey)
{
    return curve.numKeys() > 0 && curve.time(0).as(MTime::uiUnit()) < time && (curve.time(curve.numKeys() - 1).as(MTime::uiUnit()) > time || !lastKey);
}

void MotionPath::pasteKeys(const double time, const bool offset)
{
    KeyClipboard &clipboard = KeyClipboard::getClipboard();
    int size = clipboard.getSize();
    
    MVector offsetVec(0,0,0);
    if (offset)
        offsetVec = multPosByParentMatrix(getPos(time), pMatrixCache[time]);
    
    MStatus status;
    MFnAnimCurve curveX(txPlug, &status);
    if (status == MS::kNotFound)
    {
        mpManager.startDGUndoRecording();
        curveX.create(txPlug, mpManager.getDGModifierPtr());
    }
    
    MFnAnimCurve curveY(tyPlug, &status);
    if (status == MS::kNotFound)
    {
        if (mpManager.getDGModifierPtr() == NULL)
            mpManager.startDGUndoRecording();
        curveY.create(tyPlug, mpManager.getDGModifierPtr());
    }
	MFnAnimCurve curveZ(tzPlug, &status);
    if (status == MS::kNotFound)
    {
        if (mpManager.getDGModifierPtr() == NULL)
            mpManager.startDGUndoRecording();
        curveZ.create(tzPlug, mpManager.getDGModifierPtr());
    }
   
    mpManager.startAnimUndoRecording();
    
    if (clipboard.isXWeighed())
        curveX.setIsWeighted(true, mpManager.getAnimCurveChangePtr());
    if (clipboard.isYWeighed())
        curveY.setIsWeighted(true, mpManager.getAnimCurveChangePtr());
    if (clipboard.isZWeighed())
        curveZ.setIsWeighted(true, mpManager.getAnimCurveChangePtr());
    
    deleteKeyFramesBetweenTimes(time, time + clipboard.keyCopyAt(size - 1)->deltaTime, curveX, mpManager.getAnimCurveChangePtr());
    deleteKeyFramesBetweenTimes(time, time + clipboard.keyCopyAt(size - 1)->deltaTime, curveY, mpManager.getAnimCurveChangePtr());
    deleteKeyFramesBetweenTimes(time, time + clipboard.keyCopyAt(size - 1)->deltaTime, curveZ, mpManager.getAnimCurveChangePtr());
    
    //creating keyframes
    //on boundary keys set keyframes and break tangents only if there is keyframes before the first key or after the last key
    for (int i = 0; i < size; ++i)
    {
        KeyCopy *kc = clipboard.keyCopyAt(i);
        if (kc == NULL) continue;
        
        double t = time + kc->deltaTime;
        ensureParentAndPivotMatrixAtTime(t);
        MTime mtime(t, MTime::uiUnit());

        MVector pos = kc->worldPos;
        if (offset)
        {
            if (i == 0)
                pos = offsetVec;
            else
                pos = offsetVec + kc->worldPos - clipboard.keyCopyAt(0)->worldPos;
        }
        
        pos = multPosByParentMatrix(pos, pMatrixCache[t].inverse());
        bool boundaryKey = i == 0 || i == size - 1;
        
        kc->addKeyFrame(curveX, curveY, curveZ, mtime, pos, boundaryKey, mpManager.getAnimCurveChangePtr());
    }
    
    //setting tangents
    //on boundary keys set out tangents for first frame and in tangents only for the last keyframe
    for (int i = 0; i < size; ++i)
    {
        KeyCopy *kc = clipboard.keyCopyAt(i);
        if (kc == NULL) continue;
        
        double t = time + kc->deltaTime;
        MTime mtime(t, MTime::uiUnit());
        
        bool boundaryKey = i == 0  || i == size - 1;
        bool modifyInTangent = i != 0;
        bool modifyOutTangent = i != size - 1;

        bool breakTangentsX = breakTangentsForKeyCopy(curveX, t, i == size - 1);
        bool breakTangentsY = breakTangentsForKeyCopy(curveY, t, i == size - 1);
        bool breakTangentsZ = breakTangentsForKeyCopy(curveZ, t, i == size - 1);
        
        //break tangents at boundaries only if there are keyframes before/after these
        kc->setTangents(curveX, curveY, curveZ, pMatrixCache[t].inverse(), mtime, boundaryKey, modifyInTangent, modifyOutTangent, breakTangentsX, breakTangentsY, breakTangentsZ, clipboard.isXWeighed(), clipboard.isYWeighed(), clipboard.isZWeighed(), mpManager.getAnimCurveChangePtr());
    }
    
    mpManager.stopDGAndAnimUndoRecording();
}

void MotionPath::selectAllKeys()
{
    for(KeyframeMapIterator keyIt = keyframesCache.begin(); keyIt != keyframesCache.end(); keyIt++)
	{
		Keyframe* key = &keyIt->second;
        key->selectedFromTool = true;
        selectedKeyTimes.insert(key->time);
    }
}

void MotionPath::invertKeysSelection()
{
    selectedKeyTimes.clear();
    
    for(KeyframeMapIterator keyIt = keyframesCache.begin(); keyIt != keyframesCache.end(); keyIt++)
	{
		Keyframe* key = &keyIt->second;
        key->selectedFromTool = !key->selectedFromTool;
        if (key->selectedFromTool)
            selectedKeyTimes.insert(key->time);
    }
}

