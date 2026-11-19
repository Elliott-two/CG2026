#include "RayTracedRenderer.h"
#include <random>

// Get all the needed external variables and vectors

// Width and Height of the screen
extern const int WIDTH;
extern const int HEIGHT;

// Vector of all the triangles
extern std::vector<ModelTriangle> triangles;

// Initialised empty vector of non-culled triangles
extern std::vector<ModelTriangle> nonCulledTriangles;

// Camera position and orientation
extern glm::vec3 cameraPosition;
extern glm::mat3 cameraOrientation;

// Focal length and distance for depth of field calculations
extern float focalLength;
extern float focalDistance;

// Lense size and rays per pixel for depth of field calculations
extern float lenseSize;
extern int raysPerPixel;

// Position of the light source
extern glm::vec3 lightPosition;

// Vector of vertex normals for each triangle
extern std::vector<std::array<glm::vec3, 3>> vertexNormalsByTriangle;

// Vector of all the texture maps
extern std::vector<TextureMap> textures;

// Map of the enviornment texture
extern TextureMap environmentMap;

// Bool to check if we want to force a redraw of the ray traced image
extern bool forceRayTraceRedraw;

// Bool to check if a new frame has been rendered
extern bool newFrameRendered;

// Bools to check whether we want Gouraud or Phong shading
extern bool GouraudShadingEnabled;
extern bool PhongShadingEnabled;

// Flag to enable/disable world mirroring
extern bool worldMirroringEnabled;

// Flag to enable/disable world refractive materials
extern bool worldRefractiveEnabled;

// Flag to enable/disable world metallic materials
extern bool worldMetallicEnabled;

// Flag to enable/disable environment mapping
extern bool environmentMapEnabled;

// Flag to check if the environment map is seen by the camera
extern bool environmentMapSeenByCamera;

// Flag to enable/disable photon mapping
extern bool photonMappingEnabled;

// Flag to enable/disable photon debugging
extern bool photonDebuggingEnabled;

// Flag to control the time of the water
extern float waterTime;

// Flag to enable/disable depth of field effect
extern bool depthOfFieldEnabled;

// Flag to enable/disable swirl effect
extern bool swirlEnabled;

// Flag to enable/disable reflex lens effect
extern bool reflexLensEnabled;

// Flag to enable/disable soft shadows
extern bool softShadowsEnabled;

// Position of the light source and min/max values for light calculations
extern glm::vec3 lightMin;
extern glm::vec3 lightMax;

// Bools to check if the sphere and boxes are showing, for video purposes
extern bool isSphereShowing;
extern bool isBlueBoxShowing;
extern bool isRedBoxShowing;

// All of my preset needed things
constexpr float RAY_TRACED_PI = 3.14159265358979323846f;
constexpr float IMAGE_SCALE = 500.0f;                  // Scale for the image plane
constexpr float LIGHT_INTENSITY = 20.0f;               // Intensity of the light source
constexpr float AMBIENT = 0.25f;                       // Ambient light intensity
constexpr float AMBIENT_METAL = 0.05f;                 // Ambient light intensity for metallic surfaces
constexpr int SPECULAR_EXPONENT_LOG2 = 5;              // Log2 of the specular exponent for Phong shading
constexpr float REFLEX_RADIUS = 0.5f;                  // Radius for the reflex lens effect
constexpr float SWIRL_FACTOR = 4.0f;                   // Factor for the swirl effect
constexpr int SHADOW_GRID = 2;                         // Grid size for soft shadows
constexpr int MAX_REFRACTION_RECURSION_DEPTH = 6;      // Maximum recursion depth for refraction rays
constexpr int NUMBER_OF_PHOTONS = 250000;              // Number of photons to trace for photon mapping
constexpr int MAX_PHOTON_RECURSION_DEPTH = 8;          // Maximum recursion depth for photon tracing
constexpr float CAUSTIC_RADIUS = 0.04f;                // Radius for caustic photon gathering
constexpr float GLOBAL_RADIUS = 0.16f;                 // Radius for global photon gathering
constexpr float INDIRECT_STRENGTH = 0.40f;             // Strength of indirect lighting from photon mapping
constexpr uint32_t CAUSTIC_PHOTON_COLOUR = 0xFFFF00FF; // Colour for caustic photons in debugging mode (magenta)
constexpr uint32_t GLOBAL_PHOTON_COLOUR = 0xFFFDF4DC;  // Colour for global photons in debugging mode (warm white)
constexpr float SURFACE_OFFSET = 0.001f;               // Offset to avoid self-intersection when tracing rays from surfaces

// Thread-local storage for the view origin, which is used in depth of field calculations
static thread_local glm::vec3 viewOrigin(0.0f);
static std::vector<int> visibleToOriginalIndex;

// If I need a random float
static float randomFloat()
{
    // Use thread-local random number generator to avoid contention in multi-threaded environments
    thread_local std::mt19937 gen(std::random_device{}());
    thread_local std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    return dist(gen);
}

