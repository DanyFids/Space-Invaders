#pragma once
#include <entt.hpp>
#include <GLM/glm.hpp>

class Hitbox {
	glm::vec3 dimm;
public:
	Hitbox(glm::vec3 d) : dimm(d) {}

	static bool HitDetect(entt::entity t, entt::entity o);
};