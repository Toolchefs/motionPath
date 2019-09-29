

#ifndef MOTIONPATHEDITCONTEXT_H
#define MOTIONPATHEDITCONTEXT_H

#include "MotionPathEditContextMenuWidget.h"
#include "MotionPathManager.h"
#include "MotionPath.h"

#include <maya/MFn.h>
#include <maya/MPxNode.h>
#include <maya/MPxContext.h>
#include <maya/MPxContextCommand.h>
#include <maya/MGlobal.h>
#include <maya/MManipData.h>
#include <maya/MVector.h>
#include <maya/M3dView.h>
#include <maya/MPoint.h>
#include <maya/MMatrix.h>
#include <maya/MDagPath.h>
#include <maya/MAnimControl.h>
#include <maya/MUIDrawManager.h>
#include <maya/MFrameContext.h>

#include <vector>
#include <time.h>

class MotionPathEditContextCmd: public MPxContextCommand
{
	public:
        MotionPathEditContextCmd();
		virtual MPxContext* makeObj();
		static void* creator();
};

class MotionPathEditContext: public MPxContext
{
	public:
		enum EditMode{
				kNoneMode = 0,
				kFrameEditMode = 1,
				kTangentEditMode = 2,
                kShiftKeyMode = 3};

		MotionPathEditContext();
		~MotionPathEditContext();

		virtual MStatus doPress(MEvent &event);
		virtual MStatus doDrag(MEvent &event);
		virtual MStatus doRelease(MEvent &event);
    
        virtual MStatus doPress(MEvent & event, MHWRender::MUIDrawManager& drawMgr, const MHWRender::MFrameContext& context);
        virtual MStatus doDrag(MEvent & event, MHWRender::MUIDrawManager& drawMgr, const MHWRender::MFrameContext& context);
        virtual MStatus doRelease(MEvent & event, MHWRender::MUIDrawManager& drawMgr, const MHWRender::MFrameContext& context);
    
        virtual void toolOnSetup(MEvent& event);
        virtual void toolOffCleanup();
    

	private:
        int findPrefEditAxisFromVector(const MVector vec);
    
        void modifySelection(const MDoubleArray &selectedTimes, const bool ctrl, const bool shift);
    
        bool doPressCommon(MEvent &event, const bool old);
        void doDragCommon(MEvent &event, const bool old);
        void doReleaseCommon(MEvent &event, const bool old);

		MotionPath* selectedMotionPathPtr;
        EditMode currentMode;
    
        bool startedRecording;
    
        MGlobal::ListAdjustment listAdjustment;
    
        bool alongPreferredAxis;
        int prefEditAxis;
    
        bool shiftCachedDone;
    
        short initialX, initialY;
        short finalX, finalY;
    
        double lastSelectedTime;
        int selectedTangentId;
        MVector tangentWorldPosition;
        MVector keyWorldPosition;
        MVector lastWorldPosition;
        MPoint cameraPosition;
    
        M3dView activeView;
        bool fsDrawn;
    
        MMatrix inverseCameraMatrix;
    
        ContextMenuWidget*	ctxMenuWidget;
};

#endif
