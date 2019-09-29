//
//  animCurveUtils.h
//  MotionPath
//
//  Created by Daniele Federico on 15/06/15.
//
//

#ifndef __MotionPath__animCurveUtils__
#define __MotionPath__animCurveUtils__

#include <maya/MFnAnimCurve.h>
#include <maya/MPlug.h>

namespace animCurveUtils
{
    
    void restoreCurve(MFnAnimCurve &curve, const MTime &currentTime, const double oldValue, const int newKeyId, const int oldKeyId);
    
    bool updateCurve(const MPlug &plug, MFnAnimCurve &curve, const MTime &currentTime, double &oldValue, double &newValue, int &newKeyId, int &oldKeyId);
    
}


#endif
