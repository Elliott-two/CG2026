#pragma once
#include "RasterisedRenderer.h"

extern const int WIDTH;
extern const int HEIGHT;
extern std::vector<ModelTriangle> triangles;
extern glm::vec3 cameraPosition;
extern glm::mat3 cameraOrientation;
extern float focalLength;
extern bool OrbitXBool;
extern bool OrbitYBool;
extern TextureMap textureMap;
extern bool WireframeBool;

class PointCloudRenderer : public RasterisedRenderer
{
public:
    glm::mat3 rotationY(float degrees);
    glm::mat3 rotationX(float degrees);
    void orbitY(float degrees);
    void orbitX(float degrees);
    bool isTextured(const ModelTriangle &triangle);
    void draw(DrawingWindow &window) override;
    void drawStroked(DrawingWindow &window);
    CanvasPoint projectVertexOnToCanvasPoint(glm::vec3 cameraPosition, float focalLength, glm::vec3 vertexPosition);
};