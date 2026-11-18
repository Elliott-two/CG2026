#pragma once
#include "Renderer.h"
#include "RayTriangleIntersection.h"
#include "PhotonMap.h"

class RayTracedRenderer : public Renderer
{
public:
    void draw(DrawingWindow &window) override;
    void focusOnPixel(int x, int y);

private:
    // Core raytracing functions
    RayTriangleIntersection getClosestValidIntersection(glm::vec3 cameraPosition, glm::vec3 rayDirection);
    Colour calculateLighting(glm::vec3 point, glm::vec3 normal, Colour baseColour, glm::vec3 faceNormal);
    Colour texturedShading(RayTriangleIntersection intersection);

    // Shading
    std::array<Colour, 3> gourandShading(RayTriangleIntersection intersection);
    Colour phongShading(const RayTriangleIntersection &intersection);
    RayTriangleIntersection mirrorShading(RayTriangleIntersection intersection);
    Colour depthOfFieldShading(float u, float v, const glm::vec3 &rayDirection);

    // Helpers
    glm::vec2 randomPointInCircle(float radius);
    glm::vec2 randomPointInCircleSwirl(float radius, float u, float v, float swirlFactor);
    glm::vec2 randomPointInReflexLens(float outerRadius, float innerRatio);
    Colour shadingHelper(RayTriangleIntersection intersection);
    Colour enviornmentHelper(const glm::vec3 &direction);

    // Ray tracing functions
    Colour traceRay(const glm::vec3 &origin, const glm::vec3 &direction, int depth, const glm::vec3 &mediumAbsorption = glm::vec3(0.0f), bool insideMedium = false);
    Colour mirrorredRay(const RayTriangleIntersection &intersection, const glm::vec3 &direction, int depth, const glm::vec3 &mediumAbsorption, bool insideMedium, float distanceThroughMedium);
    Colour refractedRay(const RayTriangleIntersection &intersection, const glm::vec3 &direction, int depth, const glm::vec3 &mediumAbsorption, bool entering, float distanceThroughMedium);
    Colour mettalicRay(const RayTriangleIntersection &intersection, const glm::vec3 &direction, int depth, const glm::vec3 &mediumAbsorption, bool insideMedium, float distanceThroughMedium);

    void tracePhoton(glm::vec3 origin, glm::vec3 direction, glm::vec3 power, bool insideMedium = false, bool cameFromSpecular = false, int depth = 0, int diffuseDepth = 0);
    PhotonMap causticPhotonMap;
    PhotonMap globalPhotonMap;
    void buildPhotonMaps();

    // Barycentric
    glm::vec3 barycentricWeights(const RayTriangleIntersection &intersection);
    Colour baryentricColourWeights(RayTriangleIntersection intersection, const std::array<Colour, 3> &vertexColours);
};