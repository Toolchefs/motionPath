//
//  CameraCache.cpp
//  MotionPath
//
//  Created by Daniele Federico on 11/05/15.
//
//

#include <maya/MAnimControl.h>
#include <maya/MFnMatrixData.h>
#include <maya/MGlobal.h>
#include <maya/MDagPath.h>

#include "CameraCache.h"
#include "GlobalSettings.h"
#include "animCurveUtils.h"



CameraCache::CameraCache()
{
    caching = false;
    initialized = false;
}

void CameraCache::initialize(const MObject &camera)
{
    MFnDependencyNode fnCamera(camera);
    MPlug worldMatrixPlugs = fnCamera.findPlug("worldMatrix");
    worldMatrixPlugs.evaluateNumElements();
    worldMatrixPlug = worldMatrixPlugs[0];
    
    caching = false;
    initialized = true;
    
    MDagPath dagPath;
    MDagPath::getAPathTo(camera, dagPath);
    dagPath.pop(1);
    
    MFnDependencyNode transformFn(dagPath.node());
    txPlug = transformFn.findPlug("translateX");
    tyPlug = transformFn.findPlug("translateY");
    tzPlug = transformFn.findPlug("translateZ");
    
    rxPlug = transformFn.findPlug("rotateX");
    ryPlug = transformFn.findPlug("rotateY");
    rzPlug = transformFn.findPlug("rotateZ");
}


void CameraCache::cacheCamera()
{
    double currentFrame = MAnimControl::currentTime().as(MTime::uiUnit());
    
    double startFrame = currentFrame - GlobalSettings::framesBack;
    double endFrame = currentFrame + GlobalSettings::framesFront;
    
    if(startFrame < GlobalSettings::startTime)	startFrame = GlobalSettings::startTime;
	if(endFrame > GlobalSettings::endTime) 	endFrame = GlobalSettings::endTime;
    
    if (worldMatrixPlug.isNull())
        return;
    
    caching = true;
    
    MTime currentTime = MAnimControl::currentTime();
    
    MStatus xStatus, yStatus, zStatus, rotxStatus, rotyStatus, rotzStatus;
    MFnAnimCurve curveX(txPlug, &xStatus);
	MFnAnimCurve curveY(tyPlug, &yStatus);
	MFnAnimCurve curveZ(tzPlug, &zStatus);
    MFnAnimCurve curveRotX(rxPlug, &rotxStatus);
	MFnAnimCurve curveRotY(ryPlug, &rotyStatus);
	MFnAnimCurve curveRotZ(rzPlug, &rotzStatus);
    
    double newXValue, newYValue, newZValue, newRotXValue, newRotYValue, newRotZValue;
    double oldXValue, oldYValue, oldZValue, oldRotXValue, oldRotYValue, oldRotZValue;
    int newKeyX, newKeyY, newKeyZ, newKeyRotX, newKeyRotY, newKeyRotZ;
    int oldKeyX, oldKeyY, oldKeyZ, oldKeyRotX, oldKeyRotY, oldKeyRotZ;
	bool xUpdated = (xStatus == MS::kSuccess), yUpdated = (yStatus == MS::kSuccess), zUpdated = (zStatus == MS::kSuccess);
	bool rotxUpdated = (rotxStatus == MS::kSuccess), rotyUpdated = (rotyStatus == MS::kSuccess), rotzUpdated = (rotzStatus == MS::kSuccess);
    if (xStatus != MS::kNotFound)
        xUpdated = animCurveUtils::updateCurve(txPlug, curveX, currentTime, oldXValue, newXValue, newKeyX, oldKeyX);
    if (yStatus != MS::kNotFound)
        yUpdated = animCurveUtils::updateCurve(tyPlug, curveY, currentTime, oldYValue, newYValue, newKeyY, oldKeyY);
    if (zStatus != MS::kNotFound)
        zUpdated = animCurveUtils::updateCurve(tzPlug, curveZ, currentTime, oldZValue, newZValue, newKeyZ, oldKeyZ);
    if (rotxStatus != MS::kNotFound)
        rotxUpdated = animCurveUtils::updateCurve(rxPlug, curveRotX, currentTime, oldRotXValue, newRotXValue, newKeyRotX, oldKeyRotX);
    if (rotyStatus != MS::kNotFound)
        rotyUpdated = animCurveUtils::updateCurve(ryPlug, curveRotY, currentTime, oldRotYValue, newRotYValue, newKeyRotY, oldKeyRotY);
    if (rotzStatus != MS::kNotFound)
        rotzUpdated = animCurveUtils::updateCurve(rzPlug, curveRotZ, currentTime, oldRotZValue, newRotZValue, newKeyRotZ, oldKeyRotZ);
    
    matrixCache.clear();
    
    for (double i = startFrame; i <= endFrame; ++i)
    {
        MTime evalTime(i, MTime::uiUnit());
        MDGContext context(evalTime);
        
        MObject val;
        worldMatrixPlug.getValue(val, context);
        matrixCache[i] = MFnMatrixData(val).matrix().inverse();
    }
    
    //restoring the previous values if a keyframe was not actually set by the user
    if (xUpdated && xStatus != MS::kNotFound)
    {
        animCurveUtils::restoreCurve(curveX, currentTime, oldXValue, newKeyX, oldKeyX);
        txPlug.setValue(newXValue);
    }
    if (yUpdated && yStatus != MS::kNotFound)
    {
        animCurveUtils::restoreCurve(curveY, currentTime, oldYValue, newKeyY, oldKeyY);
        tyPlug.setValue(newYValue);
    }
    if (zUpdated && zStatus != MS::kNotFound )
    {
        animCurveUtils::restoreCurve(curveZ, currentTime, oldZValue, newKeyZ, oldKeyZ);
        tzPlug.setValue(newZValue);
    }
    
    if (rotxUpdated && rotxStatus != MS::kNotFound)
    {
        animCurveUtils::restoreCurve(curveRotX, currentTime, oldRotXValue, newKeyRotX, oldKeyRotX);
        rxPlug.setValue(newRotXValue);
    }
    if (rotyUpdated && rotyStatus != MS::kNotFound)
    {
        animCurveUtils::restoreCurve(curveRotY, currentTime, oldRotYValue, newKeyRotY, oldKeyRotY);
        ryPlug.setValue(newRotYValue);
    }
    if (rotzUpdated && rotzStatus != MS::kNotFound )
    {
        animCurveUtils::restoreCurve(curveRotZ, currentTime, oldRotZValue, newKeyRotZ, oldKeyRotZ);
        rzPlug.setValue(newRotZValue);
    }
    
    caching = false;
}

