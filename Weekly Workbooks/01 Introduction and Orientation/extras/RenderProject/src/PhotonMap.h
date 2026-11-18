#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <algorithm>

struct Photon
{
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 power;
};

class PhotonMap
{
public:
    void build(int numPhotons);
    void clear();
    void addPhoton(const Photon &photon);
    void findNearestPhotons(const glm::vec3 &position, float radius, std::vector<const Photon *> &nearestPhotons) const;
    glm::vec3 estimateIrradiance(const glm::vec3 &position, const glm::vec3 &normal, float radius) const;
    size_t size() const;
    const std::vector<Photon> &allPhotons() const;

private:
    std::vector<Photon> photons;
    bool isBuilt = false;
    void buildKDTree(int depth, int start, int end);
    void findNearestPhotonsRecursive(int start, int end, int depth, const glm::vec3 &point, float radiusSquared, std::vector<const Photon *> &found) const;
};
