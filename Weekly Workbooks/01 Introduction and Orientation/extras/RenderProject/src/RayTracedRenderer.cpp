#include "RayTracedRenderer.h"

extern const int WIDTH;
extern const int HEIGHT;
extern std::vector<ModelTriangle> triangles;
extern glm::vec3 cameraPosition;
extern glm::mat3 cameraOrientation;
extern float focalLength;
extern glm::vec3 lightPosition;

constexpr float IMAGE_SCALE = 500.0f;

RayTracedRenderer::RayTracedRenderer() {}

void RayTracedRenderer::draw(DrawingWindow &window)
{
    // For easier loading speeds
    static glm::vec3 prevCameraPosition = glm::vec3(std::numeric_limits<float>::max());
    static glm::mat3 prevCameraOrientation = glm::mat3(0.0f);
    static bool frameRendered = false;

    // Check if the camera has moved or rotated since the last frame
    bool cameraMoved = (cameraPosition != prevCameraPosition) || (cameraOrientation != prevCameraOrientation);

    // If the camera hasn't moved and we already rendered the frame, skip recalculating
    if (!cameraMoved && frameRendered)
    {
        return;
    }

    for (int x = 0; x < window.width; ++x)
    {
        for (int y = 0; y < window.height; ++y)
        {
            // Calculate the direction of the ray from the camera through the pixel
            float u = (x - window.width / 2.0f) / (IMAGE_SCALE * focalLength);
            float v = (window.height / 2.0f - y) / (IMAGE_SCALE * focalLength);

            glm::vec3 rayDirection = glm::normalize(cameraOrientation * glm::vec3(u, v, -1.0f));

            // Get the closest intersection of the ray with the triangles
            RayTriangleIntersection intersection = getClosestValidIntersection(cameraPosition, rayDirection);

            if (intersection.distanceFromCamera > 0) // Valid intersection found
            {
                // Calculate lighting at the intersection point
                Colour pixelColour = calculateLighting(intersection.intersectionPoint, intersection.intersectedTriangle.normal, intersection.intersectedTriangle.colour);
                window.setPixelColour(x, y, (255u << 24) | (pixelColour.red << 16) | (pixelColour.green << 8) | pixelColour.blue);
            }
            else
            {
                // No intersection, set background colour (e.g., black)
                window.setPixelColour(x, y, 0xFF000000); // Black with full opacity
            }
        }
    }

    prevCameraPosition = cameraPosition;
    prevCameraOrientation = cameraOrientation;
    frameRendered = true;
}

RayTriangleIntersection RayTracedRenderer::getClosestValidIntersection(glm::vec3 startPoint, glm::vec3 rayDirection)
{
    // Initialise the closest intersection with a large distance
    RayTriangleIntersection closestIntersection;
    closestIntersection.distanceFromCamera = std::numeric_limits<float>::max();
    closestIntersection.intersectionPoint = glm::vec3(0.0f);
    bool foundIntersection = false;

    for (const ModelTriangle &triangle : triangles)
    {
        // Given formula
        glm::vec3 e0 = triangle.vertices[1] - triangle.vertices[0];
        glm::vec3 e1 = triangle.vertices[2] - triangle.vertices[0];
        glm::vec3 SPVector = startPoint - triangle.vertices[0];
        glm::mat3 DEMatrix(-rayDirection, e0, e1);

        glm::vec3 possibleSolution = glm::inverse(DEMatrix) * SPVector;

        // Extract t, u, v from the possible solution
        float t = possibleSolution.x;
        float u = possibleSolution.y;
        float v = possibleSolution.z;

        // Sanity check for valid intersection: t > 0, u >= 0, v >= 0, and u + v <= 1
        if (t > 0 && u >= 0.0f && v >= 0.0f && (u + v) <= 1.0f)
        {
            // Update the closest intersection if this one is closer
            if (t < closestIntersection.distanceFromCamera)
            {
                foundIntersection = true;
                closestIntersection.distanceFromCamera = t;
                closestIntersection.intersectionPoint = startPoint + rayDirection * t;
                closestIntersection.intersectedTriangle = triangle;
                closestIntersection.intersectedTriangle.normal = glm::normalize(glm::cross(e0, e1));
            }
        }
    }
    if (!foundIntersection)
    {
        // If no intersection was found, set the distance to -1 to indicate no intersection
        closestIntersection.distanceFromCamera = -1.0f;
    }

    return closestIntersection;
}

Colour RayTracedRenderer::calculateLighting(glm::vec3 intersectionPoint, glm::vec3 normal, Colour baseColour)
{

    if (isInShadow(intersectionPoint, normal))
    {
        // Define a custom shadow color or scaling factor
        const float shadowFactor = 0.3f; // Adjust darkness as desired
        return Colour(int(baseColour.red * shadowFactor),
                      int(baseColour.green * shadowFactor),
                      int(baseColour.blue * shadowFactor));
    }

    // Non-shadowed pixels display their standard unlit base color
    return baseColour;
}

bool RayTracedRenderer::isInShadow(glm::vec3 point, glm::vec3 normal)
{
    // Nudge the origin off the surface so the ray doesn't hit its own triangle (shadow acne)
    glm::vec3 origin = point + glm::normalize(normal) * 0.002f;

    glm::vec3 toLight = lightPosition - origin;
    float distanceToLight = glm::length(toLight);
    glm::vec3 shadowDir = toLight / distanceToLight;

    RayTriangleIntersection hit = getClosestValidIntersection(origin, shadowDir);

    // Shadowed only if something is hit *between* the surface and the light
    return hit.distanceFromCamera > 0.0f && hit.distanceFromCamera < distanceToLight;
}