#include "ColourSpectrumRenderer.h"

void ColourSpectrumRenderer::draw(DrawingWindow &window)
{
   window.clearPixels();
   glm::vec3 topLeft(255, 0, 0);      // red
   glm::vec3 topRight(0, 0, 255);     // blue
   glm::vec3 bottomLeft(255, 255, 0); // yellow
   glm::vec3 bottomRight(0, 255, 0);  // green

   for (size_t y = 0; y < window.height; y++)
   {
      float yRatio = float(y) / float(window.height - 1);
      glm::vec3 leftColour = topLeft * (1.0f - yRatio) + bottomLeft * yRatio;
      glm::vec3 rightColour = topRight * (1.0f - yRatio) + bottomRight * yRatio;

      std::vector<glm::vec3> rowColours = interpolateThreeElementValues(leftColour, rightColour, window.width);

      for (size_t x = 0; x < window.width; x++)
      {
         glm::vec3 colourVec = rowColours[x];
         uint32_t colour = (255 << 24) + (int(colourVec.r) << 16) + (int(colourVec.g) << 8) + int(colourVec.b);
         window.setPixelColour(x, y, colour);
      }
   }
}
