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
        findNearestPhotonsRecursive(start, (start + end) / 2, depth + 1, point, radiusSquared, found);
        if (diff * diff <= radiusSquared)
        {
            findNearestPhotonsRecursive((start + end) / 2 + 1, end, depth + 1, point, radiusSquared, found);
        }
    }
    else
    {
        findNearestPhotonsRecursive((start + end) / 2 + 1, end, depth + 1, point, radiusSquared, found);
        if (diff * diff <= radiusSquared)
        {
            findNearestPhotonsRecursive(start, (start + end) / 2 - 1, depth + 1, point, radiusSquared, found);
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
    std::vector<const Photon *> nearestPhotons;
    findNearestPhotons(position, radius, nearestPhotons);
    glm::vec3 irradiance(0.0f);
    for (const Photon *photon : nearestPhotons)
    {
        if (glm::dot(photon->direction, normal) >= 0.0f)
        {
            continue;
        }
        irradiance += photon->power;
    }

    float area = 3.14159265358979323846f * radius * radius;
    return irradiance / area;
}

size_t PhotonMap::size() const
{

    return photons.size();
}
