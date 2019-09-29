//
//  MotionPathManager.cpp
//  MotionPath
//
//  Created by Daniele Federico on 09/11/14.
//
//

#include <maya/MModelMessage.h>
#include <maya/MDagMessage.h>
#include <maya/MNodeMessage.h>
#include <maya/MHardwareRenderer.h>
#include <maya/MGLFunctionTable.h>
#include <maya/MViewport2Renderer.h>
#include <maya/MDrawContext.h>

#include "MotionPathManager.h"
#include "GlobalSettings.h"

MotionPathManager::MotionPathManager()
{
    animCurveChangePtr = NULL;
    cacheDone = true;

    pathArray.clear();
    selectionObjects.clear();
    bufferPathArray.clear();
    cameraCache.clear();
}

MotionPathManager::~MotionPathManager()
{}

int MotionPathManager::panelRegistered(const MString &panelName)
{
    for (unsigned int i = 0; i < registeredPanels.size(); ++i)
        if (registeredPanels[i].name == panelName)
            return i;
    return -1;
}

void MotionPathManager::setupViewport(const MString &panelName)
{
    MStatus status;
    
    MCallbackId destroyId = MUiMessage::add3dViewDestroyMsgCallback(panelName, MotionPathManager::viewDestroyCallback, (void*) this, &status);
    
    if(destroyId == 0)
        status.perror(MString("Could not create view deletion callback for panel ") + panelName);
    
    MCallbackId postId = MUiMessage::add3dViewPostRenderMsgCallback(panelName, MotionPathManager::viewPostRenderCallback, (void*) this, &status);
    
    if(postId == 0)
        status.perror(MString("Could not create view post render callback for panel ") + panelName);
    
    MCallbackId cameraChangedId = MUiMessage::addCameraChangedCallback(panelName, MotionPathManager::viewCameraChanged, (void*) this, &status);
    if(cameraChangedId == 0)
        status.perror(MString("Could not create view camera changed callback for panel ") + panelName);
    
    M3dView view;
    status = M3dView::getM3dViewFromModelPanel(panelName, view);
    MDagPath camera;
    view.getCamera(camera);
    
    std::string cameraName(camera.fullPathName().asChar());
    createCameraCacheForCamera(camera);
    
    MCallbackId worldCallbackID  = MDagMessage::addWorldMatrixModifiedCallback(camera, cameraWorldMatrixChangedCallback,(void *) &cameraCache[cameraName], &status);
    
    MObject cameraObject = camera.node();
    MCallbackId cameraNameChangedId = MNodeMessage::addNameChangedCallback(cameraObject, MotionPathManager::viewCameraNameChanged, (void *) this, &status);
    
    RegisteredPanel rp;
    rp.name = panelName;
    rp.cameraWorldMatrixCallbackId = worldCallbackID;
    rp.cameraNameChangedId = cameraNameChangedId;
    rp.cameraChangedId = cameraChangedId;
    rp.destroyPanelCallbackId = destroyId;
    rp.postRenderCallbackId = postId;
    registeredPanels.push_back(rp);
}

void MotionPathManager::setupViewports()
{
    MStringArray panelNames;
    
	MGlobal::executeCommand("getPanel -type \"modelPanel\";", panelNames);
    
    for(unsigned int i = 0; i < panelNames.length(); i++)
		if (panelRegistered(panelNames[i]) == -1)
            setupViewport(panelNames[i]);
}

