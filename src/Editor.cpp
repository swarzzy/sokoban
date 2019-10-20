namespace soko
{
    enum ToolKind
    {
        Tool_None,
        Tool_TilePicker,
        Tool_TilePlacer,
        Tool_TileEraser,
        Tool_EntityPicker
    };

    struct EditorUI
    {
        b32 entityViewOpened;
        b32 windowOpened;
    };

    struct Editor
    {
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
        i32 selectedEntityID;
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

        if (GlobalInput.mouseButtons[AB::MBUTTON_RIGHT].pressedNow)
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
        DEBUG_OVERLAY_TRACE(camera->latitude);

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
        DEBUG_OVERLAY_TRACE(normMousePos.x);
        DEBUG_OVERLAY_TRACE(normMousePos.y);

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
    IsMouseOnUI()
    {
        ImGuiIO* io = &ImGui::GetIO();
        bool result = io->WantCaptureMouse;
        return result;
    }

    internal void
    EditorInit(Editor* editor)
    {
        editor->placerTile = TileValue_Wall;
        editor->ui.windowOpened = true;
        editor->ui.entityViewOpened = true;
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
                int tileValue = editor->placerTile - 1;
                ImGui::ListBox("", &tileValue, GetTileValueStrings() + 1, _TileValue_Count - 1);
                editor->placerTile = (TileValue)(tileValue + 1);
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
                //ImGui::SameLine();
                i32 type = (EntityType)entity->stored->type;
                ImGui::PushID("Entity type listbox");
                ImGui::Combo("", &type, GetEntityTypeStrings(), _EntityType_Count);
                ImGui::PopID();
                ImGui::Separator();
            }
        }
        ImGui::End();
    }

    internal void
    EditorDrawUI(GameSession* session)
    {
        Editor* editor = session->editor;

        DrawToolBox(editor);
        EditorEntityView(editor);
        f32 offset = 10.0f;
        ImGuiIO* io = &ImGui::GetIO();
        ImVec2 windowPos = ImVec2(io->DisplaySize.x - offset, offset + 70.0f);
        ImVec2 windowPosPivot = ImVec2(1.0f, 0.0f);
        ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, windowPosPivot);
        auto windowFlags =
            ImGuiWindowFlags_NoResize;
        ImGuiWindowFlags_AlwaysAutoResize;
        if (ImGui::Begin("Tile properties", (bool*)&editor->ui.windowOpened, windowFlags))
        {
            Tile tile = GetTile(session->level, editor->selectorBegin);
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
            ImGui::Combo("", &tileValue, GetTileValueStrings() + 1, _TileValue_Count - 1);
            ImGui::PopID();
            SetTile(session->level, editor->selectorBegin, (TileValue)(tileValue + 1));
        }
        ImGui::End();
    }

    internal void
    EditorUpdateAndRender(GameState* gameState)
    {
        auto camera = gameState->session.editorCamera;
        auto editor = gameState->session.editor;
        auto level = gameState->session.level;

        EditorCameraGatherInput(camera);
        EditorCameraUpdate(camera);

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
                            // TODO: Dynamically growing arenas for editor?
                            bool result = GenChunkMesh(chunk, &chunk->mesh, gameState->session.sessionArena);
                            SOKO_ASSERT(result);
                            chunk->loadedMesh.quadCount = RendererReloadChunkMesh(&chunk->mesh, chunk->loadedMesh.gpuHandle);
                            chunk->dirty = false;
                        }
                    }
                }
            }
        }

        if (editor->tool != Tool_EntityPicker)
        {
            editor->selectedEntityID = 0;
        }

        switch (editor->tool)
        {
        case Tool_TilePicker:
        {
            if (JustPressed(MBUTTON_LEFT) && !IsMouseOnUI())
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
                iv3 hoveredTile = raycast.tile.coord + DirToUnitOffset(raycast.tile.normalDir);
                if (JustPressed(MBUTTON_LEFT) && !IsMouseOnUI())
                {
                    editor->mouseUsed = true;
                    editor->selectorHolding = true;
                    editor->selectorBegin = hoveredTile;
                }
                else if (JustReleased(MBUTTON_LEFT) && editor->selectorHolding)
                {
                    editor->selectorHolding = false;
                    editor->selectorEnd = hoveredTile;

                    TileBox box = TileBoxFromTwoPoints(editor->selectorBegin, editor->selectorEnd);

                    for (i32 z = box.min.z; z <= box.max.z; z++)
                    {
                        for (i32 y = box.min.y; y <= box.max.y; y++)
                        {
                            for (i32 x = box.min.x; x <= box.max.x; x++)
                            {
                                SetTile(level, x, y, z, editor->placerTile);
                            }
                        }
                    }
                }

                if (editor->selectorHolding)
                {
                    editor->selectorEnd = hoveredTile;
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
                if (JustPressed(MBUTTON_LEFT) && !IsMouseOnUI())
                {
                    editor->mouseUsed = true;
                    editor->selectorHolding = true;
                    editor->selectorBegin = hoveredTile;
                }
                else if (JustReleased(MBUTTON_LEFT) && editor->selectorHolding)
                {
                    editor->selectorHolding = false;
                    editor->selectorEnd = hoveredTile;

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

                if (editor->selectorHolding)
                {
                    editor->selectorEnd = hoveredTile;
                }
            }
        }
        case Tool_EntityPicker:
        {
            if (JustPressed(MBUTTON_LEFT) && !IsMouseOnUI())
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
        EditorDrawUI(&gameState->session);
        RendererBeginFrame(gameState->renderer, V2(PlatformGlobals.windowWidth, PlatformGlobals.windowHeight));

        switch (editor->tool)
        {
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


        DirectionalLight light = {};
        light.dir = Normalize(V3(-0.3f, -1.0f, -1.0f));
        light.ambient = V3(0.3f);
        light.diffuse = V3(0.8f);
        light.specular = V3(1.0f);
        RenderCommandSetDirLight lightCommand = {};
        lightCommand.light = light;
        RenderGroupPushCommand(gameState->renderGroup, RENDER_COMMAND_SET_DIR_LIGHT,
                               (void*)&lightCommand);

        DrawRegion(simRegion, gameState, camera->targetWorldPos);
        FlushRenderGroup(gameState->renderer, gameState->renderGroup);

        EndSim(gameState->session.level, simRegion);
        EndTemporaryMemory(gameState->tempArena);
    }
}
