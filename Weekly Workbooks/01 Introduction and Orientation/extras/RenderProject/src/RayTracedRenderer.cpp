#include "RayTracedRenderer.h"
#include <random>

// Get all the needed external variables and vectors
extern const int WIDTH;
extern const int HEIGHT;
extern std::vector<ModelTriangle> triangles;
extern std::vector<ModelTriangle> nonCulledTriangles;
extern glm::vec3 cameraPosition;
extern glm::mat3 cameraOrientation;

extern float focalLength;
extern float focalDistance;
extern float lenseSize;
extern int raysPerPixel;

extern glm::vec3 lightPosition;
extern std::vector<std::array<glm::vec3, 3>> vertexNormalsByTriangle;
extern std::vector<TextureMap> textures;

// Bools to check whether we want Gouraud or Phong shading
extern bool GouraudShadingEnabled;
extern bool PhongShadingEnabled;

// Flag to enable/disable world mirroring
extern bool worldMirroringEnabled;

// Flag to enable/disable depth of field effect
extern bool depthOfFieldEnabled;
// Flag to enable/disable swirl effect
extern bool swirlEnabled;
// Flag to enable/disable reflex lens effect
extern bool reflexLensEnabled;

// Bools to check if the sphere and boxes are showing, for video purposes
extern bool isSphereShowing;
extern bool isBlueBoxShowing;
extern bool isRedBoxShowing;

// All of my preset needed things
constexpr float PI = 3.14159265358979323846f;
constexpr float IMAGE_SCALE = 500.0f;
constexpr float LIGHT_INTENSITY = 10.0f;
constexpr float AMBIENT = 0.25f;
constexpr float SPECULAR_EXPONENT = 32.0f;
constexpr float REFLEX_RADIUS = 0.5f;
constexpr float SWIRL_FACTOR = 4.0f;

// Thread-local storage for the view origin, which is used in depth of field calculations
static thread_local glm::vec3 viewOrigin(0.0f);

// Bool to check if a ray intersects with a triangle using the Möller–Trumbore intersection algorithm
// Moller Trumbore algorithm came as a result of googling how to speed up ray tracing
static bool intersectTriangle(const ModelTriangle &triangle, const glm::vec3 &origin, const glm::vec3 &direction, float &triangleDistance)
{
    // Calculate the edges of the triangle
    const glm::vec3 edge0 = triangle.vertices[1] - triangle.vertices[0];
    const glm::vec3 edge1 = triangle.vertices[2] - triangle.vertices[0];

    // Calculate the determinant to check if the ray is parallel to the triangle
    const glm::vec3 point = glm::cross(direction, edge1);
    const float det = glm::dot(edge0, point);
    if (std::abs(det) < 1e-8f)
        return false; // ray parallel to triangle

    // Calculate the inverse determinant and the barycentric coordinates (u, v) to check if the intersection point is inside the triangle
    const float inverseDet = 1.0f / det;
    const glm::vec3 directionVector = origin - triangle.vertices[0];
    const float u = glm::dot(directionVector, point) * inverseDet;
    if (u < 0.0f || u > 1.0f)
        return false;

    // Calculate the second barycentric coordinate (v) and check if the intersection point is inside the triangle
    const glm::vec3 q = glm::cross(directionVector, edge0);
    const float v = glm::dot(direction, q) * inverseDet;
    if (v < 0.0f || u + v > 1.0f)
        return false;

    // Calculate the distance from the ray origin to the intersection point and return true if the intersection is valid
    triangleDistance = glm::dot(edge1, q) * inverseDet;

    // Return true if the intersection is valid (in front of the ray origin)
    return triangleDistance > 0.0f;
}

// Check if a ray is occluded by any triangle in the scene within a specified maximum distance
static bool isOccluded(const glm::vec3 &origin, const glm::vec3 &direction, float maxDistance)
{
    // Loop over all non-culled triangles and check for intersections with the ray
    for (const ModelTriangle &triangle : nonCulledTriangles)
    {
        // Calculate the distance to the intersection point if the ray intersects with the triangle
        float triangleDistance;
        // If the ray intersects with the triangle and the intersection point is closer than the maximum distance, return true (occluded)
        if (intersectTriangle(triangle, origin, direction, triangleDistance) && triangleDistance < maxDistance)
            return true;
    }
    return false;
}

