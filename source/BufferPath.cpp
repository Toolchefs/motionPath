//
//  BufferPath.cpp
//  MotionPath
//
//  Created by Daniele Federico on 06/12/14.
//
//

#include "BufferPath.h"
#include "GlobalSettings.h"
#include "DrawUtils.h"
#include "Vp2DrawUtils.h"

BufferPath::BufferPath()
{
    black = MColor(0,0,0);
    selected = false;
}

void BufferPath::drawFrames(const double startTime, const double endTime, const MColor &curveColor, CameraCache* cachePtr, const MMatrix &currentCameraMatrix, M3dView &view, MHWRender::MUIDrawManager* drawManager, const MHWRender::MFrameContext* frameContext)
{
    int frameSize = frames.size();

    if (GlobalSettings::motionPathDrawMode == GlobalSettings::kCameraSpace)
        cachePtr->ensureMatricesAtTime(startTime);
    
	for(double i = startTime + 1; i <= endTime; i += 1.0)
	{
        if (i <= minTime || i >= minTime + frameSize)
            continue;
        
        int index = static_cast<int>(i - minTime);
        
        MVector pos1 = frames[index];
        MVector pos2 = frames[index-1];
        if (GlobalSettings::motionPathDrawMode == GlobalSettings::kCameraSpace)
        {
            cachePtr->ensureMatricesAtTime(i);
            pos1 = MPoint(pos1) * cachePtr->matrixCache[i] * currentCameraMatrix;
            pos2 = MPoint(pos2) * cachePtr->matrixCache[i-1] * currentCameraMatrix;
        }
        
		if (GlobalSettings::showPath)
		{
			if (drawManager)
				VP2DrawUtils::drawLineWithColor(pos1, pos2, GlobalSettings::pathSize, curveColor, currentCameraMatrix, drawManager, frameContext);
			else
				drawUtils::drawLineWithColor(pos1, pos2, GlobalSettings::pathSize, curveColor);
		}
        
		if (drawManager)
			VP2DrawUtils::drawPointWithColor(pos2, GlobalSettings::frameSize, curveColor, currentCameraMatrix, drawManager, frameContext);
		else
			drawUtils::drawPointWithColor(pos2, GlobalSettings::frameSize, curveColor);

        if (i == endTime || i == minTime + frameSize - 1)
		{
			if (drawManager)
				VP2DrawUtils::drawPointWithColor(pos1, GlobalSettings::frameSize, curveColor, currentCameraMatrix, drawManager, frameContext);
			else
				drawUtils::drawPointWithColor(pos1, GlobalSettings::frameSize, curveColor);
		}
	}
}

void BufferPath::drawKeyFrames(const double startTime, const double endTime, const MColor &curveColor, CameraCache* cachePtr, const MMatrix &currentCameraMatrix, M3dView &view, MHWRender::MUIDrawManager* drawManager, const MHWRender::MFrameContext* frameContext)
{
    for(BPKeyframeIterator keyIt = keyFrames.begin(); keyIt != keyFrames.end(); ++keyIt)
    {
        double time = keyIt->first;
        if (time >= startTime && time <= endTime)
        {
            MVector pos = keyIt->second;
            if (GlobalSettings::motionPathDrawMode == GlobalSettings::kCameraSpace)
            {
                cachePtr->ensureMatricesAtTime(time);
                pos = MPoint(pos) * cachePtr->matrixCache[time] * currentCameraMatrix;
            }
            
			if (drawManager)
				VP2DrawUtils::drawPointWithColor(pos, GlobalSettings::frameSize, curveColor, GlobalSettings::cameraMatrix, drawManager, frameContext);
			else
				drawUtils::drawPointWithColor(pos, GlobalSettings::frameSize * 1.5, curveColor);
        }
    }
}

void BufferPath::draw(M3dView &view, CameraCache* cachePtr, MHWRender::MUIDrawManager* drawManager, const MHWRender::MFrameContext* frameContext)
{
    MColor curveColor = GlobalSettings::bufferPathColor;
    
    if(selected)
    {
        curveColor.r = 1.0 - curveColor.r;
        curveColor.g = 1.0 - curveColor.g;
        curveColor.b = 1.0 - curveColor.b;
    }
    
    curveColor.a = 0.5;
    
    double currentTime = MAnimControl::currentTime().as(MTime::uiUnit());
    
    double startTime = currentTime - GlobalSettings::framesBack;
    double endTime = currentTime + GlobalSettings::framesFront;
    
    MMatrix currentCameraMatrix;
    if (GlobalSettings::motionPathDrawMode == GlobalSettings::kCameraSpace)
    {
        double currentTime = MAnimControl::currentTime().as(MTime::uiUnit());
        cachePtr->ensureMatricesAtTime(currentTime);
        currentCameraMatrix = cachePtr->matrixCache[currentTime].inverse();
    }
    
    drawFrames(startTime, endTime, curveColor, cachePtr, GlobalSettings::cameraMatrix, view, drawManager, frameContext);
    if (GlobalSettings::showKeyFrames)
        drawKeyFrames(startTime, endTime, curveColor, cachePtr, GlobalSettings::cameraMatrix, view, drawManager, frameContext);
    
    //draw current frame
    if (currentTime >= minTime && currentTime <= minTime + frames.size())
    {
        MColor currentColor = GlobalSettings::currentFrameColor * 0.8;
        currentColor.a = 0.7;
        
		int numFrame = static_cast<int>(currentTime) - static_cast<int>(minTime);
		if (numFrame < 0 || numFrame > frames.size() - 1)
			return;

        MVector pos = frames[static_cast<int>(currentTime) - static_cast<int>(minTime)];
        if (GlobalSettings::motionPathDrawMode == GlobalSettings::kCameraSpace)
        {
            cachePtr->ensureMatricesAtTime(currentTime);
            pos = MPoint(pos) * cachePtr->matrixCache[currentTime] * currentCameraMatrix;
        }
        
		if (drawManager)
			VP2DrawUtils::drawPointWithColor(pos, GlobalSettings::frameSize, curveColor, GlobalSettings::cameraMatrix, drawManager, frameContext);
		else
			drawUtils::drawPointWithColor(pos, GlobalSettings::frameSize * 1.6, currentColor);
    }
}
