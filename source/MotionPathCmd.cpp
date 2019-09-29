#include <maya/MPointArray.h>
#include <maya/MFnNurbsCurve.h>
#include "MotionPathCmd.h"

extern MotionPathManager mpManager;


void* MotionPathCmd::creator()
{
	return new MotionPathCmd;
}

MotionPathCmd::MotionPathCmd()
{
	this->animCurveChangePtr = NULL;
    this->dgModifierPtr = NULL;
	this->animUndoable = false;
    this->dgUndoable = false;
    this->keySelectionUndoable = false;
    this->selectionUndoable = false;
}

MotionPathCmd::~MotionPathCmd()
{
	if (this->animCurveChangePtr)
		delete this->animCurveChangePtr;
    
    if (this->dgModifierPtr)
  		delete this->dgModifierPtr;
}

bool MotionPathCmd::isUndoable() const
{
	return this->animUndoable || this->dgUndoable || this->keySelectionUndoable|| this->selectionUndoable;
}

MColor getColorFromArg(const MArgDatabase &argData, const char *flagName)
{
    double r, g, b;
    argData.getFlagArgument(flagName, 0, r);
    argData.getFlagArgument(flagName, 1, g);
    argData.getFlagArgument(flagName, 2, b);
    return MColor(r, g, b);
}


MSyntax MotionPathCmd::syntaxCreator()
{
    MSyntax syntax;
	
    syntax.addFlag("-e", "-enable", MSyntax::kBoolean);
    syntax.addFlag("-gsl", "-getCurrentSL", MSyntax::kNoArg);
    syntax.addFlag("-rdt", "-refreshdt", MSyntax::kNoArg);
    
    syntax.addFlag("-bf", "-framesBefore",  MSyntax::kLong);
    syntax.addFlag("-af", "-framesAfter",  MSyntax::kLong);
    syntax.addFlag("-tfr", "-frameRange", MSyntax::kLong, MSyntax::kLong);
    syntax.addFlag("-st", "-showTangents", MSyntax::kBoolean);
    syntax.addFlag("-sp", "-showPath", MSyntax::kBoolean);
    syntax.addFlag("-sk", "-showKeyFrames", MSyntax::kBoolean);
    syntax.addFlag("-srk", "-showRotationKeyFrames", MSyntax::kBoolean);
    syntax.addFlag("-skn", "-showKeyFrameNumbers", MSyntax::kBoolean);
    syntax.addFlag("-sfn", "-showFrameNumbers", MSyntax::kBoolean);
    
    syntax.addFlag("-alf", "-alternatingFrames", MSyntax::kBoolean);
    syntax.addFlag("-up", "-usePivots", MSyntax::kBoolean);
    
    syntax.addFlag("-abp", "-addBufferPaths", MSyntax::kNoArg);
    syntax.addFlag("-dbs", "-deleteAllBufferPaths", MSyntax::kNoArg);
    syntax.addFlag("-dbi", "-deleteBufferPathAtIndex", MSyntax::kLong);
    syntax.addFlag("-sbp", "-selectBufferPathAtIndex", MSyntax::kLong);
    syntax.addFlag("-dbp", "-deselectBufferPathAtIndex", MSyntax::kLong);

    syntax.addFlag("-fs", "-frameSize", MSyntax::kDouble);
    syntax.addFlag("-ps", "-pathSize", MSyntax::kDouble);
    
    syntax.addFlag("-mdm", "-drawMode", MSyntax::kLong);
    
    syntax.addFlag("-cfc", "-currentFrameColor", MSyntax::kDouble, MSyntax::kDouble, MSyntax::kDouble);
    syntax.addFlag("-pc", "-pathColor", MSyntax::kDouble, MSyntax::kDouble, MSyntax::kDouble);
    syntax.addFlag("-tc", "-tangentColor", MSyntax::kDouble, MSyntax::kDouble, MSyntax::kDouble);
    syntax.addFlag("-btc", "-brokenTangentColor", MSyntax::kDouble, MSyntax::kDouble, MSyntax::kDouble);
    syntax.addFlag("-bpc", "-bufferPathColor", MSyntax::kDouble, MSyntax::kDouble, MSyntax::kDouble);
    syntax.addFlag("-wpc", "-weightedPathColor", MSyntax::kDouble, MSyntax::kDouble, MSyntax::kDouble);
    syntax.addFlag("-wtc", "-weightedPathTangentColor", MSyntax::kDouble, MSyntax::kDouble, MSyntax::kDouble);
    syntax.addFlag("-fnc", "-frameNumberColor", MSyntax::kDouble, MSyntax::kDouble, MSyntax::kDouble);
    
    syntax.addFlag("-dti", "-drawTimeInterval", MSyntax::kDouble);
    syntax.addFlag("-fi", "-frameInterval", MSyntax::kLong);
    syntax.addFlag("-sm", "-strokeMode", MSyntax::kLong);
    
    syntax.addFlag("-sdc", "-storeDGAndCurveChange", MSyntax::kNoArg);
    
    syntax.addFlag("-cbp", "-convertBufferPath", MSyntax::kLong);

    syntax.addFlag("-ksc", "-keySelectionChanged", MSyntax::kNoArg);
    syntax.addFlag("-sc", "-selectionChanged", MSyntax::kNoArg);
    
    syntax.addFlag("-l", "-lockedMode", MSyntax::kBoolean);
    syntax.addFlag("-lmi", "-lockedModeInteractive", MSyntax::kBoolean);
    syntax.addFlag("-rls", "-refreshLockedSelection", MSyntax::kNoArg);
    
    syntax.useSelectionAsDefault(false);
    syntax.setObjectType(MSyntax::kSelectionList, 0);
    
	return syntax;
}

