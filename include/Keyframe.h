
#ifndef KEYFRAME_H
#define KEYFRAME_H

#include <maya/MTime.h>
#include <maya/MVector.h>
#include <maya/MColor.h>
#include <maya/MIntArray.h>
#include <maya/MAnimControl.h>
#include <maya/MFnAnimCurve.h>
#include <maya/MAngle.h>
#include <maya/MQuaternion.h>
#include <maya/MGlobal.h>

#include <vector>
#include <map>

#include "GlobalSettings.h"

class Keyframe
{
	public:
		enum Axis{
				kAxisX = 0,
				kAxisY = 1,
				kAxisZ = 2};

		enum Tangent{
			kOutTangent = 0,
   			kInTangent = 1};

		Keyframe();
		bool hasTranslationXYZ();
        bool hasRotationXYZ();
		void setTangent(int keyIndex, const MFnAnimCurve &curve, const Keyframe::Axis &axisName, const Keyframe::Tangent &tangentName);
		void setTangentValue(double value, const Keyframe::Axis &axisName, const Keyframe::Tangent &tangentName);
		void setKeyId(int id, const Keyframe::Axis &axisName);
        void setRotKeyId(int id, const Keyframe::Axis &axisName);
    
        void getKeyTranslateAxis(std::vector<Keyframe::Axis> &axis);
        void getKeyRotateAxis(std::vector<Keyframe::Axis> &axis);
    
        static void getColorForAxis(const Keyframe::Axis axis, MColor &color);
    
        void setSelectedFromTool(bool value){selectedFromTool = value;};

		int id;
		MVector position;
		MVector worldPosition;
        MVector projPosition;
		double time;
        bool tangentsLocked;
        bool showInTangent;
        bool showOutTangent;
		int xKeyId;
		int yKeyId;
		int zKeyId;
    
        int xRotKeyId;
        int yRotKeyId;
        int zRotKeyId;
    
		MVector inTangent;
		MVector outTangent;
		MVector inTangentWorld;
		MVector outTangentWorld;
        MVector inTangentWorldFromCurve;
        MVector outTangentWorldFromCurve;
    
        bool selectedFromTool;
};

typedef std::map<double, Keyframe> KeyframeMap;
typedef std::map<double, Keyframe>::iterator KeyframeMapIterator;

#endif
