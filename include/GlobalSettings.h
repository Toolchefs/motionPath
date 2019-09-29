
#ifndef GLOBALSETTINGS_H
#define GLOBALSETTINGS_H

#include <maya/MColor.h>
#include <maya/MMatrix.h>

#include <map>

class GlobalSettings
{
	public:
        enum DrawMode{
            kWorldSpace = 0,
            kCameraSpace = 1};
    
		static double startTime;
		static double endTime;
		static double framesBack;
		static double framesFront;
		static MColor pathColor;
		static MColor currentFrameColor;
		static MColor tangentColor;
		static MColor brokenTangentColor;
        static MColor bufferPathColor;
        static MColor weightedPathTangentColor;
        static MColor weightedPathColor;
        static MColor frameLabelColor;
		static double pathSize;
		static double frameSize;
		static bool showTangents;
        static bool showKeyFrames;
        static bool showKeyFrameNumbers;
        static bool showFrameNumbers;
        static bool showRotationKeyFrames;
        static bool showPath;
        static double drawTimeInterval;
        static int drawFrameInterval;
		static MMatrix cameraMatrix;
        static int portWidth;
        static int portHeight;
        static bool lockedMode;
        static bool enabled;
        static bool alternatingFrames;
        static bool lockedModeInteractive;
        static bool usePivots;
        static int strokeMode;
        static DrawMode motionPathDrawMode;
};

#endif