MStatus MotionPathCmd::doIt(const MArgList& args)
{
    MArgDatabase argData(syntax(), args);
    
	if(argData.isFlagSet("-enable"))
	{
		bool enable;
        argData.getFlagArgument("-enable", 0, enable);

		if(enable)
		{           
			mpManager.setupViewports();
			mpManager.addCallbacks();

            MotionPathManager::selectionChangeCallback(&mpManager);
		}
		else
		{
			mpManager.cleanupViewports();
			mpManager.removeCallbacks();
		}
        
        GlobalSettings::enabled = enable;
	}
	else if(argData.isFlagSet("-getCurrentSL"))
	{
		this->setResult(mpManager.getSelectionList());
	}
	else if(argData.isFlagSet("-frameRange"))
	{
        int initialFrame, lastFrame;
        argData.getFlagArgument("-frameRange", 0, initialFrame);
        argData.getFlagArgument("-frameRange", 1, lastFrame);
		mpManager.setTimeRange(initialFrame, lastFrame);
	}
	else if(argData.isFlagSet("-framesBefore"))
	{
		int beforeFrame;
        argData.getFlagArgument("-framesBefore", 0, beforeFrame);

        if (beforeFrame < 0)
            beforeFrame = 0;
        
        GlobalSettings::framesBack = beforeFrame;
	}
    else if(argData.isFlagSet("-framesAfter"))
	{
		int afterFrame;
        argData.getFlagArgument("-framesAfter", 0, afterFrame);
        
        if (afterFrame < 0)
            afterFrame = 0;
        
        GlobalSettings::framesFront = afterFrame;
	}
	else if(argData.isFlagSet("-rdt"))
	{
		mpManager.refreshDisplayTimeRange();
	}
    else if(argData.isFlagSet("-showTangents"))
    {
        bool showTangents;
        argData.getFlagArgument("-showTangents", 0, showTangents);
        GlobalSettings::showTangents = showTangents;
    }
    else if(argData.isFlagSet("-showKeyFrames"))
    {
        bool showKeyFrames;
        argData.getFlagArgument("-showKeyFrames", 0, showKeyFrames);
        GlobalSettings::showKeyFrames = showKeyFrames;
    }
    else if(argData.isFlagSet("-showPath"))
    {
        bool showPath;
        argData.getFlagArgument("-showPath", 0, showPath);
        GlobalSettings::showPath = showPath;
    }
    else if (argData.isFlagSet("-showRotationKeyFrames"))
    {
        bool showRotationKeyFrames;
        argData.getFlagArgument("-showRotationKeyFrames", 0, showRotationKeyFrames);
        GlobalSettings::showRotationKeyFrames = showRotationKeyFrames;
    }
    else if (argData.isFlagSet("-showKeyFrameNumbers"))
    {
        bool showKeyFrameNumbers;
        argData.getFlagArgument("-showKeyFrameNumbers", 0, showKeyFrameNumbers);
        GlobalSettings::showKeyFrameNumbers = showKeyFrameNumbers;
    }
    else if (argData.isFlagSet("-showFrameNumbers"))
    {
        bool showFrameNumbers;
        argData.getFlagArgument("-showFrameNumbers", 0, showFrameNumbers);
        GlobalSettings::showFrameNumbers = showFrameNumbers;
    }
    else if (argData.isFlagSet("-alternatingFrames"))
    {
        bool alternatingFrames;
        argData.getFlagArgument("-alternatingFrames", 0, alternatingFrames);
        GlobalSettings::alternatingFrames = alternatingFrames;
    }
    else if (argData.isFlagSet("-usePivots"))
    {
        bool usePivots;
        argData.getFlagArgument("-usePivots", 0, usePivots);
        GlobalSettings::usePivots = usePivots;
        
        mpManager.clearParentMatrixCaches();
        mpManager.refreshDisplayTimeRange();
    }
    else if (argData.isFlagSet("-pathSize"))
    {
        double pathSize;
        argData.getFlagArgument("-pathSize", 0, pathSize);
        GlobalSettings::pathSize = pathSize;
    }
    else if (argData.isFlagSet("-frameSize"))
    {
        double frameSize;
        argData.getFlagArgument("-frameSize", 0, frameSize);
        GlobalSettings::frameSize = frameSize;
    }
    else if (argData.isFlagSet("-drawTimeInterval"))
    {
        double drawTimeInterval;
        argData.getFlagArgument("-drawTimeInterval", 0, drawTimeInterval);
        GlobalSettings::drawTimeInterval = drawTimeInterval;
    }
    else if (argData.isFlagSet("-strokeMode"))
    {
        int strokeMode;
        argData.getFlagArgument("-strokeMode", 0, strokeMode);
        GlobalSettings::strokeMode = strokeMode;
    }
    else if (argData.isFlagSet("-drawMode"))
    {
        int drawMode;
        argData.getFlagArgument("-drawMode", 0, drawMode);
        
        if (GlobalSettings::motionPathDrawMode != drawMode)
        {
            /*
            if (drawMode == 0)
                mpManager.destroyCameraCachesAndCameraCallbacks();
            else
                mpManager.createCameraCachesAndCameraCallbacks();
            */
            
            if (drawMode < 0)
                drawMode = 0;
                
            if (drawMode > 1)
                drawMode = 1;
            
            mpManager.cacheCameras();
            GlobalSettings::motionPathDrawMode = (GlobalSettings::DrawMode) drawMode;
        }
    }
    else if (argData.isFlagSet("-frameInterval"))
    {
        int frameInterval;
        argData.getFlagArgument("-frameInterval", 0, frameInterval);
        GlobalSettings::drawFrameInterval = frameInterval;
    }
    else if (argData.isFlagSet("-currentFrameColor"))
    {
        GlobalSettings::currentFrameColor = getColorFromArg(argData, "-currentFrameColor");
    }
    else if (argData.isFlagSet("-pathColor"))
    {
        GlobalSettings::pathColor = getColorFromArg(argData, "-pathColor");
    }
    else if (argData.isFlagSet("-tangentColor"))
    {
        GlobalSettings::tangentColor = getColorFromArg(argData, "-tangentColor");
    }
	else if (argData.isFlagSet("-brokenTangentColor"))
    {
        GlobalSettings::brokenTangentColor = getColorFromArg(argData, "-brokenTangentColor");
    }
    else if (argData.isFlagSet("-bufferPathColor"))
    {
        GlobalSettings::bufferPathColor = getColorFromArg(argData, "-bufferPathColor");
    }
    else if (argData.isFlagSet("-weightedPathColor"))
    {
        GlobalSettings::weightedPathColor = getColorFromArg(argData, "-weightedPathColor");
    }
    else if (argData.isFlagSet("-weightedPathTangentColor"))
    {
        GlobalSettings::weightedPathTangentColor = getColorFromArg(argData, "-weightedPathTangentColor");
    }
    else if (argData.isFlagSet("-frameNumberColor"))
    {
        GlobalSettings::frameLabelColor = getColorFromArg(argData, "-frameNumberColor");
    }
    else if (argData.isFlagSet("-addBufferPaths"))
    {
        mpManager.addBufferPaths();
    }
    else if (argData.isFlagSet("-deleteAllBufferPaths"))
    {
        mpManager.deleteAllBufferPaths();
    }
    else if (argData.isFlagSet("-deleteBufferPathAtIndex"))
    {
        int index;
        argData.getFlagArgument("-deleteBufferPathAtIndex", 0, index);
        mpManager.deleteBufferPathAtIndex(index);
    }
    else if (argData.isFlagSet("-selectBufferPathAtIndex"))
    {
        int index;
        argData.getFlagArgument("-selectBufferPathAtIndex", 0, index);
        mpManager.setSelectStateForBufferPathAtIndex(index, true);
    }
    else if (argData.isFlagSet("-deselectBufferPathAtIndex"))
    {
        int index;
        argData.getFlagArgument("-deselectBufferPathAtIndex", 0, index);
        mpManager.setSelectStateForBufferPathAtIndex(index, false);
    }
    else if(argData.isFlagSet("-lockedMode"))
	{
        bool value;
        argData.getFlagArgument("-lockedMode", 0, value);
        GlobalSettings::lockedMode = value;
        if (value)
            mpManager.createMotionPathWorldCallback();
        else
            mpManager.destroyMotionPathWorldCallback();
	}
    else if(argData.isFlagSet("-lockedModeInteractive"))
	{
        bool lockedModeInteractive;
        argData.getFlagArgument("-lockedModeInteractive", 0, lockedModeInteractive);
        GlobalSettings::lockedModeInteractive = lockedModeInteractive;
	}
    else if(argData.isFlagSet("-refreshLockedSelection"))
	{
        mpManager.clearParentMatrixCaches();
        mpManager.refreshDisplayTimeRange();
	}
	else if (argData.isFlagSet("-storeDGAndCurveChange"))
	{
        dgModifierPtr = mpManager.getDGModifierPtr();
        animCurveChangePtr = mpManager.getAnimCurveChangePtr();

		if(dgModifierPtr)
			dgUndoable = true;
		else
			dgModifierPtr = NULL;
        
        if(animCurveChangePtr)
			animUndoable = true;
		else
			animCurveChangePtr = NULL;
	}
    else if (argData.isFlagSet("-convertBufferPath"))
    {
        int index;
        argData.getFlagArgument("-convertBufferPath", 0, index);
        BufferPath *bp = mpManager.getBufferPathAtIndex(index);
        
        if (bp == NULL)
        {
            MGlobal::displayError("tcMotionPathCmd: wrong buffer path index given.");
            return MS::kFailure;
        }
        
        mpManager.startDGUndoRecording();
        if (!createCurveFromBufferPath(bp))
        {
            mpManager.stopDGAndAnimUndoRecording();
            MGlobal::displayError("tcMotionPathCmd: could not convert curve.");
            return MS::kFailure;
        }
        mpManager.stopDGAndAnimUndoRecording();
    }
    else if (argData.isFlagSet("-keySelectionChanged"))
    {
        keySelectionUndoable = true;
        mpManager.getPreviousKeySelection(initialSelection);
        mpManager.getCurrentKeySelection(finalSelection);
        return redoIt();
    }
    else if(argData.isFlagSet("-selectionChanged"))
	{
        if (argData.getObjects(newSelection) == MS::kFailure)
        {
            MGlobal::displayError("tcMotionPathCmd: failed while parsing arguments");
            return MS::kFailure;
        }
        
        selectionUndoable = true;
        mpManager.getPreviousKeySelection(initialSelection);
        MGlobal::getActiveSelectionList(oldSelection);
        return redoIt();
    }
	else
	{
		MGlobal::displayError("tcMotionPathCmd: wrong flag.");
        return MS::kFailure;
	}

	return MS::kSuccess;
}

