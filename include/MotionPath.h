//
//  MotionPath.h
//  MotionPath
//
//  Created by Daniele Federico on 11/11/14.
//
//

#ifndef MOTIONPATH_H
#define MOTIONPATH_H

#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MDagPath.h>
#include <maya/MAnimControl.h>
#include <maya/MObjectArray.h>
#include <maya/MFnAnimCurve.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnMatrixData.h>
#include <maya/MStringArray.h>
#include <maya/M3dView.h>
#include <maya/MQuaternion.h>
#include <maya/MDagMessage.h>
#include <maya/MNodeMessage.h>
#include <maya/MViewport2Renderer.h>

#include <Keyframe.h>
#include <DrawUtils.h>
#include <BufferPath.h>
#include "KeyClipboard.h"
#include "CameraCache.h"

#include <map>

class MotionPath
{
    public:
        MotionPath(const MObject &object);
        ~MotionPath();
    
        MObject& object(){return thisObject;};
    
        bool isCacheDone(){return this->cacheDone;};
        void setTimeRange(double startTime, double endTime);
        void setDisplayTimeRange(double start, double end);
        void growParentAndPivotMatrixCache(double time, double expansion);
        void draw(M3dView &view, CameraCache* cachePtr, MHWRender::MUIDrawManager* drawManager = NULL, const MHWRender::MFrameContext* frameContext = NULL);
    
        bool isConstrained(){return constrained;};
    
        void selectKeyAtTime(const double time){selectedKeyTimes.insert(time);};
        void deselectAllKeys(){selectedKeyTimes.clear();};
        void deselectKeyAtTime(const double time){if (isKeyAtTimeSelected(time)) selectedKeyTimes.erase(time);};
        void selectAllKeys();
        void invertKeysSelection();
        bool isKeyAtTimeSelected(const double time){return selectedKeyTimes.find(time)!=selectedKeyTimes.end();};
        MDoubleArray getSelectedKeys();
        MDoubleArray getKeys();
    
        void setSelectedFromTool(bool value){selectedFromTool = value;};
        void setColorMultiplier(double colorMultiplier){this->colorMultiplier = colorMultiplier;};
    
        //void drawWorldSpace(M3dView &view, CameraCache* cachePtr, const bool selecting);
        //void drawCameraSpace(M3dView &view, CameraCache* cachePtr, const bool selecting);
        void drawPath(M3dView &view, CameraCache* cachePtr, const MMatrix &currentCameraMatrix, const bool selecting, MHWRender::MUIDrawManager* drawManager = NULL, const MHWRender::MFrameContext* frameContext = NULL);
    
        void drawCurvesForSelection(M3dView &view, CameraCache* cachePtr);
        void drawTangentsForSelection(M3dView &view, CameraCache *cachePtr);
        void drawKeysForSelection(M3dView &view, CameraCache* cachePtr);
        void drawFramesForSelection(M3dView &view, CameraCache* cachePtr);
    
        void getTangentHandleWorldPosition(const double keyTime, const Keyframe::Tangent &tangentName, MVector &tangentWorldPosition);
        void getKeyWorldPosition(const double keyTime, MVector &keyWorldPosition);
        double getTimeFromKeyId(const int id);
        void deleteKeyFrameWithId(const int id, MAnimCurveChange *change);
        void addKeyFrameAtTime(const double time, MAnimCurveChange *change, MVector *position=NULL, bool useCache=true);
        void deleteKeyFrameAtTime(const double time, MAnimCurveChange *change, const bool useCache=true);
    
        void offsetWorldPosition(const MVector &offset, const double time, MAnimCurveChange *change);
        void setFrameWorldPosition(const MVector &position, const double time, MAnimCurveChange *change);
        void setTangentWorldPosition(const MVector &position, const double time, Keyframe::Tangent tangentId, const MMatrix &toWorldMatrix, MAnimCurveChange *change);
        void rotateTangentWorldPositionAroundAxis(const double angle, const MVector &axis, const double stretch, const double time, Keyframe::Tangent tangentId, MAnimCurveChange *change);
        static void setTangentValue(float value, int key, MFnAnimCurve &curve, Keyframe::Tangent tangentName, const MTime &time, MAnimCurveChange *change);
    
        void copyKeyFrameFromTo(const double from, const double to, const MVector &cachedPosition, MAnimCurveChange *change);
    
        void deleteAllKeyFramesAfterTime(const double time, MAnimCurveChange *change);
        void deleteKeyFramesBetweenTimes(const double startTime, const double endTime, MFnAnimCurve &curve, MAnimCurveChange *change);
    
        void getBoundariesForTime(const double time, double *minBoundary, double *maxBoundary);
        int getNumKeyFrames();
        MVector getPos(double time);
        MVector getWorldPositionAtTime(const double time);
    
        void clearParentMatrixCache();
        void cacheParentMatrixRange();
    
        void setIsDrawing(const bool value){isDrawing = value;};
        void setEndrawingTime(const double value){endDrawingTime = value;};
    
        BufferPath createBufferPath();
    
        static bool hasAnimationLayers(const MObject &object);
    
        void storeSelectedKeysInClipboard();
        void pasteKeys(const double time, const bool offset);
    
        static MVector multPosByParentMatrix(const MVector &vec, const MMatrix &mat);
    
        static MMatrix getMatrixFromPlug(const MPlug &matrixPlug, const MTime &t);
    
        void addWorldMatrixCallback();
        void removeWorldMartrixCallback();
    
        bool getWorldSpaceCallbackCalled();
        void setWorldSpaceCallbackCalled(const bool value, const MObject &tempAncestorNode);

		KeyframeMap *keyFramesCachePtr() { return &keyframesCache; }
		void getFramePositions(std::vector<std::pair<int, MVector>> &vec);

