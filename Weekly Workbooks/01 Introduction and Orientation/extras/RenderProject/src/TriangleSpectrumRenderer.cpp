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

            glm::vec2 p(x, y); // Our point in 2D space

            glm::vec2 tM((window.width - 1) / 2, 0);             // Top middle vertex
            glm::vec2 bR((window.width - 1), window.height - 1); // Bottom right vertex
            glm::vec2 bL(0, window.height - 1);                  // Bottom left vertex

            // Given Conversion Function
            glm::vec3 barycentricCoords = convertToBarycentricCoordinates(tM, bR, bL, p);
            if (barycentricCoords.x < 0 || barycentricCoords.y < 0 || barycentricCoords.z < 0)
            {
                // Pixel is outside the triangle
                continue;
            }
            // Interpolate the color based on the barycentric coordinates
            glm::vec3 color =
                barycentricCoords.x * bottomRight +
                barycentricCoords.y * bottomLeft +
                barycentricCoords.z * topMiddle;

            // Set the pixel color in the window
            uint32_t pixelColor = (ALPHA << 24) |
                                  (int(color.r) << 16) |
                                  (int(color.g) << 8) |
                                  (int(color.b));
            window.setPixelColour(x, y, pixelColor);
        }
    }
}