/*
void MotionPathManager::destroyCameraCachesAndCameraCallbacks()
{
    cameraCache.clear();
    for (unsigned int i = 0; i < registeredPanels.size(); ++i)
    {
        if (registeredPanels[i].cameraWorldMatrixCallbackId)
        {
            MMessage::removeCallback(registeredPanels[i].cameraWorldMatrixCallbackId);
            registeredPanels[i].cameraWorldMatrixCallbackId = 0;
        }
        
        if (registeredPanels[i].cameraNameChangedId)
        {
            MMessage::removeCallback(registeredPanels[i].cameraNameChangedId);
            registeredPanels[i].cameraNameChangedId = 0;
        }
        
        if (registeredPanels[i].cameraChangedId)
        {
            MMessage::removeCallback(registeredPanels[i].cameraChangedId);
            registeredPanels[i].cameraChangedId = 0;
        }
    }
}


void MotionPathManager::createCameraCachesAndCameraCallbacks()
{
    MStringArray panelNames;
	MGlobal::executeCommand("getPanel -type \"modelPanel\";", panelNames);
    
    for(unsigned int i = 0; i < panelNames.length(); i++)
	{
        int index = panelRegistered(panelNames[i]);
		if (index == -1)
            setupViewport(panelNames[i]);
        else
        {
            MStatus status;
        
            registeredPanels[index].cameraChangedId = MUiMessage::addCameraChangedCallback(registeredPanels[index].name, MotionPathManager::viewCameraChanged, (void*) this, &status);
            if(registeredPanels[index].cameraChangedId == 0)
                status.perror(MString("Could not create view camera changed callback for panel ") + registeredPanels[index].name);
            
            M3dView view;
            status = M3dView::getM3dViewFromModelPanel(registeredPanels[index].name, view);
            MDagPath camera;
            view.getCamera(camera);
            
            std::string cameraName(camera.fullPathName().asChar());
            createCameraCacheForCamera(camera);
            
            registeredPanels[index].cameraWorldMatrixCallbackId  = MDagMessage::addWorldMatrixModifiedCallback(camera, cameraWorldMatrixChangedCallback,(void *) &cameraCache[cameraName], &status);
            
            MObject cameraObject = camera.node();
            registeredPanels[index].cameraNameChangedId = MNodeMessage::addNameChangedCallback(cameraObject, MotionPathManager::viewCameraNameChanged, (void *) this, &status);
        }
    }
}
 */

void MotionPathManager::createCameraCacheForCamera(const MDagPath &camera)
{
    std::string cameraName(camera.fullPathName().asChar());
    if (cameraCache.find(cameraName) == cameraCache.end())
    {
        CameraCache cc;
        cameraCache[cameraName] = cc;
    }
}

void MotionPathManager::viewCameraNameChanged(MObject &node, const MString &str, void *  data)
{
    if (!GlobalSettings::enabled || GlobalSettings::motionPathDrawMode == GlobalSettings::kWorldSpace)
        return;
    
    MotionPathManager* mpManager = (MotionPathManager*) data;
    
	if(mpManager)
    {
        MDagPath camera;
        MDagPath::getAPathTo(node, camera);
        mpManager->createCameraCacheForCamera(camera);
    }
}

void MotionPathManager::refreshCameraCallbackForPanel(const MString &panelName, MDagPath &camera)
{
    for (unsigned int i = 0; i < registeredPanels.size(); ++i)
        if (registeredPanels[i].name == panelName)
        {
            if (registeredPanels[i].cameraWorldMatrixCallbackId)
            {
                MMessage::removeCallback(registeredPanels[i].cameraWorldMatrixCallbackId);
                registeredPanels[i].cameraWorldMatrixCallbackId = 0;
            }
            if (registeredPanels[i].cameraNameChangedId)
            {
                MMessage::removeCallback(registeredPanels[i].cameraNameChangedId);
                registeredPanels[i].cameraNameChangedId = 0;
            }
            
            if (camera.isValid())
            {
                MStatus status;
                std::string cameraName(camera.fullPathName().asChar());
                registeredPanels[i].cameraWorldMatrixCallbackId = MDagMessage::addWorldMatrixModifiedCallback(camera, cameraWorldMatrixChangedCallback,(void *) &cameraCache[cameraName], &status);

                MObject cameraObject = camera.node();
                registeredPanels[i].cameraNameChangedId = MNodeMessage::addNameChangedCallback(cameraObject, MotionPathManager::viewCameraNameChanged, (void *) this, &status);
            }
            return;
        }
}

void MotionPathManager::viewCameraChanged(const MString &str, MObject &node, void *data)
{
    if (!GlobalSettings::enabled || GlobalSettings::motionPathDrawMode == GlobalSettings::kWorldSpace)
        return;
    
    MotionPathManager* mpManager = (MotionPathManager*) data;
    
	if(mpManager)
    {
        MDagPath camera;
        MDagPath::getAPathTo(node, camera);
        
        //camera callbacks
        if (camera.isValid())
            mpManager->createCameraCacheForCamera(camera);
        
        //panel callbacks
        mpManager->refreshCameraCallbackForPanel(str, camera);
    }
}

