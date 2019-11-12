namespace soko
{
    enum ToolKind
    {
        Tool_None,
        Tool_TilePicker,
        Tool_TilePlacer,
        Tool_TileEraser,
        Tool_EntityPicker,
        Tool_EntityPlacer,
    };

    constant u32 EDITOR_UI_LEVEL_NAME_SIZE = 256;

    struct EditorUI
    {
        b32 entityListerOpened;
        b32 entityViewOpened;
        b32 tileViewOpened;
        b32 windowOpened;
        b32 chunkListerOpened;
        b32 exitToMainMenu;
        b32 levelSettingsOpened;
        b32 wantsToSaveLevel;
        b32 saveAs;
        i32 chunkListerZLevel;
        char levelName[EDITOR_UI_LEVEL_NAME_SIZE];
        wchar_t wLevelName[EDITOR_UI_LEVEL_NAME_SIZE];
    };

    struct Editor
    {
        GameSession* session;
        GameState* gameState;
        ToolKind tool;
        EditorUI ui;
        // NOTE: Camera should not gather input if mouse used
        b32 mouseUsed;
        b32 selectorHolding;
        iv3 selectorBegin;
        iv3 selectorEnd;
        b32 entityPickerHolding;
        iv3 entityPickerTile;
        TileValue placerTile;
        b32 tilePlacerDeleteMode;
        i32 selectedEntityID;
        b32 showChunkBounds;
        // NOTE: Changes every frame
        SimRegion* region;
    };

    struct EditorCamera
    {
        CameraConfig conf;
        f32 longitude;
        f32 latitude;
        f32 distance;
        v2 targetOrbit;
        f32 targetDistance;
        f32 rotSpeed;
        f32 zoomSpeed;
        f32 latSmooth;
        f32 longSmooth;
        f32 distSmooth;
        f32 moveSpeed;
        f32 moveFriction;
        v3 velocity;
        // NOTE: Position of actual camera
        WorldPos worldPos;
        // NOTE: Position of a point on camera looks
        WorldPos targetWorldPos;
        v3 frameAcceleration;
        i32 frameScrollOffset;

        v3 mouseRayRH;
    };

    internal void
    EditorCameraInit(EditorCamera* camera)
    {
        CameraConfig conf = {};
        conf.position = V3(0.0f);
        conf.front = V3(0.0f, 0.0f, -1.0f);
        conf.fovDeg = 45.0f;
        conf.aspectRatio = 16.0f / 9.0f;
        conf.nearPlane = 0.1f;
        conf.farPlane = 100.0f;

        camera->conf = conf;
        camera->longSmooth = 30.0f;
        camera->latSmooth = 30.0f;
        camera->distSmooth = 30.0f;
        camera->rotSpeed = 1000.0f;
        camera->zoomSpeed = 200.0f;
        camera->moveSpeed = 500.0f;
        camera->moveFriction = 10.0f;
    }

    internal void
    EditorCameraGatherInput(EditorCamera* camera)
    {
        v3 frontDir = V3(camera->conf.front.x, 0.0f, camera->conf.front.z);
        v3 rightDir = Cross(V3(0.0f, 1.0f, 0.0f), frontDir);
        rightDir = Normalize(rightDir);

        // TODO: Why they are flipped?
        camera->frameAcceleration = {};
        if (GlobalInput.keys[AB::KEY_W].pressedNow)
        {
            camera->frameAcceleration += frontDir;
        }
        if (GlobalInput.keys[AB::KEY_S].pressedNow)
        {
            camera->frameAcceleration -= frontDir;
        }
        if (GlobalInput.keys[AB::KEY_A].pressedNow)
        {
            camera->frameAcceleration += rightDir;
        }
        if (GlobalInput.keys[AB::KEY_D].pressedNow)
        {
            camera->frameAcceleration -= rightDir;
        }

        camera->frameAcceleration = RHToWorld(camera->frameAcceleration);

        if (GlobalInput.mouseButtons[AB::MBUTTON_MIDDLE].pressedNow)
        {
            v2 mousePos;
            f32 speed = camera->rotSpeed;
            mousePos.x = GlobalInput.mouseFrameOffsetX * speed;
            mousePos.y = GlobalInput.mouseFrameOffsetY * speed;
            camera->targetOrbit.x += mousePos.x;
            camera->targetOrbit.y -= mousePos.y;
        }

        camera->frameScrollOffset = GlobalInput.scrollFrameOffset;
    }

    internal void
    EditorCameraUpdate(EditorCamera* camera)
    {
        auto acceleration = camera->frameAcceleration;
        acceleration = Normalize(acceleration);
        acceleration *= camera->moveSpeed;

        f32 friction = camera->moveFriction;
        acceleration = acceleration - camera->velocity * friction;

        v3 movementDelta;
        movementDelta = 0.5f * acceleration *
            Square(GlobalAbsDeltaTime) +
            camera->velocity *
            GlobalAbsDeltaTime;

        camera->targetWorldPos = OffsetWorldPos(camera->targetWorldPos, movementDelta);
        camera->velocity += acceleration * GlobalAbsDeltaTime;
        camera->frameAcceleration = {};

        camera->targetDistance -= camera->frameScrollOffset * camera->zoomSpeed * GlobalAbsDeltaTime;
        camera->frameScrollOffset = {};

        if (camera->targetDistance < 5.0f)
        {
            camera->targetDistance = 5.0f;
        }
        else if (camera->targetDistance > 50.0f)
        {
            camera->targetDistance = 50.0f;
        }

        if (camera->targetOrbit.y < 95.0f)
        {
            camera->targetOrbit.y = 95.0f;
        }
        else if (camera->targetOrbit.y > 170.0f)
        {
            camera->targetOrbit.y = 170.0f;
        }

        camera->latitude = Lerp(camera->latitude, camera->targetOrbit.y,
                                GlobalAbsDeltaTime * camera->latSmooth);
        //DEBUG_OVERLAY_TRACE(camera->latitude);

        camera->longitude = Lerp(camera->longitude, camera->targetOrbit.x,
                                 GlobalAbsDeltaTime * camera->longSmooth);

        camera->distance = Lerp(camera->distance, camera->targetDistance,
                                GlobalAbsDeltaTime * camera->distSmooth);

        f32 latitude = ToRadians(camera->latitude);
        f32 longitude = ToRadians(camera->longitude);
        f32 polarAngle = PI_32 - latitude;

        f32 x = camera->distance * Sin(polarAngle) * Cos(longitude);
        f32 y = camera->distance * Cos(polarAngle);
        f32 z = camera->distance * Sin(polarAngle) * Sin(longitude);

        camera->conf.position = V3(x, y, z);
        camera->conf.front = -Normalize(camera->conf.position);

        camera->worldPos = OffsetWorldPos(camera->targetWorldPos, RHToWorld(V3(x, y, z)));

        v2 normMousePos;
        normMousePos.x = 2.0f * GlobalInput.mouseX - 1.0f;
        normMousePos.y = 2.0f * GlobalInput.mouseY - 1.0f;
        //DEBUG_OVERLAY_TRACE(normMousePos.x);
        //DEBUG_OVERLAY_TRACE(normMousePos.y);

        v4 mouseClip = V4(normMousePos, -1.0f, 0.0f);

        // TODO: IMPORTANT: @Speed: Extreme sloooooowness here!!!
        // Do this in renderer or in some other place
        // which has all this matrices
        m4x4 lookAt = LookAtDirRH(camera->conf.position, camera->conf.front, V3(0.0f, 1.0f, 0.0f));
        m4x4 proj = PerspectiveOpenGLRH(camera->conf.fovDeg, camera->conf.aspectRatio,
                                        camera->conf.nearPlane, camera->conf.farPlane);
        m4x4 invLookAt = lookAt;
        bool inv = Inverse(&invLookAt);
        SOKO_ASSERT(inv);
        m4x4 invProj = proj;
        inv = Inverse(&invProj);
        SOKO_ASSERT(inv);

        v4 mouseView = MulM4V4(invProj, mouseClip);
        mouseView = V4(mouseView.xy, -1.0f, 0.0f);
        v3 mouseWorld = MulM4V4(invLookAt, mouseView).xyz;
        mouseWorld = Normalize(mouseWorld);
        camera->mouseRayRH = mouseWorld;
    }

    inline bool
    IsMouseCapturedByUI()
    {
        ImGuiIO* io = &ImGui::GetIO();
        bool result = io->WantCaptureMouse;
        return result;
    }

    inline bool
    IsKeyboradCapturedByUI()
    {
        ImGuiIO* io = &ImGui::GetIO();
        bool result = io->WantCaptureKeyboard;
        return result;
    }

    inline void
    EditorAddAnchorIfChunkEmpty(Chunk* chunk)
    {
        // TODO: Add anchor only if the whole world is empty
        if (!chunk->filledTileCount)
        {
            uv3 center = UV3(CHUNK_DIM / 2, CHUNK_DIM / 2, 0);
            SetTileInChunk(chunk, center, TileValue_Wall);
        }
    }

    internal void
    EditorInit(GameState* gameState)
    {
        GameSession* session = &gameState->session;
        Editor* editor = session->editor;

        editor->session = session;
        editor->gameState = gameState;
        editor->placerTile = TileValue_Wall;

        Chunk* chunk = GetChunk(session->level, 0, 0, 0);
        if (!chunk)
        {
            chunk = AddChunk(session->level, 0, 0, 0);
            SOKO_ASSERT(chunk);
        }
    }

    inline void
    DrawToolBox(Editor* editor)
    {
        f32 offset = 10.0f;
        ImGuiIO* io = &ImGui::GetIO();
        ImVec2 windowPos = ImVec2(offset, io->DisplaySize.y - offset);
        ImVec2 windowPosPivot = ImVec2(0.0f, 1.0f);
        ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, windowPosPivot);
        auto windowFlags =
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse;
        bool opened = true;
        if (ImGui::Begin("Tools window", &opened, windowFlags))
        {
            if (editor->tool == Tool_TilePlacer)
            {
                ImGui::PushID("Tile picker listbox");
                int tileValue = editor->placerTile;
                ImGui::ListBox("", &tileValue, TypeInfo(TileValue).names, TypeTraits(TileValue)::MemberCount);
                editor->placerTile = (TileValue)(tileValue);
                ImGui::PopID();
                if (ImGui::Button("Go back"))
                {
                    editor->tool = Tool_None;
                }
            }
            else
            {
                ImGui::Text("Tools");
                ImGui::Separator();
                if (ImGui::Selectable("None", editor->tool == Tool_None)) editor->tool = Tool_None;
                if (ImGui::Selectable("Tile picker", editor->tool == Tool_TilePicker)) editor->tool = Tool_TilePicker;
                if (ImGui::Selectable("Tile placer", editor->tool == Tool_TilePlacer))editor->tool = Tool_TilePlacer;
                if (ImGui::Selectable("Tile eraser", editor->tool == Tool_TileEraser))editor->tool = Tool_TileEraser;
                if (ImGui::Selectable("Entity picker", editor->tool == Tool_EntityPicker))editor->tool = Tool_EntityPicker;
                if (ImGui::Selectable("Entity placer", editor->tool == Tool_EntityPlacer))editor->tool = Tool_EntityPlacer;
            }
        }
        ImGui::End();
    }

    internal void
    EditorLevelSettings(Editor* editor)
    {
        ImGuiIO* io = &ImGui::GetIO();
        ImGui::SetNextWindowSize({400, 150});
        auto windowFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize;
        if (ImGui::Begin("Level settings", (bool*)&editor->ui.levelSettingsOpened, windowFlags))
        {
            char string[32];
            FormatString(string, 32, "id: %u32", editor->session->level->spawnerID);
            if (ImGui::BeginCombo("Spawner entity", string))
            {
                for (u32 i = 0 ; i < LEVEL_ENTITY_TABLE_SIZE; i++)
                {
                    Entity* e = editor->session->level->entities[i];
                    while (e)
                    {
                        if (e->type == EntityType_Spawner)
                        {
                            bool wasSelected = (e->id == editor->selectedEntityID);
                            FormatString(string, 32, "id: %u32", e->id);
                            bool selected = ImGui::Selectable(string, wasSelected);
                            if (selected)
                            {
                                editor->session->level->spawnerID = e->id;
                            }
                        }
                        e = e->nextEntity;
                    }
                }

                ImGui::EndCombo();
            }

        }
        ImGui::End();
    }

    internal void
    EditorEntityView(Editor* editor)
    {
        ImGuiIO* io = &ImGui::GetIO();
        ImGui::SetNextWindowSize({300, 600});
        auto windowFlags =
            ImGuiWindowFlags_NoResize;
        ImGuiWindowFlags_AlwaysAutoResize;
        if (ImGui::Begin("Entity view", (bool*)&editor->ui.entityViewOpened, windowFlags))
        {
            if (!editor->selectedEntityID)
            {
                ImGui::Text("Entity not selected");
            }
            else
            {
                SimEntity* entity = GetEntity(editor->region, editor->selectedEntityID);

                char buffer[16];
                FormatString(buffer, 16, "%i32", editor->selectedEntityID);
                ImGui::Text("ID %s", buffer);
                ImGui::SameLine();
                if (ImGui::Button("Delete"))
                {
                    editor->selectedEntityID = 0;
                    DeleteEntity(editor->session->level, entity->stored);
                }
                else
                {
                    ImGui::Separator();

                    ImGui::Text("Position");
                    WorldPos pos = GetWorldPos(editor->region->origin, entity->pos);
                    //ImGui::SameLine();
                    ImGui::PushID("Entity position drag");
                    // TODO: Bounds of sim region
                    ImGui::InputInt("x", &pos.tile.x);
                    ImGui::InputInt("y", &pos.tile.y);
                    ImGui::InputInt("z", &pos.tile.z);


                    v3 newPos = GetRelPos(editor->region->origin, pos);
                    entity->pos = newPos;
                    ImGui::PopID();
                    ImGui::Separator();

                    ImGui::Text("Type\t ");
                    i32 type = entity->stored->type;
                    ImGui::PushID("Entity type listbox");
                    ImGui::Combo("", &type, TypeInfo(EntityType).names, TypeTraits(EntityType)::MemberCount);
                    entity->stored->type = (EntityType)type;
                    ImGui::PopID();
                    ImGui::Separator();

                    ImGui::Text("Entity flags");
                    if (ImGui::CollapsingHeader("Flags"))
                    {
                        for (u32 i = 0; i < TypeTraits(EntityFlags)::MemberCount; i++)
                        {
                            bool isSet = IsSet(entity->stored, TypeInfo(EntityFlags).values[i]);
                            ImGui::Checkbox(TypeInfo(EntityFlags).names[i], &isSet);
                            if (isSet)
                            {
                                SetFlag(entity->stored, TypeInfo(EntityFlags).values[i]);
                            }
                            else
                            {
                                UnsetFlag(entity->stored, TypeInfo(EntityFlags).values[i]);
                            }
                        }
                    }
                    ImGui::Separator();

                    ImGui::Text("Mesh");
                    i32 mesh = entity->stored->mesh;
                    ImGui::PushID("Entity mesh listbox");
                    ImGui::Combo("", &mesh, TypeInfo(EntityMesh).names, TypeTraits(EntityMesh)::MemberCount);
                    entity->stored->mesh = (EntityMesh)mesh;
                    ImGui::PopID();
                    ImGui::Separator();

                    ImGui::Text("Material");
                    i32 material = entity->stored->material;
                    ImGui::PushID("Entity material listbox");
                    ImGui::Combo("", &material, TypeInfo(EntityMaterial).names, TypeTraits(EntityMaterial)::MemberCount);
                    entity->stored->material = (EntityMaterial)material;
                    ImGui::PopID();
                    ImGui::Separator();

                    ImGui::Text("Material properties");
                    ImGui::ColorEdit3("albedo", entity->stored->materialAlbedo.data);
                    ImGui::SliderFloat("roughness", &entity->stored->materialRoughness, 0.0f, 1.0f);
                    ImGui::SliderFloat("metallic", &entity->stored->materialMetallic, 0.0f, 1.0f);
                }
            }
        }
        ImGui::End();
    }

    internal u32
    EditorEntityLister(Editor* editor)
    {
        u32 selectedEntity = editor->selectedEntityID;
        ImGuiIO* io = &ImGui::GetIO();
        ImGui::SetNextWindowSize({200, 400});
        auto windowFlags =
            ImGuiWindowFlags_NoResize;
        ImGuiWindowFlags_AlwaysAutoResize;
        if (ImGui::Begin("Entity lister", (bool*)&editor->ui.entityListerOpened, windowFlags))
        {
            ImGui::BeginChild("Entity lister list");
            for (u32 i = 0 ; i < LEVEL_ENTITY_TABLE_SIZE; i++)
            {
                Entity* e = editor->session->level->entities[i];
                while (e)
                {
                    bool wasSelected = (e->id == editor->selectedEntityID);
                    char buffer[128];
                    FormatString(buffer, 128, "ID: %u32, %s", e->id,  EnumeratorName(e->type));
                    bool selected = ImGui::Selectable(buffer, wasSelected);
                    if (selected)
                    {
                        selectedEntity = e->id;
                    }
                    e = e->nextEntity;
                }
            }
            ImGui::EndChild();
        }
        ImGui::End();
        return selectedEntity;
    }

    internal void
    EditorChunkLister(Editor* editor)
    {
        ImGuiIO* io = &ImGui::GetIO();
        ImGui::SetNextWindowSize({230, 300});
        auto windowFlags =
            ImGuiWindowFlags_AlwaysAutoResize;
        if (ImGui::Begin("Chunk lister", (bool*)&editor->ui.chunkListerOpened, windowFlags))
        {
#if 1
            ImGui::BeginChild("Chunk lister list");
            for (i32 z = LEVEL_MIN_DIM_CHUNKS; z <= LEVEL_MAX_DIM_CHUNKS; z++)
            {
                for (i32 y = LEVEL_MIN_DIM_CHUNKS; y <= LEVEL_MAX_DIM_CHUNKS; y++)
                {
                    for (i32 x = LEVEL_MIN_DIM_CHUNKS; x <= LEVEL_MAX_DIM_CHUNKS; x++)
                    {
                        Chunk* chunk = GetChunk(editor->session->level, x, y, z);
                        char buffer[64];
                        FormatString(buffer, 64, "(x: %i32, y: %i32, z %i32)", x, y, z);
                        if (!chunk)
                        {
                            ImGui::PushID(buffer);
                            if (ImGui::Button("Add"))
                            {
                                Chunk* chunk = AddChunk(editor->session->level, x, y, z);
                                SOKO_ASSERT(chunk);
                                // TODO: Store meshes on the cpu and load on the gpu only when necessary
                                ChunkMesh mesh = {};
                                if (GenChunkMesh(editor->session->level, chunk, &mesh))
                                {
                                    // TODO: Check if loading failed
                                    LoadedChunkMesh loadedMesh = RendererLoadChunkMesh(&mesh);
                                    chunk->loadedMesh = loadedMesh;
                                    chunk->mesh = mesh;
                                }

                            }
                            ImGui::PopID();
                            ImGui::SameLine();
                        }
                        ImGui::Text("%s", buffer);
                    }
                }
            }
            ImGui::EndChild();
#else
            // TODO: Propper font scaling
            //ImGui::PushFont(editor->gameState->notoMonoFont10px);
            ImGui::SliderInt("Z level", &editor->ui.chunkListerZLevel, LEVEL_MIN_DIM_CHUNKS, LEVEL_MAX_DIM_CHUNKS);

            for (i32 y = LEVEL_MAX_DIM_CHUNKS; y >= LEVEL_MIN_DIM_CHUNKS; y--)
            {
                for (i32 x = LEVEL_MIN_DIM_CHUNKS; x <= LEVEL_MAX_DIM_CHUNKS; x++)
                {
                    Chunk* chunk = GetChunk(editor->session->level, x, y, editor->ui.chunkListerZLevel);
                    char string[32];
                    FormatString(string, 32, "(%i32, %i32)", x, y);
                    ImGui::Button(string, ImVec2(40.0f, 40.0f));
                    ImGui::SameLine();
                }
                ImGui::NewLine();
            }
            //ImGui::PushFont(0);
#endif
        }
        ImGui::End();
    }

    internal void
    EditorTileView(Editor* editor)
    {
        f32 offset = 10.0f;
        ImGuiIO* io = &ImGui::GetIO();
        ImVec2 windowPos = ImVec2(io->DisplaySize.x - offset, offset + 70.0f);
        ImVec2 windowPosPivot = ImVec2(1.0f, 0.0f);
        ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, windowPosPivot);
        auto windowFlags =
            ImGuiWindowFlags_NoResize;
        ImGuiWindowFlags_AlwaysAutoResize;
        if (ImGui::Begin("Tile properties", (bool*)&editor->ui.tileViewOpened, windowFlags))
        {
            Tile tile = GetTile(editor->session->level, editor->selectorBegin);
            ImGui::Separator();
            char buffer[128];
            iv3 pos = editor->selectorBegin;
            FormatString(buffer, 128, "Position: (x: %i32; y: %i32; z: %i32)", pos.x, pos.y, pos.z);
            ImGui::Text("%s", buffer);
            ImGui::Separator();
            ImGui::Text("Tile type:");
            ImGui::SameLine();
            ImGui::PushID("Tile combo box");
            i32 tileValue = (i32)tile.value - 1;
            ImGui::Combo("", &tileValue, TypeInfo(TileValue).names, TypeTraits(TileValue)::MemberCount);
            ImGui::PopID();
            SetTile(editor->session->level, editor->selectorBegin, (TileValue)(tileValue + 1));
        }
        ImGui::End();
    }

    internal void
    EditorSaveAs(Editor* editor)
    {
        if (!ImGui::IsPopupOpen("Save as"))
        {
            ImGui::OpenPopup("Save as");
        }
        if (ImGui::BeginPopupModal("Save as", 0, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::PushID("Save as filename input");
            if (ImGui::InputText("", editor->ui.levelName, EDITOR_UI_LEVEL_NAME_SIZE))
            {
                mbstowcs(editor->ui.wLevelName, editor->ui.levelName, EDITOR_UI_LEVEL_NAME_SIZE);
            }
            ImGui::PopID();

            if (ImGui::Button("Save"))
            {
                bool fileNotExit = !DebugGetFileSize(editor->ui.wLevelName);
                // TODO: Show message if spawner is not selected
                if (fileNotExit && editor->session->level->spawnerID)
                {
                    editor->ui.wantsToSaveLevel = true;
                    editor->ui.saveAs = false;
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::SameLine();

            if (ImGui::Button("Close"))
            {
                editor->ui.saveAs = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    internal void
    EditorDrawUI(Editor* editor)
    {
        //local_persist bool show = false;
        //ImGui::ShowDemoWindow(&show);

        ImGui::BeginMainMenuBar();
        if (ImGui::BeginMenu("Level"))
        {
            if (ImGui::MenuItem("Save as"))
            {
                editor->ui.saveAs = true;
            }
            if (ImGui::MenuItem("Exit to main menu"))
            {
                editor->ui.exitToMainMenu = true;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Windows"))
        {
            if (ImGui::MenuItem("Entity lister", 0, editor->ui.entityListerOpened))
            {
                editor->ui.entityListerOpened = !editor->ui.entityListerOpened;
            }
            if (ImGui::MenuItem("Entity view", 0, editor->ui.entityViewOpened))
            {
                editor->ui.entityViewOpened = !editor->ui.entityViewOpened;
            }
            if (ImGui::MenuItem("Tile view", 0, editor->ui.tileViewOpened))
            {
                editor->ui.tileViewOpened = !editor->ui.tileViewOpened;
            }
            if (ImGui::MenuItem("Chunk lister", 0, editor->ui.chunkListerOpened))
            {
                editor->ui.chunkListerOpened = !editor->ui.chunkListerOpened;
            }
            if (ImGui::MenuItem("Level settings", 0, editor->ui.levelSettingsOpened))
            {
                editor->ui.levelSettingsOpened = !editor->ui.levelSettingsOpened;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Debug"))
        {
            if (ImGui::MenuItem("Show chunk bounds", 0, editor->showChunkBounds))
            {
                editor->showChunkBounds = !editor->showChunkBounds;
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();

        DrawToolBox(editor);

        if (editor->ui.entityViewOpened)
        {
            EditorEntityView(editor);
        }
        if (editor->ui.entityListerOpened)
        {
            editor->selectedEntityID = EditorEntityLister(editor);
        }
        if (editor->ui.tileViewOpened)
        {
            EditorTileView(editor);
        }
        if (editor->ui.chunkListerOpened)
        {
            EditorChunkLister(editor);
        }
        if (editor->ui.saveAs)
        {
            EditorSaveAs(editor);
        }
        if (editor->ui.levelSettingsOpened)
        {
            EditorLevelSettings(editor);
        }
    }

    internal void
    EditorUpdateAndRender(GameState* gameState)
    {
        auto camera = gameState->session.editorCamera;
        auto editor = gameState->session.editor;
        auto level = gameState->session.level;

        DEBUG_OVERLAY_TRACE((u32)IsKeyboradCapturedByUI());
        DEBUG_OVERLAY_TRACE((u32)IsMouseCapturedByUI());
        DEBUG_OVERLAY_TRACE(editor->tilePlacerDeleteMode);
        DEBUG_OVERLAY_TRACE(GlobalInput.mouseButtons[MBUTTON_LEFT].pressedNow);
        DEBUG_OVERLAY_TRACE(GlobalInput.mouseButtons[MBUTTON_RIGHT].pressedNow);

        // TODO: Not here
        if (!IsKeyboradCapturedByUI())
        {
            EditorCameraGatherInput(camera);
            EditorCameraUpdate(camera);
        }

        Chunk* zeroChunk = GetChunk(level, 0, 0, 0);
        EditorAddAnchorIfChunkEmpty(zeroChunk);

        BeginTemporaryMemory(gameState->tempArena, true);
        SimRegion* simRegion = BeginSim(gameState->tempArena,
                                        gameState->session.level,
                                        gameState->session.editorCamera->targetWorldPos,
                                        2);
        editor->region = simRegion;

        i32 actualRadius = simRegion->radius - 1;
        iv3 minBound = GetChunkCoord(simRegion->origin.tile) - actualRadius;
        iv3 maxBound = GetChunkCoord(simRegion->origin.tile) + actualRadius;

        for (i32 chunkZ = minBound.z; chunkZ <= maxBound.z; chunkZ++)
        {
            for (i32 chunkY = minBound.y; chunkY <= maxBound.y; chunkY++)
            {
                for (i32 chunkX = minBound.x; chunkX <= maxBound.x; chunkX++)
                {
                    Chunk* chunk = GetChunk(simRegion->level, chunkX, chunkY, chunkZ);
                    if (chunk)
                    {
                        if (chunk->dirty)
                        {
                            RemeshChunk(level, chunk);
                        }
                    }
                }
            }
        }

        if ((editor->tool != Tool_EntityPicker) &&
            (editor->tool != Tool_EntityPlacer))
        {
            editor->selectedEntityID = 0;
        }

        switch (editor->tool)
        {
        case Tool_EntityPlacer:
        {
            if (JustPressed(MBUTTON_RIGHT) && !IsMouseCapturedByUI())
            {
                v3 from = RHToWorld(camera->conf.position);
                v3 ray = RHToWorld(camera->mouseRayRH);
                auto raycast = Raycast(simRegion, from, ray, Raycast_Entities);
                if (raycast.hit == RaycastResult::Entity)
                {
                    Entity* entity = GetEntity(level, raycast.entity.id);
                    SOKO_ASSERT(entity);
                    DeleteEntity(level, entity);
                    editor->selectedEntityID = 0;
                }
            }
            else
            {
                v3 from = RHToWorld(camera->conf.position);
                v3 ray = RHToWorld(camera->mouseRayRH);
                auto raycast = Raycast(simRegion, from, ray, Raycast_Tilemap);

                if (raycast.hit == RaycastResult::Tile)
                {
                    iv3 hoveredTile = raycast.tile.coord + DirToUnitOffset(raycast.tile.normalDir);
                    editor->selectorBegin = hoveredTile;
                    if (JustPressed(MBUTTON_LEFT) && !IsMouseCapturedByUI())
                    {
                        u32 id = AddEntity(level, EntityType_Block, hoveredTile, 0.0f,
                                           EntityMesh_Cube, EntityMaterial_Block);
                        if (id)
                        {
                            SimEntity* entity = AddEntityToRegion(simRegion, GetEntity(level, id));
                            SOKO_ASSERT(entity);
                            editor->selectedEntityID = id;
                        }
                    }
                }
            }
        } break;
        case Tool_TilePicker:
        {
            if (JustPressed(MBUTTON_LEFT) && !IsMouseCapturedByUI())
            {
                v3 from = RHToWorld(camera->conf.position);
                v3 ray = RHToWorld(camera->mouseRayRH);
                auto raycast = Raycast(simRegion, from, ray, Raycast_Tilemap);

                if (raycast.hit)
                {
                    editor->selectorBegin = raycast.tile.coord;
                }
            }
        } break;
        case Tool_TilePlacer:
        {
            v3 from = RHToWorld(camera->conf.position);
            v3 ray = RHToWorld(camera->mouseRayRH);
            auto raycast = Raycast(simRegion, from, ray, Raycast_Tilemap);

            if (raycast.hit == RaycastResult::Tile)
            {
                if (!IsMouseCapturedByUI() && !editor->selectorHolding)
                {
                    if (JustPressed(MBUTTON_LEFT))
                    {
                        editor->mouseUsed = true;
                        editor->tilePlacerDeleteMode = false;
                        editor->selectorHolding = true;
                        editor->selectorBegin = raycast.tile.coord + DirToUnitOffset(raycast.tile.normalDir);
                    }
                    else if (JustPressed(MBUTTON_RIGHT))
                    {
                        editor->mouseUsed = true;
                        editor->tilePlacerDeleteMode = true;
                        editor->selectorHolding = true;
                        editor->selectorBegin = raycast.tile.coord;
                    }
                }

                if (editor->selectorHolding)
                {
                    editor->selectorEnd = editor->tilePlacerDeleteMode ? raycast.tile.coord : raycast.tile.coord + DirToUnitOffset(raycast.tile.normalDir);
                }
            }
            if (editor->selectorHolding)
            {
                if (JustPressed(MBUTTON_LEFT) && editor->tilePlacerDeleteMode)
                {
                    editor->selectorHolding = false;
                    editor->tilePlacerDeleteMode = false;
                }
                if (JustPressed(MBUTTON_RIGHT) && !editor->tilePlacerDeleteMode)
                {
                    editor->selectorHolding = false;
                    editor->tilePlacerDeleteMode = false;
                }

                if ((JustReleased(MBUTTON_LEFT) || JustReleased(MBUTTON_RIGHT)))
                {
                    editor->selectorHolding = false;

                    TileBox box = TileBoxFromTwoPoints(editor->selectorBegin, editor->selectorEnd);
                    TileValue value = editor->tilePlacerDeleteMode ? TileValue_Empty : editor->placerTile;

                    editor->tilePlacerDeleteMode = false;

                    for (i32 z = box.min.z; z <= box.max.z; z++)
                    {
                        for (i32 y = box.min.y; y <= box.max.y; y++)
                        {
                            for (i32 x = box.min.x; x <= box.max.x; x++)
                            {
                                iv3 c = GetChunkCoord(x, y, z);
                                uv3 t = GetTileCoordInChunk(x, y, z);
                                Chunk* chunk = GetChunk(level, c);
                                if (chunk)
                                {
                                    SetTileInChunk(chunk, t, value);
                                }
                                else
                                {
                                    Chunk* newChunk = AddChunk(level, c);
                                    if (newChunk)
                                    {
                                        SetTileInChunk(newChunk, t, value);
                                    }
                                    else
                                    {
                                        // TODO: This will leak memory for now
                                    }
                                }
                            }
                        }
                    }
                }
            }

        } break;
        case Tool_TileEraser:
        {
            v3 from = RHToWorld(camera->conf.position);
            v3 ray = RHToWorld(camera->mouseRayRH);
            auto raycast = Raycast(simRegion, from, ray, Raycast_Tilemap);

            if (raycast.hit == RaycastResult::Tile)
            {
                iv3 hoveredTile = raycast.tile.coord;
                if (JustPressed(MBUTTON_LEFT) && !IsMouseCapturedByUI())
                {
                    editor->mouseUsed = true;
                    editor->selectorHolding = true;
                    editor->selectorBegin = hoveredTile;
                }

                if (editor->selectorHolding)
                {
                    editor->selectorEnd = hoveredTile;
                }
            }

            if (JustReleased(MBUTTON_LEFT) && editor->selectorHolding)
            {
                editor->selectorHolding = false;

                TileBox box = TileBoxFromTwoPoints(editor->selectorBegin, editor->selectorEnd);

                for (i32 z = box.min.z; z <= box.max.z; z++)
                {
                    for (i32 y = box.min.y; y <= box.max.y; y++)
                    {
                        for (i32 x = box.min.x; x <= box.max.x; x++)
                        {
                            SetTile(level, x, y, z, TileValue_Empty);
                        }
                    }
                }
            }

        }
        case Tool_EntityPicker:
        {
            if (JustPressed(MBUTTON_LEFT) && !IsMouseCapturedByUI())
            {
                v3 from = RHToWorld(camera->conf.position);
                v3 ray = RHToWorld(camera->mouseRayRH);
                auto raycast = Raycast(simRegion, from, ray, Raycast_Entities);

                if (raycast.hit == RaycastResult::Entity)
                {
                    editor->selectedEntityID = raycast.entity.id;
                }
            }
        } break;
        default:
        {
            //editor->selectedTile = {};
        } break;
        }

        RenderGroupSetCamera(gameState->renderGroup, &gameState->session.editorCamera->conf);
        EditorDrawUI(editor);

        if (editor->showChunkBounds)
        {
            for (i32 z = minBound.z; z <= maxBound.z; z++)
            {
                for (i32 y = minBound.y; y <= maxBound.y; y++)
                {
                    for (i32 x = minBound.x; x <= maxBound.x; x++)
                    {
                        Chunk* chunk = GetChunk(simRegion->level, x, y, z);
                        if (chunk)
                        {
                            v3 color = V3(0.0f, 0.0f, 1.0f);
                            WorldPos chunkPos = MakeWorldPos(IV3(x, y, z) * CHUNK_DIM);
                            v3 camOffset = WorldToRH(GetRelPos(camera->targetWorldPos, chunkPos));
                            v3 offset = camOffset;

                            f32 chunkDim = CHUNK_DIM * LEVEL_TILE_SIZE;
                            v3 tileOff = V3(LEVEL_TILE_RADIUS, LEVEL_TILE_RADIUS, -LEVEL_TILE_RADIUS);

                            DrawAlignedBoxOutline(gameState->renderGroup,
                                                  offset - tileOff,
                                                  offset + V3(chunkDim, chunkDim, -chunkDim) - tileOff,
                                                  color, 2.0f);
                        }
                    }
                }
            }
        }

        switch (editor->tool)
        {
        case Tool_EntityPlacer:
        {
            v3 selTileRelPos = GetRelPos(camera->targetWorldPos, editor->selectorBegin);
            DrawAlignedBoxOutline(gameState->renderGroup,
                                  WorldToRH(selTileRelPos - V3(LEVEL_TILE_RADIUS)),
                                  WorldToRH(selTileRelPos + V3(LEVEL_TILE_RADIUS)),
                                  V3(1.0f, 0.0f, 0.0f), 2.0f);
        } break;
        case Tool_TilePlacer:
        {
            if (editor->selectorHolding)
            {
                TileBox box = TileBoxFromTwoPoints(editor->selectorBegin, editor->selectorEnd);
                v3 minRelPos = GetRelPos(camera->targetWorldPos, box.min);
                v3 maxRelPos = GetRelPos(camera->targetWorldPos, box.max);

                DrawAlignedBoxOutline(gameState->renderGroup,
                                      WorldToRH(minRelPos - V3(LEVEL_TILE_RADIUS)),
                                      WorldToRH(maxRelPos + V3(LEVEL_TILE_RADIUS)),
                                      V3(0.0f, 1.0f, 0.0f), 2.0f);
            }
        } break;
        case Tool_TilePicker:
        {
            v3 selTileRelPos = GetRelPos(camera->targetWorldPos, editor->selectorBegin);
            DrawAlignedBoxOutline(gameState->renderGroup,
                                  WorldToRH(selTileRelPos - V3(LEVEL_TILE_RADIUS)),
                                  WorldToRH(selTileRelPos + V3(LEVEL_TILE_RADIUS)),
                                  V3(1.0f, 0.0f, 0.0f), 2.0f);
        } break;
        case Tool_TileEraser:
        {
            if (editor->selectorHolding)
            {
                TileBox box = TileBoxFromTwoPoints(editor->selectorBegin, editor->selectorEnd);
                v3 minRelPos = GetRelPos(camera->targetWorldPos, box.min);
                v3 maxRelPos = GetRelPos(camera->targetWorldPos, box.max);

                DrawAlignedBoxOutline(gameState->renderGroup,
                                      WorldToRH(minRelPos - V3(LEVEL_TILE_RADIUS)),
                                      WorldToRH(maxRelPos + V3(LEVEL_TILE_RADIUS)),
                                      V3(0.0f, 1.0f, 0.0f), 2.0f);
            }
        } break;
        case Tool_EntityPicker:
        {
            if (editor->selectedEntityID)
            {
                SimEntity* entity = GetEntity(simRegion, editor->selectedEntityID);
                // TODO: This is correct only while region
                // origin and camera origin are the same value
                DrawAlignedBoxOutline(gameState->renderGroup,
                                      WorldToRH(entity->pos - V3(LEVEL_TILE_RADIUS)),
                                      WorldToRH(entity->pos + V3(LEVEL_TILE_RADIUS)),
                                      V3(1.0f, 0.0f, 0.0f), 2.0f);
            }
        }
        }

        DEBUG_OVERLAY_SLIDER(gameState->renderer->gamma, 1.0f, 3.0f);
        DEBUG_OVERLAY_SLIDER(gameState->renderer->exposure, 0.0f, 3.0f);

        DirectionalLight light = {};
        light.dir = Normalize(V3(-0.3f, -1.0f, -1.0f));
        light.ambient = V3(0.6f);
        local_persist v3 diffuse = V3(1.0f);
        DEBUG_OVERLAY_SLIDER(diffuse, 0.0f, 100.0f);
        light.diffuse = diffuse;
        light.specular = V3(2.0f);
        RenderCommandSetDirLight lightCommand = {};
        lightCommand.light = light;
        RenderGroupPushCommand(gameState->renderGroup, RENDER_COMMAND_SET_DIR_LIGHT,
                               (void*)&lightCommand);

        DrawRegion(simRegion, gameState, camera->targetWorldPos);
        RendererBeginFrame(gameState->renderer, V2(PlatformGlobals.windowWidth, PlatformGlobals.windowHeight));
        FlushRenderGroup(gameState->renderer, gameState->renderGroup);
        RendererEndFrame(gameState->renderer);

        EndSim(gameState->session.level, simRegion);
        EndTemporaryMemory(gameState->tempArena);

        if (editor->ui.wantsToSaveLevel)
        {
            editor->ui.wantsToSaveLevel = false;
            BeginTemporaryMemory(gameState->tempArena);
            bool saved = SaveLevel(editor->session->level, editor->ui.wLevelName, gameState->tempArena);
            SOKO_ASSERT(saved);
            EndTemporaryMemory(gameState->tempArena);
        }
        if (editor->ui.exitToMainMenu)
        {
            DestroyGameSession(&gameState->session);
            gameState->globalGameMode = GAME_MODE_MENU;
        }

    }
}
