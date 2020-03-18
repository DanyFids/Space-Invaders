#include "ControlBehaviour.h"
#include "florp/app/Window.h"
#include "florp/app/Application.h"
#include "florp/game/Transform.h"
#include "florp/game/SceneManager.h"
#include "florp/app/Timing.h"
#include "florp/graphics/Texture2D.h"


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

	translate *= Timing::DeltaTime * mySpeed;

	if (glm::length(translate) > 0) {
		translate = glm::mat3(transform.GetLocalTransform()) * translate;
		translate += transform.GetLocalPosition();
		transform.SetPosition(translate);
	}
	//// Update Audio Engine
	AudioEngine::Update();
}

void ControlBehaviour::PlayerShoot(entt::entity player)
{
	using namespace florp::graphics;
	using namespace florp::game;

	MeshData bul_mesh = ObjLoader::LoadObj("monkey.obj", glm::vec4(1.0f));
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

	t.SetPosition(pt.GetLocalPosition() + glm::vec3(0.0f, 0.5f, 0.0f));
	scene->AddBehaviour<BulletBehaviour>(newBul, glm::vec3(0.0f, 8.0f, 0.0f));
}

void BulletBehaviour::Update(entt::entity entity)
{
	auto& transform = CurrentRegistry().get<florp::game::Transform>(entity);

	glm::vec3 translate = mySpeed * florp::app::Timing::DeltaTime;

	translate = glm::mat3(transform.GetLocalTransform()) * translate;
	translate += transform.GetLocalPosition();
	transform.SetPosition(translate);

	lifetime-= florp::app::Timing::DeltaTime;

	if (lifetime <= 0.0f) {
		CurrentRegistry().destroy(entity);
	}
}