void RayTracedRenderer::draw(DrawingWindow &window)
{

    // For easier loading speeds
    static glm::vec3 prevCameraPosition = glm::vec3(std::numeric_limits<float>::max());
    static glm::mat3 prevCameraOrientation = glm::mat3(0.0f);
    static bool frameRendered = false;

    // Read the current mode each frame so changes made by MainApplication are observed.
    const int shadingMode = GouraudShadingEnabled ? 1
                            : PhongShadingEnabled ? 2
                                                  : 0; // 1 for Gouraud, 2 for Phong, 0 for flat

    // Bools and ints to check if the shading mode or visibility of objects has changed since the last frame
    static int prevShadingMode = shadingMode;
    static bool prevSphereShowing = isSphereShowing;
    static bool prevBlueBoxShowing = isBlueBoxShowing;
    static bool prevRedBoxShowing = isRedBoxShowing;
    static bool prevWorldMirroringEnabled = worldMirroringEnabled;
    static bool prevDepthOfFieldEnabled = depthOfFieldEnabled;
    static float prevFocalDistance = focalDistance;
    static bool prevSwirlEnabled = swirlEnabled;
    static bool prevReflexLensEnabled = reflexLensEnabled;

    // Check for change
    if (prevShadingMode != shadingMode ||
        prevSphereShowing != isSphereShowing ||
        prevBlueBoxShowing != isBlueBoxShowing ||
        prevRedBoxShowing != isRedBoxShowing ||
        prevWorldMirroringEnabled != worldMirroringEnabled ||
        prevDepthOfFieldEnabled != depthOfFieldEnabled ||
        prevFocalDistance != focalDistance ||
        prevSwirlEnabled != swirlEnabled ||
        prevReflexLensEnabled != reflexLensEnabled)

    {
        frameRendered = false; // Force re-render if anything visible changed
        prevShadingMode = shadingMode;
        prevSphereShowing = isSphereShowing;
        prevBlueBoxShowing = isBlueBoxShowing;
        prevRedBoxShowing = isRedBoxShowing;
        prevWorldMirroringEnabled = worldMirroringEnabled;
        prevDepthOfFieldEnabled = depthOfFieldEnabled;
        prevFocalDistance = focalDistance;
        prevSwirlEnabled = swirlEnabled;
        prevReflexLensEnabled = reflexLensEnabled;
    }

    // Check if the camera has moved or rotated since the last frame
    bool cameraMoved = (cameraPosition != prevCameraPosition) || (cameraOrientation != prevCameraOrientation);

    // If the camera hasn't moved and we already rendered the frame, skip recalculating
    if (!cameraMoved && frameRendered)
    {
        return;
    }

    nonCulledTriangles.clear();

    for (const ModelTriangle &triangle : triangles)
    {
        // Skip rendering based on object name and visibility flags
        if ((triangle.objectName == "sphere" && !isSphereShowing) ||
            (triangle.objectName == "tall_box" && !isBlueBoxShowing) ||
            (triangle.objectName == "short_box" && !isRedBoxShowing))
        {
            continue; // Skip this triangle
        }
        nonCulledTriangles.push_back(triangle);
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

            // Define Pixel colour first
            Colour pixelColour;

            // Check if depth of field is enabled, if so use the depth of field shading, otherwise use the standard shading
            if (depthOfFieldEnabled)
            {
                // Call depthOfFieldShading with the pixel coordinates (u, v) to get the pixel colour
                pixelColour = depthOfFieldShading(u, v);
            }
            else
            {
                // Set the view origin to the camera position for standard shading
                viewOrigin = cameraPosition;

                // Get the closest valid intersection for the ray and calculate the pixel colour using shadingHelper
                RayTriangleIntersection intersection = getClosestValidIntersection(cameraPosition, rayDirection);

                // Use shadingHelper to determine the pixel colour based on the intersection
                pixelColour = shadingHelper(intersection);
            }

            // Convert the Colour object to a 32-bit integer in ARGB format for rendering
            uint32_t pixelColourValue = (255 << 24) | (pixelColour.red << 16) | (pixelColour.green << 8) | pixelColour.blue;

            window.setPixelColour(x, y, pixelColourValue);
        }
    }

    prevCameraPosition = cameraPosition;
    prevCameraOrientation = cameraOrientation;
    frameRendered = true;
}

