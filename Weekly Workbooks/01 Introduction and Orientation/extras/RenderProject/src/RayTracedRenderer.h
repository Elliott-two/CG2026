#pragma once
#include "Renderer.h"
#include "RayTriangleIntersection.h"

class RayTracedRenderer : public Renderer
{
public:
    RayTracedRenderer();
    void draw(DrawingWindow &window) override;

private:
    // Core raytracing functions
    RayTriangleIntersection getClosestValidIntersection(glm::vec3 cameraPosition, glm::vec3 rayDirection);
    Colour calculateLighting(glm::vec3 point, glm::vec3 normal, Colour baseColour, glm::vec3 faceNormal);
    std::array<glm::vec3, 3> vertexNormals(const ModelTriangle &triangle);

    // Shading
    std::array<Colour, 3> gourandShading(RayTriangleIntersection intersection);
    Colour phongShading(const RayTriangleIntersection &intersection);

    // Barycentric
    glm::vec3 barycentricWeights(const RayTriangleIntersection &intersection);
    Colour baryentricColourWeights(RayTriangleIntersection intersection, const std::array<Colour, 3> &vertexColours);
};