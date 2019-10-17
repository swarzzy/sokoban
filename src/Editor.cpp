namespace soko
{
    struct Editor
    {
        iv3 selectedTile;
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

    internal void
    EditorInit(Editor* editor)
    {
        editor->selectedTile = {};
    }

    internal void
    EditorUpdateAndRender(GameState* gameState)
    {
        auto camera = gameState->session.editorCamera;
        auto editor = gameState->session.editor;

        EditorCameraGatherInput(camera);
        EditorCameraUpdate(camera);

        BeginTemporaryMemory(gameState->tempArena, true);
        SimRegion* simRegion = BeginSim(gameState->tempArena,
                                        gameState->session.level,
                                        gameState->session.editorCamera->targetWorldPos,
                                        2);

        if (JustPressed(MBUTTON_LEFT))
        {
            v3 from = RHToWorld(camera->conf.position);
            v3 ray = RHToWorld(camera->mouseRayRH);
            auto raycast = Raycast(simRegion, from, ray);
            if (raycast.hit)
            {
                PrintString("Hit tile (x: %i32, y: %i32, z:%i32), tMin = %f32\n",
                            raycast.tile.x, raycast.tile.y, raycast.tile.z, raycast.tMin);
                editor->selectedTile = raycast.tile;
            }
        }

        RenderGroupSetCamera(gameState->renderGroup, &gameState->session.editorCamera->conf);
        RendererBeginFrame(gameState->renderer, V2(PlatformGlobals.windowWidth, PlatformGlobals.windowHeight));

        v3 selTileRelPos = GetRelPos(camera->targetWorldPos, editor->selectedTile);
        DrawAlignedBoxOutline(gameState->renderGroup,
                              WorldToRH(selTileRelPos - V3(LEVEL_TILE_RADIUS)),
                              WorldToRH(selTileRelPos + V3(LEVEL_TILE_RADIUS)),
                              V3(1.0f, 0.0f, 0.0f), 2.0f);

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
