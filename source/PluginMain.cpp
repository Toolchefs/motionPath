
/*
MotionPath is an interactive tool to show and manipulate animation curves in maya's 3d viewport.
This plugin hooks to maya's postRenderCallback in order to draw the curves in openGL.
*/


#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>

#include "MotionPathCmd.h"
#include "MotionPathManager.h"
#include "MotionPathEditContext.h"
#include "MotionPathDrawContext.h"
#include "MotionPathOverride.h"


MotionPathManager mpManager;


MStatus initializePlugin(MObject obj)
{
	MFnPlugin plugin(obj, "ToolChefs_MotionPath", "1.0", "Any");

	MStatus status = MStatus::kSuccess;

	if ((MGlobal::mayaState() == MGlobal::kBatch) || (MGlobal::mayaState() == MGlobal::kLibraryApp))
	{
		MGlobal::displayInfo("Batch mode - tcMotionpath disabled");
		return status;
	}

	status = plugin.registerContextCommand("tcMotionPathEditContext", MotionPathEditContextCmd::creator);
	if (!status)
	{
		MGlobal::displayError("Error registering tcMotionPathEditContext");
		return status;
	}

	status = plugin.registerContextCommand("tcMotionPathDrawContext", MotionPathDrawContextCmd::creator);
	if (!status)
	{
		MGlobal::displayError("Error registering tcMotionPathDrawContext");
		return status;
	}

	status = plugin.registerCommand("tcMotionPathCmd", MotionPathCmd::creator, MotionPathCmd::syntaxCreator);
	if (!status)
	{
		MGlobal::displayError("Error registering tcMotionPathCmd");
		return status;
	}

	MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
	if (renderer)
	{
		// We register with a given name
		MotionPathRenderOverride *overridePtr = new MotionPathRenderOverride(MOTION_PATH_RENDER_OVERRIDE_NAME);
		if (overridePtr)
			renderer->registerOverride(overridePtr);
	}
	

	MString addMenu;
	addMenu +=
	"global proc loadTcMotionPath()\
	{\
		python(\"from tcMotionPath import tcMotionPath\\ntcMotionPath.run()\");\
	}\
	\
	global proc addTcMotionPathToShelf()\
	{\
		global string $gShelfTopLevel;\
		\
		string $shelves[] = `tabLayout - q - childArray $gShelfTopLevel`;\
			string $shelfName = \"\";\
		int $shelfFound = 0;\
		for ($shelfName in $shelves)\
		{\
			if ($shelfName == \"Toolchefs\")\
			{\
				$shelfFound = 1;\
			}\
		}\
		if ($shelfFound == 0)\
		{\
			addNewShelfTab \"Toolchefs\";\
		}\
		\
		string $buttons[] = `shelfLayout -q -childArray \"Toolchefs\"`;\
		int $buttonExists = 0;\
		for ($button in $buttons)\
		{\
			string $lab = `shelfButton - q - label $button`;\
			if ($lab == \"tcMotionPath\")\
			{\
				$buttonExists = 1;\
				break;\
			}\
		}\
		\
		if ($buttonExists == 0)\
		{\
			string $myButton = `shelfButton\
			- parent Toolchefs\
			- enable 1\
			- width 34\
			- height 34\
			- manage 1\
			- visible 1\
			- annotation \"Load tcMotionPath\"\
			- label \"tcMotionPath\"\
			- image1 \"tcMotionPath.png\"\
			- style \"iconOnly\"\
			- sourceType \"python\"\
			- command \"from tcMotionPath import tcMotionPath\\ntcMotionPath.run()\"`;\
		}\
	}\
	global proc addTcMotionPathToMenu()\
	{\
		global string $gMainWindow;\
		global string $showToolochefsMenuCtrl;\
		if (!(`menu - exists $showToolochefsMenuCtrl`))\
		{\
			string $name = \"Toolchefs\";\
			$showToolochefsMenuCtrl = `menu -p $gMainWindow -to true -l $name`;\
			string $tcToolsMenu = `menuItem -subMenu true -label \"Tools\" -p $showToolochefsMenuCtrl \"tcToolsMenu\"`;\
			menuItem -label \"Load tcMotionPath\" -p $tcToolsMenu -c \"loadTcMotionPath\" \"tcActiveMotionPathItem\";\
		}\
		else\
		{\
			int $deformerMenuExist = false;\
			string $defMenu = \"\";\
			string $subitems[] = `menu -q -itemArray $showToolochefsMenuCtrl`;\
			for ($item in $subitems)\
			{\
				if ($item == \"tcToolsMenu\")\
				{\
					$deformerMenuExist = true;\
					$defMenu = $item;\
					break;\
				}\
			}\
			if (!($deformerMenuExist))\
			{\
				string $tcToolsMenu = `menuItem -subMenu true -label \"Tools\" -p $showToolochefsMenuCtrl \"tcToolsMenu\"`;\
				menuItem -label \"Load tcMotionPath\" -p $tcToolsMenu -c \"loadTcMotionPath\" \"tcActiveMotionPathItem\";\
			}\
			else\
			{\
				string $subitems2[] = `menu -q -itemArray \"tcToolsMenu\"`;\
				int $deformerExists = 0;\
				for ($item in $subitems2)\
				{\
					if ($item == \"tcActiveMotionPathItem\")\
					{\
						$deformerExists = true;\
						break;\
					}\
				}\
				if (!$deformerExists)\
				{\
					menuItem -label \"Load tcMotionPath\" -p $defMenu -c \"loadTcMotionPath\" \"tcActiveMotionPathItem\";\
				}\
			}\
		}\
	};addTcMotionPathToMenu();addTcMotionPathToShelf();";
	MGlobal::executeCommand(addMenu, false, false);

	return status;
}

MStatus uninitializePlugin(MObject obj)
{
	MStatus status;
	MFnPlugin plugin(obj);

	if ((MGlobal::mayaState() == MGlobal::kBatch) || (MGlobal::mayaState() == MGlobal::kLibraryApp))
	{
		MGlobal::displayInfo("Batch mode - tcMotionpath disabled");
		return status;
	}

	mpManager.cleanupViewports();
	mpManager.removeCallbacks();

	status = plugin.deregisterContextCommand("tcMotionPathEditContext");
	if (!status)
	{
		MGlobal::displayError("Error deregistering tcMotionPathEditContext");
		return status;
	}
    
    status = plugin.deregisterContextCommand("tcMotionPathDrawContext");
	if (!status)
	{
		MGlobal::displayError("Error deregistering tcMotionPathDrawContext");
		return status;
	}

	status = plugin.deregisterCommand("tcMotionPathCmd");
	if (!status)
	{
		MGlobal::displayError("Error deregistering tcMotionPathCmd");
		return status;
	}

	MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
	if (renderer)
	{
		// Find override with the given name and deregister
		const MHWRender::MRenderOverride* overridePtr = renderer->findRenderOverride(MOTION_PATH_RENDER_OVERRIDE_NAME);
		if (overridePtr)
		{
			renderer->deregisterOverride(overridePtr);
			delete overridePtr;
		}
	}

	return status;
}