void MotionPathManager::cacheCameras()
{
     for (unsigned int i = 0; i < registeredPanels.size(); ++i)
     {
         M3dView view;
         M3dView::getM3dViewFromModelPanel(registeredPanels[i].name, view);
         MDagPath camera;
         view.getCamera(camera);
         
         if (camera.isValid())
         {
             createCameraCacheForCamera(camera);
             
             std::string cameraName(camera.fullPathName().asChar());
             cameraCache[cameraName].cacheCamera();
         }
     }
}

void MotionPathManager::cameraWorldMatrixChangedCallback(MObject& transformNode, MDagMessage::MatrixModifiedFlags& modified, void* data)
{
    if (!GlobalSettings::enabled || GlobalSettings::motionPathDrawMode == GlobalSettings::kWorldSpace)
        return;
    
    CameraCache *cachePtr  = (CameraCache *) data;
    if (cachePtr->isCaching())
        return;
    
    if (!cachePtr->isInitialized())
        return;
    
    bool autokey = MAnimControl::autoKeyMode();
    
    //check if the camera has the needed data
    if (MAnimControl::isPlaying() && autokey)
    {
        cachePtr->checkRangeIsCached();
        return;
    }
    
    #if MAYA_API_VERSION > 201500
        //check if the camera has the needed data
        if (MAnimControl::isScrubbing() && autokey)
        {
            cachePtr->checkRangeIsCached();
            return;
        }
    #endif
    
    cachePtr->cacheCamera();
}

CameraCache *MotionPathManager::getCameraCachePtrFromView(M3dView &view)
{
    MDagPath camera;
    view.getCamera(camera);
    
    //camera.pop(1);
    std::string cameraName(camera.fullPathName().asChar());
    CameraCacheMapIterator it = cameraCache.find(cameraName);
    if (it == cameraCache.end())
        return NULL;
   
    return &it->second;
}

void MotionPathManager::drawBufferPaths(M3dView &view, CameraCache* cachePtr, MHWRender::MUIDrawManager* drawManager, const MHWRender::MFrameContext* frameContext)
{
	for (int i = 0; i < bufferPathArray.size(); ++i)
		bufferPathArray[i].draw(view, cachePtr, drawManager, frameContext);
}

void MotionPathManager::drawPaths(M3dView view, CameraCache* cachePtr, MHWRender::MUIDrawManager* drawManager, const MHWRender::MFrameContext* frameContext)
{
	for (int i = 0; i < pathArray.size(); ++i)
		pathArray[i].draw(view, cachePtr, drawManager, frameContext);
}

void MotionPathManager::viewPostRenderCallback(const MString& panelName, void* data)
{
    MotionPathManager* mpManager = (MotionPathManager*) data;
    
	if(mpManager)
	{
		M3dView view;
		MStatus status = M3dView::getM3dViewFromModelPanel(panelName, view);
		if(status && view.display())
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
                cachePtr = mpManager->getCameraCachePtrFromView(view);
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

			
			MString name = view.rendererString();
			if (name != "hwRender_OpenGL_Renderer" && name != "base_OpenGL_Renderer")
			{
				return;
			}
			
			view.beginGL();
            
			glPushAttrib(GL_ALL_ATTRIB_BITS);
            glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
			glPushMatrix();
            
			glDisable(GL_LIGHTING);
			glDisable(GL_DEPTH_TEST);
            
            for (int i = 0; i < mpManager->bufferPathArray.size(); ++i)
                mpManager->bufferPathArray[i].draw(view, cachePtr);
            
			for(int i = 0; i < mpManager->pathArray.size(); ++i)
                mpManager->pathArray[i].draw(view, cachePtr);
            
			glPopMatrix();
			glPopAttrib();
            glPopClientAttrib();
			view.endGL();
			
		}
	}
}

