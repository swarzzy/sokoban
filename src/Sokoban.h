#pragma once
#include "Platform.h"
#include "Renderer.h"


namespace soko
{
	struct GameState
	{
		Renderer* renderer;
		RenderGroup* renderGroup;
	};
}