    private:
    
        MObject thisObject;
        MPlug txPlug, tyPlug, tzPlug, rxPlug, ryPlug, rzPlug;
        double startTime, endTime;
        double displayStartTime, displayEndTime;
        double startTimeCached, endTimeCached;
        double colorMultiplier;
        bool constrained;
        bool selectedFromTool;
        MPlug pMatrixPlug;
        std::map<double, MMatrix> pMatrixCache;
        bool cacheDone;
        bool worldSpaceCallbackCalled;
        KeyframeMap keyframesCache;
    
        MCallbackId worldMatrixCallbackId;
    
        MObject tempAncestorNode;
    
        std::map<double, MPoint> frameScreenSpacePositions;
    
        //Pivot stuff
        MPlug rpxPlug, rpyPlug, rpzPlug, rptxPlug, rptyPlug, rptzPlug;
        //
    
        bool isWeighted;
    
        std::set<double> selectedKeyTimes;
    
        bool isDrawing;
        double endDrawingTime;
    
        void ensureParentAndPivotMatrixAtTime(const double time);
        MMatrix getPMatrixAtTime(const MTime &evalTime);
        MMatrix getPivotMatrix(const MTime &evalTime);
        MVector getVectorFromPlugs(const MTime &evalTime, const MPlug &x, const MPlug &y, const MPlug &z);
    
        bool isCurveTypeAnimatable(MFnAnimCurve::AnimCurveType type);
        bool isConstrained(const MFnDagNode &dagNodeFn);
        void findParentMatrixPlug(const MObject &transform, const bool isConstrained, MPlug &matrixPlug);
        void findPivotPlugs(MFnDependencyNode &depNodFn);
        void expandKeyFramesCache(const MFnAnimCurve &curve, const Keyframe::Axis &axisName, bool isTranslate);
        void cacheKeyFrames(const MFnAnimCurve &curveTX, const MFnAnimCurve &curveTY, const MFnAnimCurve &curveTZ, const MFnAnimCurve &curveRX, const MFnAnimCurve &curveRY, const MFnAnimCurve &curveRZ, CameraCache* cachePtr, const MMatrix &currentCameraMatrix);
        void setShowInOutTangents(const MFnAnimCurve &curveTX, const MFnAnimCurve &curveTY, const MFnAnimCurve &curveTZ);
        bool showTangent(const double time, const int firstId, const double firstTime, const int secondId, const double secondTime);
    
        void copyKeyFrameFromToOnCurve(MFnAnimCurve &curve, int keyId, double value, double time, MAnimCurveChange *change);
    
        void deleteKeyFramesAfterTime(const double time, MFnAnimCurve &curve, MAnimCurveChange *change);
    
        void drawFrames(CameraCache* cachePtr, const MMatrix &currentCameraMatrix, M3dView &view, MHWRender::MUIDrawManager* drawManager, const MHWRender::MFrameContext* frameContext);
        void drawFrame(const double time, const MVector &pos, const MColor &color, double alpha, M3dView &view, MMatrix &currentCameraMatrix, MHWRender::MUIDrawManager* drawManager, const MHWRender::MFrameContext* frameContext);
        void drawCurrentFrame(CameraCache* cachePtr, const MMatrix &currentCameraMatrix, M3dView &view, MHWRender::MUIDrawManager* drawManager, const MHWRender::MFrameContext* frameContext);
        void drawKeyFrames(CameraCache *cachePtr, MMatrix &currentCameraMatrix, MHWRender::MUIDrawManager* drawManager, const MHWRender::MFrameContext* frameContext);
        void drawTangents(M3dView &view, MMatrix &currentCameraMatrix, MHWRender::MUIDrawManager* drawManager, const MHWRender::MFrameContext* frameContext);
        void drawFrameLabels(M3dView &view, CameraCache* cachePtr, const MMatrix &currentCameraMatrix, MHWRender::MUIDrawManager* drawManager, const MHWRender::MFrameContext* frameContext);
    
        /*void drawCameraSpaceFrames(CameraCache* cachePtr, std::map<double, MPoint> &framePositions, std::map<double, MPoint> &frameScreenSpacePositions, const MMatrix &currentCameraMatrix);
        void drawCameraSpaceCurrentFrame(CameraCache* cachePtr, std::map<double, MPoint> &frameScreenSpacePositions,const double currentTimeValue);
        void drawCameraSpaceFrameLabels(M3dView view, CameraCache* cachePtr, const MMatrix &currentCamerMatrix, std::map<double, MPoint> &positions, std::map<double, MPoint> &screenSpacePositions);
        void drawCameraSpaceKeyFrames(CameraCache* cachePtr, std::vector<Keyframe *>keys);
        void convertKeyFramesInCameraSpace(CameraCache* cachePtr, std::map<double, MPoint> &framePositions, std::map<double, MPoint> &frameScreenSpacePositions, const MMatrix &currentCameraMatrix, std::vector<Keyframe *> &keys);
        void drawCameraSpaceTangents(CameraCache* cachePtr, const MMatrix &currentCameraMatrix);
         */
    
        int getMinTime(MFnAnimCurve &curveX, MFnAnimCurve &curveY, MFnAnimCurve &curveZ);
        int getMaxTime(MFnAnimCurve &curveX, MFnAnimCurve &curveY, MFnAnimCurve &curveZ);
        void expandeBufferPathKeyFrames(MFnAnimCurve &curve, std::map<double, MVector> &keyFrames);
    
        static void worldMatrixChangedCallback(MObject& transformNode, MDagMessage::MatrixModifiedFlags& modified, void* data);
        void cacheParentMatrixRangeForWorldCallback(MObject &transformNode);

};

#endif
