#include "AudioLayer.h"
#include "AudioEngine.h"

void AudioLayer::Initialize()
{
	AudioEngine::Init();
	AudioEngine::LoadBank("MasterB", FMOD_STUDIO_LOAD_BANK_NORMAL);
	AudioEngine::LoadEvent("Bullet", "{5eebb49c-e17f-447c-90b0-ca24a4ddcb08}");
	glm::vec3 startPosition = { 0,0,0 };
	//// Set initial position  
	AudioEngine::SetEventPosition("Bullet", startPosition);
}

void AudioLayer::Shutdown()
{
	AudioEngine::Shutdown();
}

void AudioLayer::Update()
{
	AudioEngine::Update();
}
