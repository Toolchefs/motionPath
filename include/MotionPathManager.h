//
//  MotionPathManager.h
//  MotionPath
//
//  Created by Daniele Federico on 09/11/14.
//
//

#ifndef MOTIONPATHMANAGER_H
#define MOTIONPATHMANAGER_H

#include "MotionPathEditContext.h"
#include "MotionPath.h"

#include <time.h>

#include <maya/MGlobal.h>
#include <maya/MUiMessage.h>
#include <maya/MModelMessage.h>
#include <maya/MEventMessage.h>
#include <maya/MConditionMessage.h>
#include <maya/MAnimMessage.h>
#include <maya/MDGMessage.h>
#include <maya/MDGModifier.h>
#include <maya/M3dView.h>
#include <maya/MIntArray.h>
#include <maya/MStringArray.h>
#include <maya/MItSelectionList.h>
#include <maya/MDagPath.h>
#include <maya/MSelectionList.h>
#include <maya/MIntArray.h>
#include <maya/MAnimControl.h>
#include <maya/MStringArray.h>
#include <maya/MObjectArray.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnCamera.h>
#include <maya/MCommandMessage.h>
#include <maya/MAnimCurveChange.h>
#include <maya/MCallbackIdArray.h>
#include <maya/MFloatMatrix.h>
#include <maya/MViewport2Renderer.h>

#include <vector>
#include <map>

struct RegisteredPanel
{
	MString name;
	MCallbackId destroyPanelCallbackId;
	MCallbackId postRenderCallbackId;
    MCallbackId cameraWorldMatrixCallbackId;
    MCallbackId cameraChangedId;
    MCallbackId cameraNameChangedId;
};

typedef std::vector<RegisteredPanel> RegisteredPanelArray;

class MotionPathManager
{
public:
    MotionPathManager();
    ~MotionPathManager();
    
    void setupViewports();
    void cleanupViewports();
    void addCallbacks();
    void removeCallbacks();
    
    MStringArray getSelectionList();
    void refreshDisplayTimeRange();
    void setTimeRange(const double start, const double end);
    bool expandParentMatrixAndPivotCache(const double currentTimeValue);
    MotionPath* getMotionPathPtr(const int id);
    int getMotionPathsCount(){return pathArray.size();};
    void drawCurvesForSelection(M3dView &view, CameraCache *cachePtr);

    void addBufferPaths();
    void deleteAllBufferPaths();
    void deleteBufferPathAtIndex(const int index);
    void setSelectStateForBufferPathAtIndex(const int index, const bool value);
    
    void startAnimUndoRecording();
    MAnimCurveChange* getAnimCurveChangePtr(){return this->animCurveChangePtr;};
    void startDGUndoRecording();
    MDGModifier* getDGModifierPtr(){return this->dgModifierPtr;};
    void stopDGAndAnimUndoRecording();
    
    BufferPath* getBufferPathAtIndex(int index);
    
    void storePreviousKeySelection();
    void getPreviousKeySelection(std::vector<MDoubleArray> &sel);
    void getCurrentKeySelection(std::vector<MDoubleArray> &sel);
    
    static void selectionChangeCallback(void *data);
    
    void clearParentMatrixCaches();
    
    CameraCache *getCameraCachePtrFromView(M3dView &view);
    
    void refreshCameraCallbackForPanel(const MString &panelName, MDagPath &camera);
    void createCameraCacheForCamera(const MDagPath &camera);
    
    void cacheCameras();
    
    void createMotionPathWorldCallback();
    void destroyMotionPathWorldCallback();

	void drawBufferPaths(M3dView &view, CameraCache* cachePtr, MHWRender::MUIDrawManager* drawManager = NULL, const MHWRender::MFrameContext* frameContext = NULL);
	void drawPaths(M3dView view, CameraCache* cachePtr, MHWRender::MUIDrawManager* drawManager = NULL, const MHWRender::MFrameContext* frameContext = NULL);
    
    //void destroyCameraCachesAndCameraCallbacks();
    //void createCameraCachesAndCameraCallbacks();
    
private:
    bool cacheDone;
    MCallbackIdArray cbIDs;
    RegisteredPanelArray registeredPanels;
    MObjectArray selectionObjects;
    std::vector<MotionPath> pathArray;
    std::vector<BufferPath> bufferPathArray;
    MAnimCurveChange* animCurveChangePtr;
    MDGModifier *dgModifierPtr;
    CameraCacheMap cameraCache;
    
    std::vector<MDoubleArray> previousKeySelection;
    
    int isMObjectContained(const MObject &obj, const MObjectArray &a);
    
    void getDagPath(const MString &name, MDagPath &dp);
    
    void getSelection(MObjectArray &objArray);
    void setSelectionList(const MObjectArray &list);
    
    bool hasSelectionListChanged(const MObjectArray &objArray);
    bool isContainedInMObjectArray(const MObjectArray &objArray, const MObject &obj);
    void highlightSelection(const MObjectArray &objArray);
    void setupViewport(const MString &panelName);
    
    static void timeChangeEvent(MTime &currentTime,  void* data);
    static void commandEvent(const MString &message, MCommandMessage::MessageType messageType, void *data);
    static void viewPostRenderCallback(const MString& panelName, void* data);
    static void viewDestroyCallback(const MString& panelName, void* data);
    static void autoKeyframeCallback(bool state, void* data);
    static void deleteAllCallback(void *data);
    static void sceneOpenedCallback(void *data);
    static void cameraWorldMatrixChangedCallback(MObject& transformNode, MDagMessage::MatrixModifiedFlags& modified,void* data);
    static void viewCameraChanged(const MString &str, MObject &node, void *data);
    static void viewCameraNameChanged(MObject &node, const MString &str, void *data);
    
    void removePanelCallback(const RegisteredPanel &panel);
    
    int panelRegistered(const MString &panelName);
};


#endif
