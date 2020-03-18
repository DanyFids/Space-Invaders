#include "SceneBuildLayer.h"
#include "florp/game/SceneManager.h"
#include "florp/game/RenderableComponent.h"
#include <florp\graphics\MeshData.h>
#include <florp\graphics\MeshBuilder.h>
#include <florp\graphics\ObjLoader.h>

#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <florp\game\Transform.h>
#include "RotateBehaviour.h"
#include "CameraComponent.h"
#include "florp/app/Application.h"
#include <ControlBehaviour.h>
#include <ShadowLight.h>
#include "PointLightComponent.h"
#include "LightFlickerBehaviour.h"

/*
 * Helper function for creating a shadow casting light
 * @param scene The scene to create the light in
 * @param entityOut An optional pointer to receive the ENTT entity that was created (set to nullptr if you don't care)
 * @param pos The position of the light in world space
 * @param target The point for the light to look at, in world space
 * @param up A unit vector indicating what axis is considered 'up'
 * @param distance The far clipping plane of the light
 * @param fov The field of view of the light, in degrees
 * @param bufferSize The size of the buffer to create for the light (default 1024x1024)
 * @param name The name to associate with the light's buffer
 */
ShadowLight& CreateShadowCaster(florp::game::Scene* scene, entt::entity* entityOut, glm::vec3 pos, glm::vec3 target, glm::vec3 up, float distance = 10.0f, float fov = 60.0f, glm::ivec2 bufferSize = { 1024, 1024 }, const char* name = nullptr)
{
	// The depth attachment is a texture, with 32 bits for depth
	RenderBufferDesc depth = RenderBufferDesc();
	depth.ShaderReadable = true;
	depth.Attachment = RenderTargetAttachment::Depth;
	depth.Format = RenderTargetType::Depth32;

	// Our shadow buffer is depth-only
	FrameBuffer::Sptr shadowBuffer = std::make_shared<FrameBuffer>(bufferSize.x, bufferSize.y);
	shadowBuffer->AddAttachment(depth);
	shadowBuffer->Validate();
	if (name != nullptr)
		shadowBuffer->SetDebugName(name);

	// Create a new entity
	entt::entity entity = scene->CreateEntity();

	// Assign and initialize a shadow light component
	ShadowLight& light = scene->Registry().assign<ShadowLight>(entity);
	light.ShadowBuffer = shadowBuffer;
	light.Projection = glm::perspective(glm::radians(fov), (float)bufferSize.x / (float)bufferSize.y, 0.25f, distance);
	light.Attenuation = 1.0f / distance;
	light.Color = glm::vec3(1.0f);

	// Assign and initialize the transformation
	florp::game::Transform& t = scene->Registry().get<florp::game::Transform>(entity);
	t.SetPosition(pos);
	t.LookAt(target, up);

	// Send out the entity ID if we passed in a place to store it
	if (entityOut != nullptr)
		*entityOut = entity;

	return light;
}

florp::graphics::Texture2D::Sptr CreateSolidTexture(glm::vec4 color)
{
	using namespace florp::graphics;
	static std::unordered_map<glm::vec4, Texture2D::Sptr> cache;

	// If a texture for that color exists in the cache, return it
	if (cache.find(color) != cache.end())
		return cache[color];
	// Otherwise, we'll create a new texture, cache it, then return it
	else {
		// We'll disable essentially anything fancy for our single-pixel color
		Texture2dDescription desc = Texture2dDescription();
		desc.Width = desc.Height = 1;
		desc.Format = InternalFormat::RGBA8;
		desc.MagFilter = MagFilter::Nearest;
		desc.MinFilter = MinFilter::Nearest;
		desc.MipmapLevels = 1;
		desc.WrapS = desc.WrapT = WrapMode::ClampToEdge;

		// By using the float pixel type, we can simply feed in the address of our color
		Texture2dData data = Texture2dData();
		data.Width = data.Height = 1;
		data.Format = PixelFormat::Rgba;
		data.Type = PixelType::Float;
		data.Data = &color.r;

		// Create the texture, and load the single pixel data
		Texture2D::Sptr result = std::make_shared<Texture2D>(desc);
		result->SetData(data);

		// Store in the cache
		cache[color] = result;
		
		return result;
	}
}

