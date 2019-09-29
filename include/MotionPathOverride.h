#pragma once

#include <maya/MString.h>
#include <maya/MViewport2Renderer.h>
#include <maya/MRenderTargetManager.h>

#define MOTION_PATH_RENDER_OVERRIDE_NAME "ToolchefsMotionPathOverride"


class MotionPathUserOperation : public MHWRender::MHUDRender
{
public:
	MotionPathUserOperation(const MString &name);
	virtual ~MotionPathUserOperation();

	virtual MStatus execute(const MHWRender::MDrawContext & drawContext);
	virtual bool hasUIDrawables() const;
	virtual void addUIDrawables(
		MHWRender::MUIDrawManager& drawManager,
		const MHWRender::MFrameContext& frameContext);

	void setPanelName(const MString & name)
	{
		mPanelName.set(name.asChar());
	}


private:
	MString mPanelName;
};



class MotionPathRenderOverride : public MHWRender::MRenderOverride
{
public:
	// operation names
	static const MString kMotionPathPassName;

	MotionPathRenderOverride(const MString & name);
	virtual ~MotionPathRenderOverride();
	virtual MHWRender::DrawAPI supportedDrawAPIs() const;

	// Basic setup and cleanup
	virtual MStatus setup(const MString & destination);
	virtual MStatus cleanup();

	// UI name
	virtual MString uiName() const
	{
		return mUIName;
	}

protected:

	// UI name 
	MString mUIName;
	MString mPanelName;
	MotionPathUserOperation* mMotionPathOp;
};