MStatus MotionPathCmd::redoIt()
{
    if(animUndoable && animCurveChangePtr)
		animCurveChangePtr->redoIt();
    
    if (dgUndoable && dgModifierPtr)
        dgModifierPtr->doIt();
    
    if (keySelectionUndoable)
    {
        if (GlobalSettings::enabled)
        {
            restoreKeySelection(finalSelection);
            mpManager.storePreviousKeySelection();
        }
    }
    
    if (selectionUndoable)
        MGlobal::setActiveSelectionList(newSelection);
    
	return MS::kSuccess;
}

MStatus MotionPathCmd::undoIt()
{
	if(animUndoable && animCurveChangePtr)
        animCurveChangePtr->undoIt();
    
    if (dgUndoable && dgModifierPtr)
        dgModifierPtr->undoIt();
    
    if (keySelectionUndoable)
    {
        if (GlobalSettings::enabled)
        {
            restoreKeySelection(initialSelection);
            mpManager.storePreviousKeySelection();
        }
    }
    
    if (selectionUndoable)
    {
        MGlobal::setActiveSelectionList(oldSelection);
        if (GlobalSettings::enabled)
        {
            restoreKeySelection(initialSelection);
            mpManager.storePreviousKeySelection();
        }
    }

	return MS::kSuccess;
}

