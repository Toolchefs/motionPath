
#ifndef DRAWUTILS_H
#define DRAWUTILS_H

#include <maya/M3dView.h>
#include <maya/MVector.h>
#include <maya/MColor.h>
#include <maya/MDagPath.h>
#include <maya/MMatrix.h>
#include <maya/MGlobal.h>

#include <set>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include <Keyframe.h>
#include <CameraCache.h>

namespace drawUtils
{
	void drawLineStipple(const MVector &origin, const MVector &target, float lineWidth, const MColor &color);
	
	void drawLine(const MVector &origin, const MVector &target, float lineWidth);
	
	void drawLineWithColor(const MVector &origin, const MVector &target, float lineWidth, const MColor &color);
    
	void drawPoint(const MVector &point, float size);
	
	void drawPointWithColor(const MVector &point, float size, const MColor &color);
    
    void drawKeyFramePoints(KeyframeMap &keyframesCache, const float size, const double colorMultiplier, const int portWidth, const int portHeight, const bool showRotationKeyframes);
    
    void drawKeyFrames(std::vector<Keyframe *> keys, const float size, const double colorMultiplier,const int portWidth, const int portHeight, const bool showRotationKeyframes);
    
    void convertWorldSpaceToCameraSpace(CameraCache* cachePtr, std::map<double, MPoint> &positions, std::map<double, MPoint> &screenSpacePositions);
    
    void drawFrameLabel(double frame, const MVector &framePos, M3dView &view, const double sizeOffset, const MColor &color, const MMatrix &refMatrix);
    /*
    void drawCameraSpaceFrames(CameraCache* cachePtr, const MColor &color, std::map<double, MPoint> &positions, const double startFrame, const double endFrame);

    void drawCameraSpacePointWithColor(CameraCache *cachePtr, const MColor &color, const MPoint &position, const double size);
    
    void convertKeyFrameTangentsToCameraSpace(CameraCache *cachePtr, KeyframeMap &keyframesCache, const MMatrix &currentCameraMatrix);
    
    void drawCameraSpaceKeyFrameTangents(CameraCache *cachePtr, KeyframeMap &keyframesCache);
    
    void drawCameraSpaceFramesForSelection(M3dView &view, CameraCache *cachePtr, std::map<double, MPoint> &frameScreenSpacePositions);
    
    void drawCameraSpaceKeyFramesForSelection(M3dView &view, CameraCache *cachePtr, KeyframeMap &keyframesCache);
    
    void drawCameraSpaceKeyFrameTangentsForSelection(M3dView &view, CameraCache *cachePtr, KeyframeMap &keyframesCache);
     */
}

#endif
