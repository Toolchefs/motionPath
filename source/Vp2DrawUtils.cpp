//
//  VP2DrawUtils.cpp
//  MotionPath
//
//  Created by Daniele Federico on 16/11/14.
//
//

#include "Vp2DrawUtils.h"
#include <maya/MPointArray.h>

#define PI 3.1415

void VP2DrawUtils::drawLineStipple(const MVector &origin, const MVector &target, float lineWidth, const MColor &color, const MMatrix &cameraMatrix, MHWRender::MUIDrawManager* drawManager, const MHWRender::MFrameContext* frameContext)
{
	MVector zVec(cameraMatrix[2][0], cameraMatrix[2][1], cameraMatrix[2][2]);
	MVector cPos(cameraMatrix[3][0], cameraMatrix[3][1], cameraMatrix[3][2]);
	if ((cPos - origin) * zVec <= 0.0001)
		return;

	drawManager->setColor(color);
	drawManager->setLineWidth(lineWidth);
	drawManager->setPaintStyle(MHWRender::MUIDrawManager::kStippled);
	drawManager->setLineStyle(8, 0xAAAA);
	double x1, y1, x2, y2;
	frameContext->worldToViewport(origin, x1, y1);
	frameContext->worldToViewport(target, x2, y2);
	drawManager->line2d(MPoint(x1, y1), MPoint(x2, y2));
}


void VP2DrawUtils::drawLine(const MVector &origin, const MVector &target, float lineWidth, const MMatrix &cameraMatrix, MHWRender::MUIDrawManager* drawManager,  const MHWRender::MFrameContext* frameContext)
{
	MVector zVec(cameraMatrix[2][0], cameraMatrix[2][1], cameraMatrix[2][2]);
	MVector cPos(cameraMatrix[3][0], cameraMatrix[3][1], cameraMatrix[3][2]);
	if ((cPos - origin) * zVec <= 0.0001)
		return;

	drawManager->setLineWidth(lineWidth);
	double x1, y1, x2, y2;
	frameContext->worldToViewport(origin, x1, y1);
	frameContext->worldToViewport(target, x2, y2);
	drawManager->line2d(MPoint(x1, y1), MPoint(x2, y2));
}


void VP2DrawUtils::drawLineWithColor(const MVector &origin, const MVector &target, float lineWidth, const MColor &color, const MMatrix &cameraMatrix, MHWRender::MUIDrawManager* drawManager, const MHWRender::MFrameContext* frameContext)
{
	drawManager->setColor(color);
	VP2DrawUtils::drawLine(origin, target, lineWidth, cameraMatrix, drawManager, frameContext);
}


void VP2DrawUtils::drawPoint(const MVector &point, float size, const MMatrix &cameraMatrix, MHWRender::MUIDrawManager* drawManager, const MHWRender::MFrameContext* frameContext)
{
	MVector zVec(cameraMatrix[2][0], cameraMatrix[2][1], cameraMatrix[2][2]);
	MVector cPos(cameraMatrix[3][0], cameraMatrix[3][1], cameraMatrix[3][2]);
	if ((cPos - point) * zVec  <= 0.0001)
		return;

	double x, y; 
	frameContext->worldToViewport(point, x, y);
	drawManager->circle2d(MPoint(x, y), size / 2, true);
}


void VP2DrawUtils::drawPointWithColor(const MVector &point, float size, const MColor &color, const MMatrix &cameraMatrix, MHWRender::MUIDrawManager* drawManager, const MHWRender::MFrameContext* frameContext)
{
	drawManager->setColor(color);
	VP2DrawUtils::drawPoint(point, size, cameraMatrix, drawManager, frameContext);
}

