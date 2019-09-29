//
//  KeyClipboard.cpp
//  MotionPath
//
//  Created by Daniele Federico on 27/01/15.
//
//

#include "MotionPath.h"
#include "KeyClipboard.h"

KeyCopy::KeyCopy()
{
    deltaTime = 0.0;
    
    //we need to store these as well, as otherwise we would be missing some extra information which we need when setting weighted keys
    xInX = xOutX = xInY = xOutY = xInZ = xOutZ = 0;
    wInX = wOutX = wInY = wOutY = wInZ = wOutZ = 0;
    
    hasKeyX = hasKeyY = hasKeyZ = false;
    tangentsLockedX = tangentsLockedY = tangentsLockedZ = true;
    weightsLockedX = weightsLockedY = weightsLockedZ = true;
}
    
void KeyCopy::copyKeyTangentStatus(MFnAnimCurve &curve, unsigned int keyId, const Keyframe::Axis axis)
{
    bool tangentsLocked = curve.tangentsLocked(keyId);
    bool weightLocked = curve.weightsLocked(keyId);
    MFnAnimCurve::TangentType tin = curve.inTangentType(keyId);
    MFnAnimCurve::TangentType tout = curve.outTangentType(keyId);
    
    //WE NEED TO STORE THE EXTRA VALUES IN NON WEIGHTED MODE AND IN WEIGHTED MODE
    //THIS IS BECAUSE WE DO NOT KNOW IF THE THE DESTINATION CURVE IS WEIGHTED OR NOT
    
    bool isWeighted = curve.isWeighted();
    
    MAngle angle;
#if defined(MAYA2018)
	MFnAnimCurve::TangentValue y;
#else
    float y;
#endif
    switch (axis)
    {
        case Keyframe::kAxisX:
            tangentsLockedX = tangentsLocked;
            weightsLockedX = weightLocked;
            
            curve.setIsWeighted(true);		
			curve.getTangent(keyId, xInX, y, Keyframe::kInTangent);
			curve.getTangent(keyId, xOutX, y, Keyframe::kOutTangent);
            curve.setIsWeighted(false);
            curve.getTangent(keyId, angle, wInX, Keyframe::kInTangent);
            curve.getTangent(keyId, angle, wOutX, Keyframe::kOutTangent);

            tinX = tin;
            toutX = tout;
            break;
        
        case Keyframe::kAxisY:
            tangentsLockedY = tangentsLocked;
            weightsLockedY = weightLocked;
            
            curve.setIsWeighted(true);
            curve.getTangent(keyId, xInY, y, Keyframe::kInTangent);
            curve.getTangent(keyId, xOutY, y, Keyframe::kOutTangent);
            curve.setIsWeighted(false);
            curve.getTangent(keyId, angle, wInY, Keyframe::kInTangent);
            curve.getTangent(keyId, angle, wOutY, Keyframe::kOutTangent);
            
            tinY = tin; 
            toutY = tout;
            break;

        case Keyframe::kAxisZ:
            tangentsLockedZ = tangentsLocked;
            weightsLockedZ = weightLocked;
            
            curve.setIsWeighted(true);
            curve.getTangent(keyId, xInZ, y, Keyframe::kInTangent);
            curve.getTangent(keyId, xOutZ, y, Keyframe::kOutTangent);
            curve.setIsWeighted(false);
            curve.getTangent(keyId, angle, wInZ, Keyframe::kInTangent);
            curve.getTangent(keyId, angle, wOutZ, Keyframe::kOutTangent);
            
            tinZ = tin;
            toutZ = tout;
            break;
    }
    
    //setting back the initial curve state
    curve.setIsWeighted(isWeighted);
}

void KeyCopy::addKeyFrame(MFnAnimCurve &cx, MFnAnimCurve &cy, MFnAnimCurve &cz, const MTime &time, const MVector &pos, const bool isBoundary, MAnimCurveChange *change)
{
    unsigned int keyID;
    
    // we break the tangents as they could be various, we are going to restore their state in the setTangents method
    
    if (hasKeyX || isBoundary)
    {
        if (!cx.find(time, keyID))
            keyID = cx.addKey(time, pos.x, tinX, toutX, change);
        else
            cx.setValue(keyID, pos.x, change);
    
        cx.setTangentsLocked(keyID, false, change);
        cx.setWeightsLocked(keyID, false, change);
    }
    
    if (hasKeyY || isBoundary)
    {
        if (!cy.find(time, keyID))
            keyID = cy.addKey(time, pos.y, tinY, toutY, change);
        else
            cy.setValue(keyID, pos.y, change);
        
        cy.setTangentsLocked(keyID, false, change);
        cy.setWeightsLocked(keyID, false, change);
    }
            
    if (hasKeyZ || isBoundary)
    {
        if (!cz.find(time, keyID))
            keyID = cz.addKey(time, pos.z, tinZ, toutZ, change);
        else
            cz.setValue(keyID, pos.z, change);
        
        cz.setTangentsLocked(keyID, false, change);
        cz.setWeightsLocked(keyID, false, change);
    }
    
}

