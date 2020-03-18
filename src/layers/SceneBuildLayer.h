#pragma once
#include "florp/app/ApplicationLayer.h"
#include <AudioEngine.h>

class SceneBuilder : public florp::app::ApplicationLayer {
public:
	void InitSound();
	void ShutdownSound();
	void Initialize() override;

	AudioEngine audioEngine;
};
