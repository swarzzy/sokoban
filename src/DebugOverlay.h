#pragma once
#include "Platform.h"

namespace soko
{
	void DrawOverlay(GameState* gameState);
	void BeginDebugOverlay();
	void DebugOverlayPushStr(const char* string);
	void DebugOverlayPushVar(const char* title, v3u var);
	void DebugOverlayPushVar(const char* title, v3i var);
	void DebugOverlayPushVar(const char* title, v3 var);
	void DebugOverlayPushSlider(const char* title, v3* var, f32 min, f32 max);
}

#define DEBUG_OVERLAY_TRACE(var) soko::DebugOverlayPushVar(#var, var)
#define DEBUG_OVERLAY_SLIDER(var, min, max) soko::DebugOverlayPushSlider(#var, &var, min, max)

