namespace soko
{
    internal Level*
    InitializeLevel(const wchar_t* filename, AB::MemoryArena* levelArena, AB::MemoryArena* tempArena)
    {
        BeginTemporaryMemory(tempArena);
        Level* level = LoadLevel(filename, levelArena, tempArena);
        EndTemporaryMemory(tempArena);
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
            if (menu->session.sessionArena)
            {
                PLATFORM_FREE_ARENA(menu->session.sessionArena);
                menu->session.client = 0;
            }

        }

        if (flags & MenuCleanup_Common)
        {
            ZERO_FROM_MEMBER(GameMenu, levelPathBuffer, menu);
            menu->session = {};
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
        // TODO: Maybe stop using gameState's temp arena for that
        Level* level = InitializeLevel(menu->wLevelPathBuffer, levelArena, gameState->tempArena);
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

    inline void
    SetLevelAvailability(MenuLevelCache* levelCache, b32 value)
    {
        for (u32 i = 0; i < levelCache->dirScanResult.count; i++)
        {
            levelCache->availableOnClient[i] = value;
        }
    }

    inline void
    ResetLevelIndexToValid(MenuLevelCache* levelCache, bool multiplayer)
    {
        levelCache->selectedIndex = 0;
        for (u32 i = 0; i < levelCache->dirScanResult.count; i++)
        {
            bool isLevel = levelCache->isLevel[i];
            bool valid = isLevel;
            if (multiplayer)
            {
                bool available = levelCache->availableOnClient[i];
                bool supportsMultiplayer = levelCache->supportsMultiplayer[i];
                valid = valid && available && supportsMultiplayer;
            }
            if (valid)
            {
                levelCache->selectedIndex = i + 1;
                break;
            }
        }
    }

    internal void
    FillLevelCache(MenuLevelCache* levelCache, AB::MemoryArena* tempArena)
    {
        SOKO_ASSERT(!levelCache->initialized);

        levelCache->dirScanResult = EnumerateFilesInDirectory(L".", tempArena);

        levelCache->isLevel = PUSH_ARRAY(tempArena, b32, levelCache->dirScanResult.count);
        SOKO_ASSERT(levelCache->isLevel);

        levelCache->GUIDs = PUSH_ARRAY(tempArena, u64, levelCache->dirScanResult.count);
        SOKO_ASSERT(levelCache->GUIDs);

        levelCache->availableOnClient = PUSH_ARRAY(tempArena, b32, levelCache->dirScanResult.count);
        SOKO_ASSERT(levelCache->availableOnClient);

        levelCache->supportsMultiplayer = PUSH_ARRAY(tempArena, b32, levelCache->dirScanResult.count);
        SOKO_ASSERT(levelCache->supportsMultiplayer);

        SetLevelAvailability(levelCache, true);

        for (u32 i = 0; i < levelCache->dirScanResult.count; i++)
        {
            LevelMetaInfo info = {};
            b32 isLevel = GetLevelMetaInfo(levelCache->dirScanResult.filenames[i], &info);

            levelCache->isLevel[i] = isLevel;
            levelCache->GUIDs[i] = info.guid;
            levelCache->supportsMultiplayer[i] = info.supportsMultiplayer;
        }
        ResetLevelIndexToValid(levelCache, true);
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
        // TODO: Maybe stop using gameState's temp arena for that
        Level* level = InitializeLevel(menu->wLevelPathBuffer, levelArena, gameState->tempArena);
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
    ServerLoadLevel(GameMenu* menu, GameState* gameState)
    {
        MainMenuState nextState = MainMenu_ConfigureServer;
        uptr arenaSize = CalcLevelArenaSize(&menu->levelMetaInfo, ENTITY_MEMORY_SIZE_FOR_LEVEL);
        MemoryArena* levelArena = PLATFORM_QUERY_NEW_ARENA(arenaSize);
        SOKO_ASSERT(levelArena);

        // TODO: Maybe stop using gameState's temp arena for that
        Level* level = InitializeLevel(menu->wLevelPathBuffer, levelArena, gameState->tempArena);
        if (level)
        {
            menu->session.sessionArena = levelArena;
            menu->session.level = level;
            bool p1Added = AddPlayer(&menu->session, level->firstPlayerSpawnPos, PlayerSlot_First);
            SOKO_ASSERT(p1Added);

            SOKO_ASSERT(level->hasSecondPlayer);
            // TODO: Add connected player
            bool p2Added = AddPlayer(&menu->session, level->secondPlayerSpawnPos, PlayerSlot_Second);
            SOKO_ASSERT(p2Added);

            nextState = MainMenu_EnterLevel;

            ServerEnterLevelMessage message = {};
            message.levelGUID = level->guid;
            auto sendResult = NetSend(menu->server.connectionSocket, (void*)(&message), sizeof(message), {});
            SOKO_LOG_SERVER_SEND(sendResult, ServerEnterLevelMessage);
        }
        else
        {
            PLATFORM_FREE_ARENA(levelArena);
        }
        menu->state = nextState;
    }

    enum ServerLobbyAction
    {
        ServerLobby_None = 0,
        ServerLobby_Refresh,
        ServerLobby_Kick,
        ServerLobby_Load,
        ServerLobby_Return
    };

    internal ServerLobbyAction
    ServerLobbyUI(GameMenu* menu, GameState* gameState)
    {
        ServerLobbyAction action = {};

        ImGui::Text("Session settings:");
        ImGui::Separator();
        SOKO_ASSERT(menu->server.port);
        ImGui::Text("Server port: %d", (int)menu->server.port);
        ImGui::Separator();

        ImGui::Text("Level");
        char string[256];
        if (!menu->levelCache.selectedIndex)
        {
            strcpy(string, "No levels found");
        }
        else
        {
            wcstombs(string, menu->levelCache.dirScanResult.filenames[menu->levelCache.selectedIndex - 1], 256);
        }
        if (ImGui::BeginCombo("Available levels", string))
        {
            for (u32 i = 0; i < menu->levelCache.dirScanResult.count; i++)
            {
                bool supportsMultiplayer = menu->levelCache.supportsMultiplayer[i];
                if (menu->levelCache.isLevel[i] && supportsMultiplayer)
                {
                    if (wcstombs(string, menu->levelCache.dirScanResult.filenames[i], 256) == (size_t)(-1))
                    {
                        strcpy(string, "<error>");
                    }
                    bool available = menu->levelCache.availableOnClient[i];
                    ImGuiSelectableFlags flags = available ? 0 : ImGuiSelectableFlags_Disabled;

                    if (ImGui::Selectable(string, false, flags) && available)
                    {
                        menu->levelCache.selectedIndex = i + 1; // NOTE: Zero is invalid value
                    }
                }
            }

            ImGui::EndCombo();
        }

        ImGui::SameLine();
        if (ImGui::Button("Refresh"))
        {
            action = ServerLobby_Refresh;
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
        ImGui::Text("Player 2:\t %s", menu->server.connectedPlayerName);
        if (menu->server.state == Server::Connected)
        {
            ImGui::SameLine();
            if (ImGui::SmallButton("Kick"))
            {
                action = ServerLobby_Kick;
            }
        }

        ImGui::Separator();
        if (ImGui::Button("Load", ImVec2(60, 20)))
        {
            if (menu->levelCache.selectedIndex)
            {
                action = ServerLobby_Load;
            }
        }
        ImGui::SameLine();

        if (ImGui::Button("Return", ImVec2(100, 20)))
        {
            action = ServerLobby_Return;
        }
        return action;
    }

    internal void
    MenuServerSettings(GameState* gameState, GameMenu* menu, AB::MemoryArena* tempArena)
    {
        MainMenuState nextState = MainMenu_ConfigureServer;
        bool serverInitialized = false;

        if (!menu->server.state)
        {
            InitServer(&menu->server);
        }
        else
        {
            if (!menu->levelCache.initialized)
            {
                BeginTemporaryMemory(tempArena);
                FillLevelCache(&menu->levelCache, tempArena);
            }

            ServerLobbyAction action = ServerLobbyUI(menu, gameState);
            switch (action)
            {
            case ServerLobby_Refresh:
            {
                menu->levelCache = {};
                EndTemporaryMemory(tempArena);
                BeginTemporaryMemory(tempArena);
                FillLevelCache(&menu->levelCache, tempArena);
                if (menu->server.playerConnected)
                {
                    SetLevelAvailability(&menu->levelCache, false);
                }
                else
                {
                    SetLevelAvailability(&menu->levelCache, true);
                }

                ResetLevelIndexToValid(&menu->levelCache, true);

                ServerLevelListQueryMessage msg = {};
                auto levelQueryResult = NetSend(menu->server.connectionSocket, (void*)(&msg), sizeof(msg), {});
                SOKO_LOG_SERVER_SEND(levelQueryResult, ServerLevelListQueryMessage);
            } break;
            case ServerLobby_Kick:
            {
                ServerDisconnectPlayer(&menu->server);
                SetLevelAvailability(&menu->levelCache, true);
                ResetLevelIndexToValid(&menu->levelCache, true);
            } break;
            case ServerLobby_Load:
            {
                if (menu->server.state == Server::Connected)
                {
                    wchar_t* levelName = menu->levelCache.dirScanResult.filenames[menu->levelCache.selectedIndex - 1];
                    if (GetLevelMetaInfo(levelName, &menu->levelMetaInfo))
                    {
                        wcscpy_s(menu->wLevelPathBuffer, LEVEL_PATH_BUFFER_SIZE, levelName);
                        nextState = MainMenu_ServerLoadLevel;
                    }
                }
            } break;
            case ServerLobby_Return:
            {
                nextState = MainMenu_ModeSelection;
            } break;
            default: {} break;
            }


            if (menu->server.state == Server::Listening)
            {
                Socket connectionSocket = NetAccept(menu->server.listenSocket);
                if (connectionSocket.handle)
                {
                    //NetCloseSocket(menu->serverListenSocket);
                    menu->server.state = Server::Connected;
                    menu->server.connectionSocket = connectionSocket;
                    menu->server.presenceTimer = PRESENCE_MESSAGE_TIMEOUT;
                    menu->server.connectionTimer = CONNECTION_TIMEOUT;
                    SOKO_INFO("Connection established!");
                }
            }
            else if (menu->server.state == Server::Connected)
            {
                auto recieveResult = BeginReadIncomingMessages(menu->server.connectionSocket, &menu->socketBuffer);
                // TODO: Handle errors and normal disconnects differently
                if (recieveResult.status == NetRecieveResult::ConnectionClosed ||
                    recieveResult.status == NetRecieveResult::ConnectionReset ||
                    recieveResult.status == NetRecieveResult::Error)
                {
                    EndReadIncomingMessages(&menu->socketBuffer);
                    ServerDisconnectPlayer(&menu->server);
                    SetLevelAvailability(&menu->levelCache, true);
                    ResetLevelIndexToValid(&menu->levelCache, true);
                }
                else if (recieveResult.status == NetRecieveResult::Success)
                {
                    NetMessageHeader* header;
                    do
                    {
                        header = GetNextIncomingMessage(&menu->socketBuffer);
                        if (header)
                        {
                            // TODO: Not reset timer if message is broken
                            menu->server.connectionTimer = CONNECTION_TIMEOUT;
                            switch (header->type)
                            {
                            case NetMessageHeader::ClientPresenceMessage:
                            {
                                SOKO_LOG_SERVER_RECV(ClientPresenceMessage);
                            } break;
                            case NetMessageHeader::ClientConnectMessage:
                            {
                                SOKO_LOG_SERVER_RECV(ClientConnectMessage);
                                ClientConnectMessage* msg = (ClientConnectMessage*)header;
                                strcpy(menu->server.connectedPlayerName, msg->playerName);
                                menu->server.playerConnected = true;

                                {
                                    ServerConnectMessage message = {};
                                    strcpy(message.playerName, gameState->playerName);
                                    auto result = NetSend(menu->server.connectionSocket, (void*)(&message), sizeof(message), {});
                                    SOKO_LOG_SERVER_SEND(result, ServerConnectMessage);
                                }

                                SetLevelAvailability(&menu->levelCache, false);
                                ServerLevelListQueryMessage queryMsg = {};
                                auto levelQueryResult = NetSend(menu->server.connectionSocket, (void*)(&queryMsg), sizeof(queryMsg), {});
                                SOKO_LOG_SERVER_SEND(levelQueryResult, ServerLevelListQueryMessage);

                            } break;
                            case NetMessageHeader::ClientLevelListMessage:
                            {
                                SOKO_INFO("Server: recieve <ClientLevelListMessage> message");
                                ClientLevelListMessage* msg = (ClientLevelListMessage*)header;
                                u32 numLevels = msg->numLevels;
                                u64* GUIDs = &msg->firstGUID;

                                for (u32 j = 0; j < menu->levelCache.dirScanResult.count; j++)
                                {
                                    if (menu->levelCache.isLevel[j])
                                    {
                                        u64 serverGUID = menu->levelCache.GUIDs[j];
                                        menu->levelCache.availableOnClient[j] = false;

                                        for (u32 i = 0; i < numLevels; i++)
                                        {
                                            u64 clientGUID = GUIDs[i];
                                            if (serverGUID == clientGUID)
                                            {
                                                menu->levelCache.availableOnClient[j] = true;
                                                break;
                                            }
                                        }
                                    }
                                }
                                ResetLevelIndexToValid(&menu->levelCache, true);
                            } break;
                            default: { SOKO_INFO("Server recieved broken message"); } break;
                            }
                        }
                    }
                    while (header);

                }
                EndReadIncomingMessages(&menu->socketBuffer);

                // NOTE: Send presence message
                // TODO: For debug only!!!
                if (JustPressed(KEY_P))
                {
                    menu->server.stopSendPresenceMessages = !menu->server.stopSendPresenceMessages;
                }
                if (!menu->server.stopSendPresenceMessages)
                {
                    menu->server.presenceTimer -= GlobalAbsDeltaTime;
                    if (menu->server.presenceTimer < 0.0f)
                    {
                        menu->server.presenceTimer = PRESENCE_MESSAGE_TIMEOUT;
                        ServerPresenceMessage message = {};
                        auto result = NetSend(menu->server.connectionSocket, (void*)(&message), sizeof(message), {});
                        SOKO_LOG_SERVER_SEND(result, ServerPresenceMessage);
                    }
                }
                menu->server.connectionTimer -= GlobalAbsDeltaTime;
                if (menu->server.connectionTimer < 0.0f)
                {
                    SOKO_INFO("Server: connection timeout");
                    ServerDisconnectPlayer(&menu->server);
                    SetLevelAvailability(&menu->levelCache, true);
                    ResetLevelIndexToValid(&menu->levelCache, true);
                }
            }

            menu->state = nextState;

            if (menu->state != MainMenu_ConfigureServer)
            {
                menu->levelCache = {};
                EndTemporaryMemory(tempArena);
                if (menu->state != MainMenu_ServerLoadLevel)
                {
                    ShutdownServer(&menu->server);
                }
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
                Socket socket = NetCreateSocket(SocketType_TCP);
                if (socket.handle)
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
                        menu->client.socket = socket;
                        menu->state = MainMenu_ClientWaitForConnection;
                        menu->client.connectionTimeout = CONNECTION_ESTABLISH_TIMEOUT;
                    }
                }
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Return", ImVec2(100, 20)))
        {
            menu->state = MainMenu_ModeSelection;
        }
    }

    internal void
    MenuWaitForConnection(GameMenu* menu)
    {
        if (menu->client.connectionTimeout > 0.0f)
        {
            menu->client.connectionTimeout -= GlobalAbsDeltaTime;
            ImGui::Text("Connecting...");
            NetAddress address;
            address.ip = menu->clientConf.serverAddress.ip;
            address.port = menu->clientConf.serverAddress.port;
            auto connectionResult = NetConnect(menu->client.socket, address);
            switch (connectionResult)
            {
            case ConnectionStatus_Connected: { menu->state = MainMenu_ClientSessionLobby; } break;
                // TODO: Show error
            case ConnectionStatus_Error: { NetCloseSocket(menu->client.socket); menu->client.socket = {}; menu->state = MainMenu_ConnectionError;} break;
            default: {} break;
            }
        }
        else
        {
            menu->state = MainMenu_ConnectionError;
        }
    }

    internal void
    MenuConnectionError(GameMenu* menu)
    {
        ImGui::Text("Connection error.");
        if (ImGui::Button("Back"))
        {
            menu->state = MainMenu_ConnectToServer;
        }
    }

    internal void
    ClientLoadLevel(GameMenu* menu, GameState* gameState)
    {
        MainMenuState nextState = MainMenu_ClientSessionLobby;
        uptr arenaSize = CalcLevelArenaSize(&menu->levelMetaInfo, ENTITY_MEMORY_SIZE_FOR_LEVEL);
        MemoryArena* levelArena = PLATFORM_QUERY_NEW_ARENA(arenaSize);
        SOKO_ASSERT(levelArena);

        SOKO_ASSERT(!gameState->tempArena->isTmpAllocated);
        BeginTemporaryMemory(gameState->tempArena);
        FillLevelCache(&menu->levelCache, gameState->tempArena);
        EndTemporaryMemory(gameState->tempArena);

        bool foundLevel = false;
        for (u32 i = 0; i < menu->levelCache.dirScanResult.count; i++)
        {
            if (menu->levelCache.GUIDs[i] == menu->client.levelGUID)
            {
                wchar_t* levelName = menu->levelCache.dirScanResult.filenames[i];
                wcscpy_s(menu->wLevelPathBuffer, LEVEL_PATH_BUFFER_SIZE, levelName);
                foundLevel = true;
                break;
            }
        }

        if (foundLevel)
        {
            // TODO: Maybe stop using gameState's temp arena for that
            Level* level = InitializeLevel(menu->wLevelPathBuffer, levelArena, gameState->tempArena);
            if (level)
            {
                menu->session.sessionArena = levelArena;
                menu->session.level = level;
                bool p1Added = AddPlayer(&menu->session, level->firstPlayerSpawnPos, PlayerSlot_First);
                SOKO_ASSERT(p1Added);

                SOKO_ASSERT(level->hasSecondPlayer);
                // TODO: Add connected player
                bool p2Added = AddPlayer(&menu->session, level->secondPlayerSpawnPos, PlayerSlot_Second);
                SOKO_ASSERT(p2Added);

                nextState = MainMenu_EnterLevel;
            }
            else
            {
                PLATFORM_FREE_ARENA(levelArena);
                menu->client.levelGUID = 0;
            }
        }
        menu->state = nextState;
        menu->levelCache = {};
    }

    internal void
    MenuClientSessionLobby(GameMenu* menu, GameState* gameState)
    {
        // NOTE: Showing players
        ImGui::Separator();
        ImGui::Text("Players:");
        ImGui::Text("Player 1:\t %s", gameState->playerName);
        ImGui::Text("Player 2:\t %s", menu->client.connectedPlayerName);

        if (!menu->client.connectionEstablished)
        {
            menu->client.connectionEstablished = true;

            NetSendResult connectMsgResult = {};

            ClientConnectMessage msg = {};
            msg.header.type = NetMessageHeader::ClientConnectMessage;
            strcpy(msg.playerName, gameState->playerName);
            msg.header.messageSize = sizeof(ClientConnectMessage);
            connectMsgResult = NetSend(menu->client.socket, (void*)(&msg), sizeof(msg), {});
            SOKO_LOG_CLIENT_SEND(connectMsgResult, ClientConnectMessage);

            if (!connectMsgResult.status)
            {
                NetCloseSocket(menu->client.socket);
                menu->client.connectionEstablished = false;
                menu->state = MainMenu_ConnectToServer;
            }

            menu->client.presenceTimer = PRESENCE_MESSAGE_TIMEOUT;
            menu->client.connectionTimer = CONNECTION_TIMEOUT;
        }
        else
        {
            auto recieveResult = BeginReadIncomingMessages(menu->client.socket, &menu->socketBuffer);
            // TODO: Handle errors and normal disconnect differently
            if (recieveResult.status == NetRecieveResult::ConnectionClosed ||
                recieveResult.status == NetRecieveResult::ConnectionReset ||
                recieveResult.status == NetRecieveResult::Error)
            {
                ClientDisconnectFromServer(&menu->client);
                menu->state = MainMenu_ConnectToServer;
                EndReadIncomingMessages(&menu->socketBuffer);
            }
            else if (recieveResult.status == NetRecieveResult::Success)
            {
                NetMessageHeader* header;
                do
                {
                    header = GetNextIncomingMessage(&menu->socketBuffer);
                    if (header)
                    {
                        // TODO: Not reset the timer if message are broken
                        menu->client.connectionTimer = CONNECTION_TIMEOUT;
                        switch (header->type)
                        {
                        case NetMessageHeader::ServerEnterLevelMessage:
                        {
                            ServerEnterLevelMessage* msg = (ServerEnterLevelMessage*)header;
                            menu->client.levelGUID = msg->levelGUID;
                            menu->state = MainMenu_ClientLoadLevel;
                        } break;
                        case NetMessageHeader::ServerPresenceMessage:
                        {
                            SOKO_LOG_CLIENT_RECV(ServerPresenceMessage);
                        } break;
                        // TODO: Ensure that this is the first message recieved
                        case NetMessageHeader::ServerConnectMessage:
                        {
                            SOKO_LOG_CLIENT_RECV(ServerConnectMessage);
                            ServerConnectMessage* msg = (ServerConnectMessage*)header;
                            strcpy(menu->client.connectedPlayerName, msg->playerName);
                        } break;
                        case NetMessageHeader::ServerLevelListQueryMessage:
                        {
                            SOKO_LOG_CLIENT_RECV(ServerLevelListQueryMessage);

                            BeginTemporaryMemory(gameState->tempArena);
                            FillLevelCache(&menu->levelCache, gameState->tempArena);

                            const u32 bufferSize = 1024;
                            byte buffer[bufferSize];
                            ZERO_ARRAY(byte, bufferSize, buffer);

                            ClientLevelListMessage* msg = (ClientLevelListMessage*)buffer;
                            msg->header.type = NetMessageHeader::ClientLevelListMessage;
                            msg->numLevels = menu->levelCache.dirScanResult.count;

                            u32 buffSizeForGUIDs = bufferSize - sizeof(ClientLevelListMessage) + sizeof(u64);
                            u32 numLevelsFit = buffSizeForGUIDs / sizeof(u64);
                            if (numLevelsFit < msg->numLevels)
                            {
                                msg->numLevels = numLevelsFit;
                            }

                            u64* GUIDs = &msg->firstGUID;

                            for (u32 i = 0; i < msg->numLevels; i++)
                            {
                                // TODO: Send only valid guids
                                u64 guid = menu->levelCache.GUIDs[i];
                                GUIDs[i] = guid;
                            }

                            menu->levelCache = {};
                            EndTemporaryMemory(gameState->tempArena);

                            u32 msgSize = sizeof(msg) + ((msg->numLevels - 1) * sizeof(u64));
                            msg->header.messageSize = msgSize;

                            auto levelsMsgResult = NetSend(menu->client.socket, (void*)msg, msgSize, {});
                            SOKO_LOG_CLIENT_SEND(levelsMsgResult, ClientLevelListMessage);
                        } break;
                        default: { SOKO_INFO("Client: recieved broken message"); } break;
                        }

                    }
                }
                while (header);
            }

            EndReadIncomingMessages(&menu->socketBuffer);

            // NOTE: Send presence message
            // TODO: For debug only!!!
            if (JustPressed(KEY_P))
            {
                menu->client.stopSendPresenceMessages = !menu->client.stopSendPresenceMessages;
            }
            if (!menu->client.stopSendPresenceMessages)
            {
                menu->client.presenceTimer -= GlobalAbsDeltaTime;
                if (menu->client.presenceTimer < 0.0f)
                {
                    menu->client.presenceTimer = PRESENCE_MESSAGE_TIMEOUT;
                    ClientPresenceMessage message = {};
                    auto result = NetSend(menu->client.socket, (void*)(&message), sizeof(message), {});
                    SOKO_LOG_CLIENT_SEND(result, ClientPresenceMessage);
                }
            }

            menu->client.connectionTimer -= GlobalAbsDeltaTime;
            if (menu->client.connectionTimer < 0.0f)
            {
                SOKO_INFO("Client: connection timeout");
                ClientDisconnectFromServer(&menu->client);
                menu->state = MainMenu_ConnectToServer;
            }

        }

        ImGui::Text("Connected to server");
        if (ImGui::Button("Disconnect"))
        {
            ClientDisconnectFromServer(&menu->client);
            menu->state = MainMenu_ConnectToServer;
        }

    }

    internal void
    MenuEnterLevel(GameMenu* menu, GameState* gameState)
    {
        if (menu->session.gameMode == GAME_MODE_SERVER)
        {
            menu->session.server = &menu->server;
            menu->session.socketBuffer = &menu->socketBuffer;
        }
        else if (menu->session.gameMode == GAME_MODE_CLIENT)
        {
            menu->session.client = &menu->client;
            menu->session.socketBuffer = &menu->socketBuffer;
        }

        gameState->globalGameMode = menu->session.gameMode;
        gameState->session = menu->session;
        menu->session = {};
        menu->state = MainMenu_ModeSelection;

        gameState->session.level->session = &gameState->session;

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
        case MainMenu_ServerLoadLevel: { ServerLoadLevel(menu, gameState); } break;
        case MainMenu_ConnectToServer: { MenuConnectToServer(menu); } break;
        case MainMenu_ClientWaitForConnection: { MenuWaitForConnection(menu); } break;
        case MainMenu_ClientSessionLobby: { MenuClientSessionLobby(menu, gameState); } break;
        case MainMenu_ClientLoadLevel : { ClientLoadLevel(menu, gameState); } break;
        case MainMenu_ConnectionError: { MenuConnectionError(menu); } break;
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
