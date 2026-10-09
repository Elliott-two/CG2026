#include "PointCloudRenderer.h"
#include <vector>
#include <cmath>

extern const int WIDTH;
extern const int HEIGHT;
extern std::vector<ModelTriangle> triangles;
extern glm::vec3 cameraPosition;
extern float focalLength;
static const float IMAGE_SCALE = 500.0f;

void PointCloudRenderer::draw(DrawingWindow &window)
{
    window.clearPixels();
    resetDepthBuffer();
    for (const ModelTriangle &t : triangles)
    {
        CanvasPoint v0 = projectVertexOnToCanvasPoint(cameraPosition, focalLength, t.vertices[0]);
        CanvasPoint v1 = projectVertexOnToCanvasPoint(cameraPosition, focalLength, t.vertices[1]);
        CanvasPoint v2 = projectVertexOnToCanvasPoint(cameraPosition, focalLength, t.vertices[2]);
        filledTriangle(window, CanvasTriangle(v0, v1, v2), t.colour);
    }
}

// projectVertexOnToCanvasPoint stays exactly as you have it
CanvasPoint PointCloudRenderer::projectVertexOnToCanvasPoint(glm::vec3 cameraPosition, float focalLength, glm::vec3 vertexPosition)
{
    glm::vec3 rel = vertexPosition - cameraPosition;
    if (rel.z == 0.0f)
        rel.z = 0.0001f;
    float u = (-focalLength * (rel.x / rel.z) * IMAGE_SCALE) + (WIDTH / 2.0f);
    float v = (focalLength * (rel.y / rel.z) * IMAGE_SCALE) + (HEIGHT / 2.0f);
    float depth = -1.0f / rel.z;
    return CanvasPoint(u, v, depth);
}