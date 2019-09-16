#include "GameMenu.h"

namespace soko
{


    Level* InitializeLevel(const wchar_t* filename, GameState* gameState)
    {
        BeginTemporaryMemory(gameState->tempArena);
        gameState->level = LoadLevel(L"testLevel.aab", gameState->memoryArena, gameState->tempArena);
        EndTemporaryMemory(gameState->tempArena);

        auto level = gameState->level;

        Entity entity1 = {};
        entity1.type = ENTITY_TYPE_BLOCK;
        entity1.flags = ENTITY_FLAG_COLLIDES | ENTITY_FLAG_MOVABLE;
        entity1.coord = V3I(5, 7, 1);
        entity1.mesh = &gameState->cubeMesh;
        entity1.material = &gameState->tileBlockMaterial;

        AddEntity(level, entity1, gameState->memoryArena);
        //AddEntity(playerLevel)

        Entity entity2 = {};
        entity2.type = ENTITY_TYPE_BLOCK;
        entity2.flags = ENTITY_FLAG_COLLIDES | ENTITY_FLAG_MOVABLE;
        entity2.coord = V3I(5, 8, 1);
        entity2.mesh = &gameState->cubeMesh;
        entity2.material = &gameState->tileBlockMaterial;

        AddEntity(level, entity2, gameState->memoryArena);

        Entity entity3 = {};
        entity3.type = ENTITY_TYPE_BLOCK;
        entity3.flags = ENTITY_FLAG_COLLIDES | ENTITY_FLAG_MOVABLE;
        entity3.coord = V3I(5, 9, 1);
        entity3.mesh = &gameState->cubeMesh;
        entity3.material = &gameState->tileBlockMaterial;

        AddEntity(level, entity3, gameState->memoryArena);

        Entity plate = {};
        plate.type = ENTITY_TYPE_PLATE;
        plate.flags = 0;
        plate.coord = V3I(10, 9, 1);
        plate.mesh = &gameState->plateMesh;
        plate.material = &gameState->redPlateMaterial;

        AddEntity(level, plate, gameState->memoryArena);

        Entity portal1 = {};
        portal1.type = ENTITY_TYPE_PORTAL;
        portal1.flags = 0;
        portal1.coord = V3I(12, 12, 1);
        portal1.mesh = &gameState->portalMesh;
        portal1.material = &gameState->portalMaterial;
        portal1.portalDirection = DIRECTION_NORTH;

        Entity* portal1Entity = GetEntity(level, AddEntity(level, portal1, gameState->memoryArena));

        Entity portal2 = {};
        portal2.type = ENTITY_TYPE_PORTAL;
        portal2.flags = 0;
        portal2.coord = V3I(17, 17, 1);
        portal2.mesh = &gameState->portalMesh;
        portal2.material = &gameState->portalMaterial;
        portal2.portalDirection = DIRECTION_WEST;

        Entity* portal2Entity = GetEntity(level, AddEntity(level, portal2, gameState->memoryArena));

        portal1Entity->bindedPortalID = portal2Entity->id;
        portal2Entity->bindedPortalID = portal1Entity->id;

        AddEntity(level, ENTITY_TYPE_SPIKES, V3I(15, 15, 1),
                  &gameState->spikesMesh, &gameState->spikesMaterial, gameState->memoryArena);
        Entity* button = GetEntity(level, AddEntity(level, ENTITY_TYPE_BUTTON, V3I(4, 4, 1),
                                                    &gameState->buttonMesh, &gameState->buttonMaterial,
                                                    gameState->memoryArena));
        // TODO(emacs): Lambdas indenting
        button->updateProc = [](Level* level, Entity* entity, void* data) {
            GameState* gameState = (GameState*)data;
            AddEntity(level, ENTITY_TYPE_BLOCK, V3I(4, 5, 1),
                      &gameState->cubeMesh, &gameState->tileBlockMaterial,
                      gameState->memoryArena);
        };
        button->updateProcData = (void*)gameState;
        return gameState->level;
    }