// Bool to check if a ray intersects with a triangle using the Möller–Trumbore intersection algorithm
// Moller Trumbore algorithm came as a result of googling how to speed up ray tracing
static bool intersectTriangle(const ModelTriangle &triangle, const glm::vec3 &origin, const glm::vec3 &direction, float &triangleDistance)
{
    // Calculate the edges of the triangle
    const glm::vec3 edge0 = triangle.vertices[1] - triangle.vertices[0];
    const glm::vec3 edge1 = triangle.vertices[2] - triangle.vertices[0];

    // Calculate the determinant to check if the ray is parallel to the triangle
    const glm::vec3 point = glm::cross(direction, edge1);
    const float determinant = glm::dot(edge0, point);
    if (std::abs(determinant) < 1e-8f)
        return false; // ray parallel to triangle

    // Calculate the inverse determinant and the barycentric coordinates (u, v) to check if the intersection point is inside the triangle
    const float inverseDet = 1.0f / determinant;
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

// Struct to hold information about a triangle hit by a shadow ray
struct hitShadow
{
    float distance;
    const ModelTriangle *triangle;
};

static glm::vec3 computeBeerLambertTransmission(const glm::vec3 &absorption, float distance)
{

    // Calculate the transmission for each colour channel using the Beer-Lambert law
    return glm::vec3(
        std::exp(-absorption.x * distance),
        std::exp(-absorption.y * distance),
        std::exp(-absorption.z * distance));
}

static glm::vec3 howOccluded(const glm::vec3 &origin, const glm::vec3 &direction, float maxDistance)
{
    static thread_local std::vector<hitShadow> hits;
    hits.clear();
    // Loop over all non-culled triangles and check for intersections with the ray
    for (const ModelTriangle &triangle : nonCulledTriangles)
    {

        // Calculate the distance to the intersection point if the ray intersects with the triangle
        float triangleDistance;
        // If the ray intersects with the triangle and the intersection point is closer than the maximum distance, return true (occluded)
        if (!intersectTriangle(triangle, origin, direction, triangleDistance) || triangleDistance >= maxDistance)
        {
            continue;
        }
        if (!triangle.isRefractive || !worldRefractiveEnabled)
        {
            return glm::vec3(0.0f); // Occluded
        }
        hits.push_back({triangleDistance, &triangle});
    }
    if (hits.empty())
        return glm::vec3(1.0f); // Not occluded

    // Sort the hits by distance and remove duplicates based on object name and distance
    std::sort(hits.begin(), hits.end(), [](const hitShadow &shadowA, const hitShadow &shadowB)
              { return shadowA.distance < shadowB.distance; });

    // Remove duplicates based on object name and distance
    size_t uniqueCount = 0;
    for (size_t index = 0; index < hits.size(); index++)
    {
        // Skip duplicate triangles
        if (uniqueCount > 0 &&
            hits[uniqueCount - 1].triangle->objectName == hits[index].triangle->objectName &&
            std::abs(hits[index].distance - hits[uniqueCount - 1].distance) < 1e-4f)
        {
            continue;
        }
        hits[uniqueCount++] = hits[index];
    }
    // Resize the hits vector to contain only unique hits
    hits.resize(uniqueCount);

    // Calculate the transmission through the refractive triangles using the Beer-Lambert law
    glm::vec3 transmission(1.0f);

    // Loop through the hits and calculate the transmission through each refractive triangle
    for (size_t index = 0; index < hits.size();)
    {
        // Check if the ray is entering or exiting the refractive triangle based on the dot product of the ray direction and the triangle normal
        const bool entering = glm::dot(direction, hits[index].triangle->normal) < 0.0f;

        // If the ray is exiting, calculate the transmission through the triangle using the Beer-Lambert law and continue to the next hit
        if (!entering)
        {
            transmission *= computeBeerLambertTransmission(hits[index].triangle->absorption, hits[index].distance);
            index += 1;
            continue;
        }

        // Entry and exit distances for the refractive triangle, and the index of the next hit to process
        float entryDistance = hits[index].distance;
        float exitDistance = maxDistance;
        size_t nextIndex = index + 1;

        for (size_t j = index + 1; j < hits.size(); j++)
        {
            if (hits[j].triangle->objectName == hits[index].triangle->objectName)
            {
                exitDistance = hits[j].distance;
                nextIndex = j + 1;
                break;
            }
        }

        // Calculate the thickness of the refractive triangle and update the transmission using the Beer-Lambert law
        float thickness = std::max(0.0f, exitDistance - entryDistance);
        transmission *= computeBeerLambertTransmission(hits[index].triangle->absorption, thickness);
        index = nextIndex;
    }

    return transmission;
}
// Calculate the specular power for a given value using repeated squaring
static float specularPower(float specularPowerValue) //
{
    for (int index = 0; index < SPECULAR_EXPONENT_LOG2; index++)
        specularPowerValue *= specularPowerValue;
    return specularPowerValue;
}

// Use MATHS?!?
static float lightFalloff(float distanceSquared)
{
    return LIGHT_INTENSITY / (4.0f * RAY_TRACED_PI * distanceSquared);
}

// Calculating the light sample for a given point
static std::vector<glm::vec3> sampleLight(const glm::vec3 &origin, const glm::vec3 &shadingNormal, const glm::vec3 &viewDir, const glm::vec3 &target)
{
    // Create empty vector
    glm::vec3 diffuse(0.0f);
    glm::vec3 specular(0.0f);

    // Calculate the direction from the shading point to the light source
    glm::vec3 toLight = target - origin;

    // Calculate the squared distance to the light source and check if it's too close to avoid division by zero
    const float distanceSquared = glm::dot(toLight, toLight);
    if (distanceSquared < 1e-12f)
        return std::vector<glm::vec3>{diffuse, specular};

    float distance = std::sqrt(distanceSquared);

    // Check if the distance is zero to avoid division by zero
    if (distance == 0.0f)
        return std::vector<glm::vec3>{diffuse, specular}; // Avoid division by zero

    glm::vec3 lightDir = toLight / distance;

    // Check if in full shadow
    glm::vec3 transmission = howOccluded(origin, lightDir, distance - 0.003f);
    if (transmission == glm::vec3(0.0f))
        return std::vector<glm::vec3>{diffuse, specular};

    // Calculate the incident angle between the shading normal and the light direction
    float incidentAngle = glm::dot(shadingNormal, lightDir);

    // If the incident angle is less than 0, return the result with diffuse and specular set to 0 (no light contribution)
    if (incidentAngle <= 0.0f)
        return std::vector<glm::vec3>{diffuse, specular};

    // Calculate using MATHS?!?!
    // Calculate the diffuse and specular contributions based on the incident angle and proximity to the light source
    float diffuseContribution = lightFalloff(distanceSquared) * incidentAngle;

    // Given formula
    glm::vec3 reflected = (2.0f * incidentAngle * shadingNormal) - lightDir;

    // Calculate the specular contribution using the Phong reflection model and the specular power function
    float specularContribution = 0.5f * specularPower(std::max(glm::dot(reflected, viewDir), 0.0f));

    // Calculate the final diffuse and specular contributions by multiplying with the transmission factor
    diffuse = transmission * diffuseContribution;
    specular = transmission * specularContribution;

    return std::vector<glm::vec3>{diffuse, specular};
}

void RayTracedRenderer::draw(DrawingWindow &window)
{

    // For easier loading speeds
    static glm::vec3 prevCameraPosition = glm::vec3(std::numeric_limits<float>::max());
    static glm::mat3 prevCameraOrientation = glm::mat3(0.0f);
    static bool frameRendered = false;

    // Read the current mode each frame so changes made by MainApplication are observed.
    int shadingMode = 0; // 1 for Gouraud, 2 for Phong, 0 for flat
    if (GouraudShadingEnabled)
    {
        shadingMode = 1;
    }
    else if (PhongShadingEnabled)
    {
        shadingMode = 2;
    }

    // Bools and ints to check if anything has changed
    static int prevShadingMode = shadingMode;                                // Previous shading mode
    static bool prevSphereShowing = isSphereShowing;                         // Previous sphere showing state
    static bool prevBlueBoxShowing = isBlueBoxShowing;                       // Previous blue box showing state
    static bool prevRedBoxShowing = isRedBoxShowing;                         // Previous red box showing state
    static bool prevWorldMirroringEnabled = worldMirroringEnabled;           // Previous world mirroring state
    static bool prevDepthOfFieldEnabled = depthOfFieldEnabled;               // Previous depth of field state
    static float prevFocalDistance = focalDistance;                          // Previous focal distance
    static bool prevSwirlEnabled = swirlEnabled;                             // Previous swirl effect state
    static bool prevReflexLensEnabled = reflexLensEnabled;                   // Previous reflex lens effect state
    static bool prevSoftShadowsEnabled = softShadowsEnabled;                 // Previous soft shadows state
    static bool prevWorldRefractiveEnabled = worldRefractiveEnabled;         // Previous world refractive state
    static bool prevWorldMetallicEnabled = worldMetallicEnabled;             // Previous world metallic state
    static bool prevEnvironmentMapEnabled = environmentMapEnabled;           // Previous environment map state
    static bool prevEnvironmentMapSeenByCamera = environmentMapSeenByCamera; // Previous environment map seen by camera state
    static bool prevPhotonMappingEnabled = photonMappingEnabled;             // Previous photon mapping state
    static bool prevPhotonDebuggingEnabled = photonDebuggingEnabled;         // Previous photon debugging state

    static bool photonMapChanged = true; // Flag to indicate if the photon map has changed and needs to be rebuilt

    // Check if any relevant state has changed since the last frame
    bool changed = (prevShadingMode != shadingMode) ||
                   (prevSphereShowing != isSphereShowing) ||
                   (prevBlueBoxShowing != isBlueBoxShowing) ||
                   (prevRedBoxShowing != isRedBoxShowing) ||
                   (prevWorldMirroringEnabled != worldMirroringEnabled) ||
                   (prevDepthOfFieldEnabled != depthOfFieldEnabled) ||
                   (prevFocalDistance != focalDistance) ||
                   (prevSwirlEnabled != swirlEnabled) ||
                   (prevReflexLensEnabled != reflexLensEnabled) ||
                   (prevSoftShadowsEnabled != softShadowsEnabled) ||
                   (prevWorldRefractiveEnabled != worldRefractiveEnabled) ||
                   (prevWorldMetallicEnabled != worldMetallicEnabled) ||
                   (prevEnvironmentMapEnabled != environmentMapEnabled) ||
                   (prevEnvironmentMapSeenByCamera != environmentMapSeenByCamera) ||
                   (prevPhotonMappingEnabled != photonMappingEnabled) ||
                   (prevPhotonDebuggingEnabled != photonDebuggingEnabled);

    // Bools relating to lighting and photon mapping changes
    bool lightPathsChanged = (prevSphereShowing != isSphereShowing) ||
                             (prevBlueBoxShowing != isBlueBoxShowing) ||
                             (prevRedBoxShowing != isRedBoxShowing) ||
                             (prevWorldMirroringEnabled != worldMirroringEnabled) ||
                             (prevWorldRefractiveEnabled != worldRefractiveEnabled) ||
                             (prevWorldMetallicEnabled != worldMetallicEnabled);
    // Check for change
    if (forceRayTraceRedraw || changed || lightPathsChanged)
    {
        frameRendered = false;       // Force re-render if anything visible changed
        forceRayTraceRedraw = false; // Reset the force redraw flag
        prevShadingMode = shadingMode;
        prevSphereShowing = isSphereShowing;
        prevBlueBoxShowing = isBlueBoxShowing;
        prevRedBoxShowing = isRedBoxShowing;
        prevWorldMirroringEnabled = worldMirroringEnabled;
        prevDepthOfFieldEnabled = depthOfFieldEnabled;
        prevFocalDistance = focalDistance;
        prevSwirlEnabled = swirlEnabled;
        prevReflexLensEnabled = reflexLensEnabled;
        prevSoftShadowsEnabled = softShadowsEnabled;
        prevWorldRefractiveEnabled = worldRefractiveEnabled;
        prevWorldMetallicEnabled = worldMetallicEnabled;
        prevEnvironmentMapEnabled = environmentMapEnabled;
        prevEnvironmentMapSeenByCamera = environmentMapSeenByCamera;
        if (lightPathsChanged)
        {
            photonMapChanged = true;
        }

        prevPhotonMappingEnabled = photonMappingEnabled;
        prevPhotonDebuggingEnabled = photonDebuggingEnabled;
    }

    // Check if the camera has moved or rotated since the last frame
    bool cameraMoved = (cameraPosition != prevCameraPosition) || (cameraOrientation != prevCameraOrientation);

    // If the camera hasn't moved and we already rendered the frame, skip recalculating
    if (!cameraMoved && frameRendered)
    {
        return;
    }

    // Clear the non-culled triangles and visible-to-original index vectors
    nonCulledTriangles.clear();
    visibleToOriginalIndex.clear();
    // Populate the non-culled triangles and visible-to-original index vectors based on the current visibility settings
    for (size_t index = 0; index < triangles.size(); index++)
    {
        const ModelTriangle &triangle = triangles[index];
        if ((triangle.objectName == "sphere" && !isSphereShowing) ||
            (triangle.objectName == "tall_box" && !isBlueBoxShowing) ||
            (triangle.objectName == "short_box" && !isRedBoxShowing))
            continue;
        nonCulledTriangles.push_back(triangle);
        visibleToOriginalIndex.push_back(static_cast<int>(index));
    }

    // If photon mapping is enabled and the photon map has changed, rebuild the photon maps
    if (photonMappingEnabled && photonMapChanged)
    {
        buildPhotonMaps();
        photonMapChanged = false;
    }

    for (int x = 0; x < WIDTH; x++)
    {
        for (int y = 0; y < HEIGHT; y++)
        {
            // Calculate the direction of the ray from the camera through the pixel
            float u = (x + 0.5f - WIDTH / 2.0f) / (IMAGE_SCALE * focalLength);
            float v = (HEIGHT / 2.0f - (y + 0.5f)) / (IMAGE_SCALE * focalLength);

            // Normailise for consistency
            glm::vec3 rayDirection = glm::normalize(cameraOrientation * glm::vec3(u, v, -1.0f));

            // Define Pixel colour first
            Colour pixelColour;

            // Check if depth of field is enabled, if so use the depth of field shading, otherwise use the standard shading
            if (depthOfFieldEnabled)
            {
                // Call depthOfFieldShading with the pixel coordinates (u, v) to get the pixel colour
                pixelColour = depthOfFieldShading(u, v, rayDirection);
            }
            else
            {
                pixelColour = traceRay(cameraPosition, rayDirection, 0);
            }

            // Convert the Colour object to a 32-bit integer in ARGB format for rendering
            uint32_t pixelColourValue = (255 << 24) | (pixelColour.red << 16) | (pixelColour.green << 8) | pixelColour.blue;

            window.setPixelColour(x, y, pixelColourValue);
        }
    }

    // If photon debugging is enabled and photon mapping is enabled, render the photon maps for debugging purposes
    if (photonDebuggingEnabled && photonMappingEnabled)
    {
        // Clear the window pixels before rendering the photon maps
        window.clearPixels();

        // Define an array of pointers to the photon maps and their corresponding colours for rendering
        const PhotonMap *photonMaps[] = {&causticPhotonMap, &globalPhotonMap};
        const uint32_t photonColours[] = {CAUSTIC_PHOTON_COLOUR, GLOBAL_PHOTON_COLOUR};

        // Loop through each photon map and render the photons onto the window
        for (int mapIndex = 0; mapIndex < 2; mapIndex++)
        {
            for (const Photon &photon : photonMaps[mapIndex]->allPhotons())
            {
                glm::vec3 screenPosition = glm::transpose(cameraOrientation) * (photon.position - cameraPosition);
                if (screenPosition.z >= 0.0f)
                {
                    continue; // Photon is behind the camera
                }

                float u = screenPosition.x / -screenPosition.z;
                float v = screenPosition.y / -screenPosition.z;

                int x = static_cast<int>(u * IMAGE_SCALE * focalLength + WIDTH / 2.0f);
                int y = static_cast<int>(HEIGHT / 2.0f - v * IMAGE_SCALE * focalLength);
                if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT)
                {
                    continue;
                }
                window.setPixelColour(x, y, photonColours[mapIndex]);
            }
        }
    }
    prevCameraPosition = cameraPosition;
    prevCameraOrientation = cameraOrientation;
    frameRendered = true;
    newFrameRendered = true; // Set the flag to indicate that a new frame has been rendered
}

