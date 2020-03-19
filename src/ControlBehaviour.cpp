#include "ControlBehaviour.h"
#include "florp/app/Window.h"
#include "florp/app/Application.h"
#include "florp/game/Transform.h"
#include "florp/game/SceneManager.h"
#include "florp/app/Timing.h"
#include "florp/graphics/Texture2D.h"
#include "HitboxComponent.h"

#include "florp/game/SceneManager.h"
#include "florp/game/RenderableComponent.h"
#include <florp\graphics\MeshData.h>
#include <florp\graphics\MeshBuilder.h>
#include <florp\graphics\ObjLoader.h>


#define GLM_ENABLE_EXPERIMENTAL
#include <GLM/gtx/wrap.hpp>
#include <AudioEngine.h>

template <typename T>
T wrap(const T& value, const T& min, const T& max) {
	//(((x - x_min) % (x_max - x_min)) + (x_max - x_min)) % (x_max - x_min) + x_min;
	T range = max - min;
	return glm::mod(glm::mod(value - min, range) + range, range )+ min;
}

void ControlBehaviour::Update(entt::entity entity) {
	using namespace florp::app;
	auto& transform = CurrentRegistry().get<florp::game::Transform>(entity);
	Window::Sptr window = Application::Get()->GetWindow();

	glm::vec3 translate = glm::vec3(0.0f);
	if (window->IsKeyDown(Key::A))
		translate.x -= 1.0f;
	if (window->IsKeyDown(Key::D))
		translate.x += 1.0f;

	static bool space_p = false;
	if (window->IsKeyDown(Key::Space) && !space_p) {
		PlayerShoot(entity);
		space_p = true;
		AudioEngine::PlayEvent("Bullet");//Should Play bullet audio
	}
	if (window->GetKeyState(Key::Space) == ButtonState::Released) {
		space_p = false;
	}

	if (window->IsKeyDown(Key::Esc)) {
		window->Close();
	}

	translate *= Timing::DeltaTime * mySpeed;

	if (glm::length(translate) > 0) {
		translate = glm::mat3(transform.GetLocalTransform()) * translate;
		translate += transform.GetLocalPosition();
		transform.SetPosition(translate);
	}

	auto& pl = CurrentRegistry().get<Life>(entity);
	if (pl.life <= 0) {
		window->Close();
	}

	if (EnemyBehaviour::cooldown > 0.0f) EnemyBehaviour::cooldown -= Timing::DeltaTime;
	if(EnemyBehaviour::mv_time > 0.0f) EnemyBehaviour::mv_time -= Timing::DeltaTime;
	if (EnemyBehaviour::mv_time <= 0.0f) {
		EnemyBehaviour::mv_time = EnemyBehaviour::MOVE_TIME;
		EnemyBehaviour::dir = !EnemyBehaviour::dir;
	}
	AudioEngine::Update();
}

void ControlBehaviour::PlayerShoot(entt::entity player)
{
	using namespace florp::graphics;
	using namespace florp::game;

	MeshData bul_mesh = ObjLoader::LoadObj("Bullet.obj", glm::vec4(1.0f));
	auto* scene = SceneManager::Get("main");

	static Shader::Sptr shader = nullptr;
	if (shader == nullptr) {
		shader = std::make_shared<Shader>();
		shader->LoadPart(ShaderStageType::VertexShader, "shaders/lighting.vs.glsl");
		shader->LoadPart(ShaderStageType::FragmentShader, "shaders/forward.fs.glsl");
		shader->Link();
	}

	static Material::Sptr marbleMat = nullptr;
	if (marbleMat == nullptr) {
		marbleMat = std::make_shared<Material>(shader);
		marbleMat->Set("s_Albedo", Texture2D::LoadFromFile("marble.png", false, true, true));
	}


	auto newBul = scene->CreateEntity();
	RenderableComponent& renderable = scene->Registry().assign<RenderableComponent>(newBul);
	renderable.Mesh = MeshBuilder::Bake(bul_mesh);
	renderable.Material = marbleMat;
	Transform& t = scene->Registry().get<Transform>(newBul);
	Transform& pt = scene->Registry().get<Transform>(player);	

	Hitbox& h = scene->Registry().assign<Hitbox>(newBul, glm::vec3(0.1f, 0.4f, 0.1f));

	t.SetPosition(pt.GetLocalPosition() + glm::vec3(0.0f, 0.5f, 0.0f));
	scene->AddBehaviour<BulletBehaviour>(newBul, glm::vec3(0.0f, 8.0f, 0.0f));
}

