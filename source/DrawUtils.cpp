//
//  DrawUtils.cpp
//  MotionPath
//
//  Created by Daniele Federico on 16/11/14.
//
//

#include "DrawUtils.h"

#define PI 3.1415

void drawUtils::drawLineStipple(const MVector &origin, const MVector &target, float lineWidth, const MColor &color)
{
    glEnable(GL_BLEND);
    glColor4d(color.r, color.g, color.b, color.a);
    
    float prevLineWidth;
    glGetFloatv(GL_LINE_WIDTH, &prevLineWidth);
    
    glLineWidth(lineWidth);
    
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(8, 0xAAAA);
    
    glBegin(GL_LINES);
    glVertex3f(origin.x, origin.y, origin.z);
    glVertex3f(target.x, target.y, target.z);
    glEnd();
    
    glDisable(GL_LINE_STIPPLE);
    
    glLineWidth(prevLineWidth);
}


void drawUtils::drawLine(const MVector &origin, const MVector &target, float lineWidth)
{
    float prevLineWidth;
    glGetFloatv(GL_LINE_WIDTH, &prevLineWidth);
    
    glLineWidth(lineWidth);
    
    glBegin(GL_LINES);
    glVertex3f(origin.x, origin.y, origin.z);
    glVertex3f(target.x, target.y, target.z);
    glEnd();
    
    glLineWidth(prevLineWidth);//important, as maya won't restore its width automatically
}


