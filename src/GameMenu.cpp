namespace soko
{
    internal Level*
    InitializeLevel(const wchar_t* filename, AB::MemoryArena* levelArena, GameState* gameState)
    {
        BeginTemporaryMemory(gameState->tempArena);
        Level* level = LoadLevel(filename, levelArena, gameState->tempArena);
        EndTemporaryMemory(gameState->tempArena);
        if (level)
        {
            Entity entity1 = {};
            entity1.type = ENTITY_TYPE_BLOCK;
            entity1.flags = ENTITY_FLAG_COLLIDES | ENTITY_FLAG_MOVABLE;
            entity1.coord = V3I(5, 7, 1);
            entity1.mesh = EntityMesh_Cube;
            entity1.material = EntityMaterial_Block;

            AddEntity(level, entity1, gameState->memoryArena);
            //AddEntity(playerLevel)

            Entity entity2 = {};
            entity2.type = ENTITY_TYPE_BLOCK;
            entity2.flags = ENTITY_FLAG_COLLIDES | ENTITY_FLAG_MOVABLE;
            entity2.coord = V3I(5, 8, 1);
            entity2.mesh = EntityMesh_Cube;
            entity2.material = EntityMaterial_Block;

            AddEntity(level, entity2, gameState->memoryArena);

            Entity entity3 = {};
            entity3.type = ENTITY_TYPE_BLOCK;
            entity3.flags = ENTITY_FLAG_COLLIDES | ENTITY_FLAG_MOVABLE;
            entity3.coord = V3I(5, 9, 1);
            entity3.mesh = EntityMesh_Cube;
            entity3.material = EntityMaterial_Block;

            AddEntity(level, entity3, gameState->memoryArena);

            Entity plate = {};
            plate.type = ENTITY_TYPE_PLATE;
            plate.flags = 0;
            plate.coord = V3I(10, 9, 1);
            plate.mesh = EntityMesh_Plate;
            plate.material = EntityMaterial_RedPlate;

            AddEntity(level, plate, gameState->memoryArena);

            Entity portal1 = {};
            portal1.type = ENTITY_TYPE_PORTAL;
            portal1.flags = 0;
            portal1.coord = V3I(12, 12, 1);
            portal1.mesh = EntityMesh_Portal;
            portal1.material = EntityMaterial_Portal;
            portal1.portalDirection = DIRECTION_NORTH;

            Entity* portal1Entity = GetEntity(level, AddEntity(level, portal1, gameState->memoryArena));

            Entity portal2 = {};
            portal2.type = ENTITY_TYPE_PORTAL;
            portal2.flags = 0;
            portal2.coord = V3I(17, 17, 1);
            portal2.mesh = EntityMesh_Portal;
            portal2.material = EntityMaterial_Portal;
            portal2.portalDirection = DIRECTION_WEST;

            Entity* portal2Entity = GetEntity(level, AddEntity(level, portal2, gameState->memoryArena));

            portal1Entity->bindedPortalID = portal2Entity->id;
            portal2Entity->bindedPortalID = portal1Entity->id;

            AddEntity(level, ENTITY_TYPE_SPIKES, V3I(15, 15, 1),
                      EntityMesh_Spikes, EntityMaterial_Spikes, gameState->memoryArena);
            Entity* button = GetEntity(level, AddEntity(level, ENTITY_TYPE_BUTTON, V3I(4, 4, 1),
                                                        EntityMesh_Button, EntityMaterial_Button,

                                                        gameState->memoryArena));
            // TODO: Entity custom behavior
#if 0
            button->updateProc = [](Level* level, Entity* entity, void* data) {
                GameState* gameState = (GameState*)data;
                AddEntity(level, ENTITY_TYPE_BLOCK, V3I(4, 5, 1),
                          EntityMesh_Cube, EntityMaterial_Block,
                          gameState->memoryArena);
            };
            button->updateProcData = (void*)gameState;
#endif
        }

        return level;
    }

    enum MenuCleanupFlags : u32
    {
        MenuCleanup_Common = 0x1,
        MenuCleanup_NetSpecific = 0x2
    };

    internal void
    MenuCleanup(GameMenu* menu, MainMenuState nextState, u32 flags)
    {
        if (flags & MenuCleanup_NetSpecific)
        {
            if (menu->session.gameMode = GAME_MODE_SERVER)
            {
                if (menu->session.server)
                {
                    bool result = NetCloseSocket(menu->session.server->socket);
                    SOKO_ASSERT(result);
                }
            }
            else if (menu->session.gameMode = GAME_MODE_CLIENT)
            {
                if (menu->session.client)
                {
                    bool result = NetCloseSocket(menu->session.client->socket);
                    SOKO_ASSERT(result);
                }
            }
            else
            {
                INVALID_CODE_PATH;
            }


            if (menu->session.sessionArena)
            {
                PLATFORM_FREE_ARENA(menu->session.sessionArena);
                menu->session.client = 0;
            }

        }

        if (flags & MenuCleanup_Common)
        {
            ZERO_FROM_MEMBER(GameMenu, levelPathBuffer, menu);
        }

        menu->state = nextState;
    }

    internal void
    MenuModeSelection(GameMenu* menu)
    {
        if(ImGui::Button("Single", ImVec2(100, 60))) { menu->state = MainMenu_SingleSelectLevel; menu->session.gameMode = GAME_MODE_SINGLE; };
        if(ImGui::Button("Create session", ImVec2(100, 60))) { menu->state = MainMenu_ConfigureServer; menu->session.gameMode = GAME_MODE_SERVER; };
        if(ImGui::Button("Connect", ImVec2(100, 60))) { menu->state = MainMenu_ConfigureClient; menu->session.gameMode = GAME_MODE_CLIENT; };
        if(ImGui::Button("Gen test level", ImVec2(100, 60))) { menu->state = MainMenu_GenTestLevel; };
    }

    internal void
    MenuSingleSelectLevel(GameMenu* menu)
    {
        ImGui::Text("Load level");
        ImGui::Separator();
        ImGui::Text("Path:");
        ImGui::PushID("level path input");
        if (ImGui::InputText("", menu->levelPathBuffer, LEVEL_PATH_BUFFER_SIZE))
        {
            mbstowcs(menu->wLevelPathBuffer, menu->levelPathBuffer, LEVEL_PATH_BUFFER_SIZE);
        }
        ImGui::PopID();
        if (ImGui::Button("Load", ImVec2(60, 20)))
        {
            if (GetLevelMetaInfo(menu->wLevelPathBuffer, &menu->levelMetaInfo))
            {
                menu->state = MainMenu_SingleLoadLevel;
            }
        }
        ImGui::SameLine();

        if (ImGui::Button("Create new", ImVec2(100, 20)))
        {
        }

        ImGui::Separator();

        if (ImGui::Button("Return", ImVec2(100, 20)))
        {
            menu->state = MainMenu_ModeSelection;
        }

    }

    internal void
    SingleLoadLevel(GameMenu* menu, GameState* gameState)
    {
        MainMenuState nextState = MainMenu_SingleSelectLevel;
        uptr arenaSize = CalcLevelArenaSize(&menu->levelMetaInfo);
        MemoryArena* levelArena = PLATFORM_QUERY_NEW_ARENA(arenaSize);
        SOKO_ASSERT(levelArena);
        // TODO: Remove level from GameState
        Level* level = InitializeLevel(menu->wLevelPathBuffer, levelArena, gameState);
        if (level)
        {
            menu->session.sessionArena = levelArena;
            menu->session.level = level;
            nextState = MainMenu_EnterLevel;
        }
        else
        {
            PLATFORM_FREE_ARENA(levelArena);
        }
        menu->state = nextState;
    }


    internal void
    MenuServerSettings(GameMenu* menu)
    {
        ImGui::Text("Create server:");
        ImGui::Separator();

        auto numFilter = [](ImGuiInputTextCallbackData* data) -> int {
            return !(data->EventChar >= '0' && data->EventChar <='9');
        };

        char buf[64] = "";
        FormatString(buf, 64, "%i16", menu->serverConf.port);
        ImGui::InputText("port", buf, 64, ImGuiInputTextFlags_CallbackCharFilter, numFilter);
        u64 val = strtol(buf, 0, 10);
        bool portIsCorrect = false;
        if (val > 1023 && val < 65536)
        {
            menu->serverConf.port = (u16)val;
        }
        if (menu->serverConf.port > 1023 && menu->serverConf.port < 65535)
        {
            portIsCorrect = true;
        }

        ImGui::Separator();

        ImGui::Text("Level:");
        ImGui::Text("Path:");
        ImGui::PushID("level path input");
        if (ImGui::InputText("", menu->levelPathBuffer, LEVEL_PATH_BUFFER_SIZE))
        {
            mbstowcs(menu->wLevelPathBuffer, menu->levelPathBuffer, LEVEL_PATH_BUFFER_SIZE);
        }
        ImGui::PopID();

        if (ImGui::Button("Load", ImVec2(60, 20)))
        {
            if (GetLevelMetaInfo(menu->wLevelPathBuffer, &menu->levelMetaInfo))
            {
                if (portIsCorrect)
                {
                    menu->state = MainMenu_CreateServer;
                }
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Create new", ImVec2(100, 20)))
        {
        }
        ImGui::Separator();

        if (ImGui::Button("Return", ImVec2(100, 20)))
        {
            menu->state = MainMenu_ModeSelection;
        }
    }

    internal void
    MenuCreateServer(GameState* gameState, GameMenu* menu)
    {
        MainMenuState nextState = MainMenu_ConfigureServer;
        uptr arenaSize = CalcLevelArenaSize(&menu->levelMetaInfo);
        arenaSize += sizeof(net::Server);
        MemoryArena* levelArena = PLATFORM_QUERY_NEW_ARENA(arenaSize);
        SOKO_ASSERT(levelArena);
        menu->session.sessionArena = levelArena;

        net::Server* server = net::InitializeServer(levelArena, menu->serverConf.port);
        if (server)
        {
            menu->session.server = server;
            Level* level = InitializeLevel(menu->wLevelPathBuffer, levelArena, gameState);
            if (level)
            {
                COPY_BYTES(SERVER_MAX_LEVEL_NAME_LEN, server->levelName, menu->levelPathBuffer);
                //gameState->port = menu->serverConf.port;
                // TODO: Player placement (level start positions)
                gameState->controlledPlayer = AddPlayer(gameState, level, V3I(10, 10, 1), gameState->memoryArena);
                if (gameState->controlledPlayer)
                {
                    if (net::ServerAddPlayer(server,
                                             gameState->controlledPlayer,
                                             net::SERVER_LOCAL_PLAYER_SLOT, {}))
                    {
                        menu->state = MainMenu_EnterLevel;
                        menu->session.level = level;
                        return;
                    }
                }
            }
        }
        MenuCleanup(menu, MainMenu_ConfigureServer,
                    MenuCleanup_NetSpecific);
    }

    internal void
    MenuClientSettings(GameMenu* menu)
    {
        ImGui::Text("Connect to server:");
        ImGui::Separator();

        float width = 150.0f;
        ImGui::BeginGroup();
        ImGui::PushID("IP");
        ImGui::TextUnformatted("IP");
        ImGui::SameLine();
        bool ipInputSucceed = true;
        for (u32 i = 0; i < 4; i++)
        {
            ImGui::PushItemWidth(width / 4.0f);
            ImGui::PushID(i);
            bool invalidOctet = false;
            if (menu->clientConf.ipOctets[i] > 255)
            {
                menu->clientConf.ipOctets[i] = 255;
                invalidOctet = true;
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
            }
            if (menu->clientConf.ipOctets[i] < 0)
            {
                menu->clientConf.ipOctets[i] = 0;
                invalidOctet = true;
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
            }
            ImGui::InputInt("##v", (int*)(menu->clientConf.ipOctets + i), 0, 0, ImGuiInputTextFlags_CharsDecimal);
            if (invalidOctet)
            {
                if (ipInputSucceed)
                {
                    ipInputSucceed = false;
                }
                ImGui::PopStyleColor();
            }
            ImGui::SameLine();


            ImGui::PopID();
            ImGui::PopItemWidth();
        }

        if (ipInputSucceed)
        {
            menu->clientConf.serverAddress.ip =
                (menu->clientConf.ipOctets[0] << 24) |
                (menu->clientConf.ipOctets[1] << 16) |
                (menu->clientConf.ipOctets[2] << 8) |
                (menu->clientConf.ipOctets[3]);
        }

        ImGui::Text(":");
        ImGui::SameLine();
        ImGui::PushItemWidth(width / 4.0f);
        ImGui::InputInt("##v", (int*)(&menu->clientConf.inputPort), 0, 0, ImGuiInputTextFlags_CharsDecimal);

        bool inputSucceed = false;

        if (menu->clientConf.inputPort > 1023 &&
            menu->clientConf.inputPort < 65536 &&
            ipInputSucceed)
        {
            inputSucceed = true;
            menu->clientConf.serverAddress.port = (i16)menu->clientConf.inputPort;
        }

        ImGui::PopID();
        ImGui::EndGroup();

        if(ImGui::Button("Connect", ImVec2(70.0f, 20.0f)))
        {
            if (inputSucceed)
            {
                Socket socket = NetCreateSocket();
                if (socket)
                {
                    if (net::SendServerStateQuery(socket, menu->clientConf.serverAddress))
                    {
                        menu->clientConf.socket = socket;
                        menu->state = MainMenu_ClientWaitForServerState;
                    }
                    else
                    {
                        bool result = NetCloseSocket(socket);
                        SOKO_ASSERT(result);
                    }

                }
            }
        }

        ImGui::Separator();

        if (ImGui::Button("Return", ImVec2(100, 20)))
        {
            menu->state = MainMenu_ModeSelection;
        }
    }

    internal void
    MenuClientWaitForServerState(GameMenu* menu)
    {
        // TODO: Waiting timer
        byte buffer[512];
        i32 result = net::ClientWaitForServerState(menu->clientConf.socket, buffer, 512);
        switch (result)
        {
        case 1:
        {
            auto stateMsg = (ServerMsgState*)(buffer + sizeof(ServerMsgHeader));
            auto levelName = (char*)stateMsg + sizeof(ServerMsgState);
            SOKO_ASSERT(stateMsg->levelNameStrLen <= SERVER_MAX_LEVEL_NAME_LEN);
            if (stateMsg->hasAvailableSlot)
            {
                COPY_BYTES(stateMsg->levelNameStrLen, menu->levelPathBuffer, levelName);
                mbstowcs(menu->wLevelPathBuffer, levelName, SERVER_MAX_LEVEL_NAME_LEN);

                if (GetLevelMetaInfo(menu->wLevelPathBuffer, &menu->levelMetaInfo))
                {
                    menu->state = MainMenu_ClientLoadLevel;
                    return;
                }
                else
                {
                    SOKO_INFO("Can not find level file");
                }
            }
            else
            {
                SOKO_INFO("Server is full");
            }

        } break;
        case -1:
        {
            SOKO_INFO("Failed to get server state.");
        } break;
        case 0: { return; } break;
        default: {} break;
        }
        MenuCleanup(menu, MainMenu_ConfigureClient,
                    MenuCleanup_Common | MenuCleanup_NetSpecific);
    }

    internal void
    ClientLoadLevel(GameMenu* menu, GameState* gameState)
    {
        uptr arenaSize = CalcLevelArenaSize(&menu->levelMetaInfo);
        arenaSize += sizeof(net::Client);
        arenaSize += 8; // Safety pad
        MemoryArena* levelArena = PLATFORM_QUERY_NEW_ARENA(arenaSize);
        SOKO_ASSERT(levelArena);
        menu->session.sessionArena = levelArena;
        net::Client* client = PUSH_STRUCT(levelArena, net::Client);
        if (client)
        {
            menu->session.client = client;
            client->serverAddr = menu->clientConf.serverAddress;
            client->socket = menu->clientConf.socket;
            COPY_BYTES(SERVER_MAX_LEVEL_NAME_LEN, &client->levelName, &menu->levelPathBuffer);

            Level* level = InitializeLevel(menu->wLevelPathBuffer, levelArena, gameState);
            if (level)
            {
                if (net::ClientSendConnectionQuery(client->socket, client->serverAddr))
                {
                    menu->session.level = level;
                    menu->state = MainMenu_ClientConnectToServer;
                    return;
                }
            }
        }

        MenuCleanup(menu, MainMenu_ConfigureClient,
                    MenuCleanup_Common | MenuCleanup_NetSpecific);
    }

    internal void
    MenuClientConnectToServer(GameMenu* menu, GameState* gameState)
    {
        i32 result = net::ClientWaitForConnectionResult(menu->session.client->socket, menu->session.client->socketBuffer, net::SERVER_SOCKET_BUFFER_SIZE);

        if (result > 0)
        {
            auto msg = (ServerJoinResultMsg*)(menu->session.client->socketBuffer + sizeof(ServerMsgHeader));
            u32 msgSize = (u32)result - sizeof(ServerMsgHeader);
            if (net::ClientEstablishConnection(menu->session.client, msg, msgSize, gameState, menu->session.level))
            {
                menu->state = MainMenu_EnterLevel;
            }

        }
        else if (result == 0)
        {

        }
        else
        {
            SOKO_INFO("Error while trying connct to server");
            MenuCleanup(menu, MainMenu_ConfigureClient,
                        MenuCleanup_Common | MenuCleanup_NetSpecific);
        }
    }

    internal void
    MenuEnterLevel(GameMenu* menu, GameState* gameState)
    {
        gameState->globalGameMode = menu->session.gameMode;
        gameState->session = menu->session;

        BeginTemporaryMemory(gameState->tempArena, true);
        EntityStr string = CreateEntityStr(gameState->tempArena);
        for (u32 i = 0; i < LEVEL_ENTITY_TABLE_SIZE; i++)
        {
            Entity* e = gameState->session.level->entities[i];
            if (e)
            {
                do
                {
                    SerializeEntity(&string, e, gameState->tempArena);
                    e = e->nextEntity;
                } while (e);
            }
        }
        WriteEntityStringToFile(L"Entities.txt", &string);
        EndTemporaryMemory(gameState->tempArena);
    }

    internal void
    MenuUpdateAndRender(GameMenu* menu, GameState* gameState)
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
        switch (menu->state)
        {
        case MainMenu_ModeSelection: { MenuModeSelection(menu); } break;
        case MainMenu_SingleSelectLevel: { MenuSingleSelectLevel(menu); } break;
        case MainMenu_SingleLoadLevel: { SingleLoadLevel(menu, gameState); } break;
        case MainMenu_ConfigureServer: { MenuServerSettings(menu); } break;
        case MainMenu_CreateServer: { MenuCreateServer(gameState, menu); } break;
        case MainMenu_ConfigureClient: { MenuClientSettings(menu); } break;
        case MainMenu_ClientWaitForServerState: { MenuClientWaitForServerState(menu); } break;
        case MainMenu_ClientConnectToServer: { MenuClientConnectToServer(menu, gameState); } break;
        case MainMenu_ClientLoadLevel: { ClientLoadLevel(menu, gameState); } break;
        case MainMenu_GenTestLevel: { GenTestLevel(gameState->tempArena); menu->state = MainMenu_ModeSelection; } break;
        case MainMenu_EnterLevel: { MenuEnterLevel(menu, gameState); } break;
        INVALID_DEFAULT_CASE;
        }

        ImGui::End();
    }
}
