#pragma once
#include "florp/game/IBehaviour.h"
#include <GLM/glm.hpp>

class ControlBehaviour : public florp::game::IBehaviour {
public:
	ControlBehaviour(const glm::vec3& speed) : IBehaviour(), mySpeed(speed), myYawPitch(glm::vec2(0.0f)) {};
	virtual ~ControlBehaviour() = default;

	virtual void Update(entt::entity entity) override;

	void PlayerShoot(entt::entity player);

private:

	glm::vec3 mySpeed;
	glm::vec2 myYawPitch;
};

class BulletBehaviour : public florp::game::IBehaviour {
public:
	static entt::entity player;
	static std::vector<std::vector<entt::entity>>* aliens;

	BulletBehaviour(const glm::vec3& spd) : IBehaviour(), mySpeed(spd) {};
	virtual ~BulletBehaviour() = default;

	virtual void Update(entt::entity entity) override;
private:

	glm::vec3 mySpeed;
	float lifetime = 3.0f;
};

class EnemyBehaviour : public florp::game::IBehaviour {
public:
	static entt::entity player;
	static std::vector<std::vector<entt::entity>> aliens;
	static const float COOLDOWN_TIME;
	static float cooldown;
	static float mv_time;
	static const float MOVE_TIME; 
	static bool dir;

	virtual void Update(entt::entity entity) override;

private:
};