void KeyCopy::setTangent(MFnAnimCurve &curve, const float value, const unsigned int keyID, const double weight, const float x, const bool inTangent, const bool wasWeighted, MAnimCurveChange *change)
{
    if (!curve.isWeighted())
    {
        MAngle angle(atan(value*weight));
        curve.setTangent(keyID, angle, weight, inTangent, change);
    }
    else
    {
        float y = value * 3.0;
        
        MTime convert(1.0, MTime::kSeconds);
		float _x = x * (float) convert.as(MTime::uiUnit());
        
        curve.setTangent(keyID, _x, y, inTangent, change);
    }
}

void KeyCopy::setTangents(MFnAnimCurve &cx, MFnAnimCurve &cy, MFnAnimCurve &cz, const MMatrix &pMatrix, const MTime &time, const bool isBoundary, const bool modifyInTangent, const bool modifyOutTangent, const bool breakTangentsX, const bool breakTangentsY, const bool breakTangentsZ, const bool xWasWeighted, const bool yWasWeighted, const bool zWasWeighted, MAnimCurveChange *change)
{
    unsigned int keyID;
    MVector in = (inWorldTangent - worldPos) * pMatrix;
    MVector out = (outWorldTangent - worldPos) * pMatrix;
    MVector inWeighted = (inWeightedWorldTangent - worldPos) * pMatrix;
    MVector outWeighted = (outWeightedWorldTangent - worldPos) * pMatrix;
    
    //we want to edit the out tanget, only when they are broken or we must modify the out tangent or we didn't modify the in tangent
    
    float inValue, outValue;
    if (hasKeyX || isBoundary)
    {
        if (cx.isWeighted())
        {
            inValue = inWeighted.x;
            outValue = outWeighted.x;
        }
        else
        {
            inValue = in.x;
            outValue = out.x;
        }
            
        
        cx.find(time, keyID);
        if (modifyInTangent)
            setTangent(cx, -inValue, keyID, wInX, xInX, true, xWasWeighted, change);
        if (modifyOutTangent)
            setTangent(cx, outValue, keyID, wOutX, xOutX, false, xWasWeighted, change);
        
        if (breakTangentsX)
            cx.setTangentsLocked(keyID, false, change);
        else
            cx.setTangentsLocked(keyID, tangentsLockedX, change);
        cx.setWeightsLocked(keyID, weightsLockedX, change);
    }
    
    if (hasKeyY || isBoundary)
    {
        if (cy.isWeighted())
        {
            inValue = inWeighted.y;
            outValue = outWeighted.y;
        }
        else
        {
            inValue = in.y;
            outValue = out.y;
        }
        
        cy.find(time, keyID);
        
        if (modifyInTangent)
            setTangent(cy, -inValue, keyID, wInY, xInY, true, yWasWeighted, change);
        if (modifyOutTangent)
            setTangent(cy, outValue, keyID, wOutY, xOutY, false, yWasWeighted, change);
        
        if (breakTangentsY)
            cy.setTangentsLocked(keyID, false, change);
        else
            cy.setTangentsLocked(keyID, tangentsLockedY, change);
        cy.setWeightsLocked(keyID, weightsLockedY, change);
    }
    
    if (hasKeyZ || isBoundary)
    {
        if (cz.isWeighted())
        {
            inValue = inWeighted.z;
            outValue = outWeighted.z;
        }
        else
        {
            inValue = in.z;
            outValue = out.z;
        }
        
        cz.find(time, keyID);
        
        if (modifyInTangent)
            setTangent(cz, -in.z, keyID, wInZ, xInZ, true, zWasWeighted, change);
        if (modifyOutTangent)
            setTangent(cz, out.z, keyID,  wOutZ, xOutZ, false, zWasWeighted, change);
        
        if (breakTangentsZ)
            cz.setTangentsLocked(keyID, false, change);
        else
            cz.setTangentsLocked(keyID, tangentsLockedZ, change);
        cz.setWeightsLocked(keyID, weightsLockedZ, change);
    }
}

