#include "PhotonMap.h"

std::vector<Photon> photons;

bool isBuilt = false;

void buildKDTree(int depth, int start, int end)
{
    if (start >= (end - 1))
    {
        return;
    }

    int axis = depth % 3;
    int mid = (start + end) / 2;
    std::sort(photons.begin() + start, photons.begin() + end + 1,
              [axis](const Photon &a, const Photon &b)
              {
                  return a.position[axis] < b.position[axis];
              });
    buildKDTree(depth + 1, start, mid);
    buildKDTree(depth + 1, mid + 1, end);
}

void build(int numPhotons)
{
    photons.reserve(numPhotons);
    buildKDTree(0, 0, numPhotons - 1);
    isBuilt = true;
}

void clear()
{
    photons.clear();
    isBuilt = false;
}

void addPhoton(const Photon &photon)
{
    photons.push_back(photon);
}

const std::vector<Photon> &allPhotons()
{
    return photons;
}

void findNearestPhotonsRecursive(int start, int end, int depth, const glm::vec3 &point, float radiusSquared, std::vector<const Photon *> &found)
{
    if (start >= end)
    {
        return;
    }

    Photon medianPhoton = photons[(start + end) / 2];

    glm::vec3 photonPosition = medianPhoton.position;
    float distanceSquared = glm::length(photonPosition - point);

    if (distanceSquared <= radiusSquared)
    {
        found.push_back(&medianPhoton);
    }

    int axis = depth % 3;

    float diff = point[axis] - photonPosition[axis];

    if (diff < 0)
    {
        findNearestPhotonsRecursive(start, (start + end) / 2 - 1, depth + 1, point, radiusSquared, found);
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
};

void findNearestPhotons(const glm::vec3 &position, float radius, std::vector<const Photon *> &nearestPhotons)
{
    nearestPhotons.clear();
    float radiusSquared = radius * radius;
    findNearestPhotonsRecursive(0, photons.size() - 1, 0, position, radiusSquared, nearestPhotons);
}

glm::vec3 estimateIrradiance(const glm::vec3 &position, const glm::vec3 &normal, float radius)
{
    std::vector<const Photon *> nearestPhotons;
    findNearestPhotons(position, radius, nearestPhotons);
    glm::vec3 irradiance(0.0f);
    for (const Photon *photon : nearestPhotons)
    {
        if (glm::dot(photon->direction, normal) < 0.0f)
        {
            continue;
        }
        irradiance += photon->power;
    }

    float area = 3.14159265358979323846f * radius * radius;
    return irradiance / area;
}

size_t size()
{

    return photons.size();
}
