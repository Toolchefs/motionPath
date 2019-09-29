//
//  animCurveUtils.cpp
//  MotionPath
//
//  Created by Daniele Federico on 15/06/15.
//
//

#include "animCurveUtils.h"


bool animCurveUtils::updateCurve(const MPlug &plug, MFnAnimCurve &curve, const MTime &currentTime, double &oldValue, double &newValue, int &newKeyId, int &oldKeyId)
{
    curve.evaluate(currentTime, oldValue);
    newValue = plug.asDouble();
    
	newKeyId = -1;
	if(newValue != oldValue)
	{
		unsigned int id;
		if(curve.find(currentTime, id))
		{
			curve.setValue(id, newValue);
			oldKeyId = id;
		}
		else
			newKeyId = curve.addKeyframe(currentTime, newValue);
        
		return true;
	}
    
	return false;
}

void animCurveUtils::restoreCurve(MFnAnimCurve &curve, const MTime &currentTime, const double oldValue, const int newKeyId, const int oldKeyId)
{
    if(newKeyId > -1)
	{
		unsigned int id;
		curve.find(currentTime, id);
		curve.remove(id);
	}
	else
		curve.setValue(oldKeyId, oldValue);
}

