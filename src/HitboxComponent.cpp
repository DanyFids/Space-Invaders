#include "HitboxComponent.h"
#include "florp/game/SceneManager.h"
#include "florp/game/Transform.h"

bool Hitbox::HitDetect(entt::entity t, entt::entity o)
{
	Hitbox h_this = CurrentRegistry().get<Hitbox>(t);
	Hitbox h_other = CurrentRegistry().get<Hitbox>(o);
	auto& t_this = CurrentRegistry().get<florp::game::Transform>(t);
	auto& t_other = CurrentRegistry().get<florp::game::Transform>(o);


	if (t_this.GetLocalPosition().x < t_other.GetLocalPosition().x + h_other.dimm.x && t_this.GetLocalPosition().x + h_this.dimm.x >  t_other.GetLocalPosition().x&&
		t_this.GetLocalPosition().y < t_other.GetLocalPosition().y + h_other.dimm.y && t_this.GetLocalPosition().y + h_this.dimm.y >  t_other.GetLocalPosition().y&&
		t_this.GetLocalPosition().z < t_other.GetLocalPosition().z + h_other.dimm.z && t_this.GetLocalPosition().z + h_this.dimm.z >  t_other.GetLocalPosition().z)
		return true;
	else
		return false;
	return false;
}
