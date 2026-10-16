#include "PointCloudRenderer.h"
#include <vector>
#include <cmath>

// Stopping and starting orbiting
bool OrbitXBool = false;
bool OrbitYBool = false;

// Useful for cleaner code
namespace
{
    // Image Scale
    constexpr float IMAGE_SCALE = 500.0f;

    // Speed of orbit
    constexpr float ORBIT_Y_DEGREES_PER_FRAME = 0.5f;
    constexpr float ORBIT_X_DEGREES_PER_FRAME = 0.5f;

    // PI
    constexpr float PI = 3.14159265358979323846f;

    // Rotate Y for orbit
    glm::mat3 rotationY(float degrees)
    {
        double degreeInRadians = degrees * (PI / 180.0);
        double cos_value = std::cos(degreeInRadians);
        double sin_value = std::sin(degreeInRadians);

        glm::mat3 rotationMatrix = glm::mat3(
            glm::vec3(cos_value, 0.0, -sin_value),
            glm::vec3(0.0, 1.0, 0.0),
            glm::vec3(sin_value, 0.0, cos_value));
        return rotationMatrix;
    }

    // Rotate X for orbit
    glm::mat3 rotationX(float degrees)
    {
        double degreeInRadians = degrees * (PI / 180.0);
        double cos_value = std::cos(degreeInRadians);
        double sin_value = std::sin(degreeInRadians);

        glm::mat3 rotationMatrix = glm::mat3(
            glm::vec3(1.0, 0.0, 0.0),
            glm::vec3(0.0, cos_value, sin_value),
            glm::vec3(0.0, -sin_value, cos_value));
        return rotationMatrix;
    }

    // Check for textured
    bool isTextured(const ModelTriangle &triangle)
    {
        for (const TexturePoint &texturePoint : triangle.texturePoints)
        {

            if (texturePoint.x != 0.0f || texturePoint.y != 0.0f)
            {
                return true;
            }
        }
        return false;
    }
}

// Orbit code
void PointCloudRenderer::orbitY(float degrees)
{
    const glm::mat3 rotationMatrix = rotationY(degrees);
    cameraPosition = rotationMatrix * cameraPosition;
    cameraOrientation = rotationMatrix * cameraOrientation;
}

// Orbit code
void PointCloudRenderer::orbitX(float degrees)
{
    const glm::mat3 rotationMatrix = rotationX(degrees);
    cameraPosition = rotationMatrix * cameraPosition;
    cameraOrientation = rotationMatrix * cameraOrientation;
}

// Main Drawing
void PointCloudRenderer::draw(DrawingWindow &window)
{
    // Clear and reset
    window.clearPixels();
    resetDepthBuffer();

    // Check orbiting and not exclusive
    if (OrbitXBool)
    {
        orbitX(ORBIT_X_DEGREES_PER_FRAME);
    }
    if (OrbitYBool)
    {
        orbitY(ORBIT_Y_DEGREES_PER_FRAME);
    }

    // For all the triangles
    for (const ModelTriangle &triangle : triangles)
    {
        // Convert vertexs
        CanvasPoint vertex[3];
        for (int i = 0; i < 3; i++)
        {
            vertex[i] = projectVertexOnToCanvasPoint(cameraPosition, focalLength, triangle.vertices[i]);
        }

        // Check textured
        if (isTextured(triangle))
        {
            // If textured, loop through textures with depth
            for (int i = 0; i < 3; i++)
            {
                vertex[i].texturePoint = TexturePoint(
                    triangle.texturePoints[i].x * vertex[i].depth,
                    triangle.texturePoints[i].y * vertex[i].depth);
            }

            // Textured
            texturedModelTriangle(
                window,
                CanvasTriangle(vertex[0], vertex[1], vertex[2]), textureMap);
        }
        else
        {
            // Regular filled
            filledTriangle(
                window,
                CanvasTriangle(vertex[0], vertex[1], vertex[2]), triangle.colour);
        }
    }
}

// Project onto canvas
CanvasPoint PointCloudRenderer::projectVertexOnToCanvasPoint(glm::vec3 cameraPosition, float focalLength, glm::vec3 vertexPosition)
{
    glm::vec3 relative = glm::inverse(cameraOrientation) * (vertexPosition - cameraPosition);
    if (relative.z == 0.0f)
        relative.z = 0.0001f;
    float u = (-focalLength * (relative.x / relative.z) * IMAGE_SCALE) + (WIDTH / 2.0f);
    float v = (focalLength * (relative.y / relative.z) * IMAGE_SCALE) + (HEIGHT / 2.0f);
    float depth = -1.0f / relative.z;
    return CanvasPoint(u, v, depth);
}