void SceneBuilder::InitSound()
{
	//// Init AudioEngine (Don't forget to shut down and update)
	audioEngine.Init();

	//// Load a bank (Use the flag FMOD_STUDIO_LOAD_BANK_NORMAL)
	audioEngine.LoadBank("Master", FMOD_STUDIO_LOAD_BANK_NORMAL);

	//// Load an event
	audioEngine.LoadEvent("Bullet", "{5eebb49c-e17f-447c-90b0-ca24a4ddcb08}");

	//// Play the event
	audioEngine.PlayEvent("Bullet");

	////Vectors for positioning the 3D sound
	glm::vec3 startPosition = { 0,0,0 };
	//// Set initial position  
	audioEngine.SetEventPosition("Bullet", startPosition);



	//For use in an update function
	//if (TTK::Input::GetKeyPressed(TTK::KeyCode::Space)) {
	//  //// Play the event
	//  audioEngine.PlayEvent("Bullet");
	//}


	//Stops sound
	//audioEngine.StopEvent("Bullet");

	//Update Audio
	audioEngine.Update();
}

void SceneBuilder::ShutdownSound()
{
	audioEngine.Shutdown();
}

void SceneBuilder::Initialize()
{
	florp::app::Application* app = florp::app::Application::Get();
	
	using namespace florp::game;
	using namespace florp::graphics;
	
	auto* scene = SceneManager::RegisterScene("main");
	SceneManager::SetCurrentScene("main");

	// We'll load in a Ship head to render something interesting
	MeshData data = ObjLoader::LoadObj("Ship.obj", glm::vec4(1.0f));
	MeshData dataA = ObjLoader::LoadObj("Alien.obj", glm::vec4(1.0f));
	MeshData dataW = ObjLoader::LoadObj("Wall.obj", glm::vec4(1.0f));
	MeshData dataWH = ObjLoader::LoadObj("WallHit.obj", glm::vec4(1.0f));
	MeshData dataB = ObjLoader::LoadObj("Bullet.obj", glm::vec4(1.0f));

	Shader::Sptr shader = std::make_shared<Shader>();
	shader->LoadPart(ShaderStageType::VertexShader, "shaders/lighting.vs.glsl");
	shader->LoadPart(ShaderStageType::FragmentShader, "shaders/forward.fs.glsl");
	shader->Link();

	// This is our emissive lighting shader
	Shader::Sptr emissiveShader = std::make_shared<Shader>();
	emissiveShader->LoadPart(ShaderStageType::VertexShader, "shaders/lighting.vs.glsl");
	emissiveShader->LoadPart(ShaderStageType::FragmentShader, "shaders/forward-emissive.fs.glsl");
	emissiveShader->Link();

	// Creating our 'console'
	{
		// Material for the frame
		Material::Sptr consoleMat = std::make_shared<Material>(emissiveShader);
		consoleMat->Set("s_Albedo", Texture2D::LoadFromFile("console_albedo.png", false, true, true));
		consoleMat->Set("s_Emissive", Texture2D::LoadFromFile("console_emissive.png", false, true, true));
		consoleMat->Set("a_EmissiveStrength", 4.0f); 

		// Load the mesh for the frame
		MeshData console = ObjLoader::LoadObj("console.obj", glm::vec4(1.0f));

		// Create the entity, attach renderable, set position
		entt::entity consoleEntt = scene->CreateEntity();
		RenderableComponent& consoleRenderable = scene->Registry().assign<RenderableComponent>(consoleEntt);
		consoleRenderable.Mesh = MeshBuilder::Bake(console);
		consoleRenderable.Material = consoleMat;
		Transform& t = scene->Registry().get<Transform>(consoleEntt);
		t.SetPosition(glm::vec3(0.0f, -1.0f, -4.0f));

		// We'll use a scrolling shader to make our screen more interesting
		Shader::Sptr scrollShader = std::make_shared<Shader>();
		scrollShader->LoadPart(ShaderStageType::VertexShader, "shaders/lighting.vs.glsl");
		scrollShader->LoadPart(ShaderStageType::FragmentShader, "shaders/forward-scrolling.fs.glsl");
		scrollShader->Link(); 

		// We'll load in an image for our screen, and set it to be fairly strongly emmisive, and scroll along the y axis
		Material::Sptr monitorMat = std::make_shared<Material>(scrollShader);
		monitorMat->Set("s_Albedo", Texture2D::LoadFromFile("matrix.png", false, true, true));
		monitorMat->Set("s_Emissive", Texture2D::LoadFromFile("matrix.png", false, true, true));
		monitorMat->Set("a_EmissiveStrength", 2.0f);
		monitorMat->Set("a_ScrollDir", glm::vec2(0.0f, 1.0f));

		// We'll make a plane out of a flattened cube
		MeshData data = MeshBuilder::Begin("Monitor");
		MeshBuilder::AddAlignedCube(data, glm::vec3(0.0f), glm::vec3(0.41f, 0.36f, 0.0f));
		Mesh::Sptr mesh = MeshBuilder::Bake(data);

		// Attach a new entity to the console, give it a renderable, and position it juust right
		entt::entity monitor = scene->CreateEntity();
		RenderableComponent& renderable = scene->Registry().assign<RenderableComponent>(monitor);
		renderable.Mesh = mesh;
		renderable.Material = monitorMat;
		Transform& t2 = scene->Registry().get<Transform>(monitor);
		t2.SetParent(consoleEntt);
		t2.SetPosition(glm::vec3(0.0f, 1.45f, -0.14f));
		t2.SetEulerAngles(glm::vec3(-45.0f, 0.0f, 0.0f));
	}
		   	
	// Load and set up our simple test material
	Material::Sptr monkeyMat = std::make_shared<Material>(emissiveShader);
	monkeyMat->Set("s_Albedo", Texture2D::LoadFromFile("marble.png", false, true, true));
	monkeyMat->Set("s_Emissive", Texture2D::LoadFromFile("monkey_emissive.png", false, true, true)); 
	monkeyMat->Set("a_EmissiveStrength", 4.0f);

	// We'll have another material for the marble without any emissive spots
	Material::Sptr marbleMat = std::make_shared<Material>(shader);
	marbleMat->Set("s_Albedo", Texture2D::LoadFromFile("marble.png", false, true, true));

	// This will be for the polka-cube
	Material::Sptr mat2 = std::make_shared<Material>(emissiveShader);
	mat2->Set("s_Albedo", Texture2D::LoadFromFile("polka.png", false, true, true));
	mat2->Set("s_Emissive", Texture2D::LoadFromFile("polka.png", false, true, true));
	mat2->Set("a_EmissiveStrength", 1.0f);

	// PLAYER
	{
		entt::entity player = scene->CreateEntity();
		RenderableComponent& renderable = scene->Registry().assign<RenderableComponent>(player);
		renderable.Mesh = MeshBuilder::Bake(data);
		renderable.Material = monkeyMat;
		Transform& t = scene->Registry().get<Transform>(player);
		t.SetEulerAngles({0.f, 0.0f, 0.0f});
		// Make our monkeys spin around the center
		scene->AddBehaviour<ControlBehaviour>(player, glm::vec3(2.0f, 0.0f, 0.0f));
	}
	

	// We'll use a tiny cube to cast a shadow from our camera, and to indicate where the light sources are
	MeshData indicatorCube = MeshBuilder::Begin();
	MeshBuilder::AddAlignedCube(indicatorCube, glm::vec3(0.0f, 0, 0.0), glm::vec3(0.1f, 0.1f, 0.1f));
	Mesh::Sptr indicatorMesh = MeshBuilder::Bake(indicatorCube);
		
	// Creates our main camera
	{
		// The color buffer should be marked as shader readable, so that we generate a texture for it
		RenderBufferDesc mainColor = RenderBufferDesc();
		mainColor.ShaderReadable = true;
		mainColor.Attachment = RenderTargetAttachment::Color0;
		mainColor.Format = RenderTargetType::ColorRgb8;

		// The normal buffer
		RenderBufferDesc normalBuffer = RenderBufferDesc();
		normalBuffer.ShaderReadable = true;
		normalBuffer.Attachment = RenderTargetAttachment::Color1;
		normalBuffer.Format = RenderTargetType::ColorRgb10; // Note: this format is 10 bits per component

		// The normal buffer
		RenderBufferDesc emissiveBuffer = RenderBufferDesc();
		emissiveBuffer.ShaderReadable = true;
		emissiveBuffer.Attachment = RenderTargetAttachment::Color2;
		emissiveBuffer.Format = RenderTargetType::ColorRgb10; // Note: this format is 10 bits per component

		// The depth attachment does not need to be a texture (and would cause issues since the format is DepthStencil)
		RenderBufferDesc depth = RenderBufferDesc();
		depth.ShaderReadable = true;
		depth.Attachment = RenderTargetAttachment::Depth;
		depth.Format = RenderTargetType::Depth32;

		// Our main frame buffer needs a color output, and a depth output
		FrameBuffer::Sptr buffer = std::make_shared<FrameBuffer>(app->GetWindow()->GetWidth(), app->GetWindow()->GetHeight(), 4);
		buffer->AddAttachment(mainColor);
		buffer->AddAttachment(normalBuffer);
		buffer->AddAttachment(emissiveBuffer);
		buffer->AddAttachment(depth);
		buffer->Validate();
		buffer->SetDebugName("MainBuffer");

		// We'll create an entity, and attach a camera component to it
		entt::entity camera = scene->CreateEntity();
		CameraComponent& cam = scene->Registry().assign<CameraComponent>(camera);
		cam.BackBuffer = buffer;
		cam.FrontBuffer = buffer->Clone();
		cam.IsMainCamera = true;
		cam.Projection = glm::perspective(glm::radians(60.0f), 1.0f, 0.1f, 1000.0f);

		// We'll add our control behaviour so that we can fly the camera around
		//scene->AddBehaviour<ControlBehaviour>(camera, glm::vec3(1.0f));

		auto& camTransform = scene->Registry().get<Transform>(camera);
		camTransform.SetPosition(glm::vec3(0.0f, 5.0f, 15.0f));
		camTransform.LookAt(glm::vec3(0, 5, 0), glm::vec3(0, 1, 0));

		// We'll attach a cube to the camera so that it casts shadows
		RenderableComponent& renderable = scene->Registry().assign<RenderableComponent>(camera);
		renderable.Mesh = indicatorMesh;
		renderable.Material = marbleMat;

	}

	// We'll create a projector to cast our smile on the floor
	entt::entity lightEnt = entt::null;
	auto& light = CreateShadowCaster(scene, &lightEnt, glm::vec3(0.0f, 15.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), 25.0f, 90.0f, { 2048,2048 });
	light.Color = glm::vec3(1.0f, 1.0f, 1.0f) * 0.5f;
	light.Attenuation = 1.0f / 15.0f; 
	//light.ProjectorImage = Texture2D::LoadFromFile("light_projection.png", false, false, true);
	//scene->AddBehaviour<LightFlickerBehaviour>(lightEnt, 10.0f);
	
			
	// Our floor plane
	{
		// Building the mesh
		MeshData data = MeshBuilder::Begin();
		MeshBuilder::AddAlignedCube(data, glm::vec3(0.0f, -1.0f, 0.0), glm::vec3(100.0f, 0.1f, 100.0f));
		Mesh::Sptr mesh = MeshBuilder::Bake(data);

		// Creating the entity and attaching the renderable 
		entt::entity entity = scene->CreateEntity();
		RenderableComponent& renderable = scene->Registry().assign<RenderableComponent>(entity);
		renderable.Mesh = MeshBuilder::Bake(data);
		renderable.Material = marbleMat;
	}
}
