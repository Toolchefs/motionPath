

#ifndef MOTIONPATHDRAWCONTEXT_H
#define MOTIONPATHDRAWCONTEXT_H

#include "MotionPathManager.h"
#include "MotionPath.h"

#include <maya/MFn.h>
#include <maya/MPxNode.h>
#include <maya/MPxManipContainer.h>
#include <maya/MPxSelectionContext.h>
#include <maya/MPxContextCommand.h>
#include <maya/MGlobal.h>
#include <maya/MManipData.h>
#include <maya/MVector.h>
#include <maya/M3dView.h>
#include <maya/MPoint.h>
#include <maya/MMatrix.h>
#include <maya/MDagPath.h>
#include <maya/MAnimControl.h>
#include <maya/MVectorArray.h>

#include <vector>
#include <time.h>

class MotionPathDrawContextCmd: public MPxContextCommand
{
public:
    virtual MPxContext* makeObj();
    
public:
    static void* creator();
};

class MotionPathDrawContext: public MPxContext
{
public:
    enum DrawMode{
        kNoneMode = 0,
        kClickAddWorld = 1,
        kDraw = 2,
        kStroke = 3
    };
    
    MotionPathDrawContext();
    
    virtual MStatus doPress(MEvent &event);
    virtual MStatus doDrag(MEvent &event);
    virtual MStatus doRelease(MEvent &event);
    
    virtual MStatus doPress(MEvent & event, MHWRender::MUIDrawManager& drawMgr, const MHWRender::MFrameContext& context);
    virtual MStatus doDrag(MEvent & event, MHWRender::MUIDrawManager& drawMgr, const MHWRender::MFrameContext& context);
    virtual MStatus doRelease(MEvent & event, MHWRender::MUIDrawManager& drawMgr, const MHWRender::MFrameContext& context);
    
    virtual void toolOnSetup(MEvent& event);
    virtual void toolOffCleanup();
    
private:
    void drawStroke();
    void drawStrokeNew(MHWRender::MUIDrawManager& drawMgr);
    bool doPressCommon(MEvent &event, const bool old);
    bool doDragCommon(MEvent &event, const bool old);
    bool doReleaseCommon(MEvent &event, const bool old);
    
    int getStrokeDirection(MVector directionalVector, const MDoubleArray &keys, const int selectedIndex);
    MVector getkeyScreenPosition(const double index);
    MVector getClosestPointOnPolyLine(const MVector &q);
    MVector getSpreadPointOnPolyLine(const int i, const int pointSize, const double strokeLenght, const std::vector<double> &segmentLenghts);
    
    MotionPath* selectedMotionPathPtr;
    DrawMode currentMode;
    
    MGlobal::ListAdjustment listAdjustment;
    
    MVectorArray strokePoints;
    
    M3dView activeView;
    bool fsDrawn;
    
    short initialX, initialY, finalX, finalY;
    double selectedTime;
    int selectedKeyId;
    MVector keyWorldPosition;
    MPoint cameraPosition;
    
    MMatrix inverseCameraMatrix;
    
    double maxTime;
    double steppedTime;
    clock_t initialClock;
};

typedef struct st_StrokeCache
{
    MVector screenPosition;
    MVector originalScreenPosition;
    MVector originalWorldPosition;
    double time;
} StrokeCache;

#endif
