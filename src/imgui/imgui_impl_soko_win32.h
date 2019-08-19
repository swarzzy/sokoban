#pragma once
#include "Win32Platform.h"

namespace soko::ImGui
{
	void ImplSokoWin32_Init(AB::Application* application);
	bool ImplSokoWin32_UpdateMouseCursor();
	void ImplSokoWin32_NewFrame(AB::Application* application);
}