void MotionPathManager::viewDestroyCallback(const MString& panelName, void* data)
{
    MotionPathManager *manager = (MotionPathManager *) data;
    
    if(manager)
    {
        for (unsigned int i = 0; i < manager->registeredPanels.size(); ++i)
        {
            if (manager->registeredPanels[i].name == panelName)
            {
                manager->removePanelCallback(manager->registeredPanels[i]);
                manager->registeredPanels.erase(manager->registeredPanels.begin() + i);
                return;
            }
        }
    }
}

void MotionPathManager::removePanelCallback(const RegisteredPanel &panel)
{
    if (panel.destroyPanelCallbackId)
        MMessage::removeCallback(panel.destroyPanelCallbackId);
    if (panel.postRenderCallbackId)
        MMessage::removeCallback(panel.postRenderCallbackId);
    if (panel.cameraWorldMatrixCallbackId)
        MMessage::removeCallback(panel.cameraWorldMatrixCallbackId);
    if (panel.cameraChangedId)
        MMessage::removeCallback(panel.cameraChangedId);
    if (panel.cameraNameChangedId)
        MMessage::removeCallback(panel.cameraNameChangedId);
}

void MotionPathManager::cleanupViewports()
{
    for (unsigned int i = 0; i < registeredPanels.size(); ++i)
        removePanelCallback(registeredPanels[i]);
    
    registeredPanels.clear();
    pathArray.clear();
    selectionObjects.clear();
    bufferPathArray.clear();
    cameraCache.clear();
}

void MotionPathManager::createMotionPathWorldCallback()
{
    for(int i = 0; i < pathArray.size(); i++)
        pathArray[i].addWorldMatrixCallback();
}

void MotionPathManager::destroyMotionPathWorldCallback()
{
    for(int i = 0; i < pathArray.size(); i++)
        pathArray[i].removeWorldMartrixCallback();
}

void MotionPathManager::addCallbacks()
{
    MCallbackId id = MDGMessage::addTimeChangeCallback(timeChangeEvent, this);
    this->cbIDs.append(id);
    
	id = MCommandMessage::addCommandOutputCallback(commandEvent, this);
	this->cbIDs.append(id);
    
    id = MEventMessage::addEventCallback("deleteAll", deleteAllCallback, this);
    this->cbIDs.append(id);
    
    id = MEventMessage::addEventCallback("SceneOpened", sceneOpenedCallback, this);
    this->cbIDs.append(id);
    
    id = MModelMessage::addCallback(MModelMessage::kActiveListModified, selectionChangeCallback, this);
    this->cbIDs.append(id);
}

void MotionPathManager::sceneOpenedCallback(void *data)
{
    MotionPathManager* mpManager = (MotionPathManager*) data;
	if(!mpManager)
		return;
    
    mpManager->cleanupViewports();
    mpManager->setupViewports();
}

void MotionPathManager::removeCallbacks()
{
    for (unsigned int i = 0; i < this->cbIDs.length(); ++i)
    {
        MStatus status = MMessage::removeCallback(this->cbIDs[i]);
        if(!status)
		{
			MGlobal::displayError("MotionPath: could not remove callback");
			if(status == MS::kInvalidParameter)
				MGlobal::displayError("MotionPath: an invalid callback id was specified");
			else if(status == MS::kFailure)
				MGlobal::displayError("MotionPath: callback has already been removed");
		}
    }
    
    this->cbIDs.clear();
}

void MotionPathManager::getSelection(MObjectArray &objArray)
{
    MSelectionList list;
	MGlobal::getActiveSelectionList(list);
	MItSelectionList iter(list, MFn::kInvalid);
    
	for(; !iter.isDone(); iter.next())
	{
		MObject dependNode;
		iter.getDependNode(dependNode);
		if(!dependNode.isNull() && dependNode.hasFn(MFn::kDependencyNode))
		{
			MFnDependencyNode dependNodeFn(dependNode);
			if (!dependNodeFn.findPlug("translate").isNull())
				objArray.append(dependNode);
		}
	}
}

bool MotionPathManager::hasSelectionListChanged(const MObjectArray &objArray)
{
    if (objArray.length() != selectionObjects.length())
        return true;
    
    for(unsigned int i = 0; i < selectionObjects.length(); i++)
		if(selectionObjects[i] != objArray[i])
			return true;

    return false;
}

