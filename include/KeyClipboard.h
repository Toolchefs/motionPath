//
//  KeyClipboard.h
//  MotionPath
//
//  Created by Daniele Federico on 25/01/15.
//
//

#include <maya/MVector.h>
#include <maya/MFnAnimCurve.h>

#include <Keyframe.h>

#include <vector>

#ifndef __MotionPath__KeyClipboard__
#define __MotionPath__KeyClipboard__

class KeyCopy
{
public:
    KeyCopy();
    
    void copyKeyTangentStatus(MFnAnimCurve &curve, unsigned int keyId, const Keyframe::Axis axis);
    void addKeyFrame(MFnAnimCurve &cx, MFnAnimCurve &cy, MFnAnimCurve &cz, const MTime &time, const MVector &pos, const bool isBoundary, MAnimCurveChange *change);
    void setTangents(MFnAnimCurve &cx, MFnAnimCurve &cy, MFnAnimCurve &cz, const MMatrix &pMatrix, const MTime &time, const bool isBoundary, const bool modifyInTangent, const bool modifyOutTangent, const bool breakTangentsX, const bool breakTangentsY, const bool breakTangentsZ,  const bool xWasWeighted, const bool yWasWeighted, const bool zWasWeighted, MAnimCurveChange *change);
    void setTangent(MFnAnimCurve &curve, const float value, const unsigned int keyID, const double weight, const float x, const bool inTangent, const bool wasWeighted, MAnimCurveChange *change);
    
    double deltaTime;
    MVector worldPos;
    MVector inWorldTangent;
    MVector outWorldTangent;
    MVector inWeightedWorldTangent;
    MVector outWeightedWorldTangent;

    bool hasKeyX, hasKeyY, hasKeyZ;
    MFnAnimCurve::TangentType tinX, tinY, tinZ;
    MFnAnimCurve::TangentType toutX, toutY, toutZ;
    bool tangentsLockedX, tangentsLockedY, tangentsLockedZ;
    bool weightsLockedX, weightsLockedY, weightsLockedZ;
    
#if defined(MAYA2018)
	MFnAnimCurve::TangentValue xInX, xOutX, xInY, xOutY, xInZ, xOutZ;
#else
    float xInX, xOutX, xInY, xOutY, xInZ, xOutZ;
#endif
    double wInX, wOutX, wInY, wOutY, wInZ, wOutZ;
};

class KeyClipboard
{
    public:
        static KeyClipboard& getClipboard(){static KeyClipboard clipboard; return clipboard;}
    
        void clearClipboard(){keys.clear();};
        void setClipboardSize(int size){keys.reserve(size);};
        void addKey(KeyCopy key){keys.push_back(key);};
        int getSize(){return keys.size();};
        KeyCopy *keyCopyAt(const int index){return &keys[index];};
    
        void setXWeighted(bool value){xWeighted = value;};
        void setYWeighted(bool value){yWeighted = value;};
        void setZWeighted(bool value){zWeighted = value;};
    
        bool isXWeighed(){return xWeighted;};
        bool isYWeighed(){return yWeighted;};
        bool isZWeighed(){return zWeighted;};
    
    private:
        KeyClipboard() {};
        std::vector<KeyCopy> keys;
        bool xWeighted, yWeighted, zWeighted;
};


#endif /* defined(__MotionPath__KeyClipboard__) */
