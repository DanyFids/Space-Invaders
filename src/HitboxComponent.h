#pragma once
#include <entt.hpp>
#include <GLM/glm.hpp>

class Hitbox {
public:
	Hitbox(glm::vec3 d) : dimm(d) {}

	glm::vec3 dimm;

	static bool HitDetect(glm::vec3 t_pos, glm::vec3 t_dim, glm::vec3 o_pos, glm::vec3 o_dim);
};

class Life {
public:
	Life(int l = 1): life(l) {};
	~Life() {};

	int life;
};

class WallLife {
public:
	WallLife(int l = 1) : life(l) {};
	~WallLife() {};

	int life;
};