bool MotionPathManager::isContainedInMObjectArray(const MObjectArray &objArray, const MObject &obj)
{
    for (int i = 0; i < objArray.length(); i++)
        if (obj == objArray[i])
            return true;
    return false;
}

void MotionPathManager::highlightSelection(const MObjectArray &objArray)
{
    for (int i = 0; i < selectionObjects.length(); i++)
    {
        if (isContainedInMObjectArray(objArray, selectionObjects[i]))
            pathArray[i].setColorMultiplier(1.0);
        else
            pathArray[i].setColorMultiplier(0.4);
    }
}

void MotionPathManager::selectionChangeCallback(void *data)
{
    MotionPathManager* mpManager = (MotionPathManager*) data;
	if(!mpManager)
		return;
    
    MObjectArray objArray;
    mpManager->getSelection(objArray);
    
    if (mpManager->hasSelectionListChanged(objArray) && !GlobalSettings::lockedMode)
    {
        mpManager->setSelectionList(objArray);
        mpManager->refreshDisplayTimeRange();
    }
    else
        mpManager->highlightSelection(objArray);
}

void MotionPathManager::deleteAllCallback(void *data)
{
	MotionPathManager* mpManager = (MotionPathManager*) data;
	if(!mpManager)
		return;
    
    mpManager->cleanupViewports();
    mpManager->setupViewports();
}

void MotionPathManager::timeChangeEvent(MTime &currentTime,  void* data)
{
    MotionPathManager* mpManager = (MotionPathManager*) data;
	if(mpManager)
		mpManager->refreshDisplayTimeRange();


}

void MotionPathManager::commandEvent(const MString &message, MCommandMessage::MessageType messageType, void *data)
{
    MotionPathManager* mpManager = (MotionPathManager*) data;
	if(mpManager)
	{
		if(message.indexW("setKeyframe") > -1)
		{
			// will cause a refresh once maya is done with updating the curves
			MGlobal::executeCommandOnIdle("refresh");
		}
	}
}

void MotionPathManager::getDagPath(const MString &name, MDagPath &dp)
{
    MSelectionList sList;
    MStatus status = sList.add(name);
    
    if (status)
        sList.getDagPath(0, dp);
}

void MotionPathManager::refreshDisplayTimeRange()
{
    double currentFrame = MAnimControl::currentTime().as(MTime::uiUnit());
    
    double startFrame = currentFrame - GlobalSettings::framesBack;
    double endFrame = currentFrame + GlobalSettings::framesFront;
    
    if(startFrame < GlobalSettings::startTime)	startFrame = GlobalSettings::startTime;
	if(endFrame > GlobalSettings::endTime) 	endFrame = GlobalSettings::endTime;
    
    if(!cacheDone)
		cacheDone = expandParentMatrixAndPivotCache(currentFrame);
    
	for(int i = 0; i < pathArray.size(); i++)
		pathArray[i].setDisplayTimeRange(startFrame, endFrame);
}

void MotionPathManager::clearParentMatrixCaches()
{
    for(int i = 0; i < pathArray.size(); i++)
		pathArray[i].clearParentMatrixCache();
}

bool MotionPathManager::expandParentMatrixAndPivotCache(const double currentTimeValue)
{
    double timeout = 2.0;
	time_t startTime = time(NULL);
    
	double nUpdqates = GlobalSettings::framesFront;
	if(GlobalSettings::framesBack > GlobalSettings::framesFront)
		nUpdqates = GlobalSettings::framesBack;
    
	bool cacheCompleted = true;
    
	for(double i = 0; i <= nUpdqates; i++)
	{
		time_t currentTime = time(NULL);
		double secs = difftime(currentTime, startTime);
        
		if(secs < timeout)
		{
			cacheCompleted = true;
            
			for(int j = 0; j < pathArray.size(); j++)
			{
				if(!pathArray[j].isCacheDone())
				{
					pathArray[j].growParentAndPivotMatrixCache(currentTimeValue, i);
					cacheCompleted = false;
				}
			}
		}
		else
			break;
	}
    
	return cacheCompleted;
}