Colour RayTracedRenderer::shadingHelper(RayTriangleIntersection intersection)
{
    if (intersection.distanceFromCamera > 0) // Valid intersection found
    {
        // Define colour and copy RayTriangleIntersection for shading calculations
        Colour pixelColour;
        RayTriangleIntersection shaded = intersection;

        // If the intersected triangle has a texture, apply textured shading
        if (shaded.intersectedTriangle.textureIndex >= 0)
        {
            // Apply textured shading
            shaded.intersectedTriangle.colour = texturedShading(intersection);
        }

        if (shaded.intersectedTriangle.isMirror && worldMirroringEnabled)
        {
            // Apply mirror shading
            shaded = mirrorShading(intersection);
        }

        // Determine the shading method based on the enabled flags
        if (GouraudShadingEnabled)
        {
            // Gouraud shading: Calculate light at vertices, then interpolate vertex colors using barycentric coordinates
            pixelColour = baryentricColourWeights(shaded, gourandShading(shaded));
        }
        else if (PhongShadingEnabled)
        {
            // Phong shading: Interpolate normals, then calculate light per-pixel
            pixelColour = phongShading(shaded);
        }
        else
        {
            // Flat shading (Default): Calculate light once for the whole face using the face normal
            pixelColour = calculateLighting(
                shaded.intersectionPoint,
                shaded.intersectedTriangle.normal,
                shaded.intersectedTriangle.colour,
                shaded.intersectedTriangle.normal);
        }

        return pixelColour;
    }
    else
    {
        return Colour(0, 0, 0); // Black with full opacity
    }
}

