#pragma once

#include "../engine/ecs/registry.h"

namespace P3D::Application {

struct Frames {
	
	static void onInit(Engine::Registry64& registry);
	static void onRender(Engine::Registry64& registry);

};

};