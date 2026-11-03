#pragma once
#include "Renderer.h"
#include "DrawingWindow.h"
#include "RayTriangleIntersection.h"

class RayTracedRenderer : public Renderer
{
public:
    RayTracedRenderer();
    void draw(DrawingWindow &window) override;

private:
    // Core raytracing skeleton methods
    RayTriangleIntersection getClosestValidIntersection(glm::vec3 cameraPosition, glm::vec3 rayDirection);
    Colour calculateLighting(glm::vec3 intersectionPoint, glm::vec3 normal, Colour baseColour);
    bool isInShadow(glm::vec3 point, glm::vec3 normal);
};