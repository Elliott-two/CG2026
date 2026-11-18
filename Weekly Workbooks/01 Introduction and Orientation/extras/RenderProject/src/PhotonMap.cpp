#include "PhotonMap.h"

void PhotonMap::buildKDTree(int depth, int start, int end)
{
    if (end - start <= 1)
    {
        return;
    }

    int axis = depth % 3;
    int mid = (start + end) / 2;
    std::sort(photons.begin() + start, photons.begin() + end,
              [axis](const Photon &a, const Photon &b)
              {
                  return a.position[axis] < b.position[axis];
              });
    buildKDTree(depth + 1, start, mid);
    buildKDTree(depth + 1, mid + 1, end);
}

void PhotonMap::build()
{
    buildKDTree(0, 0, photons.size());
    isBuilt = true;
}

void PhotonMap::clear()
{
    photons.clear();
    isBuilt = false;
}

void PhotonMap::addPhoton(const Photon &photon)
{
    photons.push_back(photon);
}

const std::vector<Photon> &PhotonMap::allPhotons() const
{
    return photons;
}

void PhotonMap::findNearestPhotonsRecursive(int start, int end, int depth, const glm::vec3 &point, float radiusSquared, std::vector<const Photon *> &found) const
{
    if (start >= end)
    {
        return;
    }
    int mid = (start + end) / 2;

    const Photon &medianPhoton = photons[mid];

    glm::vec3 photonPosition = medianPhoton.position;
    glm::vec3 offset = photonPosition - point;
    float distanceSquared = glm::dot(offset, offset);

    if (distanceSquared <= radiusSquared)
    {
        found.push_back(&medianPhoton);
    }

    int axis = depth % 3;

    float diff = point[axis] - photonPosition[axis];

    if (diff < 0)
    {
        findNearestPhotonsRecursive(start, mid, depth + 1, point, radiusSquared, found);
        if (diff * diff <= radiusSquared)
        {
            findNearestPhotonsRecursive(mid + 1, end, depth + 1, point, radiusSquared, found);
        }
    }
    else
    {
        findNearestPhotonsRecursive(mid + 1, end, depth + 1, point, radiusSquared, found);
        if (diff * diff <= radiusSquared)
        {
            findNearestPhotonsRecursive(start, mid, depth + 1, point, radiusSquared, found);
        }
    }
}

void PhotonMap::findNearestPhotons(const glm::vec3 &position, float radius, std::vector<const Photon *> &nearestPhotons) const
{
    nearestPhotons.clear();
    float radiusSquared = radius * radius;
    if (!isBuilt || photons.empty())
    {
        return;
    }
    findNearestPhotonsRecursive(0, photons.size(), 0, position, radiusSquared, nearestPhotons);
}

glm::vec3 PhotonMap::estimateIrradiance(const glm::vec3 &position, const glm::vec3 &normal, float radius) const
{
    const float k = 1.1f;

    static thread_local std::vector<const Photon *> nearestPhotons;
    glm::vec3 irradiance(0.0f);
    for (const Photon *photon : nearestPhotons)
    {
        if (glm::dot(photon->direction, normal) >= 0.0f)
        {
            continue;
        }
        const float distance = glm::length(photon->position - position);
        const float weight = 1.0f - distance / (k * radius);
        irradiance += photon->power * weight;
    }

    const float normalisation = 1.0f - 2.0f / (3.0f * k);
    const float area = 3.14159265358979323846f * radius * radius;
    return irradiance / (normalisation * area);
}

size_t PhotonMap::size() const
{

    return photons.size();
}
