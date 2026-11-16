#pragma once

#include <glm/glm.hpp>
#include <string>
#include <array>
#include "Colour.h"
#include "TexturePoint.h"

struct ModelTriangle
{
	std::array<glm::vec3, 3> vertices{};
	std::array<TexturePoint, 3> texturePoints{};
	Colour colour{};
	glm::vec3 normal{};

	// Added objectName and textureIndex to ModelTriangle for better identification and texture mapping
	std::string objectName{};
	int textureIndex = -1;
	bool isMirror = false;		  // Flag to indicate if the triangle is a mirror
	bool isRefractive = false;	  // Flag to indicate if the triangle is refractive
	float refractiveIndex = 1.0f; // Refractive index for the triangle, default is 1.0 (no refraction)
	glm::vec3 absorption = glm::vec3(0.0f);
	bool isWater = false; // Flag to indicate if the triangle is water

	ModelTriangle();
	ModelTriangle(const glm::vec3 &v0, const glm::vec3 &v1, const glm::vec3 &v2, Colour trigColour);
	friend std::ostream &operator<<(std::ostream &os, const ModelTriangle &triangle);
};
