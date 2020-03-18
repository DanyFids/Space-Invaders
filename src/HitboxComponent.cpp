#include "HitboxComponent.h"
#include "florp/game/SceneManager.h"
#include "florp/game/Transform.h"

bool Hitbox::HitDetect(glm::vec3 t_pos, glm::vec3 t_dim, glm::vec3 o_pos, glm::vec3 o_dim)
{
	float t_shallow = t_pos.z - (t_dim.z / 2.0f);
	float o_shallow = o_pos.z - (o_dim.z / 2.0f);
	float t_deep = t_pos.z + (t_dim.z / 2.0f);
	float o_deep = o_pos.z + (o_dim.z / 2.0f);

	float t_left = t_pos.x - (t_dim.x / 2.0f);
	float o_left = o_pos.x - (o_dim.x / 2.0f);
	float t_right = t_pos.x + (t_dim.x / 2.0f);
	float o_right = o_pos.x + (o_dim.x / 2.0f);

	float t_bottom = t_pos.y - (t_dim.y / 2.0f);
	float o_bottom = o_pos.y - (o_dim.y / 2.0f);
	float t_top = t_pos.y + (t_dim.y / 2.0f);
	float o_top = o_pos.y + (o_dim.y / 2.0f);

	if (t_left < o_right && t_right > o_left && t_top > o_bottom && t_bottom < o_top && t_shallow < o_deep && t_deep > o_shallow)
		return true;
	else
		return false;
	return false;
}
