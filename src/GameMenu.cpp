namespace soko
{
    internal Level*
    InitializeLevel(const wchar_t* filename, AB::MemoryArena* levelArena, GameState* gameState)
    {
        BeginTemporaryMemory(gameState->tempArena);
        Level* level = LoadLevel(filename, levelArena, gameState->tempArena);
        EndTemporaryMemory(gameState->tempArena);
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
            COPY_BYTES(StrSize(DEFAULT_LEVEL_NAME), menu->levelPathBuffer, DEFAULT_LEVEL_NAME);
            COPY_BYTES(StrSize(DEFAULT_LEVEL_NAME_W), menu->wLevelPathBuffer, DEFAULT_LEVEL_NAME_W);

        }

        menu->state = nextState;
    }

    internal void
    MenuModeSelection(GameMenu* menu)
    {
        // TODO: No need for full cleanup here
        MenuCleanup(menu, MainMenu_ModeSelection, MenuCleanup_Common | MenuCleanup_NetSpecific);
        if(ImGui::Button("Editor", ImVec2(100, 60))) { menu->state = MainMenu_EditorConf; menu->session.gameMode = GAME_MODE_EDITOR; };
        if(ImGui::Button("Single", ImVec2(100, 60))) { menu->state = MainMenu_SingleSelectLevel; menu->session.gameMode = GAME_MODE_SINGLE; };
        if(ImGui::Button("Create session", ImVec2(100, 60))) { menu->state = MainMenu_ConfigureServer; menu->session.gameMode = GAME_MODE_SERVER; };
        if(ImGui::Button("Connect", ImVec2(100, 60))) { menu->state = MainMenu_ConnectToServer; menu->session.gameMode = GAME_MODE_CLIENT; };
        if(ImGui::Button("Gen test level", ImVec2(100, 60))) { menu->state = MainMenu_GenTestLevel; };
    }

    internal void
    MenuEditorConf(GameMenu* menu)
    {
        ImGui::Text("Editor");
        ImGui::Separator();
        ImGui::Text("Name:");
        ImGui::PushID("level path input");
        if (ImGui::InputText("", menu->levelPathBuffer, LEVEL_PATH_BUFFER_SIZE))
        {
            mbstowcs(menu->wLevelPathBuffer, menu->levelPathBuffer, LEVEL_PATH_BUFFER_SIZE);
        }
        ImGui::PopID();
        ImGui::SameLine();
        if (ImGui::Button("Load", ImVec2(60, 20)))
        {
            if (GetLevelMetaInfo(menu->wLevelPathBuffer, &menu->levelMetaInfo))
            {
                menu->state = MainMenu_EditorLoadLevel;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Create", ImVec2(60, 20)))
        {
            menu->state = MainMenu_EditorCreateLevel;
        }
        ImGui::SameLine();
        if (ImGui::Button("Return", ImVec2(100, 20)))
        {
            menu->state = MainMenu_ModeSelection;
        }
    }

    internal void
    MenuEditorLoadLevel(GameMenu* menu, GameState* gameState)
    {
        MainMenuState nextState = MainMenu_EditorConf;
        uptr arenaSize = CalcLevelArenaSize(&menu->levelMetaInfo, ENTITY_MEMORY_SIZE_FOR_LEVEL);
        MemoryArena* levelArena = PLATFORM_QUERY_NEW_ARENA(arenaSize);
        SOKO_ASSERT(levelArena);
        // TODO: Remove level from GameState
        Level* level = InitializeLevel(menu->wLevelPathBuffer, levelArena, gameState);
        if (level)
        {
            // TODO: Player spawn position
            menu->session.sessionArena = levelArena;
            menu->session.level = level;
            //menu->session.controlledPlayer = AddPlayer(&menu->session, IV3(10, 10, 1));
            nextState = MainMenu_EnterEditor;
        }
        else
        {
            PLATFORM_FREE_ARENA(levelArena);
        }
        menu->state = nextState;
    }

    internal void
    MenuEditorCreateLevel(GameMenu* menu, GameState* gameState)
    {
        MainMenuState nextState = MainMenu_EditorConf;
        // TODO: Dynamically growing arenas
        uptr arenaSize = MEGABYTES(16);
        MemoryArena* levelArena = PLATFORM_QUERY_NEW_ARENA(arenaSize);
        SOKO_ASSERT(levelArena);
        // TODO: Remove level from GameState
        Level* level = CreateLevel(levelArena);
        if (level)
        {
            // TODO: Player spawn position
            menu->session.sessionArena = levelArena;
            menu->session.level = level;
            //menu->session.controlledPlayer = AddPlayer(&menu->session, IV3(10, 10, 1));
            nextState = MainMenu_EnterEditor;
        }
        else
        {
            PLATFORM_FREE_ARENA(levelArena);
        }
        menu->state = nextState;
    }

    internal void
    MenuEnterEditor(GameMenu* menu, GameState* gameState)
    {
        // TODO: @Cleanup: Pull out this general stuff to separate menu stage
        gameState->globalGameMode = menu->session.gameMode;
        gameState->session = menu->session;
        menu->session = {};
        menu->state = MainMenu_ModeSelection;
        // TODO: Allocate session in session
        // arena and store just a pointer in gameState
        // to avoid copying session to gameState
        gameState->session.firstPlayer = 0;
        gameState->session.secondPlayer = 0;



        gameState->session.editorCamera = PUSH_STRUCT(gameState->session.sessionArena, EditorCamera);
        gameState->session.editor = PUSH_STRUCT(gameState->session.sessionArena, Editor);
        SOKO_ASSERT(gameState->session.editorCamera);
        SOKO_ASSERT(gameState->session.editor);
        EditorInit(gameState);
        EditorCameraInit(gameState->session.editorCamera);
        RenderGroupSetCamera(gameState->renderGroup, &gameState->session.editorCamera->conf);
    }

    internal void
    FillLevelCache(MenuLevelCache* levelCache, AB::MemoryArena* tempArena)
    {
        SOKO_ASSERT(!levelCache->initialized);

        levelCache->dirScanResult = EnumerateFilesInDirectory(L".", tempArena);

        levelCache->isLevel = PUSH_ARRAY(tempArena, b32, levelCache->dirScanResult.count);
        SOKO_ASSERT(levelCache->isLevel);

        for (u32 i = 0; i < levelCache->dirScanResult.count; i++)
        {
            LevelMetaInfo info = {};
            b32 isLevel = GetLevelMetaInfo(levelCache->dirScanResult.filenames[i], &info);
            if (isLevel && !levelCache->initialized)
            {
                levelCache->selectedIndex = i;
            }
            levelCache->isLevel[i] = isLevel;
        }
        levelCache->initialized = true;
    }

    // TODO: Better tempArena
    internal void
    MenuSingleSelectLevel(GameMenu* menu, AB::MemoryArena* tempArena)
    {
        MainMenuState nextState = MainMenu_SingleSelectLevel;
        if (!menu->levelCache.initialized)
        {
            BeginTemporaryMemory(tempArena);
            FillLevelCache(&menu->levelCache, tempArena);
        }

        ImGui::Text("Load level");

        char string[256];
        wcstombs(string, menu->levelCache.dirScanResult.filenames[menu->levelCache.selectedIndex], 256);
        if (ImGui::BeginCombo("Available levels", string))
        {
            for (u32 i = 0; i < menu->levelCache.dirScanResult.count; i++)
            {
                if (menu->levelCache.isLevel[i])
                {
                    if (wcstombs(string, menu->levelCache.dirScanResult.filenames[i], 256) == (size_t)(-1))
                    {
                        strcpy(string, "<error>");
                    }
                    if (ImGui::Selectable(string))
                    {
                        menu->levelCache.selectedIndex = i;
                    }
                }
            }

            ImGui::EndCombo();
        }

        ImGui::Separator();
        if (ImGui::Button("Load", ImVec2(60, 20)))
        {
            wchar_t* levelName = menu->levelCache.dirScanResult.filenames[menu->levelCache.selectedIndex];
            if (GetLevelMetaInfo(levelName, &menu->levelMetaInfo))
            {
                wcscpy_s(menu->wLevelPathBuffer, LEVEL_PATH_BUFFER_SIZE, levelName);
                nextState = MainMenu_SingleLoadLevel;
            }
        }
        ImGui::SameLine();

        if (ImGui::Button("Return", ImVec2(100, 20)))
        {
            nextState = MainMenu_ModeSelection;
        }

        menu->state = nextState;

        if (menu->state != MainMenu_SingleSelectLevel)
        {
            menu->levelCache = {};
            EndTemporaryMemory(tempArena);
        }
    }

    internal void
    SingleLoadLevel(GameMenu* menu, GameState* gameState)
    {
        MainMenuState nextState = MainMenu_SingleSelectLevel;
        uptr arenaSize = CalcLevelArenaSize(&menu->levelMetaInfo, ENTITY_MEMORY_SIZE_FOR_LEVEL);
        MemoryArena* levelArena = PLATFORM_QUERY_NEW_ARENA(arenaSize);
        SOKO_ASSERT(levelArena);
        // TODO: Remove level from GameState
        Level* level = InitializeLevel(menu->wLevelPathBuffer, levelArena, gameState);
        if (level)
        {
            menu->session.sessionArena = levelArena;
            menu->session.level = level;
            bool p1Added = AddPlayer(&menu->session, level->firstPlayerSpawnPos, PlayerSlot_First);
            SOKO_ASSERT(p1Added);
            if (level->hasSecondPlayer)
            {
                bool p2Added = AddPlayer(&menu->session, level->secondPlayerSpawnPos, PlayerSlot_Second);
                SOKO_ASSERT(p2Added);
            }
            nextState = MainMenu_EnterLevel;
        }
        else
        {
            PLATFORM_FREE_ARENA(levelArena);
        }
        menu->state = nextState;
    }

    internal void
    ChangePlayerNameModal(GameState* gameState, GameMenu* menu)
    {
        if (ImGui::BeginPopupModal("Change player name", 0, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::InputText("Player name", menu->playerNameCache, PLAYER_NAME_LEN);
            if (ImGui::Button("Apply"))
            {
                strcpy(gameState->playerName, menu->playerNameCache);
                menu->playerNameCache[0] = 0;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Close"))
            {
                menu->playerNameCache[0] = 0;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    internal void
    InitServer(GameMenu* menu)
    {
        SOKO_ASSERT(menu->serverState == ServerState_NotInitialized);
        SOKO_ASSERT(!menu->serverListenSocket);

        uptr listenSocket = NetCreateSocket(SocketType_TCP);
        if (listenSocket)
        {
            u16 port = NetBindSocket(listenSocket);
            if (port)
            {
                if (NetListen(listenSocket, 4))
                {
                    menu->serverPort = port;
                    menu->serverListenSocket = listenSocket;
                    menu->serverState = ServerState_Listening;
                }
                else
                {
                    SOKO_INFO("Failed to listen socket");
                    NetCloseSocket(listenSocket);
                }
            }
            else
            {
                SOKO_INFO("Failed to bind socket");
                NetCloseSocket(listenSocket);
            }
        }
    }

    internal void
    ShutdownServer(GameMenu* menu)
    {
        if (menu->serverListenSocket)
        {
            NetCloseSocket(menu->serverListenSocket);
            menu->serverListenSocket = 0;
        }
        if (menu->serverConnectionSocket)
        {
            NetCloseSocket(menu->serverConnectionSocket);
            menu->serverConnectionSocket = 0;
        }
        menu->serverState = ServerState_NotInitialized;
        menu->serverPort = 0;
    }

    internal void
    MenuServerSettings(GameState* gameState, GameMenu* menu, AB::MemoryArena* tempArena)
    {
        MainMenuState nextState = MainMenu_ConfigureServer;
        bool serverInitialized = false;
        if (!menu->serverState)
        {
            InitServer(menu);
        }

        if (menu->serverState)
        {
            if (!menu->levelCache.initialized)
            {
                BeginTemporaryMemory(tempArena);
                FillLevelCache(&menu->levelCache, tempArena);
            }

            ImGui::Text("Session settings:");
            ImGui::Separator();
            SOKO_ASSERT(menu->serverPort);
            ImGui::Text("Server port: %d", (int)menu->serverPort);
            ImGui::Separator();

            ImGui::Text("Level");
            char string[256];
            if (!menu->levelCache.selectedIndex)
            {
                strcpy(string, "No levels found");
            }
            else
            {
                wcstombs(string, menu->levelCache.dirScanResult.filenames[menu->levelCache.selectedIndex], 256);
            }
            if (ImGui::BeginCombo("Available levels", string))
            {
                for (u32 i = 0; i < menu->levelCache.dirScanResult.count; i++)
                {
                    if (menu->levelCache.isLevel[i])
                    {
                        if (wcstombs(string, menu->levelCache.dirScanResult.filenames[i], 256) == (size_t)(-1))
                        {
                            strcpy(string, "<error>");
                        }
                        if (ImGui::Selectable(string))
                        {
                            menu->levelCache.selectedIndex = i + 1; // NOTE: Zero is invalid value
                        }
                    }
                }

                ImGui::EndCombo();
            }

            ImGui::Separator();
            ImGui::Text("Players:");
            ImGui::Text("Player 1:\t %s", gameState->playerName);
            ImGui::SameLine();
            if (ImGui::SmallButton("Change name"))
            {
                if (!ImGui::IsPopupOpen("Change player name"))
                {
                    strcpy(menu->playerNameCache, gameState->playerName);
                    ImGui::OpenPopup("Change player name");
                }
            }
            if (ImGui::IsPopupOpen("Change player name"))
            {
                ChangePlayerNameModal(gameState, menu);
            }
            ImGui::Text("Player 2:\t <not connected>");

            ImGui::Separator();
            if (ImGui::Button("Load", ImVec2(60, 20)))
            {
                if (menu->levelCache.selectedIndex)
                {
                    wchar_t* levelName = menu->levelCache.dirScanResult.filenames[menu->levelCache.selectedIndex - 1];
                    if (GetLevelMetaInfo(levelName, &menu->levelMetaInfo))
                    {
                        wcscpy_s(menu->wLevelPathBuffer, LEVEL_PATH_BUFFER_SIZE, levelName);
                        nextState = MainMenu_SingleLoadLevel;
                    }
                }
            }
            ImGui::SameLine();

            if (ImGui::Button("Return", ImVec2(100, 20)))
            {
                nextState = MainMenu_ModeSelection;
            }

            if (menu->serverState == ServerState_Listening)
            {
                uptr connectionSocket = NetAccept(menu->serverListenSocket);
                if (connectionSocket)
                {
                    NetCloseSocket(menu->serverListenSocket);
                    menu->serverState = ServerState_Connected;
                    menu->serverConnectionSocket = connectionSocket;
                    SOKO_INFO("Connection established!");
                }
            }

            menu->state = nextState;

            if (menu->state != MainMenu_ConfigureServer)
            {
                ShutdownServer(menu);
                menu->levelCache = {};
                EndTemporaryMemory(tempArena);
            }
        }

    }

    internal void
    MenuConnectToServer(GameMenu* menu)
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
                uptr socket = NetCreateSocket(SocketType_TCP);
                if (socket)
                {
                    NetAddress address;
                    address.ip = menu->clientConf.serverAddress.ip;
                    address.port = menu->clientConf.serverAddress.port;
                    auto connectionResult = NetConnect(socket, address);
                    if (connectionResult == ConnectionStatus_Error)
                    {
                        NetCloseSocket(socket);
                        // TODO: Log error
                    }
                    else
                    {
                        menu->clientSocket = socket;
                        menu->state = MainMenu_ClientWaitForConnection;
                    }
                }
            }
        }
    }

    internal void
    MenuWaitForConnection(GameMenu* menu)
    {
        ImGui::Text("Connecting...");
        NetAddress address;
        address.ip = menu->clientConf.serverAddress.ip;
        address.port = menu->clientConf.serverAddress.port;
        auto connectionResult = NetConnect(menu->clientSocket, address);
        switch (connectionResult)
        {
        case ConnectionStatus_Connected: { menu->state = MainMenu_ClientSessionLobby; } break;
        // TODO: Show error
        case ConnectionStatus_Error: { NetCloseSocket(menu->clientSocket); menu->clientSocket = 0; menu->state = MainMenu_ConnectToServer;} break;
        default: {} break;
        }
    }

    internal void
    MenuClientSessionLobby(GameMenu* menu)
    {
        ImGui::Text("Connected to server");
    }

#if 0
    internal void
    MenuCreateServer(GameState* gameState, GameMenu* menu)
    {
        MainMenuState nextState = MainMenu_ConfigureServer;
        uptr arenaSize = CalcLevelArenaSize(&menu->levelMetaInfo, ENTITY_MEMORY_SIZE_FOR_LEVEL);
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
                gameState->session.controlledPlayer = AddPlayer(&gameState->session, IV3(10, 10, 1));
                if (gameState->session.controlledPlayer)
                {
                    if (net::ServerAddPlayer(server,
                                             gameState->session.controlledPlayer,
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
        uptr arenaSize = CalcLevelArenaSize(&menu->levelMetaInfo, ENTITY_MEMORY_SIZE_FOR_LEVEL);
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
#endif
    internal void
    MenuEnterLevel(GameMenu* menu, GameState* gameState)
    {
        gameState->globalGameMode = menu->session.gameMode;
        gameState->session = menu->session;
        menu->session = {};
        menu->state = MainMenu_ModeSelection;
        // TODO: Allocate session in session
        // arena and store just a pointer in gameState
        // to avoid copying session to gameState
        //gameState->session.controlledPlayer = gameState->session.players;

        InitCameras(&gameState->session.camera, MakeWorldPos(gameState->session.firstPlayer->pos), &gameState->session.debugCamera);
        RenderGroupSetCamera(gameState->renderGroup, &gameState->session.camera.conf);
    }

    internal void
    MenuLevelCompleted(GameMenu* menu, GameState* gameState)
    {
        ImGui::Text("Level completed!");
        if (ImGui::Button("Go to menu"))
        {
            menu->state = MainMenu_ModeSelection;
        }
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
        case MainMenu_ModeSelection:
        {
            MenuModeSelection(menu);
            ImGui::Text("Main arena free space: %llu", gameState->memoryArena->free);
        } break;
        // NOTE: MenuSingleSelectLevel() uses tempArena
        case MainMenu_SingleSelectLevel: { MenuSingleSelectLevel(menu, gameState->tempArena); } break;
        case MainMenu_SingleLoadLevel: { SingleLoadLevel(menu, gameState); } break;
        case MainMenu_ConfigureServer: { MenuServerSettings(gameState, menu, gameState->tempArena); } break;
//        case MainMenu_CreateServer: { MenuCreateServer(gameState, menu); } break;
        case MainMenu_ConnectToServer: { MenuConnectToServer(menu); } break;
        case MainMenu_ClientWaitForConnection: { MenuWaitForConnection(menu); } break;
        case MainMenu_ClientSessionLobby: { MenuClientSessionLobby(menu); } break;
//        case MainMenu_ClientWaitForServerState: { MenuClientWaitForServerState(menu); } break;
//        case MainMenu_ClientConnectToServer: { MenuClientConnectToServer(menu, gameState); } break;
//        case MainMenu_ClientLoadLevel: { ClientLoadLevel(menu, gameState); } break;
        case MainMenu_GenTestLevel: { GenTestLevel(gameState->tempArena); menu->state = MainMenu_ModeSelection; } break;
        case MainMenu_EnterLevel: { MenuEnterLevel(menu, gameState); } break;
        case MainMenu_EnterEditor: { MenuEnterEditor(menu, gameState); } break;
        case MainMenu_EditorConf: { MenuEditorConf(menu); } break;
        case MainMenu_EditorLoadLevel: { MenuEditorLoadLevel(menu, gameState); } break;
        case MainMenu_EditorCreateLevel: { MenuEditorCreateLevel(menu, gameState); } break;
        case MainMenu_LevelCompleted: { MenuLevelCompleted(menu, gameState); } break;
        INVALID_DEFAULT_CASE;
        }

        ImGui::End();
    }
}
