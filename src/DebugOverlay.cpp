#include "DebugOverlay.h"
#include "imgui/imgui.h"

namespace soko
{
    void
    DrawOverlay(GameState* gameState)
    {
        const float DISTANCE = 10.0f;
        int corner = gameState->overlayCorner;
        ImGuiIO& io = ImGui::GetIO();
        if (corner != -1)
        {
            ImVec2 window_pos = ImVec2((corner & 1) ? io.DisplaySize.x - DISTANCE : DISTANCE, (corner & 2) ? io.DisplaySize.y - DISTANCE : DISTANCE);
            ImVec2 window_pos_pivot = ImVec2((corner & 1) ? 1.0f : 0.0f, (corner & 2) ? 1.0f : 0.0f);
            ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        }
        ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
        bool open = true;
        if (ImGui::Begin("Overlay", &open, (corner != -1 ? ImGuiWindowFlags_NoMove : 0) | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
        {
            char fpsBuffer[128];
            FormatString(fpsBuffer, 128, "FPS: %11i32\nUPS: % 11i32\ndT(abs):  %.4f32\ndT(game): %.4f32",
                         PlatformGlobals.fps, PlatformGlobals.ups,
                         GlobalAbsDeltaTime, GlobalGameDeltaTime);
            ImGui::Text(fpsBuffer);
            if (ImGui::BeginPopupContextWindow())
            {
                if (ImGui::MenuItem("Custom",       NULL, corner == -1)) corner = -1;
                if (ImGui::MenuItem("Top-left",     NULL, corner == 0)) corner = 0;
                if (ImGui::MenuItem("Top-right",    NULL, corner == 1)) corner = 1;
                if (ImGui::MenuItem("Bottom-left",  NULL, corner == 2)) corner = 2;
                if (ImGui::MenuItem("Bottom-right", NULL, corner == 3)) corner = 3;
                ImGui::EndPopup();
            }
        }
        ImGui::End();
        gameState->overlayCorner = corner;
    }

    void
    BeginDebugOverlay()
    {
        const float xPos = 10.0f;
        const float yPos = 10.0f;

        ImGuiIO& io = ImGui::GetIO();
        ImVec2 window_pos = ImVec2(xPos, yPos);
        ImVec2 window_pos_pivot = ImVec2(0.0f, 0.0f);
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        ImGui::SetNextWindowBgAlpha(0.5f); // Transparent background
        if (ImGui::Begin("Debug overlay", NULL,
                         //ImGuiWindowFlags_NoMove |
                         //ImGuiWindowFlags_NoDecoration |
                         ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoFocusOnAppearing |
                         ImGuiWindowFlags_NoNav))
        {
            //ImGui::Text("Debug overlay");
        }
        ImGui::End();
    }

    void
    DebugOverlayPushStr(const char* string)
    {
        if (ImGui::Begin("Debug overlay", NULL,
                         ImGuiWindowFlags_NoMove |
                         //ImGuiWindowFlags_NoDecoration |
                         ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoFocusOnAppearing |
                         ImGuiWindowFlags_NoNav))
        {
            ImGui::Separator();
            ImGui::Text(string);
        }
        ImGui::End();
    }

    void
    DebugOverlayPushVar(const char* title, v3u var)
    {
        if (ImGui::Begin("Debug overlay", NULL,
                         ImGuiWindowFlags_NoMove |
                         //ImGuiWindowFlags_NoDecoration |
                         ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoFocusOnAppearing |
                         ImGuiWindowFlags_NoNav))
        {
            ImGui::Separator();
            char buffer[128];
            FormatString(buffer, 128, "%s: x: %u32; y: %u32; z: %u32", title, var.x, var.y, var.z);
            ImGui::Text(buffer);
        }
        ImGui::End();
    }

    void
    DebugOverlayPushVar(const char* title, v3i var)
    {
        if (ImGui::Begin("Debug overlay", NULL,
                         ImGuiWindowFlags_NoMove |
                         //ImGuiWindowFlags_NoDecoration |
                         ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoFocusOnAppearing |
                         ImGuiWindowFlags_NoNav))
        {
            ImGui::Separator();
            char buffer[128];
            FormatString(buffer, 128, "%s: x: %i32; y: %i32; z: %i32", title, var.x, var.y, var.z);
            ImGui::Text(buffer);
        }
        ImGui::End();
    }


    void
    DebugOverlayPushVar(const char* title, v3 var)
    {
        if (ImGui::Begin("Debug overlay", NULL,
                         ImGuiWindowFlags_NoMove |
                         //ImGuiWindowFlags_NoDecoration |
                         ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoFocusOnAppearing |
                         ImGuiWindowFlags_NoNav))
        {
            ImGui::Separator();
            char buffer[128];
            FormatString(buffer, 128, "%s: x: %.3f32; y: %.3f32; z: %.3f32", title, var.x, var.y, var.z);
            ImGui::Text(buffer);
        }
        ImGui::End();
    }

    void
    DebugOverlayPushVar(const char* title, u32 var)
    {
        if (ImGui::Begin("Debug overlay", NULL,
                         ImGuiWindowFlags_NoMove |
                         //ImGuiWindowFlags_NoDecoration |
                         ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoFocusOnAppearing |
                         ImGuiWindowFlags_NoNav))
        {
            ImGui::Separator();
            char buffer[128];
            FormatString(buffer, 128, "%s: x: %u32", title, var);
            ImGui::Text(buffer);
        }
        ImGui::End();
    }


    void
    DebugOverlayPushSlider(const char* title, v3* var, f32 min, f32 max)
    {
        if (ImGui::Begin("Debug overlay", NULL,
                         ImGuiWindowFlags_NoMove |
                         //ImGuiWindowFlags_NoDecoration |
                         ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoFocusOnAppearing |
                         ImGuiWindowFlags_NoNav))
        {
            ImGui::Separator();
            ImGui::SliderFloat3(title, var->data, min, max);
        }
        ImGui::End();
    }

}