std::vector<std::vector<entt::entity>>* BulletBehaviour::aliens = nullptr;
std::vector<std::vector<entt::entity>> EnemyBehaviour::aliens;

std::vector<std::vector<entt::entity>>* BulletBehaviour::walls = nullptr;
std::vector<std::vector<entt::entity>> WallBehaviour::walls;


entt::entity BulletBehaviour::player;
entt::entity EnemyBehaviour::player;
entt::entity WallBehaviour::player;

const float EnemyBehaviour::COOLDOWN_TIME = 1.5f;
float EnemyBehaviour::cooldown = EnemyBehaviour::COOLDOWN_TIME;
const float EnemyBehaviour::MOVE_TIME = 2.0f;
float EnemyBehaviour::mv_time = EnemyBehaviour::MOVE_TIME;
bool EnemyBehaviour::dir = true;

void BulletBehaviour::Update(entt::entity entity)
{
	auto& transform = CurrentRegistry().get<florp::game::Transform>(entity);
	auto& hb = CurrentRegistry().get<Hitbox>(entity);

	glm::vec3 translate = mySpeed * florp::app::Timing::DeltaTime;

	translate = glm::mat3(transform.GetLocalTransform()) * translate;
	translate += transform.GetLocalPosition();
	transform.SetPosition(translate);

	lifetime-= florp::app::Timing::DeltaTime;

	if (lifetime <= 0.0f) {
		CurrentRegistry().destroy(entity);
	}

	auto& pt = CurrentRegistry().get<florp::game::Transform>(player);
	auto& phb = CurrentRegistry().get<Hitbox>(player);
	if (Hitbox::HitDetect(transform.GetLocalPosition(), hb.dimm, pt.GetLocalPosition(), phb.dimm)) {
		auto& pl = CurrentRegistry().get<Life>(player);
		pl.life--;
		CurrentRegistry().destroy(entity);
	}


	for (int r = 0; r < aliens->size(); r++) {
		for (int c = 0; c < (*aliens)[r].size(); c++) {
			entt::entity a = (*aliens)[r].at(c);
			
			auto& at = CurrentRegistry().get<florp::game::Transform>(a);
			auto& ahb = CurrentRegistry().get<Hitbox>(a);


			if (Hitbox::HitDetect(transform.GetLocalPosition(), hb.dimm, at.GetLocalPosition(), ahb.dimm)) {
				CurrentRegistry().destroy(entity);
				auto& al = CurrentRegistry().get<Life>(a);
				al.life = 0;
				return;
			}
		}
	}
	//Hopefully this works to delete the walls/barriers
	for (int r = 0; r < walls->size(); r++) {
		for (int c = 0; c < (*walls)[r].size(); c++) {
			entt::entity a = (*walls)[r].at(c);

			auto& at = CurrentRegistry().get<florp::game::Transform>(a);
			auto& ahb = CurrentRegistry().get<Hitbox>(a);


			if (Hitbox::HitDetect(transform.GetLocalPosition(), hb.dimm, at.GetLocalPosition(), ahb.dimm)) {
				CurrentRegistry().destroy(entity);
				auto& al = CurrentRegistry().get<WallLife>(a);
				al.life = 0;
				return;
			}
		}
	}

}