// Get the closest valid intersection of a ray with the triangles in the scene
RayTriangleIntersection RayTracedRenderer::getClosestValidIntersection(glm::vec3 startPoint, glm::vec3 rayDirection)
{
    // Initialize the best triangle distance to a large value and the best index to -1 (no intersection)
    float bestTriangleDistance = std::numeric_limits<float>::max();
    int bestIndex = -1;

    // Iterate through all non-culled triangles to find the closest intersection
    for (size_t index = 0; index < nonCulledTriangles.size(); ++index)
    {
        // Calculate the distance to the intersection point if the ray intersects with the triangle
        float triangleDistance;
        // If the ray intersects with the triangle and the intersection point is closer than the best distance found so far, update the best distance and index
        if (intersectTriangle(nonCulledTriangles[index], startPoint, rayDirection, triangleDistance) && triangleDistance < bestTriangleDistance)
        {
            bestTriangleDistance = triangleDistance;
            bestIndex = int(index);
        }
    }

    // Create a RayTriangleIntersection result object to store the intersection information
    RayTriangleIntersection result;
    if (bestIndex < 0)
    {
        result.distanceFromCamera = -1.0f;
        return result;
    }

    // If a valid intersection was found, populate the result with the intersection point, distance, intersected triangle, and triangle index
    result.distanceFromCamera = bestTriangleDistance;
    result.intersectionPoint = startPoint + rayDirection * bestTriangleDistance;
    result.intersectedTriangle = nonCulledTriangles[bestIndex];
    result.triangleIndex = bestIndex;
    return result;
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
        shadowed = isOccluded(origin, lightDir, distanceToLight);

    // Default ambient defined above
    float brightness = AMBIENT;
    float specular = 0.0f;

    // shadowed = false; // For testing, comment this out to see shadows

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
            glm::vec3 viewDir = glm::normalize(viewOrigin - point);

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

// Textured shading implementation based on my rasteriser, but with barycentric interpolation of the texture coordinates
// "Barycentric coordinates are great - once you have implemented them, you get perspective corrected texture mapping for free !" - Smart person
Colour RayTracedRenderer::texturedShading(RayTriangleIntersection intersection)
{
    const ModelTriangle &triangle = intersection.intersectedTriangle;

    const TextureMap &texture = textures[triangle.textureIndex];

    // Barycentric weights of the hit point
    glm::vec3 weights = barycentricWeights(intersection);

    // Interpolate the X texture point using barycentric weights
    float textureX = weights.x * triangle.texturePoints[0].x +
                     weights.y * triangle.texturePoints[1].x +
                     weights.z * triangle.texturePoints[2].x;

    // Interpolate the Y texture point using barycentric weights
    float textureY = weights.x * triangle.texturePoints[0].y +
                     weights.y * triangle.texturePoints[1].y +
                     weights.z * triangle.texturePoints[2].y;

    // Wrap the texture coordinates to ensure they are within the texture bounds
    textureX = textureX - std::floor(textureX / texture.width) * texture.width;
    textureY = textureY - std::floor(textureY / texture.height) * texture.height;
    // Round to the nearest pixel
    int texX = static_cast<int>(std::round(textureX));
    int texY = static_cast<int>(std::round(textureY));

    // Clamp to the texture, same as the rasteriser
    texX = std::max(0, std::min(texX, static_cast<int>(texture.width) - 1));
    texY = std::max(0, std::min(texY, static_cast<int>(texture.height) - 1));

    // Get the pixel colour from the texture map
    uint32_t pixel = texture.pixels[texY * static_cast<int>(texture.width) + texX];

    // Pack the pixel colour into a Colour and return it
    return Colour((pixel >> 16) & 0xFF, (pixel >> 8) & 0xFF, pixel & 0xFF);
}

// Mirror shading implementation
RayTriangleIntersection RayTracedRenderer::mirrorShading(RayTriangleIntersection intersection)
{
    // Calculate the incident direction of the ray and the normal at the intersection point
    glm::vec3 incidentDirection = glm::normalize(intersection.intersectionPoint - viewOrigin);
    // Use the normal of the intersected triangle for reflection calculations
    glm::vec3 normal = intersection.intersectedTriangle.normal;

    // Use given formula
    glm::vec3 reflectedDirection = incidentDirection - 2.0f * glm::dot(incidentDirection, normal) * normal;

    // Get the closest valid intersection for the reflected ray, offsetting the starting point slightly to avoid self-intersection
    RayTriangleIntersection reflectedIntersection = getClosestValidIntersection(intersection.intersectionPoint + normal * 0.001f, reflectedDirection);
    return reflectedIntersection;
}

// Random point in a circle for depth of field effect
// Given by Intellisense the moment i started typing
glm::vec2 RayTracedRenderer::randomPointInCircle(float radius)
{
    // Random number generation
    thread_local std::random_device rd;
    thread_local std::mt19937 gen(rd());
    thread_local std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    float r = radius * std::sqrt(dist(gen)); // Square root for uniform distribution in circle
    float theta = 2.0f * PI * dist(gen);     // Random angle

    return glm::vec2(r * std::cos(theta), r * std::sin(theta));
}

// Random point in a circle for swirl effect
glm::vec2 RayTracedRenderer::randomPointInCircleSwirl(float radius, float u, float v, float swirlFactor)
{
    // Use the given random helper
    glm::vec2 sample = randomPointInCircle(radius);

    // Calculate the distance from the center of the screen to the pixel (u, v)
    // Using length function
    glm::vec2 screenPosition(u, v);
    float distanceFromCenter = glm::length(screenPosition);
    // If the distance is very small, return the sample without swirling
    if (distanceFromCenter < 1e-5f)
    {
        return sample;
    }

    // Calculate the radial direction from the center of the screen to the pixel (u, v)
    glm::vec2 radialDirection = screenPosition / distanceFromCenter;

    // Project the sample onto the radial direction to get the radial component
    float radialComponent = glm::dot(sample, radialDirection);
    // Calculate the tangential component by subtracting the radial component from the sample
    glm::vec2 tangentialVector = sample - radialDirection * radialComponent;

    // Squash the radial width toward the edges of the frame
    float radialScale = std::max(0.15f, 1.0f - swirlFactor * distanceFromCenter);

    // Return the final offset by combining the tangential vector and the scaled radial component
    return tangentialVector + radialDirection * (radialComponent * radialScale);
}

// Equation for random point in a reflex lens found through research
glm::vec2 RayTracedRenderer::randomPointInReflexLens(float outerRadius, float innerRatio)
{
    // Generate a random point in a unit circle
    glm::vec2 unitPoint = randomPointInCircle(1.0f);

    // Calculate the squared radius of the unit point
    float radiusSquared = glm::dot(unitPoint, unitPoint);

    // Calculate the inner radius based on the outer radius and the inner ratio, clamping the inner ratio to a maximum of 0.95
    float innerRadius = outerRadius * glm::clamp(innerRatio, 0.0f, 0.95f);
    // If the radius squared is very small, return a point at the inner radius along the x-axis
    if (radiusSquared < 1e-8f)
    {
        return glm::vec2(innerRadius, 0.0f);
    }

    // Calculate the squared inner and outer radii for mapping the unit point to the reflex lens
    float innerRadiusSquared = innerRadius * innerRadius;
    float outerRadiusSquared = outerRadius * outerRadius;

    // Map the unit point to the reflex lens by calculating the mapped radius based on the inner and outer radii, and return the scaled unit point
    float mappedRadius = std::sqrt(innerRadiusSquared + radiusSquared * (outerRadiusSquared - innerRadiusSquared));
    return unitPoint * (mappedRadius / std::sqrt(radiusSquared));
}

Colour RayTracedRenderer::depthOfFieldShading(float u, float v)
{
    // Assign the camera orientation vectors for easier access
    glm::vec3 right = cameraOrientation[0];
    glm::vec3 up = cameraOrientation[1];
    glm::vec3 forward = -cameraOrientation[2];

    // Calculate the ray direction from the camera through the pixel (u, v) and normalize it
    glm::vec3 rayDirection = glm::normalize(cameraOrientation * glm::vec3(u, v, -1.0f));

    // Calculate the focal point based on the focal distance and the ray direction
    glm::vec3 focalPoint = cameraPosition + rayDirection * (focalDistance / glm::dot(rayDirection, forward));

    // Average colour for the pixel
    float redTotal = 0.0f;
    float greenTotal = 0.0f;
    float blueTotal = 0.0f;

    // Loop through the number of rays per pixel to simulate depth of field
    for (int i = 0; i < raysPerPixel; ++i)
    {
        // Generate a random offset in the lens plane, using swirl effect if enabled
        glm::vec2 randomOffset = swirlEnabled
                                     ? randomPointInCircleSwirl(lenseSize, u, v, SWIRL_FACTOR) // Swirl factor that looks nice
                                 : reflexLensEnabled
                                     ? randomPointInReflexLens(lenseSize, REFLEX_RADIUS) // Inner ratio that looks nice
                                     : randomPointInCircle(lenseSize);

        // New camera position based on the random offset in the lens plane
        // This line was given by Intellisense
        glm::vec3 origin = cameraPosition + right * randomOffset.x + up * randomOffset.y;
        glm::vec3 direction = glm::normalize(focalPoint - origin);

        // Set the view origin for shading calculations to the new camera position
        viewOrigin = origin;

        // Get the closest valid intersection for the ray and calculate the pixel colour using shadingHelper
        RayTriangleIntersection hit = getClosestValidIntersection(origin, direction);

        // Calculate the pixel colour based on the intersection using shadingHelper
        Colour pixelColour = shadingHelper(hit);

        // Add the pixel colour to the total for averaging later
        redTotal += pixelColour.red;
        greenTotal += pixelColour.green;
        blueTotal += pixelColour.blue;
    }

    return Colour(
        static_cast<int>(redTotal / raysPerPixel),
        static_cast<int>(greenTotal / raysPerPixel),
        static_cast<int>(blueTotal / raysPerPixel));
}

// Focus the camera on a specific pixel (x, y) by calculating the ray direction and finding the closest intersection
void RayTracedRenderer::focusOnPixel(int x, int y)
{
    // Calculate the ray direction from the camera through the pixel (x, y)
    float u = (x - WIDTH / 2.0f) / (IMAGE_SCALE * focalLength);
    float v = (HEIGHT / 2.0f - y) / (IMAGE_SCALE * focalLength);

    // Normalize the ray direction for consistency
    glm::vec3 rayDirection = glm::normalize(cameraOrientation * glm::vec3(u, v, -1.0f));

    // Get the closest valid intersection for the ray and calculate the focal distance based on the intersection point
    RayTriangleIntersection hit = getClosestValidIntersection(cameraPosition, rayDirection);

    // If a valid intersection is found, calculate the focal distance and print the focused object name and focal distance
    if (hit.distanceFromCamera > 0.0f)
    {
        glm::vec3 forward = glm::normalize(-cameraOrientation[2]);
        focalDistance = glm::dot(hit.intersectionPoint - cameraPosition, forward);
        std::cout << "Focused on " << hit.intersectedTriangle.objectName
                  << " (focalDistance = " << focalDistance << ")" << std::endl;
    }
}