void VP2DrawUtils::drawKeyFrames(std::vector<Keyframe *> keys, const float size, const double colorMultiplier, const int portWidth, const int portHeight, const bool showRotationKeyframes, const MMatrix &cameraMatrix, MHWRender::MUIDrawManager* drawManager, const MHWRender::MFrameContext* frameContext)
{
	for (unsigned int ki = 0; ki < keys.size(); ++ki)
	{
		Keyframe* key = keys[ki];
		if (!key)
			continue;

		std::vector<Keyframe::Axis> tAxis, rAxis;
		key->getKeyTranslateAxis(tAxis);
		//std::cout << key->time << std::endl;
		if (tAxis.size() < 1)
		{
			//std::cout << "axis 0 " << key->time << std::endl;
			continue;
		}
		if (showRotationKeyframes)
			key->getKeyRotateAxis(rAxis);

		double blackBackgroundFactor = 1.2;
		MColor color(0.0, 0.0, 0.0);
		double centerX, centerY;
		frameContext->worldToViewport(key->worldPosition, centerX, centerY);

		drawManager->setColor(color);
		drawManager->circle2d(MPoint(centerX, centerY), size * blackBackgroundFactor / 2, true);

		if (key->selectedFromTool)
		{
			MColor color(1.0, 1.0, 1.0);
			drawManager->setColor(color);
			drawManager->circle2d(MPoint(centerX, centerY), size / 2, true);
		}
		else
		{
			double stepSize = size / 2 / tAxis.size();
			for (int i = 0; i < tAxis.size(); ++i)
			{
				Keyframe::getColorForAxis(tAxis[i], color);
				color *= colorMultiplier;

				drawManager->setColor(color);
				drawManager->circle2d(MPoint(centerX, centerY), stepSize * (tAxis.size() - i), true);
			}

			/*
			int nSections = 3;
			int step = nSections / tAxis.size();
			int currentStep = 0;
			//std::cout << "triangle 1" << std::endl;
			Keyframe::getColorForAxis(tAxis[currentStep], color);
			color *= colorMultiplier;

			double angleAdd = (2.0 * double(PI) / double(nSections));
			double angle = -PI / 2;

			double x1 = 0, y1 = -1, x2, y2;
			for (int i = 0; i < nSections; ++i)
			{
				//std::cout << i << std::endl;
				if (i / step > currentStep)
				{
					currentStep = i / step;
					if (currentStep == tAxis.size()) currentStep = 0;
					Keyframe::getColorForAxis(tAxis[currentStep], color);
					color *= colorMultiplier;
				}

				angle -= angleAdd;
				x2 = size * 0.5 * sin(angle);
				y2 = size * 0.5 * cos(angle);

				MPointArray points;
				points.append(MPoint(centerX, centerY));
				points.append(MPoint(centerX + x1, centerY + y1));
				points.append(MPoint(centerX + x2, centerY + y2));

				drawManager->setColor(color);
				drawManager->mesh2d(MHWRender::MUIDrawManager::kTriangles, points);
				//drawManager->arc2d(MPoint(centerX, centerY), MVector(x1, y1), MVector(x2, y2), size / 2, true);
				x1 = x2;
				y1 = y2;
			}
			*/
			
		}

		if (rAxis.size() > 0)
		{
			double lineWidth = size / 5;
			if (lineWidth < 1) lineWidth = 1;

			double unit = size * blackBackgroundFactor / 2;
			float x1s[3] = { -unit * 0.8, unit * 1.5,  unit * -1.5 };
			float y1s[3] = { unit * 1.2,  unit * 0.1,  unit * 0.1 };

			float x2s[3] = { unit * 0.8,  unit * 0.7,  unit * -0.7 };
			float y2s[3] = { unit * 1.2,  unit * -1.2, unit * -1.2 };

			MColor color;
			MVector p1(0, 0, 0), p2(0, 0, 0);
			for (unsigned int i = 0; i < rAxis.size(); ++i)
			{
				Keyframe::getColorForAxis(rAxis[i], color);
				color *= colorMultiplier;

				p1.x = centerX + x1s[i];
				p1.y = centerY + y1s[i];

				p2.x = centerX + x2s[i];
				p2.y = centerY + y2s[i];

				drawManager->setColor(color);
				drawManager->setLineWidth(lineWidth);
				drawManager->line2d(p1, p2);
			}
		}
	}
}

void VP2DrawUtils::drawKeyFramePoints(KeyframeMap &keyframesCache, const float size, const double colorMultiplier, const int portWidth, const int portHeight, const bool showRotationKeyframes, const MMatrix &cameraMatrix, MHWRender::MUIDrawManager* drawManager, const MHWRender::MFrameContext* frameContext)
{
	std::vector<Keyframe *> keys;
	keys.reserve(keyframesCache.size());

	for (KeyframeMapIterator keyIt = keyframesCache.begin(); keyIt != keyframesCache.end(); keyIt++)
	{
		Keyframe* key = &keyIt->second;
		if (!key)
			continue;
		MVector zVec(cameraMatrix[2][0], cameraMatrix[2][1], cameraMatrix[2][2]);
		MVector cPos(cameraMatrix[3][0], cameraMatrix[3][1], cameraMatrix[3][2]);
		if ((cPos - key->worldPosition) * zVec  <= 0.0001)
			continue;
		keys.push_back(key);
	}

	drawKeyFrames(keys, size, colorMultiplier, portWidth, portHeight, showRotationKeyframes, cameraMatrix, drawManager, frameContext);
}

void VP2DrawUtils::convertWorldSpaceToCameraSpace(CameraCache* cachePtr, std::map<double, MPoint> &positions, std::map<double, MPoint> &screenSpacePositions, MHWRender::MUIDrawManager* drawManager, const MHWRender::MFrameContext* frameContext)
{
	
}

void VP2DrawUtils::drawFrameLabel(double frame, const MVector &framePos, M3dView &view, const double sizeOffset, const MColor &color, const MMatrix &cameraMatrix, MHWRender::MUIDrawManager* drawManager, const MHWRender::MFrameContext* frameContext)
{
	MVector zVec(cameraMatrix[2][0], cameraMatrix[2][1], cameraMatrix[2][2]);
	MVector cPos(cameraMatrix[3][0], cameraMatrix[3][1], cameraMatrix[3][2]);
	if ((cPos - framePos) * zVec  <= 0.0001)
		return;

	drawManager->setFontSize(MHWRender::MUIDrawManager::kDefaultFontSize);
	drawManager->setColor(color);
	
	double viewX, viewY;
	frameContext->worldToViewport(framePos, viewX, viewY);

	MString frameStr;
	frameStr = frame;

	drawManager->text(MPoint(viewX, viewY + (GlobalSettings::frameSize * sizeOffset)), frameStr, MHWRender::MUIDrawManager::kCenter);
}