void drawUtils::drawLineWithColor(const MVector &origin, const MVector &target, float lineWidth, const MColor &color)
{
    glEnable(GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4d(color.r, color.g, color.b, color.a);
    
    drawUtils::drawLine(origin, target, lineWidth);
}


void drawUtils::drawPoint(const MVector &point, float size)
{
    // if keyframe is only on one axis then use GL_POINT
    float prevSize;
    glGetFloatv(GL_POINT_SIZE, &prevSize);
    
    glPointSize(size);
    glEnable(GL_POINT_SMOOTH);
    
    glBegin(GL_POINTS);
    glVertex3f(point.x, point.y, point.z);
    glEnd();
    
    glPointSize(prevSize);
}


void drawUtils::drawPointWithColor(const MVector &point, float size, const MColor &color)
{
    glEnable(GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4d(color.r, color.g, color.b, color.a);
    
    drawUtils::drawPoint(point, size);
}

void drawUtils::drawKeyFrames(std::vector<Keyframe *> keys, const float size, const double colorMultiplier,const int portWidth, const int portHeight, const bool showRotationKeyframes)
{
    glMatrixMode(GL_MODELVIEW); //combinazione matrici model e inversa camera
    glPushMatrix();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, portWidth, portHeight, 0, 1, -1);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    for(unsigned int ki = 0; ki < keys.size(); ++ki)
    {
        Keyframe* key = keys[ki];
        if (!key)
            continue;
        
        if (key->projPosition.z > 1 || key->projPosition.z < 0)
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
        
        //converti winY in modo tale che l'origine non sia in alto a sinistra, ma in basso sinistra
        double convertY = portHeight - key->projPosition.y;
        double blackBackgroundFactor = 1.2;
        MColor color(0.0, 0.0, 0.0);
        drawPointWithColor(MVector(key->projPosition.x, convertY, 0.0f), size*blackBackgroundFactor, color);
        
        if (key->selectedFromTool)
        {
            MColor color(1.0, 1.0, 1.0);
            drawPointWithColor(MVector(key->projPosition.x, convertY, 0.0f), size, color);
        }
        else
        {
            //std::cout << "begin triangle" << std::endl;
            glBegin(GL_TRIANGLES);
            
            int nSections = 12;
            int step = nSections / tAxis.size();
            int currentStep = 0;
            //std::cout << "triangle 1" << std::endl;
            Keyframe::getColorForAxis(tAxis[currentStep], color);
            color *= colorMultiplier;
            glColor4d(color.r, color.g, color.b, color.a);
            //std::cout << "triangle 2" << std::endl;
            double angleAdd = (2.0 * double(PI) / double(nSections));
            double angle = -PI / 2;
            GLfloat x = 0;
            GLfloat y = size/2;
            //std::cout << "init section" << std::endl;
            for (int i = 0; i <= nSections; ++i)
            {
                //std::cout << i << std::endl;
                if (i / step > currentStep)
                {
                    currentStep = i / step;
                    if (currentStep == tAxis.size()) currentStep = 0;
                    Keyframe::getColorForAxis(tAxis[currentStep], color);
                    color *= colorMultiplier;
                    glColor4d(color.r, color.g, color.b, color.a);
                }
                
                //std::cout << key->projPosition.x << " " << convertY << " " << 0 << std::endl;
                //std::cout << "gl vertex" << std::endl;
                
                // Draw Triangle
                
                glVertex3f(key->projPosition.x, convertY, 0);
                //std::cout << "gl vertex 1 " << x << " " << y << " " << angle << " " << size << std::endl;
                glVertex3f(key->projPosition.x + x, convertY + y, 0.0f);
                
                angle += angleAdd;
                x = size * 0.5 * sin(angle);
                y = size * 0.5 * cos(angle);
                //std::cout << "gl vertex 2 " << x << " " << y << " " << size << " " << angle << " " << angleAdd << std::endl;
                glVertex3f(key->projPosition.x + x, convertY + y, 0.0f);
                
            }
            
            glEnd();
            //std::cout << "end triangle" << std::endl;
        }
        
        if (rAxis.size() > 0)
        {
            double lineWidth = size / 5;
            if (lineWidth < 1) lineWidth = 1;
            
            double unit = size * blackBackgroundFactor / 2;
            float x1s[3] = {-unit * 0.8, unit * 1.5,  unit * -1.5};
            float y1s[3] = {unit * 1.2,  unit * 0.1,  unit * 0.1};
            
            float x2s[3] = {unit * 0.8,  unit * 0.7,  unit * -0.7};
            float y2s[3] = {unit * 1.2,  unit * -1.2, unit * -1.2};
            
            MColor color;
            MVector p1(0,0,0), p2(0,0,0);
            for (unsigned int i = 0; i < rAxis.size(); ++i)
            {
                Keyframe::getColorForAxis(rAxis[i], color);
                color *= colorMultiplier;
                
                p1.x = key->projPosition.x + x1s[i];
                p1.y = convertY + y1s[i];
                
                p2.x = key->projPosition.x + x2s[i];
                p2.y = convertY + y2s[i];
                
                drawLineWithColor(p1, p2, lineWidth, color);
            }
        }
    }
    
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void drawUtils::drawKeyFramePoints(KeyframeMap &keyframesCache, const float size, const double colorMultiplier,const int portWidth, const int portHeight, const bool showRotationKeyframes)
{
    glMatrixMode(GL_MODELVIEW); //combinazione matrici model e inversa camera
    glPushMatrix();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    
    double modelview_matrix[16];
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview_matrix);
    double projection_matrix[16];
    glGetDoublev(GL_PROJECTION_MATRIX, projection_matrix);
    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    
    std::vector<Keyframe *> keys;
    keys.reserve(keyframesCache.size());
    for(KeyframeMapIterator keyIt = keyframesCache.begin(); keyIt != keyframesCache.end(); keyIt++)
    {
        Keyframe* key = &keyIt->second;
        if (!key)
            continue;
        gluProject(key->worldPosition.x, key->worldPosition.y, key->worldPosition.z, modelview_matrix, projection_matrix, viewport, &key->projPosition.x, &key->projPosition.y, &key->projPosition.z);
        keys.push_back(key);
    }
    
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    
    drawKeyFrames(keys, size, colorMultiplier, portWidth, portHeight, showRotationKeyframes);
}

void drawUtils::convertWorldSpaceToCameraSpace(CameraCache* cachePtr, std::map<double, MPoint> &positions, std::map<double, MPoint> &screenSpacePositions)
{
    glMatrixMode(GL_MODELVIEW); //combinazione matrici model e inversa camera
    glPushMatrix();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    
    double modelview_matrix[16];
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview_matrix);
    double projection_matrix[16];
    glGetDoublev(GL_PROJECTION_MATRIX, projection_matrix);
    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    
    for(std::map<double, MPoint>::iterator it = positions.begin(); it != positions.end(); it++)
    {
        MPoint* point = &it->second;
        if (!point)
            continue;
        
        MPoint temp;
        gluProject(point->x, point->y, point->z, modelview_matrix, projection_matrix, viewport, &temp.x, &temp.y, &temp.z);
        
        MPoint *cp = &screenSpacePositions[it->first];
        cp->x = temp.x;
        cp->y = temp.y;
        cp->z = temp.z;
    }
    
    glMatrixMode(GL_PROJECTION);
    glPopMatrix(); 
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void drawUtils::drawFrameLabel(double frame, const MVector &framePos, M3dView &view, const double sizeOffset, const MColor &color, const MMatrix &refMatrix)
{
    glColor4d(color.r, color.g, color.b, color.a);
    
	MVector cameraUp(refMatrix(1, 0), refMatrix(1, 1), refMatrix(1, 2));
    
	short viewX, viewY;
	view.worldToView(framePos, viewX, viewY);
    
	MPoint point1;
	MVector vec1;
	view.viewToWorld(viewX, viewY + short(GlobalSettings::frameSize * sizeOffset), point1, vec1);
    
	double distance = (framePos - point1).length();
    
	vec1 *= distance;
    
	point1 += vec1;
    
	double up = (framePos - point1).length();
    
	MString frameStr;
	frameStr = frame;
    
	MPoint textPos = framePos + (cameraUp * up);
	view.drawText(frameStr, textPos, M3dView::kCenter);
}

/*
void drawUtils::drawCameraSpacePointWithColor(CameraCache *cachePtr, const MColor &color, const MPoint &position, const double size)
{
    if (position.z > 1 || position.z < 0)
        return;
    
    glMatrixMode(GL_MODELVIEW); //combinazione matrici model e inversa camera
    glPushMatrix();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, cachePtr->portWidth, cachePtr->portHeight, 0, 1, -1);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    drawUtils::drawPointWithColor(MVector(position.x, cachePtr->portHeight - position.y, 0), size, color);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void drawUtils::drawCameraSpaceFramesForSelection(M3dView &view, CameraCache *cachePtr, std::map<double, MPoint> &frameScreenSpacePositions)
{
    glMatrixMode(GL_MODELVIEW); //combinazione matrici model e inversa camera
    glPushMatrix();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, cachePtr->portWidth, cachePtr->portHeight, 0, 1, -1);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    for (std::map<double, MPoint>::iterator it = frameScreenSpacePositions.begin(); it != frameScreenSpacePositions.end(); ++it)
    {
        if (it->second.z > 1 || it->second.z < 0)
            continue;
        
        view.pushName(static_cast<int>(it->first));

        MVector temp(it->second.x, cachePtr->portHeight - it->second.y, 0);
        drawUtils::drawPoint(temp, GlobalSettings::frameSize);
        view.popName();
    }
    
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void drawUtils::drawCameraSpaceKeyFramesForSelection(M3dView &view, CameraCache *cachePtr, KeyframeMap &keyframesCache)
{
    
    glMatrixMode(GL_MODELVIEW); //combinazione matrici model e inversa camera
    glPushMatrix();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, cachePtr->portWidth, cachePtr->portHeight, 0, 1, -1);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    for(KeyframeMapIterator keyIt = keyframesCache.begin(); keyIt != keyframesCache.end(); keyIt++)
    {
        Keyframe* key = &keyIt->second;
        if (key->projPosition.z > 1 || key->projPosition.z < 0)
            continue;
        
        view.pushName(key->id);
        MVector temp(key->projPosition.x, cachePtr->portHeight - key->projPosition.y, 0);
        drawUtils::drawPoint(temp, GlobalSettings::frameSize * 1.2);
        view.popName();
    }
    
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void drawUtils::drawCameraSpaceFrames(CameraCache* cachePtr, const MColor &color, std::map<double, MPoint> &positions, const double startFrame, const double endFrame)
{
    glMatrixMode(GL_MODELVIEW); //combinazione matrici model e inversa camera
    glPushMatrix();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, cachePtr->portWidth, cachePtr->portHeight, 0, 1, -1);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    MVector previousPosition = positions[startFrame];
    for (double i = startFrame+1; i <= endFrame; ++i)
    {
        MVector point = positions[i];
        if (point.z > 1 || point.z < 0 || previousPosition.z > 1 || previousPosition.z < 0)
        {
            previousPosition = point;
            continue;
        }
        
        MVector tempPrev(previousPosition.x, cachePtr->portHeight - previousPosition.y, 0);
        MVector temp(point.x, cachePtr->portHeight - point.y, 0);
        
        if (GlobalSettings::showPath)
        {
            double factor = 1;
            if (GlobalSettings::alternatingFrames)
                factor = int(i) % 2 == 1 ? 1.4 : 0.6;
            
            drawUtils::drawLineWithColor(tempPrev, temp, GlobalSettings::pathSize, color * factor);
        }

        drawPointWithColor(tempPrev, GlobalSettings::frameSize, color);
        previousPosition = point;
        
        if (i == endFrame)
            drawUtils::drawPointWithColor(temp, GlobalSettings::frameSize, color);
        
    }
    
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void drawUtils::convertKeyFrameTangentsToCameraSpace(CameraCache *cachePtr, KeyframeMap &keyframesCache, const MMatrix &currentCameraMatrix)
{
    glMatrixMode(GL_MODELVIEW); //combinazione matrici model e inversa camera
    glPushMatrix();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    
    double modelview_matrix[16];
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview_matrix);
    double projection_matrix[16];
    glGetDoublev(GL_PROJECTION_MATRIX, projection_matrix);
    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    
    MVector tempIn;
    for(KeyframeMapIterator keyIt = keyframesCache.begin(); keyIt != keyframesCache.end(); keyIt++)
    {
        Keyframe* key = &keyIt->second;
        if (!key)
            continue;
         
        if (key->showInTangent)
            gluProject(key->inTangentScreenSpace.x, key->inTangentScreenSpace.y, key->inTangentScreenSpace.z, modelview_matrix, projection_matrix, viewport, &key->inTangentProjected.x, &key->inTangentProjected.y, &key->inTangentProjected.z);
         
        if (key->showOutTangent)
            gluProject(key->outTangentScreenSpace.x, key->outTangentScreenSpace.y, key->outTangentScreenSpace.z, modelview_matrix, projection_matrix, viewport, &key->outTangentProjected.x, &key->outTangentProjected.y, &key->outTangentProjected.z);
    }
    
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void drawUtils::drawCameraSpaceKeyFrameTangents(CameraCache *cachePtr, KeyframeMap &keyframesCache)
{
    glMatrixMode(GL_MODELVIEW); //combinazione matrici model e inversa camera
    glPushMatrix();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, cachePtr->portWidth, cachePtr->portHeight, 0, 1, -1);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    MColor tangentColor;
    for(KeyframeMapIterator keyIt = keyframesCache.begin(); keyIt != keyframesCache.end(); keyIt++)
    {
        Keyframe* key = &keyIt->second;
        if (!key)
            continue;
        
        if (key->projPosition.z > 1 || key->projPosition.z < 0)
            continue;
        
        tangentColor = key->tangentsLocked ? GlobalSettings::tangentColor : GlobalSettings::brokenTangentColor;
        
        MVector p = key->projPosition;
        p.y = cachePtr->portHeight - key->projPosition.y;
        p.z = 0;
        
        if (key->showInTangent)
        {
            MVector inT = key->inTangentProjected;
            inT.y = cachePtr->portHeight - key->inTangentProjected.y;
            inT.z = 0;
            
            drawUtils::drawLineWithColor(p, inT, 1.0, tangentColor);
            drawUtils::drawPointWithColor(inT, GlobalSettings::frameSize, tangentColor);
        }
        
        if (key->showOutTangent)
		{
            MVector outT = key->outTangentProjected;
            outT.y = cachePtr->portHeight - key->outTangentProjected.y;
            outT.z = 0;
            
            drawUtils::drawLineWithColor(p, outT, 1.0, tangentColor);
            drawUtils::drawPointWithColor(outT, GlobalSettings::frameSize, tangentColor);
        }

    }
    
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

}

void drawUtils::drawCameraSpaceKeyFrameTangentsForSelection(M3dView &view, CameraCache *cachePtr, KeyframeMap &keyframesCache)
{
    glMatrixMode(GL_MODELVIEW); //combinazione matrici model e inversa camera
    glPushMatrix();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, cachePtr->portWidth, cachePtr->portHeight, 0, 1, -1);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    for(KeyframeMapIterator keyIt = keyframesCache.begin(); keyIt != keyframesCache.end(); keyIt++)
    {
        Keyframe* key = &keyIt->second;
        if (key->projPosition.z > 1 || key->projPosition.z < 0)
            continue;
        
        view.pushName(key->id);
        
        if (key->showInTangent)
        {
            view.pushName((int)Keyframe::kInTangent);
            MVector temp(key->inTangentProjected.x, cachePtr->portHeight - key->inTangentProjected.y, 0);
            drawUtils::drawPoint(temp, GlobalSettings::frameSize);
            view.popName();
        }
        
        if (key->showOutTangent)
        {
            view.pushName((int)Keyframe::kOutTangent);
            MVector temp(key->outTangentProjected.x, cachePtr->portHeight - key->outTangentProjected.y, 0);
            drawUtils::drawPoint(temp, GlobalSettings::frameSize);
            view.popName();
        }
        
        view.popName();
    }
    
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}
 */