void MotionPathManager::setTimeRange(const double start, const double end)
{
	GlobalSettings::startTime = start;
	GlobalSettings::endTime = end <= start ? start + 1.0: end;
    
	for(int i = 0; i < pathArray.size(); i++)
		pathArray[i].setTimeRange(GlobalSettings::startTime, GlobalSettings::endTime);
    
	cacheDone = false;
}

void MotionPathManager::drawCurvesForSelection(M3dView &view, CameraCache *cachePtr)
{
    for(int i = 0; i < pathArray.size(); i++)
	{
		view.pushName(i);
		pathArray[i].drawCurvesForSelection(view, cachePtr);
		view.popName();
	}
}

MotionPath* MotionPathManager::getMotionPathPtr(const int id)
{
	if(id >= 0 && id < pathArray.size())
		return &pathArray[id];
    
	return NULL;
}

int MotionPathManager::isMObjectContained(const MObject &obj, const MObjectArray &a)
{
    for (unsigned int i = 0; i < a.length(); ++i)
        if (a[i] == obj)
            return i;

    return -1;
}

void MotionPathManager::setSelectionList(const MObjectArray &list)
{
    //we keep track the old objects cause we don't want to destroy old MotionPaths if still in use
    std::vector<MotionPath> oldPathArray = pathArray;
    MObjectArray oldSelectionObjects (selectionObjects);
    
    pathArray.clear();
    selectionObjects.clear();
    
    for (unsigned int i = 0; i < list.length(); ++i)
    {
        if (!list[i].isNull())
        {
            if (MotionPath::hasAnimationLayers(list[i]))
                MGlobal::displayWarning("Motion Path does not support animation layers. The path won't be displayed in real time.");
            
            int index = isMObjectContained(list[i], oldSelectionObjects);
            
            if (index == -1)
            {
                MotionPath m =  MotionPath(list[i]);
                pathArray.push_back(m);
            }
            else
                pathArray.push_back(oldPathArray[index]);

            selectionObjects.append(list[i]);
        }
    }
    
    cacheDone = false;
}

MStringArray MotionPathManager::getSelectionList()
{
	MStringArray list;
	for(int i = 0; i < selectionObjects.length(); i++)
	{
		MFnDagNode dagNode(selectionObjects[i]);
		list.append(dagNode.fullPathName());
	}
    
	return list;
}

void MotionPathManager::addBufferPaths()
{
    for (unsigned int i = 0; i < pathArray.size(); ++i)
        bufferPathArray.push_back(pathArray[i].createBufferPath());
}

void MotionPathManager::deleteAllBufferPaths()
{
    bufferPathArray.clear();
}

void MotionPathManager::deleteBufferPathAtIndex(const int index)
{
    if (index >= 0 && index < bufferPathArray.size())
        bufferPathArray.erase(bufferPathArray.begin() + index);
}

void MotionPathManager::setSelectStateForBufferPathAtIndex(const int index, const bool value)
{
    if (index >= 0 && index < bufferPathArray.size())
        bufferPathArray[index].setSelected(value);
}

BufferPath* MotionPathManager::getBufferPathAtIndex(int index)
{
    if (index >= 0 && index < bufferPathArray.size())
        return &bufferPathArray[index];
    return NULL;
}

void MotionPathManager::startAnimUndoRecording()
{
	animCurveChangePtr = new MAnimCurveChange();
}

void MotionPathManager::startDGUndoRecording()
{
	dgModifierPtr = new MDGModifier();
}

void MotionPathManager::stopDGAndAnimUndoRecording()
{
    MGlobal::executeCommand("tcMotionPathCmd -storeDGAndCurveChange", true, true);
    
	dgModifierPtr = NULL;
    animCurveChangePtr = NULL;
}

void MotionPathManager::storePreviousKeySelection()
{
    getCurrentKeySelection(previousKeySelection);
}

void MotionPathManager::getPreviousKeySelection(std::vector<MDoubleArray> &sel)
{
    sel = previousKeySelection;
    previousKeySelection.clear();
}

void MotionPathManager::getCurrentKeySelection(std::vector<MDoubleArray> &sel)
{
    sel.clear();
    sel.reserve(pathArray.size());
    
    for (int i=0; i < pathArray.size(); ++i)
        sel.push_back(pathArray[i].getSelectedKeys());
}