    void ShowNetSettings(GameState* gameState, AB::MemoryArena* arena)
    {
        bool open = true;
        bool windowCreated = false;
        ImGui::SetNextWindowSizeConstraints(ImVec2(400, 100), ImVec2(400, 200));
        if (ImGui::Begin("LAN", &open, ImGuiWindowFlags_AlwaysAutoResize))
        {
            windowCreated = true;
            if(ImGui::RadioButton("Client", gameState->gameMode == GAME_MODE_CLIENT)) { gameState->gameMode = GAME_MODE_CLIENT; }
            ImGui::SameLine();
            if(ImGui::RadioButton("Server", gameState->gameMode == GAME_MODE_SERVER)) { gameState->gameMode = GAME_MODE_SERVER; }
            ImGui::SameLine();
            if(ImGui::RadioButton("Single", gameState->gameMode == GAME_MODE_SINGLE)) { gameState->gameMode = GAME_MODE_SINGLE; }

            ImGui::Separator();
            if (gameState->gameMode == GAME_MODE_SERVER)
            {
                // TODO(emacs): Lambda indentiation in emacs
                auto numFilter = [](ImGuiInputTextCallbackData* data) -> int
                    {
                        return !(data->EventChar >= '0' && data->EventChar <='9');
                    };

                char buf[64] = "";
                FormatString(buf, 64, "%i16", gameState->port);
                ImGui::InputText("port", buf, 64, ImGuiInputTextFlags_CallbackCharFilter, numFilter);
                u64 val = strtol(buf, nullptr, 10);
                bool buttonEnabled = false;
                if (val > 1023 && val < 65536)
                {
                    gameState->port = (u16)val;
                }
                if (gameState->port > 1023 && gameState->port < 65535)
                {
                    buttonEnabled = true;
                }

                if(ImGui::Button("Create", ImVec2(50.0f, 20.0f)))
                {
                    if (buttonEnabled)
                    {
                        gameState->gameModeReadyToInit = true;
                    }
                }
            }
            else if (gameState->gameMode == GAME_MODE_CLIENT)
            {
                float width = ImGui::CalcItemWidth();
                ImGui::BeginGroup();
                ImGui::PushID("IP");
                ImGui::TextUnformatted("IP");
                ImGui::SameLine();
                bool inputSucceed = true;
                for (u32 i = 0; i < 4; i++)
                {
                    ImGui::PushItemWidth(width / 4.0f);
                    ImGui::PushID(i);
                    bool invalidOctet = false;
                    if (gameState->ipOctets[i] > 255)
                    {
                        // Make values over 255 red, and when focus is lost reset it to 255.
                        gameState->ipOctets[i] = 255;
                        invalidOctet = true;
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                    }
                    if (gameState->ipOctets[i] < 0)
                    {
                        // Make values below 0 yellow, and when focus is lost reset it to 0.
                        gameState->ipOctets[i] = 0;
                        invalidOctet = true;
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
                    }
                    ImGui::InputInt("##v", (int*)&gameState->ipOctets[i], 0, 0, ImGuiInputTextFlags_CharsDecimal);
                    if (invalidOctet)
                    {
                        if (inputSucceed)
                        {
                            inputSucceed = false;
                        }
                        ImGui::PopStyleColor();
                    }
                    ImGui::SameLine();


                    ImGui::PopID();
                    ImGui::PopItemWidth();
                }
                if (inputSucceed)
                {
                    gameState->ipAddress =
                        (gameState->ipOctets[0] << 24) |
                        (gameState->ipOctets[1] << 16) |
                        (gameState->ipOctets[2] << 8) |
                        (gameState->ipOctets[3]);
                }
                // TODO(emacs): Lambda indentiation in emacs
                auto numFilter = [](ImGuiInputTextCallbackData* data) -> int
                    {
                        return !(data->EventChar >= '0' && data->EventChar <='9');
                    };


                ImGui::Text(":");
                ImGui::SameLine();
                ImGui::PushItemWidth(width / 4.0f);
                char buf[64] = "";
                FormatString(buf, 64, "%i16", gameState->port);
                ImGui::InputText("", buf, 64, ImGuiInputTextFlags_CallbackCharFilter, numFilter);
                u64 val = strtol(buf, nullptr, 10);
                bool buttonEnabled = false;
                if (val > 1023 && val < 65536)
                {
                    gameState->port = (u16)val;
                }
                if (gameState->port > 1023 && gameState->port < 65535 && inputSucceed)
                {
                    buttonEnabled = true;
                }

                ImGui::PopID();
                ImGui::EndGroup();


                if(ImGui::Button("Create", ImVec2(50.0f, 20.0f)))
                {
                    if (buttonEnabled)
                    {
                        gameState->gameModeReadyToInit = true;
                    }
                }
            }
            else if (gameState->gameMode == GAME_MODE_SINGLE)
            {
                if(ImGui::Button("Create", ImVec2(50.0f, 20.0f)))
                {
                    gameState->gameModeReadyToInit = true;
                }
            }

            if (gameState->gameModeReadyToInit && !gameState->gameModeInitialized)
            {
                uptr socket = NetCreateSocket();
                SOKO_ASSERT(socket);

                if (gameState->gameMode == GAME_MODE_SERVER)
                {
                    bool result = net::InitializeServer(gameState, arena, gameState->port);
                    SOKO_ASSERT(result);
                    gameState->controlledPlayer = AddPlayer(gameState, V3I(10, 10, 1), arena);
                    SOKO_ASSERT(gameState->controlledPlayer);
                    result = net::ServerAddPlayer(gameState->server,
                                                  gameState->controlledPlayer,
                                                  net::Server::LOCAL_PLAYER_SLOT, {});
                    SOKO_ASSERT(result);
                }
                else
                {
                    SOKO_ASSERT(!gameState->client);
                    gameState->client = PUSH_STRUCT(arena, net::Client);
                    SOKO_ASSERT(gameState->client);
                    gameState->client->socket = socket;
                    gameState->client->serverAddr.ip = gameState->ipAddress;
                    gameState->client->serverAddr.port = gameState->port;
                }

                gameState->gameModeReadyToInit = false;
                gameState->gameModeInitialized = true;
            }
            if (gameState->gameModeInitialized && gameState->gameMode == GAME_MODE_CLIENT)
            {
                if(ImGui::Button("Disconnect", ImVec2(50.0f, 20.0f)))
                {
                    gameState->shouldDisconnect = true;
                }
            }
        }

        ImGui::End();
    }