void MotionPathCmd::restoreKeySelection(const std::vector<MDoubleArray> &sel)
{
    for (int i = 0; i < sel.size(); ++i)
    {
        MotionPath *motionPathPtr = mpManager.getMotionPathPtr(i);
        if (motionPathPtr)
        {
            motionPathPtr->deselectAllKeys();
            for (int j = 0; j < sel[i].length(); ++j)
                motionPathPtr->selectKeyAtTime(sel[i][j]);
        }
    }
    
    M3dView::active3dView().refresh();
}

bool MotionPathCmd::createCurveFromBufferPath(BufferPath *bp)
{
    const std::vector<MVector>* points = bp->getFrames();
    if (points == NULL) return false;
    
    MString cmd = "curve -d 1 ";
    MString cvsStr = "";
    for (unsigned int i = 0; i < points->size(); ++i)
    {
        MVector v = points->at(i);
        cvsStr += MString("-p ") + v.x + " " + v.y + " " + v.z + " ";
    }
    
    MString knotsStr = "";
    for (unsigned int i = 0; i < points->size(); i++)
        knotsStr += MString("-k ") + ((double) i) + " ";
    
    MDGModifier *dg = mpManager.getDGModifierPtr();
    dg->commandToExecute(cmd + cvsStr + knotsStr);
    return dg->doIt() == MS::kSuccess;
}

