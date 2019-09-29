
#ifndef MOTIONPATHCMD_H
#define MOTIONPATHCMD_H

#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>
#include <maya/MArgList.h>
#include <maya/MGlobal.h>
#include <maya/MColor.h>
#include <maya/MAnimCurveChange.h>
#include <maya/MDGModifier.h>

#include "MotionPathManager.h"
#include "GlobalSettings.h"

class MotionPathCmd: public MPxCommand
{ 
	public:
		MotionPathCmd();
		~MotionPathCmd();

		MStatus doIt(const MArgList&);
        bool isUndoable() const;
		MStatus undoIt();
        MStatus redoIt();
    
        static MSyntax syntaxCreator();
    
        static void* creator();

	private:
        bool animUndoable;
		MAnimCurveChange* animCurveChangePtr;

        bool dgUndoable;
        MDGModifier *dgModifierPtr;
    
        bool keySelectionUndoable;
        std::vector<MDoubleArray> initialSelection;
        std::vector<MDoubleArray> finalSelection;
    
        bool selectionUndoable;
        MSelectionList newSelection, oldSelection;
    
        void restoreKeySelection(const std::vector<MDoubleArray> &sel);
    
		MColor parseColorArg(const MArgList& args);
        bool createCurveFromBufferPath(BufferPath *bp);
    
};

#endif