void EnemyBehaviour::Update(entt::entity entity)
{
	auto& al = CurrentRegistry().get<Life>(entity);

	florp::game::Transform& pt = CurrentRegistry().get<florp::game::Transform>(entity);
	florp::game::Transform& player_t = CurrentRegistry().get<florp::game::Transform>(player);
	
	int row = 0;
	int col = 0;

	int max_col = 0;

	for (int r = 0; r < aliens.size(); r++) {
		for (int c = 0; c < aliens[r].size(); c++) {
			if (aliens[r][c] == entity) {
				row = r;
				col = c;
			}
		}
	}

	if (dir) {
		glm::vec3 translate = glm::vec3(3.0f, 0.0f, 0.0f) * florp::app::Timing::DeltaTime;
		translate = glm::mat3(pt.GetLocalTransform()) * translate;
		translate += pt.GetLocalPosition();
		pt.SetPosition(translate);
	}
	else {
		glm::vec3 translate = glm::vec3(-3.0f, 0.0f, 0.0f) * florp::app::Timing::DeltaTime;
		translate = glm::mat3(pt.GetLocalTransform()) * translate;
		translate += pt.GetLocalPosition();
		pt.SetPosition(translate);
	}


	if (cooldown <= 0.0f && 
		player_t.GetLocalPosition().x >= pt.GetLocalPosition().x - 0.1f &&
		player_t.GetLocalPosition().x <= pt.GetLocalPosition().x + 0.1f &&
		row == aliens.size() - 1) {
		using namespace florp::graphics;
		using namespace florp::game;

		MeshData bul_mesh = ObjLoader::LoadObj("Bullet.obj", glm::vec4(1.0f));
		auto* scene = SceneManager::Get("main");

		static Shader::Sptr shader = nullptr;
		if (shader == nullptr) {
			shader = std::make_shared<Shader>();
			shader->LoadPart(ShaderStageType::VertexShader, "shaders/lighting.vs.glsl");
			shader->LoadPart(ShaderStageType::FragmentShader, "shaders/forward.fs.glsl");
			shader->Link();
		}

		static Material::Sptr marbleMat = nullptr;
		if (marbleMat == nullptr) {
			marbleMat = std::make_shared<Material>(shader);
			marbleMat->Set("s_Albedo", Texture2D::LoadFromFile("marble.png", false, true, true));
		}


		auto newBul = scene->CreateEntity();
		RenderableComponent& renderable = scene->Registry().assign<RenderableComponent>(newBul);
		renderable.Mesh = MeshBuilder::Bake(bul_mesh);
		renderable.Material = marbleMat;
		Transform& t = scene->Registry().get<Transform>(newBul);

		Hitbox& h = scene->Registry().assign<Hitbox>(newBul, glm::vec3(0.1f, 0.4f, 0.1f));

		t.SetPosition(pt.GetLocalPosition() - glm::vec3(0.0f, 1.0f, 0.0f));
		scene->AddBehaviour<BulletBehaviour>(newBul, glm::vec3(0.0f, -8.0f, 0.0f));

		cooldown = COOLDOWN_TIME;
	}


	if (al.life <= 0) {
		aliens[row].erase(aliens[row].begin() + col);
		if (aliens[row].size() <= 0) {
			aliens.erase(aliens.begin() + row);
		}
		CurrentRegistry().destroy(entity);
		return;
	}
}

void WallBehaviour::Update(entt::entity entity)
{
	auto& al = CurrentRegistry().get<WallLife>(entity);

	int row = 0;

	for (int r = 0; r < walls.size(); r++) {
		for (int c = 0; c < walls[r].size(); c++) {
			if (walls[r][0] == entity) {
				row = r;
			}
		}
	}

	//WORKS, this can be commented out to make walls invincible
	if (al.life <= 0) {
		walls.erase(walls.begin() + row);
	
		CurrentRegistry().destroy(entity);
		return;
	}
}