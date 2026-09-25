#include "TriangleSpectrumRenderer.h"

void TriangleSpectrumRenderer::draw(DrawingWindow &window)
{
    window.clearPixels();
    glm::vec3 bottomLeft(255, 0, 0);  // red
    glm::vec3 topMiddle(0, 255, 0);   // green
    glm::vec3 bottomRight(0, 0, 255); // blue

    for (int y = 0; y < window.height; ++y)
    {
        for (int x = 0; x < window.width; ++x)
        {
            // Calculate the barycentric coordinates of the pixel
            glm::vec2 p(x, y);
            glm::vec2 v0((window.width - 1) / 2, 0);
            glm::vec2 v1((window.width - 1), window.height - 1);
            glm::vec2 v2(0, window.height - 1);
            glm::vec3 barycentricCoords = convertToBarycentricCoordinates(v0, v1, v2, p);
            if (barycentricCoords.x < 0 || barycentricCoords.y < 0 || barycentricCoords.z < 0)
            {
                // Pixel is outside the triangle, skip it
                continue;
            }
            // Interpolate the color based on the barycentric coordinates
            glm::vec3 color =
                barycentricCoords.x * bottomRight +
                barycentricCoords.y * bottomLeft +
                barycentricCoords.z * topMiddle;

            // Set the pixel color in the window
            uint32_t pixelColor = (static_cast<uint32_t>(color.r) << 16) |
                                  (static_cast<uint32_t>(color.g) << 8) |
                                  static_cast<uint32_t>(color.b);
            window.setPixelColour(x, y, pixelColor);
        }
    }
}