void CameraCache::checkRangeIsCached()
{
    double currentFrame = MAnimControl::currentTime().as(MTime::uiUnit());
    
    double startFrame = currentFrame - GlobalSettings::framesBack;
    double endFrame = currentFrame + GlobalSettings::framesFront;
    
    if(startFrame < GlobalSettings::startTime)	startFrame = GlobalSettings::startTime;
	if(endFrame > GlobalSettings::endTime) 	endFrame = GlobalSettings::endTime;
    
    if (worldMatrixPlug.isNull())
        return;
    
    caching = true;

    for (double i = startFrame; i <= endFrame; ++i)
    {
        if (matrixCache.find(i) == matrixCache.end())
        {
            MTime evalTime(i, MTime::uiUnit());
            MDGContext context(evalTime);
            
            MObject val;
            worldMatrixPlug.getValue(val, context);
            matrixCache[i] = MFnMatrixData(val).matrix().inverse();
        }
    }
    caching = false;
}

void CameraCache::ensureMatricesAtTime(const double time, const bool force)
{
    if (matrixCache.find(time) == matrixCache.end() || force)
    {        
        if (worldMatrixPlug.isNull())
            return;
        
        std::string name = worldMatrixPlug.name().asChar();
        
        MTime evalTime(time, MTime::uiUnit());
        
        MDGContext context(evalTime);
        
        MObject val;
        worldMatrixPlug.getValue(val, context);
        matrixCache[time] = MFnMatrixData(val).matrix().inverse();
    }
}
