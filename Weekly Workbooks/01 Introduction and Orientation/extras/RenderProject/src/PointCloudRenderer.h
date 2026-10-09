#pragma once
#include "RasterisedRenderer.h"

class PointCloudRenderer : public RasterisedRenderer
{
public:
    void draw(DrawingWindow &window) override;
    CanvasPoint projectVertexOnToCanvasPoint(glm::vec3 cameraPosition, float focalLength, glm::vec3 vertexPosition);
};