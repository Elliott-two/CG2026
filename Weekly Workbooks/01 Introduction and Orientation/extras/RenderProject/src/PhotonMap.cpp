#include "PhotonMap.h"

void PhotonMap::buildKDTree(int depth, int start, int end)
{
    // If the range is empty or has only one photon, return
    if (end - start <= 1)
    {
        return;
    }

    // Sort the photons in the range [start, end) based on the current axis
    int axis = depth % 3;
    // Find the median index
    int mid = (start + end) / 2;
    // Sort the photons based on the current axis
    std::sort(photons.begin() + start, photons.begin() + end,
              [axis](const Photon &a, const Photon &b)
              {
                  return a.position[axis] < b.position[axis];
              });
    // Recursively build the left and right subtrees
    buildKDTree(depth + 1, start, mid);
    buildKDTree(depth + 1, mid + 1, end);
}

// Build the KD-tree from the photons
void PhotonMap::build()
{
    // Build the KD-tree from the photons
    buildKDTree(0, 0, photons.size());

    // Mark the photon map as built
    isBuilt = true;
}

// Clear the photon map
void PhotonMap::clear()
{
    // Clear the photon map
    photons.clear();

    // Mark the photon map as not built
    isBuilt = false;
}
// Add a photon to the photon map
void PhotonMap::addPhoton(const Photon &photon)
{
    // Add the photon to the photon map
    photons.push_back(photon);
}

// Get the list of all photons in the photon map
const std::vector<Photon> &PhotonMap::allPhotons() const
{
    return photons;
}

// Recursively find the nearest photons to a given point within a specified radius
void PhotonMap::findNearestPhotonsRecursive(int start, int end, int depth, const glm::vec3 &point, float radiusSquared, std::vector<const Photon *> &found) const
{
    // If the range is empty, return
    if (start >= end)
    {
        return;
    }

    // Find the median index
    int mid = (start + end) / 2;

    // Get the median photon
    const Photon &medianPhoton = photons[mid];

    // Assign the median photon position to a variable for clarity
    glm::vec3 medianPhotonPosition = medianPhoton.position;

    // Calculate the squared distance from the point to the median photon
    glm::vec3 offset = medianPhotonPosition - point;
    float distanceSquared = glm::dot(offset, offset);

    // If the distance is within the radius, add the median photon to the found list
    if (distanceSquared <= radiusSquared)
    {
        found.push_back(&medianPhoton);
    }

    // Assign the current axis based on the depth of the tree
    int axis = depth % 3;

    // Calculate the difference between the point and the median photon along the current axis
    float difference = point[axis] - medianPhotonPosition[axis];

    // Recursively search the left and right subtrees based on the difference
    if (difference < 0)
    {
        // Search the left subtree first
        findNearestPhotonsRecursive(start, mid, depth + 1, point, radiusSquared, found);

        // If the difference is within the radius, search the right subtree as well
        if (difference * difference <= radiusSquared)
        {
            findNearestPhotonsRecursive(mid + 1, end, depth + 1, point, radiusSquared, found);
        }
    }
    else
    {
        // Search the right subtree first
        findNearestPhotonsRecursive(mid + 1, end, depth + 1, point, radiusSquared, found);

        // If the difference is within the radius, search the left subtree as well
        if (difference * difference <= radiusSquared)
        {
            findNearestPhotonsRecursive(start, mid, depth + 1, point, radiusSquared, found);
        }
    }
}

// Find the nearest photons to a given point within a specified radius
void PhotonMap::findNearestPhotons(const glm::vec3 &position, float radius, std::vector<const Photon *> &nearestPhotons) const
{
    // Clear the nearest photons vector
    nearestPhotons.clear();

    // Calculate the squared radius for distance comparison
    float radiusSquared = radius * radius;

    // If the photon map is not built or has no photons, return early
    if (!isBuilt || photons.empty())
    {
        return;
    }

    // Start the recursive search for nearest photons
    findNearestPhotonsRecursive(0, photons.size(), 0, position, radiusSquared, nearestPhotons);
}

// Estimate the irradiance at a given position based on nearby photons
glm::vec3 PhotonMap::estimateIrradiance(const glm::vec3 &position, const glm::vec3 &normal, float radius) const
{
    // Use a thread-local vector to store the nearest photons for this thread
    static thread_local std::vector<const Photon *> nearestPhotons;

    // Initialize the irradiance to zero
    glm::vec3 irradiance(0.0f);

    // Find the nearest photons to the given position within the specified radius
    findNearestPhotons(position, radius, nearestPhotons);

    // Loop through all photons
    for (const Photon *photon : nearestPhotons)
    {
        // Check if the photon is coming from the same direction as the surface normal
        if (glm::dot(photon->direction, normal) >= 0.0f)
        {
            continue;
        }
        // Calculate the distance from the photon to the position
        const float distance = glm::length(photon->position - position);

        // Calculate the weight based on the distance
        const float weight = 1.0f - distance / (radius);

        // Accumulate the weighted power of the photon into the irradiance
        irradiance += photon->power * weight;
    }

    // Normalize the irradiance based on the area of the search radius and the adjuster
    const float normalisation = 1.0f / (3.0f);

    // Calculate the area of the search radius
    const float area = PHOTON_PI * radius * radius;

    // Return the normalized irradiance
    return irradiance / (normalisation * area);
}

// Get the number of photons in the photon map
size_t PhotonMap::size() const
{
    return photons.size();
}