Colour RayTracedRenderer::shadingHelper(RayTriangleIntersection intersection)
{
    if (intersection.distanceFromCamera <= 0)
        return Colour(0, 0, 0);

    // Define colour and copy RayTriangleIntersection for shading calculations
    Colour pixelColour;
    RayTriangleIntersection shaded = intersection;

    if (shaded.intersectedTriangle.objectName == "light")
    {
        return shaded.intersectedTriangle.colour;
    }

    if (shaded.intersectedTriangle.textureIndex >= 0)
        shaded.intersectedTriangle.colour = texturedShading(shaded);

    // Determine the shading method based on the enabled flags
    if (GouraudShadingEnabled)
    {
        // Gouraud shading: Calculate light at vertices, then interpolate vertex colours using barycentric coordinates
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

// Get the closest valid intersection of a ray with the triangles in the scene
RayTriangleIntersection RayTracedRenderer::getClosestValidIntersection(glm::vec3 startPoint, glm::vec3 rayDirection)
{
    // Initialize the best triangle distance to a large value and the best index to -1 (no intersection)
    float bestTriangleDistance = std::numeric_limits<float>::max();
    int bestIndex = -1;

    // Iterate through all non-culled triangles to find the closest intersection
    for (size_t index = 0; index < nonCulledTriangles.size(); index++)
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
    result.triangleIndex = visibleToOriginalIndex[bestIndex];
    return result;
}

static glm::vec3 jitteredLightPoint(int i, int j)
{

    const float alongX = (i + randomFloat()) / SHADOW_GRID;
    const float alongZ = (j + randomFloat()) / SHADOW_GRID;
    return glm::vec3(glm::mix(lightMin.x, lightMax.x, alongX), lightMin.y, glm::mix(lightMin.z, lightMax.z, alongZ));
}

static std::vector<glm::vec3> sampleAreaLight(const glm::vec3 &origin, const glm::vec3 &normal, const glm::vec3 &viewDir)
{
    std::vector<glm::vec3> lightSamples(2, glm::vec3(0.0f));
    std::vector<glm::vec3> total(2, glm::vec3(0.0f));
    for (int index = 0; index < SHADOW_GRID; index++)
    {
        for (int j = 0; j < SHADOW_GRID; j++)
        {
            lightSamples = sampleLight(origin, normal, viewDir, jitteredLightPoint(index, j));
            total[0] += lightSamples[0];
            total[1] += lightSamples[1];
        }
    }

    float totalSamples = float(SHADOW_GRID * SHADOW_GRID);
    float inverseTotalSamples = 1.0f / totalSamples;
    total[0] *= inverseTotalSamples;
    total[1] *= inverseTotalSamples;
    return total;
}

Colour RayTracedRenderer::calculateLighting(glm::vec3 point, glm::vec3 normal, Colour baseColour, glm::vec3 faceNormal)
{
    glm::vec3 normalForShading = glm::normalize(normal);

    // Offset to avoid shadow acne
    glm::vec3 origin = point + faceNormal * 0.005f;

    glm::vec3 viewDir = glm::normalize(viewOrigin - point);

    const bool useAreaLight = softShadowsEnabled && lightMin != lightMax;
    std::vector<glm::vec3> total(2, glm::vec3(0.0f));

    if (useAreaLight)
    {
        total = sampleAreaLight(origin, normalForShading, viewDir);
    }
    else
    {
        total = sampleLight(origin, normalForShading, viewDir, lightPosition);
    }

    if (photonMappingEnabled)
    {
        glm::vec3 surfaceNormal = glm::normalize(faceNormal);
        if (glm::dot(surfaceNormal, viewDir) < 0.0f)
        {
            surfaceNormal = -surfaceNormal; // Flip normal if facing away from the view direction
        }
        total[0] += causticPhotonMap.estimateIrradiance(point, surfaceNormal, CAUSTIC_RADIUS);
    }

    glm::vec3 brightness;
    if (photonMappingEnabled)
    {
        glm::vec3 surfaceNormal = glm::normalize(faceNormal);
        if (glm::dot(surfaceNormal, viewDir) < 0.0f)
        {
            surfaceNormal = -surfaceNormal; // Flip normal if facing away from the view direction
        }
        glm::vec3 usedAmbient = INDIRECT_STRENGTH * globalPhotonMap.estimateIrradiance(point, surfaceNormal, GLOBAL_RADIUS);

        brightness = glm::clamp(total[0] + usedAmbient, 0.0f, 1.0f);
    }
    else
    {
        brightness = glm::clamp(AMBIENT + total[0], 0.0f, 1.0f);
    }

    return Colour(
        std::min(255, int(std::round(baseColour.red * brightness.x + 255.0f * total[1].x))),
        std::min(255, int(std::round(baseColour.green * brightness.y + 255.0f * total[1].y))),
        std::min(255, int(std::round(baseColour.blue * brightness.z + 255.0f * total[1].z))));
}

std::array<Colour, 3> RayTracedRenderer::gourandShading(RayTriangleIntersection intersection)
{
    // Define the triangle and get the vertex normals
    const ModelTriangle &triangle = intersection.intersectedTriangle;

    // Bounds-checked lookup by index, falling back to the flat normal
    std::array<glm::vec3, 3> normals = {triangle.normal, triangle.normal, triangle.normal};
    if (intersection.triangleIndex < vertexNormalsByTriangle.size())
        normals = vertexNormalsByTriangle[intersection.triangleIndex];

    // Light each vertex using its smooth normal
    std::array<Colour, 3> colours;
    for (int index = 0; index < 3; index++)
        colours[index] = calculateLighting(triangle.vertices[index], normals[index], triangle.colour, triangle.normal);
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
    glm::vec3 normal = triangleNormal;
    if (glm::length(interpolated) > 1e-6f)
    {
        normal = glm::normalize(interpolated);
    }

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

// Random point in a circle for depth of field effect
// Given by Intellisense the moment i started typing
glm::vec2 RayTracedRenderer::randomPointInCircle(float radius)
{
    float random = radius * std::sqrt(randomFloat());   // Square root for uniform distribution in circle
    float theta = 2.0f * RAY_TRACED_PI * randomFloat(); // Random angle

    return glm::vec2(random * std::cos(theta), random * std::sin(theta));
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

Colour RayTracedRenderer::depthOfFieldShading(float u, float v, const glm::vec3 &rayDirection)
{
    // Assign the camera orientation vectors for easier access
    glm::vec3 right = cameraOrientation[0];
    glm::vec3 up = cameraOrientation[1];
    glm::vec3 forward = -cameraOrientation[2];

    // Calculate the focal point based on the focal distance and the ray direction
    glm::vec3 focalPoint = cameraPosition + rayDirection * (focalDistance / glm::dot(rayDirection, forward));

    // Average colour for the pixel
    float redTotal = 0.0f;
    float greenTotal = 0.0f;
    float blueTotal = 0.0f;

    // Loop through the number of rays per pixel to simulate depth of field
    for (int index = 0; index < raysPerPixel; index++)
    {
        // Generate a random offset in the lens plane, using swirl effect if enabled
        glm::vec2 randomOffset;
        if (swirlEnabled)
        {
            randomOffset = randomPointInCircleSwirl(lenseSize, u, v, SWIRL_FACTOR);
        }
        else if (reflexLensEnabled)
        {
            randomOffset = randomPointInReflexLens(lenseSize, REFLEX_RADIUS);
        }
        else
        {
            randomOffset = randomPointInCircle(lenseSize);
        }

        // New camera position based on the random offset in the lens plane
        // This line was given by Intellisense
        glm::vec3 origin = cameraPosition + right * randomOffset.x + up * randomOffset.y;
        glm::vec3 direction = glm::normalize(focalPoint - origin);

        Colour pixelColour = traceRay(origin, direction, 0);

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
    float u = (x + 0.5f - WIDTH / 2.0f) / (IMAGE_SCALE * focalLength);
    float v = (HEIGHT / 2.0f - (y + 0.5f)) / (IMAGE_SCALE * focalLength);

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
        std::cout << "Real Values" << " (x = " << x << ", y = " << y << ")" << std::endl;
    }
}

// Check if a ray can refract through a surface based on the incident direction, surface normal, and index of refraction
static bool refractRay(const glm::vec3 &incident, const glm::vec3 &normal, float indexofRefraction, glm::vec3 &outRefractedRay)
{
    // Calculate the cosine of the incident angle using the dot product
    float cosIncident = glm::dot(-incident, normal);

    // Calculate the total internal reflection using Snell's law
    float totalInternalReflection = 1.0f - indexofRefraction * indexofRefraction * (1.0f - cosIncident * cosIncident);
    // If total internal reflection occurs, return false
    if (totalInternalReflection < 0.0f)
    {
        return false; // Total internal reflection occurs
    }

    // Calculate the refracted ray direction using Snell's law and normalize it
    // We pass by reference to outRefractedRay to store the result
    outRefractedRay = glm::normalize(indexofRefraction * incident + (indexofRefraction * cosIncident - std::sqrt(totalInternalReflection)) * normal);
    return true;
}

// Calculate the Fresnel-Schlick approximation for reflectance based on the incident angle and indices of refraction
static float fresnelSchlick(float cosTheta, float n1, float n2)
{
    // Calculate the reflectance at normal incidence (R0) using the indices of refraction
    float r0 = (n1 - n2) / (n1 + n2);
    // Square R0 to get the reflectance at normal incidence
    r0 *= r0;
    // Use the Fresnel-Schlick approximation to calculate the reflectance based on the incident angle (cosTheta)
    return r0 + (1.0f - r0) * std::pow(1.0f - cosTheta, 5.0f);
}

static glm::vec3 fresnelSchlickMetallic(float cosTheta, const glm::vec3 &f0)
{
    return f0 + (glm::vec3(1.0f) - f0) * std::pow(1.0f - cosTheta, 5.0f);
}

// Attenuate the colour based on the absorption coefficients and distance traveled through the medium
static Colour attenuateColour(const Colour &colour, const glm::vec3 &absorption, float distance)
{
    // Calculate the transmission for each colour channel using the Beer-Lambert law
    glm::vec3 transmission = computeBeerLambertTransmission(absorption, distance);

    // Return the attenuated colour by multiplying the original colour by the transmission and clamping the values to a maximum of 255
    return Colour(
        std::min(static_cast<int>(colour.red * transmission.r), 255),
        std::min(static_cast<int>(colour.green * transmission.g), 255),
        std::min(static_cast<int>(colour.blue * transmission.b), 255));
}

struct Wave
{
    glm::vec3 direction;
    float frequency;
    float amplitude;
    float speed;
    float steepness;
};

static Wave makeWave(const glm::vec3 &direction, float frequency, float amplitude, float speed, float steepness)
{
    return {glm::normalize(direction), frequency, amplitude, speed, steepness};
}

static const std::array<Wave, 4> WATER_WAVES = {
    makeWave(glm::vec3(0.82f, 0.15f, 0.55f), 4.2f, 0.035f, 1.10f, 0.9f),
    makeWave(glm::vec3(-0.35f, 0.22f, 0.91f), 9.7f, 0.018f, 1.75f, 0.75f),
    makeWave(glm::vec3(0.51f, -0.40f, -0.76f), 21.3f, 0.008f, 2.85f, 0.50f),
    makeWave(glm::vec3(-0.88f, -0.18f, -0.44f), 2.0f, 0.003f, 4.40f, 0.35f),
};

static glm::vec3 waterNormal(const glm::vec3 &point, const glm::vec3 &flatNormal)
{
    glm::vec3 totalSlope(0.0f);
    glm::vec3 domainWarp(0.0f);

    for (const Wave &wave : WATER_WAVES)
    {
        // Project wave direction onto the local tangent plane so waves travel across the surface
        glm::vec3 tangentDir = wave.direction - glm::dot(wave.direction, flatNormal) * flatNormal;
        const float dirLen = glm::length(tangentDir);
        if (dirLen < 1e-5f)
            continue;
        tangentDir /= dirLen;

        // Distort the sample position using accumulated slopes from larger waves
        const glm::vec3 warpedPoint = point + domainWarp;
        const float phase = wave.frequency * glm::dot(tangentDir, warpedPoint) + wave.speed * waterTime;

        const float s = std::sin(phase);
        const float c = std::cos(phase);

        // Exponentially shape the derivative (exp(sin) derivative = cos * exp(sin))
        // This creates narrow, peaked crests and wide, smooth troughs
        const float peakFactor = std::exp(wave.steepness * s - wave.steepness);
        const float dHeight = wave.amplitude * wave.frequency * c * peakFactor;

        const glm::vec3 waveGrad = tangentDir * dHeight;
        totalSlope += waveGrad;

        // Feed a fraction of this wave's gradient into the next octave's phase
        domainWarp -= waveGrad * (0.35f / wave.frequency);
    }

    return glm::normalize(flatNormal - totalSlope);
}

Colour RayTracedRenderer::mirrorredRay(const RayTriangleIntersection &intersection, const glm::vec3 &direction, int depth, const glm::vec3 &mediumAbsorption, bool insideMedium, float distanceThroughMedium)
{
    // Calculate the reflected direction using the given reflection formula
    const ModelTriangle &triangle = intersection.intersectedTriangle;
    glm::vec3 reflectedDirection = glm::normalize(direction - 2.0f * glm::dot(direction, triangle.normal) * triangle.normal);
    // Trace the reflected ray recursively, offsetting the starting point slightly to avoid self-intersection
    Colour reflectedColour = traceRay(
        intersection.intersectionPoint + reflectedDirection * SURFACE_OFFSET,
        reflectedDirection,
        depth + 1,
        mediumAbsorption,
        insideMedium);
    return attenuateColour(reflectedColour, mediumAbsorption, distanceThroughMedium);
}

static glm::vec2 squashGrid(float u, float v)
{
    float x = 2.0f * u - 1.0f;
    float y = 2.0f * v - 1.0f;
    return glm::vec2(
        x * std::sqrt(std::max(0.0f, 1.0f - 0.5f * y * y)),
        y * std::sqrt(std::max(0.0f, 1.0f - 0.5f * x * x)));
}

Colour RayTracedRenderer::mettalicRay(const RayTriangleIntersection &intersection, const glm::vec3 &direction, int depth, const glm::vec3 &mediumAbsorption, bool insideMedium, float distanceThroughMedium)
{
    // Calculate the reflected direction using the given reflection formula
    const ModelTriangle &triangle = intersection.intersectedTriangle;

    glm::vec3 faceNormal = triangle.normal;
    if (glm::dot(direction, faceNormal) > 0.0f)
    {
        faceNormal = -faceNormal;
    }

    if (intersection.triangleIndex >= vertexNormalsByTriangle.size())
    {
        // Fall back to flat shading if something is out of sync
        return mirrorredRay(intersection, direction, depth, mediumAbsorption, insideMedium, distanceThroughMedium);
    }
    // Get the vertex normals for the triangle and calculate the barycentric weights
    std::array<glm::vec3, 3> normals = vertexNormalsByTriangle[intersection.triangleIndex];

    // Calculate the barycentric weights for the intersection point
    glm::vec3 weights = barycentricWeights(intersection);

    // Interpolate the vertex normals using the barycentric weights
    glm::vec3 interpolatedNormal = weights.x * normals[0] + weights.y * normals[1] + weights.z * normals[2];

    // Normalize the interpolated normal if its length is greater than a small threshold to avoid division by zero
    glm::vec3 normal = faceNormal;
    if (glm::length(interpolatedNormal) > 1e-6f)
    {
        normal = glm::normalize(interpolatedNormal);

        if (glm::dot(direction, normal) > 0.0f)
        {
            normal = -normal;
        }
    }

    // Calculate the reflected direction using the given formula
    glm::vec3 reflectedDirection = glm::normalize(direction - 2.0f * glm::dot(direction, normal) * normal);

    // Accumulate the colour from multiple samples to simulate roughness
    glm::vec3 accumulatedColour(0.0f);

    // Count the number of valid samples that contribute to the final colour
    int validSamples = 0;

    int seedX = static_cast<int>(std::round(intersection.intersectionPoint.x * 1000.0f));
    int seedY = static_cast<int>(std::round(intersection.intersectionPoint.y * 1000.0f));
    int seedZ = static_cast<int>(std::round(intersection.intersectionPoint.z * 1000.0f));

    std::seed_seq seed{seedX, seedY, seedZ};
    std::mt19937 generator(seed);
    std::uniform_real_distribution<float> localDistribution(0.0f, 1.0f);

    glm::vec3 helperUp = glm::vec3(0.0f, 1.0f, 0.0f);
    if (std::abs(reflectedDirection.y) > 0.999f)
    {
        helperUp = glm::vec3(0.0f, 0.0f, 1.0f);
    }

    glm::vec3 tangent = glm::normalize(glm::cross(helperUp, reflectedDirection));
    glm::vec3 bitangent = glm::cross(reflectedDirection, tangent);

    const bool isRoughPrimary = (depth == 0 && triangle.roughness > 0.01f);
    int gridSize = 1;
    if (isRoughPrimary)
    {
        gridSize = 4;
    }

    float stretchFactorX = 1.0f;
    float stretchFactorY = 1.0f;
    if (triangle.stretchFactor > 1.0f)
    {
        stretchFactorX = 3.0f;
        stretchFactorY = 0.5f;
    }

    for (int index = 0; index < gridSize; index++)
    {
        for (int j = 0; j < gridSize; j++)
        {
            glm::vec3 fuzzyDirection = reflectedDirection;

            if (isRoughPrimary)
            {
                float u = (index + localDistribution(generator)) / static_cast<float>(gridSize);
                float v = (j + localDistribution(generator)) / static_cast<float>(gridSize);

                glm::vec2 disk = squashGrid(u, v);
                float dx = disk.x * triangle.roughness * stretchFactorX;
                float dy = disk.y * triangle.roughness * stretchFactorY;

                fuzzyDirection = glm::normalize(reflectedDirection + tangent * dx + bitangent * dy);
            }

            if (glm::dot(fuzzyDirection, faceNormal) <= 0.0f)
            {
                continue;
            }

            Colour sampleColour = traceRay(
                intersection.intersectionPoint + faceNormal * SURFACE_OFFSET,
                fuzzyDirection,
                depth + 1,
                mediumAbsorption,
                insideMedium);

            accumulatedColour += glm::vec3(sampleColour.red, sampleColour.green, sampleColour.blue);
            validSamples++;
        }
    }

    // Average the accumulated colour by the number of valid samples to get the final colour
    if (validSamples > 0)
        accumulatedColour /= static_cast<float>(validSamples);

    if (validSamples == 0)
    {
        Colour sampleColour = traceRay(
            intersection.intersectionPoint + faceNormal * SURFACE_OFFSET,
            reflectedDirection,
            depth + 1,
            mediumAbsorption,
            insideMedium);
        accumulatedColour = glm::vec3(sampleColour.red, sampleColour.green, sampleColour.blue);
    }
    // Apply Conductor Fresnel tint
    // Calculate the cosine of the angle between the incident direction and the surface normal, clamped to [0, 1]
    float cosTheta = glm::clamp(glm::dot(-direction, normal), 0.0f, 1.0f);
    // Calculate the Fresnel tint using the Fresnel-Schlick approximation for metallic surfaces
    glm::vec3 fresnelTint = fresnelSchlickMetallic(cosTheta, triangle.fresnelReflectance);

    std::vector<glm::vec3> sampledLightResult;

    if (softShadowsEnabled && lightMin != lightMax)
    {
        sampledLightResult = sampleAreaLight(
            intersection.intersectionPoint + faceNormal * 0.005f,
            normal,
            -direction);
    }
    else
    {
        sampledLightResult = sampleLight(
            intersection.intersectionPoint + faceNormal * 0.005f,
            normal,
            -direction,
            lightPosition);
    }

    glm::vec3 directSpecular = sampledLightResult[1];

    float glossyFactor = std::max(1.0f - triangle.roughness, 0.05f);
    float glossyScale = glossyFactor * glossyFactor;

    glm::vec3 finalLinearColour = (accumulatedColour + directSpecular * 255.0f * glossyScale) * fresnelTint;

    Colour tintedColour(
        std::min(255, static_cast<int>(std::round(finalLinearColour.r))),
        std::min(255, static_cast<int>(std::round(finalLinearColour.g))),
        std::min(255, static_cast<int>(std::round(finalLinearColour.b))));
    return attenuateColour(tintedColour, mediumAbsorption, distanceThroughMedium);
}

Colour RayTracedRenderer::refractedRay(const RayTriangleIntersection &intersection, const glm::vec3 &direction, int depth, const glm::vec3 &mediumAbsorption, bool entering, float distanceThroughMedium)
{
    // Get the intersected triangle and the intersection point for further processing
    const ModelTriangle &triangle = intersection.intersectedTriangle;
    glm::vec3 hitPoint = intersection.intersectionPoint;

    // Calculate the surface normal based on whether the triangle is water or not, using the waterNormal function for water surfaces
    glm::vec3 surfaceNormal = triangle.normal;
    if (triangle.isWater)
    {
        surfaceNormal = waterNormal(hitPoint, triangle.normal);
    }

    glm::vec3 flatNormal;
    glm::vec3 normal;
    // Calculate the flat normal and the actual normal for refraction calculations
    if (entering)
    {
        flatNormal = triangle.normal;
        normal = surfaceNormal;
    }
    else
    {
        flatNormal = -triangle.normal;
        normal = -surfaceNormal;
    }

    if (glm::dot(direction, normal) > 0.0f)
    {
        normal = -normal;
    }

    // Determine the indices of refraction for the entering and exiting media based on whether the ray is entering or exiting the triangle
    float n1 = triangle.refractiveIndex;
    float n2 = 1.0f;
    if (entering)
    {
        n1 = 1.0f;
        n2 = triangle.refractiveIndex;
    }

    // Calculate the reflected direction using the reflection formula and normalize it
    glm::vec3 effectiveNormal = normal;
    if (glm::dot(glm::reflect(direction, effectiveNormal), flatNormal) <= 0.0f)
    {
        effectiveNormal = glm::normalize(glm::mix(normal, flatNormal, 0.5f));
    }
    glm::vec3 reflectedDirection = glm::normalize(glm::reflect(direction, effectiveNormal));

    Colour reflectedColour = traceRay(
        hitPoint + reflectedDirection * SURFACE_OFFSET,
        reflectedDirection,
        depth + 1,
        mediumAbsorption,
        !entering);

    // Calculate the refracted direction using Snell's law and check for total internal reflection
    glm::vec3 refractedDirection;
    float indexOfRefraction = n1 / n2;

    // If refraction is not possible (total internal reflection), return the attenuated reflected colour
    if (!refractRay(direction, normal, indexOfRefraction, refractedDirection))
    {
        return attenuateColour(reflectedColour, mediumAbsorption, distanceThroughMedium);
    }
    if (glm::dot(refractedDirection, flatNormal) > 0.0f &&
        !refractRay(direction, flatNormal, indexOfRefraction, refractedDirection))
    {
        return attenuateColour(reflectedColour, mediumAbsorption, distanceThroughMedium);
    }

    // Trace the refracted ray recursively, offsetting the starting point slightly to avoid self-intersection
    glm::vec3 refractedAbsorption(0.0f);
    if (entering)
    {
        refractedAbsorption = triangle.absorption;
    }

    Colour refractedColour = traceRay(
        hitPoint + refractedDirection * SURFACE_OFFSET,
        refractedDirection,
        depth + 1,
        refractedAbsorption,
        entering);

    // Calculate the Fresnel-Schlick factor based on the angle of incidence and the indices of refraction
    float cosTheta = -glm::dot(refractedDirection, normal);
    if (n1 <= n2)
    {
        cosTheta = glm::dot(-direction, normal);
    }
    float fresnelSchlickFactor = fresnelSchlick(glm::clamp(cosTheta, 0.0f, 1.0f), n1, n2);
    float refractedFactor = 1.0f - fresnelSchlickFactor;

    Colour colour(
        std::min(static_cast<int>(fresnelSchlickFactor * reflectedColour.red + refractedFactor * refractedColour.red), 255),
        std::min(static_cast<int>(fresnelSchlickFactor * reflectedColour.green + refractedFactor * refractedColour.green), 255),
        std::min(static_cast<int>(fresnelSchlickFactor * reflectedColour.blue + refractedFactor * refractedColour.blue), 255));
    return attenuateColour(colour, mediumAbsorption, distanceThroughMedium);
}

// Trace a ray through the scene, handling reflections, refractions, and shading based on the intersection with triangles
Colour RayTracedRenderer::traceRay(const glm::vec3 &origin, const glm::vec3 &direction, int depth, const glm::vec3 &mediumAbsorption, bool insideMedium)
{
    if (depth > MAX_REFRACTION_RECURSION_DEPTH)
        return Colour(0, 0, 0);

    // Get the closest valid intersection for the ray
    RayTriangleIntersection intersection = getClosestValidIntersection(origin, direction);

    // If no intersection is found or the intersection is behind the camera, return the appropriate colour based on whether the ray is inside a medium or not
    if (intersection.distanceFromCamera <= 0)
    {

        if (environmentMapEnabled && environmentMapSeenByCamera)
        {
            return enviornmentHelper(direction);
        }

        if (depth == 0)
        {
            return Colour(0, 0, 0);
        }

        if (environmentMapEnabled)
        {
            return enviornmentHelper(direction);
        }

        int sky = static_cast<int>(std::round(AMBIENT_METAL * 255.0f));
        return Colour(sky, sky, sky);
    }

    // Get the intersected triangle for further processing
    const ModelTriangle &triangle = intersection.intersectedTriangle;

    // Calculate the distance traveled through the medium based on whether the ray is inside the medium or not
    float distanceThroughMedium = 0.0f;
    if (insideMedium)
    {
        distanceThroughMedium = intersection.distanceFromCamera;
    }

    auto throughMedium = [&](const Colour &colour)
    {
        return attenuateColour(colour, mediumAbsorption, distanceThroughMedium);
    };

    // Handle different cases based on the properties of the intersected triangle
    // If the triangle is a light source, attenuate the colour based on the medium absorption and distance traveled
    if (triangle.objectName == "light")
    {
        return throughMedium(triangle.colour);
    }

    // If the triangle is a mirror and world mirroring is enabled, calculate the reflected ray and trace it recursively
    if (triangle.isMirror && worldMirroringEnabled)
    {
        return mirrorredRay(intersection, direction, depth, mediumAbsorption, insideMedium, distanceThroughMedium);
    }

    if (triangle.isMetallic && worldMetallicEnabled)
    {
        return mettalicRay(intersection, direction, depth, mediumAbsorption, insideMedium, distanceThroughMedium);
    }

    const bool entering = !insideMedium;
    // If the triangle is refractive and world glass is enabled, handle refraction and reflection using Fresnel-Schlick approximation
    if (triangle.isRefractive && worldRefractiveEnabled)
    {
        return refractedRay(intersection, direction, depth, mediumAbsorption, entering, distanceThroughMedium);
    }
    viewOrigin = origin;
    return throughMedium(shadingHelper(intersection));
}

// Sample the environment map based on the given direction, returning the corresponding colour
Colour RayTracedRenderer::enviornmentHelper(const glm::vec3 &direction)
{
    // Normalize the direction vector to ensure it has a length of 1
    glm::vec3 normalizedDirection = glm::normalize(direction);

    // Calculate the longitude and latitude based on the normalized direction vector using spherical coordinates
    float longitude = std::atan2(-normalizedDirection.z, normalizedDirection.x);

    float latitude = std::asin(glm::clamp(normalizedDirection.y, -1.0f, 1.0f));

    float u = longitude / (2.0f * RAY_TRACED_PI) + 0.5f;
    float v = 0.5f - latitude / RAY_TRACED_PI;

    const int environmentTextureWidth = static_cast<int>(environmentMap.width);
    const int environmentTextureHeight = static_cast<int>(environmentMap.height);

    int x = static_cast<int>(u * environmentTextureWidth) % environmentTextureWidth;
    if (x < 0)
    {
        x += environmentTextureWidth;
    }

    const int y = glm::clamp(static_cast<int>(v * environmentTextureHeight), 0, environmentTextureHeight - 1);

    const uint32_t packed = environmentMap.pixels[y * environmentTextureWidth + x];
    return Colour((packed >> 16) & 0xFF, (packed >> 8) & 0xFF, packed & 0xFF);
}

// Generate a random point on the light source for photon tracing
static glm::vec3 randomPointOnLight()
{
    // Generate random values for the x and z
    float randomX = randomFloat();
    float randomZ = randomFloat();

    // Calculate the y-coordinate slightly below the light source to avoid self-intersection
    float y = lightMin.y - SURFACE_OFFSET;

    // Calculate the x and z coordinates based on the random values and the light source bounds
    float x = lightMin.x + randomX * (lightMax.x - lightMin.x);
    float z = lightMin.z + randomZ * (lightMax.z - lightMin.z);

    // Return the random point on the light source as a glm::vec3
    return glm::vec3(x, y, z);
}

// Generate a random downward direction for photon tracing
static glm::vec3 randomDownwardDirection()
{
    // Generate a random point oriented downwards
    float y = -randomFloat();
    // Calculate the radius of the circle at the given y-coordinate using the Pythagorean theorem
    float r = std::sqrt(1.0f - y * y);
    // Generate a random angle
    float theta = 2.0f * RAY_TRACED_PI * randomFloat();
    // Calculate the x and z coordinates based on the radius and angle using polar coordinates
    float randomX = r * std::cos(theta);
    float randomZ = r * std::sin(theta);
    // Ensure the y-coordinate is negative to point downwards
    if (y > 0.0f)
    {
        y = -y;
    }
    // Return the normalized direction vector
    return glm::normalize(glm::vec3(randomX, y, randomZ));
}

// Generate a random direction in the hemisphere defined by the given normal vector
// Given by intellisense
static glm::vec3 randomHemisphereDirection(const glm::vec3 &normal)
{
    glm::vec3 direction;
    do
    {
        direction = glm::vec3(
            2.0f * randomFloat() - 1.0f,
            2.0f * randomFloat() - 1.0f,
            2.0f * randomFloat() - 1.0f);
    } while (glm::dot(direction, direction) > 1.0f || glm::dot(direction, direction) < 1e-8f);

    direction = glm::normalize(direction);
    if (glm::dot(direction, normal) < 0.0f)
    {
        return -direction;
    }
    return direction;
}

void RayTracedRenderer::tracePhoton(glm::vec3 origin, glm::vec3 direction, glm::vec3 power, bool insideMedium, bool cameFromSpecular, int depth, int diffuseDepth)
{
    // If the recursion depth exceeds the maximum allowed depth, return to avoid infinite recursion
    if (depth >= MAX_PHOTON_RECURSION_DEPTH)
    {
        return;
    }
    // Get the closest valid intersection for the photon ray
    RayTriangleIntersection intersection = getClosestValidIntersection(origin, direction);

    // If no intersection is found or the intersection is behind the camera, return to avoid further processing
    if (intersection.distanceFromCamera <= 0)
    {
        return;
    }

    // Assign the intersected triangle for further processing
    const ModelTriangle &triangle = intersection.intersectedTriangle;

    // If the intersected triangle is a light source, return to avoid tracing photons from light sources
    if (triangle.objectName == "light")
    {
        return;
    }

    // Handle different cases based on the properties of the intersected triangle

    // Path for refractive or water
    if ((triangle.isRefractive || triangle.isWater) && worldRefractiveEnabled)
    {
        // If the photon is in a medium
        if (insideMedium)
        {
            // Attenuate the power of the photon based on the absorption coefficients and distance traveled through the medium
            power *= computeBeerLambertTransmission(triangle.absorption, intersection.distanceFromCamera);
        }

        // if the photon is coming from a specular surface, we need to handle the Fresnel effect and determine whether it reflects or refracts
        cameFromSpecular = true;

        // Determine if the photon is entering or exiting the medium based on the current state of insideMedium
        bool entering = !insideMedium;

        glm::vec3 surfaceNormal;

        // Check for water
        if (triangle.isWater)
        {
            waterNormal(intersection.intersectionPoint, triangle.normal);
        }
        else
        {
            surfaceNormal = triangle.normal;
        }

        // Determine the side of the surface normal based on the direction of the photon and the triangle's normal
        float side;
        if (glm::dot(direction, triangle.normal) > 0.0f)
        {
            side = -1.0f;
        }
        else
        {
            side = 1.0f;
        }

        glm::vec3 normal = side * surfaceNormal;

        // Calculate the cosine of the angle between the incoming photon and the surface normal
        float cosTheta = glm::clamp(glm::dot(-direction, normal), 0.0f, 1.0f);
        float RandomChance;

        if (entering)
        {
            // Ray is entering the triangle (e.g., from air [1.0] into glass)
            RandomChance = fresnelSchlick(cosTheta, 1.0f, triangle.refractiveIndex);
        }
        else
        {
            // Ray is exiting the triangle (e.g., from glass back into air [1.0])
            RandomChance = fresnelSchlick(cosTheta, triangle.refractiveIndex, 1.0f);
        }

        if (RandomChance > randomFloat())
        {
            direction = glm::normalize(direction - 2.0f * glm::dot(direction, normal) * normal);
        }
        else
        {
            float indexOfRefraction = triangle.refractiveIndex;
            if (entering)
            {
                indexOfRefraction = 1.0f / triangle.refractiveIndex;
            }

            if (refractRay(direction, normal, indexOfRefraction, direction))
            {
                insideMedium = !insideMedium;
            }
            else
            {
                direction = glm::normalize(direction - 2.0f * glm::dot(direction, normal) * normal);
            }
        }

        tracePhoton(intersection.intersectionPoint + direction * SURFACE_OFFSET, direction, power, insideMedium, cameFromSpecular, depth + 1, diffuseDepth);
        return;
    }

    // Path for mirror
    if (triangle.isMirror && worldMirroringEnabled)
    {
        cameFromSpecular = true;
        // Calculate the reflected direction using the reflection formula and normalize it
        direction = glm::normalize(direction - 2.0f * glm::dot(direction, triangle.normal) * triangle.normal);
        // Trace the reflected photon recursively, offsetting the starting point slightly to avoid self-intersection
        tracePhoton(intersection.intersectionPoint + direction * SURFACE_OFFSET, direction, power, insideMedium, cameFromSpecular, depth + 1, diffuseDepth);
        return;
    }

    // Path for metallic surfaces
    if (triangle.isMetallic && worldMetallicEnabled)
    {
        cameFromSpecular = true;

        // Calculate the Fresnel tint using the Fresnel-Schlick approximation for metallic surfaces
        float cosTheta = glm::clamp(glm::dot(-direction, triangle.normal), 0.0f, 1.0f);
        glm::vec3 fresnelTint = fresnelSchlickMetallic(cosTheta, triangle.fresnelReflectance);

        // Attenuate the power of the photon based on the Fresnel tint
        power *= fresnelTint;

        // Determine the side of the surface normal based on the direction of the photon and the triangle's normal
        float side = 1.0f;
        if (glm::dot(direction, triangle.normal) > 0.0f)
        {
            side = -1.0f;
        }
        glm::vec3 normal = side * triangle.normal;

        // Calculate the reflected direction using the reflection formula and normalize it
        glm::vec3 reflectedDirection = direction - 2.0f * glm::dot(direction, normal) * normal;
        direction = glm::normalize(reflectedDirection);
        // Trace the reflected photon recursively, offsetting the starting point slightly to avoid self-intersection
        tracePhoton(intersection.intersectionPoint + direction * SURFACE_OFFSET, direction, power, insideMedium, cameFromSpecular, depth + 1, diffuseDepth);
        return;
    }

    // Path for diffuse surfaces

    // Calculate the surface normal and ensure it is facing the correct direction based on the incoming photon direction
    glm::vec3 surfaceNormal = triangle.normal;
    if (glm::dot(direction, surfaceNormal) > 0.0f)
    {
        surfaceNormal = -surfaceNormal;
    }

    // If the photon is coming from a specular surface and the diffuse depth is zero, add the photon to the caustic photon map
    if (diffuseDepth == 0 && cameFromSpecular)
    {
        causticPhotonMap.addPhoton({intersection.intersectionPoint, direction, power});
    }
    // If the diffuse depth is greater than zero, add the photon to the global photon map
    else if (diffuseDepth > 0)
    {
        globalPhotonMap.addPhoton({intersection.intersectionPoint, direction, power});
    }

    // Calculate the surface colour and survival probability based on the triangle's colour
    glm::vec3 surfaceColour = glm::vec3(triangle.colour.red, triangle.colour.green, triangle.colour.blue) / 255.0f;
    float survival = (surfaceColour.r + surfaceColour.g + surfaceColour.b) / 3.0f;

    // If the random float is greater than or equal to the survival probability, return to terminate the photon tracing
    if (randomFloat() >= survival)
    {
        return;
    }
    // Attenuate the power of the photon based on the surface colour and survival probability
    power = power * surfaceColour / survival;

    // Generate a new random direction in the hemisphere defined by the surface normal for diffuse reflection
    glm::vec3 newDirection = randomHemisphereDirection(surfaceNormal);

    power *= 2.0f * glm::dot(newDirection, surfaceNormal);

    tracePhoton(intersection.intersectionPoint + surfaceNormal * SURFACE_OFFSET, newDirection, power, false, false, depth + 1, diffuseDepth + 1);
}

void RayTracedRenderer::buildPhotonMaps()
{
    // Clear the two maps
    causticPhotonMap.clear();
    globalPhotonMap.clear();

    // Trace photons from the light source and store them in the photon maps
    for (int index = 0; index < NUMBER_OF_PHOTONS; index++)
    {
        // Trace a photon from a random point on the light source in a random downward direction
        tracePhoton(randomPointOnLight(), randomDownwardDirection(), glm::vec3(LIGHT_INTENSITY * 0.5f / NUMBER_OF_PHOTONS), false, false);
    }

    // Build the two maps
    causticPhotonMap.build();
    globalPhotonMap.build();

    // Print the number of photons in each photon map to the console for debugging purposes
    std::cout << "Caustic photon map built with " << causticPhotonMap.size() << " photons." << std::endl;
    std::cout << "Global photon map built with " << globalPhotonMap.size() << " photons." << std::endl;
}
