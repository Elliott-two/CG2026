#include "RayTracedRenderer.h"

extern const int WIDTH;
extern const int HEIGHT;
extern std::vector<ModelTriangle> triangles;
extern glm::vec3 cameraPosition;
extern glm::mat3 cameraOrientation;
extern float focalLength;
extern glm::vec3 lightPosition;
extern std::vector<std::array<glm::vec3, 3>> vertexNormalsByTriangle;

constexpr float PI = 3.14159265358979323846f;
constexpr float IMAGE_SCALE = 500.0f;
constexpr float LIGHT_INTENSITY = 10.0f;
constexpr float AMBIENT = 0.25f;
constexpr float SPECULAR_EXPONENT = 64.0f;

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

    for (int x = 0; x < WIDTH; ++x)
    {
        for (int y = 0; y < HEIGHT; ++y)
        {
            // Calculate the direction of the ray from the camera through the pixel
            float u = (x - WIDTH / 2.0f) / (IMAGE_SCALE * focalLength);
            float v = (HEIGHT / 2.0f - y) / (IMAGE_SCALE * focalLength);

            // Normailise for consistency
            glm::vec3 rayDirection = glm::normalize(cameraOrientation * glm::vec3(u, v, -1.0f));

            // Get the closest intersection of the ray with the triangles
            RayTriangleIntersection intersection = getClosestValidIntersection(cameraPosition, rayDirection);

            if (intersection.distanceFromCamera > 0) // Valid intersection found
            {
                // Calculate lighting at the intersection point
                // Previous methods
                // Colour pixelColour = calculateLighting(intersection.intersectionPoint, intersection.intersectedTriangle.normal, intersection.intersectedTriangle.colour);
                // Colour pixelColour = baryentricColourWeights(intersection, gourandShading(intersection));

                Colour pixelColour = phongShading(intersection);
                window.setPixelColour(x, y, (pixelColour.red << 16) | (pixelColour.green << 8) | pixelColour.blue);
            }
            else
            {
                // No intersection, set background colour to black
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

    // Flag to check if any valid intersection was found
    bool foundIntersection = false;

    // Index based loop for normal vector
    for (size_t i = 0; i < triangles.size(); ++i)
    {
        const ModelTriangle &triangle = triangles[i];

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

        // Check for valid intersection: t > 0, u >= 0, v >= 0, and u + v <= 1
        if (t > 0 && u >= 0.0f && v >= 0.0f && (u + v) <= 1.0f)
        {
            // Update the closest intersection if this one is closer
            if (t < closestIntersection.distanceFromCamera)
            {
                foundIntersection = true;
                closestIntersection.distanceFromCamera = t;
                closestIntersection.intersectionPoint = startPoint + rayDirection * t;
                closestIntersection.intersectedTriangle = triangle;

                // Index into vertextNormalsByTriangles
                closestIntersection.triangleIndex = i;
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

Colour RayTracedRenderer::calculateLighting(glm::vec3 point, glm::vec3 normal, Colour baseColour, glm::vec3 faceNormal)
{
    // Normal used for shading
    glm::vec3 n = glm::normalize(normal);

    // Offset to avoid shadow acne
    // Checking same triangle didnt work for me
    glm::vec3 origin = point + faceNormal * 0.005f;

    // Calculate the direction to the light source and its distance
    glm::vec3 toLight = lightPosition - origin;

    // Using glm::length to calculate the distance to the light source
    // As given in lectures
    float distanceToLight = glm::length(toLight);

    // Normalize the direction to the light source
    glm::vec3 lightDir = toLight / distanceToLight;

    // Bool to check if the point is in shadow
    bool shadowed = false;

    // Only if its between the point and the light source, then its in shadow
    if (glm::dot(faceNormal, lightDir) > 0.0f)
    {
        RayTriangleIntersection hit = getClosestValidIntersection(origin, lightDir);
        // Only in shadow if the blocker is between the point and the light
        shadowed = hit.distanceFromCamera > 0.0f && hit.distanceFromCamera < distanceToLight;
    }

    // Default ambient defined above
    float brightness = AMBIENT;
    float specular = 0.0f;

    // Check Shadowed
    if (!shadowed)
    {
        // Calculate the angle of incidence using the dot product
        float incidentAngle = glm::dot(n, lightDir);

        // Calculate using SCIENCE?!?! (Might change if it looks bad)
        float proximity = LIGHT_INTENSITY / (4.0f * PI * distanceToLight * distanceToLight);

        // Add the proximity calculation to the brightness, this means that nothing is dimmer than the ambient light
        brightness += proximity * std::max(incidentAngle, 0.0f);

        // If the incident angle is positive, calculate the specular reflection component
        if (incidentAngle > 0.0f)
        {
            // Reflect the light direction about the normal
            glm::vec3 reflected = (2.0f * glm::dot(lightDir, n) * n) - lightDir;

            // Calculate the view direction from the point to the camera
            glm::vec3 viewDir = glm::normalize(cameraPosition - point);

            // Specular component from given formula
            // Looks nicer like this
            specular = 0.5f * std::pow(
                                  std::max(
                                      glm::dot(
                                          reflected,
                                          viewDir),
                                      0.0f),
                                  SPECULAR_EXPONENT);
        }
    }

    // Clamp the brightness to ensure it stays within the range [0, 1]
    // Given by VSCode Intellisense
    brightness = glm::clamp(brightness, 0.0f, 1.0f);

    // Return the final colour by scaling the base colour by the calculated brightness

    return Colour(
        std::min(255, int(std::round(baseColour.red * brightness + 255.0f * specular))),
        std::min(255, int(std::round(baseColour.green * brightness + 255.0f * specular))),
        std::min(255, int(std::round(baseColour.blue * brightness + 255.0f * specular))));
}

std::array<Colour, 3> RayTracedRenderer::gourandShading(RayTriangleIntersection intersection)
{
    // Define the triangle and get the vertex normals
    const ModelTriangle &triangle = intersection.intersectedTriangle;

    // Bounds-checked lookup by index, falling back to the flat normal
    std::array<glm::vec3, 3> normals = {triangle.normal, triangle.normal, triangle.normal};
    if (intersection.triangleIndex < vertexNormalsByTriangle.size())
        normals = vertexNormalsByTriangle[intersection.triangleIndex];

    // Light each vertex using its smooth normal (and the true face normal for shadows)
    std::array<Colour, 3> colours;
    for (int i = 0; i < 3; ++i)
        colours[i] = calculateLighting(triangle.vertices[i], normals[i], triangle.colour, triangle.normal);
    return colours;
}

std::array<glm::vec3, 3> RayTracedRenderer::vertexNormals(const ModelTriangle &triangle)
{
    float smoothThreshold = -1.0f; // include all faces sharing the vertex
    std::array<glm::vec3, 3> normals;

    for (int i = 0; i < 3; ++i)
    {
        glm::vec3 vertex = triangle.vertices[i];
        glm::vec3 normalSum(0.0f);

        // Loop through all triangles to find those that share the vertex
        for (const ModelTriangle &other : triangles)
        {
            if (other.vertices[0] != vertex && other.vertices[1] != vertex && other.vertices[2] != vertex)
                continue;

            // Calculate the normal of the other triangle
            glm::vec3 otherNormal = glm::normalize(glm::cross(other.vertices[1] - other.vertices[0], other.vertices[2] - other.vertices[0]));

            // Check if the angle between the normals is within the smooth threshold
            // If the dot product is greater than the threshold, they are considered smooth
            if (glm::dot(otherNormal, triangle.normal) > smoothThreshold)
                normalSum += otherNormal;
        }
        // Normalize the summed normal to get the average normal for the vertex
        normals[i] = glm::normalize(normalSum);
    }
    return normals;
}

// Not used for Phong but for Gouraud Shading
Colour RayTracedRenderer::baryentricColourWeights(RayTriangleIntersection intersection, const std::array<Colour, 3> &vertexColours)
{
    // Using the helper
    glm::vec3 weights = barycentricWeights(intersection);
    float u = weights.x;
    float v = weights.y;
    float w = weights.z;

    // Calculate the final colour by interpolating the vertex colours using the barycentric weights
    int red = int(std::round(u * vertexColours[0].red + v * vertexColours[1].red + w * vertexColours[2].red));
    int green = int(std::round(u * vertexColours[0].green + v * vertexColours[1].green + w * vertexColours[2].green));
    int blue = int(std::round(u * vertexColours[0].blue + v * vertexColours[1].blue + w * vertexColours[2].blue));

    return Colour(red, green, blue);
}

// Same as the previous but without colour
glm::vec3 RayTracedRenderer::barycentricWeights(const RayTriangleIntersection &intersection)
{
    // Get the triangle and intersection point
    const ModelTriangle &triangle = intersection.intersectedTriangle;
    glm::vec3 point = intersection.intersectionPoint;

    glm::vec3 normal = glm::cross(triangle.vertices[1] - triangle.vertices[0], triangle.vertices[2] - triangle.vertices[0]);
    float denominator = glm::dot(normal, normal);

    float u = glm::dot(normal, glm::cross(triangle.vertices[2] - triangle.vertices[1], point - triangle.vertices[1])) / denominator;
    float v = glm::dot(normal, glm::cross(triangle.vertices[0] - triangle.vertices[2], point - triangle.vertices[2])) / denominator;
    return glm::vec3(u, v, 1.0f - u - v);
}

// Phong shading implementation
Colour RayTracedRenderer::phongShading(const RayTriangleIntersection &intersection)
{
    // Get the needed fields for aesthetic calling
    Colour colour = intersection.intersectedTriangle.colour;
    glm::vec3 triangleNormal = intersection.intersectedTriangle.normal;

    // Look up the precomputed vertex normals by triangle index (no silent zero insertion)
    if (intersection.triangleIndex >= vertexNormalsByTriangle.size())
    {
        // Fall back to flat shading if something is out of sync
        return calculateLighting(intersection.intersectionPoint, triangleNormal, colour, triangleNormal);
    }
    const std::array<glm::vec3, 3> &normals = vertexNormalsByTriangle[intersection.triangleIndex];

    glm::vec3 weights = barycentricWeights(intersection);

    // Interpolate the normal, then renormalise (interpolated normals are shorter than 1)
    glm::vec3 interpolated = weights.x * normals[0] + weights.y * normals[1] + weights.z * normals[2];
    glm::vec3 normal = (glm::length(interpolated) > 1e-6f) ? glm::normalize(interpolated) : triangleNormal;

    // Light the actual hit point with the interpolated normal
    return calculateLighting(intersection.intersectionPoint, normal, colour, triangleNormal);
}