    void MenuUpdateAndRender(GameMenu* menu, GameState* gameState)
    {
        ImGui::SetNextWindowPos(ImVec2(.0f, .0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2((f32)PlatformGlobals.windowWidth + 2.0f , (f32)PlatformGlobals.windowHeight + 2.0f), ImGuiCond_Always);
        bool show = true;
        ImGui::Begin("Main menu", &show,
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoDecoration);

        ImGui::Separator();
        if (!menu->levelLoaded)
        {
            ImGui::Text("Load level:");
            ImGui::Separator();
            ImGui::Text("Path:");
            ImGui::PushID("level path input");
            ImGui::InputText("", menu->levelPathBuffer, GameMenu::LEVEL_PATH_BUFFER_SIZE);
            ImGui::PopID();
            if (ImGui::Button("Load", ImVec2(60, 20)))
            {
                // TODO: Store and retirieve from imgui as wchar
                wchar_t buffer[GameMenu::LEVEL_PATH_BUFFER_SIZE];
                mbstowcs(buffer, menu->levelPathBuffer, GameMenu::LEVEL_PATH_BUFFER_SIZE);
                if (DebugGetFileSize(buffer))
                {
                    InitializeLevel(buffer, gameState);
                    menu->levelLoaded = true;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Create new", ImVec2(100, 20)))
            {
            }
            ImGui::Separator();
        }
        else
        {
            ShowNetSettings(gameState, gameState->memoryArena);
        }
        ImGui::End();
    }

}
