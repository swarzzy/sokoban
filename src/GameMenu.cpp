namespace soko
{
    internal void
    MenuCleanup(GameMenu* menu, MainMenuState nextState)
    {
        ZERO_FROM_MEMBER(GameMenu, levelPathBuffer, menu);
        menu->session = {};
        COPY_BYTES(StrSize(DEFAULT_LEVEL_NAME), menu->levelPathBuffer, DEFAULT_LEVEL_NAME);
        COPY_BYTES(StrSize(DEFAULT_LEVEL_NAME_W), menu->wLevelPathBuffer, DEFAULT_LEVEL_NAME_W);
        menu->state = nextState;
    }

    internal void
    MenuModeSelection(GameMenu* menu)
    {
        // TODO: No need for full cleanup here
        MenuCleanup(menu, MainMenu_ModeSelection);
        if(ImGui::Button("Editor", ImVec2(100, 60))) { menu->state = MainMenu_EditorConf; menu->session.gameMode = GAME_MODE_EDITOR; };
        if(ImGui::Button("Game", ImVec2(100, 60))) { menu->state = MainMenu_SingleSelectLevel; menu->session.gameMode = GAME_MODE_SINGLE; };
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
        BeginTemporaryMemory(gameState->tempArena);
        Level* level = LoadLevel(menu->wLevelPathBuffer, levelArena, gameState->tempArena);
        EndTemporaryMemory(gameState->tempArena);
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
        gameState->session.player = 0;

        gameState->session.editorCamera = PUSH_STRUCT(gameState->session.sessionArena, EditorCamera);
        gameState->session.editor = PUSH_STRUCT(gameState->session.sessionArena, Editor);
        SOKO_ASSERT(gameState->session.editorCamera);
        SOKO_ASSERT(gameState->session.editor);
        EditorInit(gameState);
        EditorCameraInit(gameState->session.editorCamera);
        RenderGroupSetCamera(gameState->renderGroup, &gameState->session.editorCamera->conf);
    }

    inline void
    ResetLevelIndexToValid(MenuLevelCache* levelCache)
    {
        levelCache->selectedIndex = 0;
        for (u32 i = 0; i < levelCache->dirScanResult.count; i++)
        {
            bool isLevel = levelCache->isLevel[i];
            bool valid = isLevel;
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

        for (u32 i = 0; i < levelCache->dirScanResult.count; i++)
        {
            LevelMetaInfo info = {};
            b32 isLevel = GetLevelMetaInfo(levelCache->dirScanResult.filenames[i], &info);

            levelCache->isLevel[i] = isLevel;
            levelCache->GUIDs[i] = info.guid;
        }
        ResetLevelIndexToValid(levelCache);
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
        BeginTemporaryMemory(gameState->tempArena);
        Level* level = LoadLevel(menu->wLevelPathBuffer, levelArena, gameState->tempArena);
        EndTemporaryMemory(gameState->tempArena);
        if (level)
        {
            menu->session.sessionArena = levelArena;
            menu->session.level = level;
            bool pAdded = AddPlayer(&menu->session, level->playerSpawnPos);
            // TODO: This assert should never happen
            SOKO_ASSERT(pAdded);
            nextState = MainMenu_EnterLevel;
        }
        else
        {
            PLATFORM_FREE_ARENA(levelArena);
        }
        menu->state = nextState;
    }

    internal void
    MenuEnterLevel(GameMenu* menu, GameState* gameState)
    {
        gameState->globalGameMode = menu->session.gameMode;
        gameState->session = menu->session;
        menu->session = {};
        menu->state = MainMenu_ModeSelection;

        gameState->session.level->session = &gameState->session;

        InitCameras(&gameState->session.camera, MakeWorldPos(gameState->session.player->pos), &gameState->session.debugCamera);
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
