#pragma once

#include <DrawingWindow.h>
#include <ModelTriangle.h>
#include <glm/glm.hpp>
#include <Colour.h>
#include <Utils.h>

// Use the external WIDTH and HEIGHT values defined in the MainApplication file
extern const int WIDTH;
extern const int HEIGHT;

class Renderer
{
protected:
   int redMax = 255;
   int greenMax = 255;
   int blueMax = 255;

public:
   virtual void draw(DrawingWindow &window);
   void adjustRed(int delta);
   void adjustGreen(int delta);
   void adjustBlue(int delta);
   std::vector<float> interpolateSingleFloats(float from, float to, int numberOfValues);
   std::vector<glm::vec3> interpolateThreeElementValues(glm::vec3 from, glm::vec3 to, int numberOfValues);
};
