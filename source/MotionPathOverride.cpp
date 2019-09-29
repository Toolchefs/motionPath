#include "MotionPathOverride.h"
#include "MotionPathManager.h"

#include <maya/M3dView.h>

extern MotionPathManager mpManager;

const MString MotionPathRenderOverride::kMotionPathPassName = "tMotionPathOverride";


MotionPathRenderOverride::MotionPathRenderOverride(const MString & name)
	: MRenderOverride(name)
	, mUIName("Toolchefs MotionPath")
{
	MHWRender::MRenderer *theRenderer = MHWRender::MRenderer::theRenderer();
	if (!theRenderer)
		return;

	// Create a new set of operations as required
	MHWRender::MRenderer::theRenderer()->getStandardViewportOperations(mOperations);

	mMotionPathOp = new MotionPathUserOperation(kMotionPathPassName);

	mMotionPathOp->setEnabled(true); // swirl is disabled by default

	mOperations.insertAfter(MHWRender::MRenderOperation::kStandardSceneName, mMotionPathOp);
}

MotionPathRenderOverride::~MotionPathRenderOverride()
{
}

MHWRender::DrawAPI MotionPathRenderOverride::supportedDrawAPIs() const
{
	return MHWRender::kAllDevices;
}

MStatus MotionPathRenderOverride::setup(const MString & destination)
{
	mPanelName.set(destination.asChar());
	mMotionPathOp->setPanelName(mPanelName);
	return MRenderOverride::setup(destination);
}

MStatus MotionPathRenderOverride::cleanup()
{
	return MRenderOverride::cleanup();
}


MotionPathUserOperation::MotionPathUserOperation(const MString &name)
	: MHUDRender()
{
}

MotionPathUserOperation::~MotionPathUserOperation()
{
}

MStatus MotionPathUserOperation::execute(const MHWRender::MDrawContext & drawContext)
{
	return MStatus::kSuccess;
}


bool MotionPathUserOperation::hasUIDrawables() const
{
	return true;
}

void MotionPathUserOperation::addUIDrawables(
	MHWRender::MUIDrawManager& drawManager,
	const MHWRender::MFrameContext& frameContext)
{
	M3dView view;
	if (mPanelName.length() && (M3dView::getM3dViewFromModelPanel(mPanelName, view)))
	{
		MDagPath camera;
		view.getCamera(camera);

		CameraCache* cachePtr = NULL;

		GlobalSettings::cameraMatrix = camera.inclusiveMatrix();

		//world space mode
		if (GlobalSettings::motionPathDrawMode == GlobalSettings::kWorldSpace)
		{
			GlobalSettings::portWidth = view.portWidth();
			GlobalSettings::portHeight = view.portHeight();
		}
		else // camera space mode
		{
			cachePtr = mpManager.getCameraCachePtrFromView(view);
			if (!cachePtr)
				return;

			if (!cachePtr->isInitialized())
			{
				cachePtr->initialize(camera.node());
				cachePtr->cacheCamera();
			}

			cachePtr->portWidth = view.portWidth();
			cachePtr->portHeight = view.portHeight();
		}

		drawManager.beginDrawInXray();

		mpManager.drawBufferPaths(view, cachePtr, &drawManager, &frameContext);
		mpManager.drawPaths(view, cachePtr, &drawManager, &frameContext);
		
		drawManager.endDrawInXray();
	}
	
}

