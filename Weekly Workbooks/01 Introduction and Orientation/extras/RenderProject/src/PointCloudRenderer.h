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

class PointCloudRenderer : public RasterisedRenderer
{
public:
    void draw(DrawingWindow &window) override;
    CanvasPoint projectVertexOnToCanvasPoint(glm::vec3 cameraPosition, float focalLength, glm::vec3 vertexPosition);
    void orbitY(float degrees);
    void orbitX(float degrees);
};