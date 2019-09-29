
#include "GlobalSettings.h"

double GlobalSettings::startTime = 0.0;
double GlobalSettings::endTime = 0.0;
double GlobalSettings::framesFront = 0;
double GlobalSettings::framesBack = 0;
MColor GlobalSettings::pathColor = MColor(0.5, 0.5, 0.8);
MColor GlobalSettings::currentFrameColor =  MColor(0.8, 0.8, 0.1);
MColor GlobalSettings::tangentColor =  MColor(0.5, 0.7, 0.1);
MColor GlobalSettings::brokenTangentColor =  MColor(0.1, 0.5, 0.7);
MColor GlobalSettings::bufferPathColor = MColor(0.2, 0.2, 0.2);
MColor GlobalSettings::weightedPathTangentColor = MColor(0.2, 0.2, 0.2);
MColor GlobalSettings::weightedPathColor = MColor(0.2, 0.2, 0.2);
MColor GlobalSettings::frameLabelColor = MColor(0.1, 0.1, 0.1);
double GlobalSettings::pathSize = 3.0;
double GlobalSettings::frameSize = 7.0;
bool GlobalSettings::showTangents = true;
bool GlobalSettings::showKeyFrames = true;
bool GlobalSettings::showKeyFrameNumbers = false;
bool GlobalSettings::showFrameNumbers = false;
bool GlobalSettings::showRotationKeyFrames = true;
bool GlobalSettings::showPath = true;
double GlobalSettings::drawTimeInterval = 0.1;
int GlobalSettings::drawFrameInterval = 5;
MMatrix GlobalSettings::cameraMatrix;
int GlobalSettings::portWidth = 0;
int GlobalSettings::portHeight = 0;
bool GlobalSettings::lockedMode = false;
bool GlobalSettings::enabled = false;
bool GlobalSettings::alternatingFrames = false;
bool GlobalSettings::lockedModeInteractive = true;
bool GlobalSettings::usePivots = false;
int GlobalSettings::strokeMode = 0;
GlobalSettings::DrawMode GlobalSettings::motionPathDrawMode = GlobalSettings::kWorldSpace;
