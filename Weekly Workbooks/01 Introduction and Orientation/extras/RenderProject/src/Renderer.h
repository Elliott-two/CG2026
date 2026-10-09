#pragma once

#include <DrawingWindow.h>
#include <ModelTriangle.h>
#include <glm/glm.hpp>
#include <Colour.h>
#include <Utils.h>
#include <CanvasPoint.h>
#include <CanvasTriangle.h>
#include <TextureMap.h>
// Use the external WIDTH and HEIGHT values defined in the MainApplication file
extern const int WIDTH;
extern const int HEIGHT;

class Renderer
{
protected:
   int redMax = 255;
   int greenMax = 255;
   int blueMax = 255;
   const int ALPHA = 255; // Fully opaque, unchanged in this course

public:
   virtual void draw(DrawingWindow &window);
   void adjustRed(int delta);
   void adjustGreen(int delta);
   void adjustBlue(int delta);
   std::vector<float> interpolateSingleFloats(float from, float to, int numberOfValues);
   std::vector<glm::vec3> interpolateThreeElementValues(glm::vec3 from, glm::vec3 to, int numberOfValues);
};
