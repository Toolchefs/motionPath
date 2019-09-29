//
//  BufferPath.h
//  MotionPath
//
//  Created by Daniele Federico on 06/12/14.
//
//

#ifndef BUFFERPATH_H
#define BUFFERPATH_H

#include <maya/MVector.h>
#include <maya/MFnAnimCurve.h>
#include <maya/MPlug.h>
#include <maya/MColor.h>
#include <maya/MAnimControl.h>
#include <maya/MViewport2Renderer.h>
#include <maya/M3dView.h>

#include <vector>
#include <map>

#include "GlobalSettings.h"
#include "CameraCache.h"

class BufferPath
{
    public:
        BufferPath();
    
        void draw(M3dView &view, CameraCache* cachePtr, MHWRender::MUIDrawManager* drawManager = NULL, const MHWRender::MFrameContext* frameContext = NULL);
        void setSelected(bool value){selected = value;};
        void setMinTime(double value){minTime = value;};
        void setFrames(std::vector<MVector> value){frames=value;};
        void setKeyFrames(std::map<double, MVector> value){keyFrames=value;};
        const std::vector<MVector>* getFrames(){return &frames;};
    
    private:
        void drawFrames(const double startTime, const double endTime, const MColor &curveColor, CameraCache* cachePtr, const MMatrix &currentCameraMatrix, M3dView &view, MHWRender::MUIDrawManager* drawManager, const MHWRender::MFrameContext* frameContext);
        void drawKeyFrames(const double startTime, const double endTime, const MColor &curveColor, CameraCache* cachePtr, const MMatrix &currentCameraMatrix, M3dView &view, MHWRender::MUIDrawManager* drawManager, const MHWRender::MFrameContext* frameContext);
    
        std::vector<MVector> frames;
        std::map<double, MVector> keyFrames;
        bool selected;
        MColor black;
        double minTime;   
    
};

typedef std::map<double, MVector>::iterator BPKeyframeIterator;

#endif
