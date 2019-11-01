#pragma once
#include "Platform.h"

namespace soko
{
    internal void DrawOverlay(GameState* gameState);
    internal void BeginDebugOverlay();
    inline bool DebugOverlayBeginCustom();
    inline void DebugOverlayEndCustom();
    internal void DebugOverlayPushStr(const char* string);
    internal void DebugOverlayPushVar(const char* title, uv3 var);
    internal void DebugOverlayPushVar(const char* title, iv3 var);
    internal void DebugOverlayPushVar(const char* title, v3 var);
    internal void DebugOverlayPushVar(const char* title, v4 var);
    internal void DebugOverlayPushVar(const char* title, u32 var);
    internal void DebugOverlayPushVar(const char* title, f32 var);
    internal void DebugOverlayPushSlider(const char* title, f32* var, f32 min, f32 max);
    internal void DebugOverlayPushSlider(const char* title, v3* var, f32 min, f32 max);
    internal void DebugOverlayPushSlider(const char* title, v4* var, f32 min, f32 max);
}

#define DEBUG_OVERLAY_TRACE(var) soko::DebugOverlayPushVar(#var, var)
#define DEBUG_OVERLAY_SLIDER(var, min, max) soko::DebugOverlayPushSlider(#var, &var, min, max)
