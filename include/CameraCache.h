//
//  CameraCache.h
//  MotionPath
//
//  Created by Daniele Federico on 11/05/15.
//
//

#ifndef MotionPath_CameraCache_h
#define MotionPath_CameraCache_h

#include <maya/MFloatMatrix.h>
#include <maya/MMatrix.h>
#include <maya/MFnCamera.h>
#include <maya/MDagPath.h>
#include <maya/MPlug.h>

#include <map>

class CameraCache
{
    public:
        CameraCache();
        ~CameraCache(){}
    
        std::map<double, MMatrix> matrixCache;
        //std::map<double, MMatrix> projMatrixCache;
        int portWidth;
        int portHeight;
    
        bool isCaching(){return caching;}
        bool isInitialized(){return initialized;}
    
        void initialize(const MObject &camera);
    
        void cacheCamera();
        void ensureMatricesAtTime(const double time, const bool force=false);
        void checkRangeIsCached();
    
    private:
        bool caching, initialized;
        MPlug worldMatrixPlug;
        MPlug txPlug, tyPlug, tzPlug;
        MPlug rxPlug, ryPlug, rzPlug;
};

typedef std::map<std::string, CameraCache> CameraCacheMap;
typedef std::map<std::string, CameraCache>::iterator CameraCacheMapIterator;

#endif
