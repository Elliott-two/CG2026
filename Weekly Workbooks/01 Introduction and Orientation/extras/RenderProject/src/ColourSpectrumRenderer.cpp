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
      // The y level divided by the total window height
      float yRatio = float(y) / float(window.height - 1);

      // Equations for colour calculations
      glm::vec3 leftColour = topLeft * (1.0f - yRatio) + bottomLeft * yRatio;
      glm::vec3 rightColour = topRight * (1.0f - yRatio) + bottomRight * yRatio;

      // Using helper to interpolate colours
      std::vector<glm::vec3> rowColours = interpolateThreeElementValues(leftColour, rightColour, window.width);

      // Every x pixel
      for (size_t x = 0; x < window.width; x++)
      {
         // Set the pixel colour to the interpolated colour
         glm::vec3 colourVec = rowColours[x];
         uint32_t colour = (ALPHA << 24) + (int(colourVec.r) << 16) + (int(colourVec.g) << 8) + int(colourVec.b);
         window.setPixelColour(x, y, colour);
      }
   }
}
