#include "Camera.h"

namespace soko
{
    inline v3
    GetRelPos(WorldPos origin, iv3 tile)
    {
        v3 result = {};
        iv3 tileDiff = tile - origin.tile;
        v3 offsetDiff = -origin.offset;
        result = V3(tileDiff.x * LEVEL_TILE_SIZE, tileDiff.y * LEVEL_TILE_SIZE, tileDiff.z * LEVEL_TILE_SIZE);
        result += offsetDiff;
        return result;
    }

    inline v3
    GetRelPos(WorldPos origin, WorldPos target)
    {
        v3 result = {};
        iv3 tileDiff = target.tile - origin.tile;
        v3 offsetDiff = target.offset - origin.offset;
        result = V3(tileDiff.x * LEVEL_TILE_SIZE, tileDiff.y * LEVEL_TILE_SIZE, tileDiff.z * LEVEL_TILE_SIZE);
        result += offsetDiff;
        return result;
    }


    inline void
    NormalizeWorldPos(WorldPos* p)
    {
        i32 tileOffX = Floor((p->offset.x + LEVEL_TILE_RADIUS) / LEVEL_TILE_SIZE);
        i32 tileOffY = Floor((p->offset.y + LEVEL_TILE_RADIUS) / LEVEL_TILE_SIZE);
        i32 tileOffZ = Floor((p->offset.z + LEVEL_TILE_RADIUS) / LEVEL_TILE_SIZE);

        p->offset.x -= tileOffX * LEVEL_TILE_SIZE;
        p->offset.y -= tileOffY * LEVEL_TILE_SIZE;
        p->offset.z -= tileOffZ * LEVEL_TILE_SIZE;

        p->tile += IV3(tileOffX, tileOffY, tileOffZ);

        p->tile.x = Clamp(p->tile.x, LEVEL_MIN_DIM, LEVEL_MAX_DIM);
        p->tile.y = Clamp(p->tile.y, LEVEL_MIN_DIM, LEVEL_MAX_DIM);
        p->tile.z = Clamp(p->tile.z, LEVEL_MIN_DIM, LEVEL_MAX_DIM);
    }

    inline WorldPos
    OffsetWorldPos(WorldPos origin, v3 offset)
    {
        origin.offset += offset;
        NormalizeWorldPos(&origin);
        return origin;
    }

    internal void
    InitCameras(GameCamera* camera, WorldPos targetPos, FPCamera* debugCamera)
    {
        CameraConfig conf = {};
        conf.position = V3(0.0f);
        conf.front = V3(0.0f, 0.0f, 1.0f);
        conf.fovDeg = 45.0f;
        conf.aspectRatio = 16.0f / 9.0f;
        conf.nearPlane = 0.1f;
        conf.farPlane = 60.0f;

        debugCamera->conf = conf;
        debugCamera->moveSpeed = 7.0f;
        debugCamera->rotateSpeed = 60.0f;
        debugCamera->moveSmooth = 0.8f;
        debugCamera->rotateSmooth = 0.45f;

        camera->conf = conf;
        camera->targetWorldPos = targetPos;
        camera->targetDistance = 1.0f;
        camera->longSmooth = 30.0f;
        camera->latSmooth = 30.0f;
        camera->distSmooth = 30.0f;
        camera->followSpeed = 1.0f;
        camera->rotSpeed = 1000.0f;
        camera->zoomSpeed = 200.0f;
        //camera->moveSpeed = 500.0f;
        //camera->moveFriction = 80.0f;
    }

    internal void
    UpdateCamera(FPCamera* camera)
    {
        v3 pos = camera->targetPosition;
        v3 front = camera->targetFront;

        if (GlobalInput.keys[AB::KEY_W].pressedNow)
        {
            pos += front * GlobalAbsDeltaTime * camera->moveSpeed;
        }

        if (GlobalInput.keys[AB::KEY_S].pressedNow)
        {
            pos -= front * GlobalAbsDeltaTime * camera->moveSpeed;
        }

        if (GlobalInput.keys[AB::KEY_A].pressedNow)
        {
            v3 right = Normalize(Cross(front, V3(0.0f, 1.0f, 0.0f)));
            pos -= right * GlobalAbsDeltaTime * camera->moveSpeed;
        }

        if (GlobalInput.keys[AB::KEY_D].pressedNow)
        {
            v3 right = Normalize(Cross(front, V3( 0, 1, 0 )));
            pos += right * GlobalAbsDeltaTime * camera->moveSpeed;
        }

        if (GlobalInput.keys[AB::KEY_TAB].pressedNow &&
            !GlobalInput.keys[AB::KEY_TAB].wasPressed)
        {
            camera->cursorCaptured = !camera->cursorCaptured;
            if (camera->cursorCaptured)
            {
                SetInputMode(AB::INPUT_MODE_CAPTURE_CURSOR);
            }
            else
            {
                SetInputMode(AB::INPUT_MODE_FREE_CURSOR);
            }
        }

        if (camera->cursorCaptured)
        {
            camera->pitch += GlobalInput.mouseFrameOffsetY * camera->rotateSpeed;
            camera->yaw += GlobalInput.mouseFrameOffsetX * camera->rotateSpeed;
        }


        if (camera->pitch > 89.0f)
        {
            camera->pitch = 89.0f;
        }
        if (camera->pitch < -89.0f)
        {
            camera->pitch = -89.0f;
        }

        front.x = Cos(ToRadians(camera->pitch)) * Cos(ToRadians(camera->yaw));
        front.y = Sin(ToRadians(camera->pitch));
        front.z = Cos(ToRadians(camera->pitch)) * Sin(ToRadians(camera->yaw));
        front = Normalize(front);

        camera->targetPosition = pos;
        camera->targetFront = front;

        camera->conf.position = Lerp(camera->conf.position, pos, camera->moveSmooth);
        camera->conf.front = Lerp(camera->conf.front, front, camera->rotateSmooth);

        camera->conf.viewMatrix = LookAtDirRH(camera->conf.position, camera->conf.front, V3(0.0f, 1.0f, 0.0f));
        camera->conf.projectionMatrix = PerspectiveOpenGLRH(camera->conf.fovDeg, camera->conf.aspectRatio, camera->conf.nearPlane, camera->conf.farPlane);
        camera->conf.invViewMatrix = camera->conf.viewMatrix;
        bool inv = Inverse(&camera->conf.invViewMatrix);
        SOKO_ASSERT(inv);
        camera->conf.invProjectionMatrix = camera->conf.projectionMatrix;
        inv = Inverse(&camera->conf.invProjectionMatrix);
        SOKO_ASSERT(inv);
    }

    internal void
    UpdateCamera(GameCamera* camera, const WorldPos* target)
    {
        if (GlobalInput.mouseButtons[AB::MBUTTON_RIGHT].pressedNow)
        {
            v2 mousePos;
            f32 speed = camera->rotSpeed; // 1000.0f
            mousePos.x = GlobalInput.mouseFrameOffsetX * speed;
            mousePos.y = GlobalInput.mouseFrameOffsetY * speed;
            camera->targetOrbit.x -= mousePos.x;
            camera->targetOrbit.y -= mousePos.y;
        }

        if (camera->targetOrbit.y < 95.0f)
        {
            camera->targetOrbit.y = 95.0f;
        }
        else if (camera->targetOrbit.y > 170.0f)
        {
            camera->targetOrbit.y = 170.0f;
        }

        i32 frameScrollOffset = GlobalInput.scrollFrameOffset;
        camera->targetDistance -= frameScrollOffset * camera->zoomSpeed * GlobalAbsDeltaTime; // 5.0f

        if (camera->targetDistance < 5.0f)
        {
            camera->targetDistance = 5.0f;
        }
        else if (camera->targetDistance > 50.0f)
        {
            camera->targetDistance = 50.0f;
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

        f32 x = camera->distance * Sin(polarAngle) * Sin(longitude);
        f32 z = camera->distance * Sin(polarAngle) * Cos(longitude);
        f32 y = camera->distance * Cos(polarAngle);

        camera->targetWorldPos = *target;
        v3 dist = GetRelPos(camera->targetWorldPos, camera->worldPos);
        v3 relPos = Lerp(dist, V3(0.0f), GlobalAbsDeltaTime * camera->followSpeed);
        camera->worldPos = OffsetWorldPos(camera->targetWorldPos, relPos);

        //camera->worldOffset = -camera->targetPos;
        camera->conf.position = V3(x, y, z);
        // NOTE: It's pointing backwards
        camera->conf.front = -Normalize(V3(x, y, z));

        v2 normMousePos;
        normMousePos.x = 2.0f * GlobalInput.mouseX - 1.0f;
        normMousePos.y = 2.0f * GlobalInput.mouseY - 1.0f;

        v4 mouseClip = V4(normMousePos, -1.0f, 0.0f);

        camera->conf.viewMatrix = LookAtDirRH(camera->conf.position, camera->conf.front, V3(0.0f, 1.0f, 0.0f));
        camera->conf.projectionMatrix = PerspectiveOpenGLRH(camera->conf.fovDeg, camera->conf.aspectRatio, camera->conf.nearPlane, camera->conf.farPlane);
        camera->conf.invViewMatrix = camera->conf.viewMatrix;
        bool inv = Inverse(&camera->conf.invViewMatrix);
        SOKO_ASSERT(inv);
        camera->conf.invProjectionMatrix = camera->conf.projectionMatrix;
        inv = Inverse(&camera->conf.invProjectionMatrix);
        SOKO_ASSERT(inv);

        v4 mouseView = MulM4V4(camera->conf.invProjectionMatrix, mouseClip);
        mouseView = V4(mouseView.xy, -1.0f, 0.0f);
        v3 mouseWorld = MulM4V4(camera->conf.invViewMatrix, mouseView).xyz;
        mouseWorld = Normalize(mouseWorld);
        camera->mouseRayRH = mouseWorld;
        DEBUG_OVERLAY_TRACE(camera->conf.front);

    }

    internal void
    EditorCameraInit(EditorCamera* camera)
    {
        CameraConfig conf = {};
        conf.position = V3(0.0f);
        conf.front = V3(0.0f, 0.0f, -1.0f);
        conf.fovDeg = 45.0f;
        conf.aspectRatio = 16.0f / 9.0f;
        conf.nearPlane = 0.1f;
        conf.farPlane = 60.0f;

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
            camera->frameAcceleration -= frontDir;
        }
        if (GlobalInput.keys[AB::KEY_S].pressedNow)
        {
            camera->frameAcceleration += frontDir;
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

        camera->worldPos = OffsetWorldPos(camera->worldPos, movementDelta);
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

        //camera->worldPos = OffsetWorldPos(camera->targetWorldPos, RHToWorld(V3(x, y, z)));

        v2 normMousePos;
        normMousePos.x = 2.0f * GlobalInput.mouseX - 1.0f;
        normMousePos.y = 2.0f * GlobalInput.mouseY - 1.0f;
        //DEBUG_OVERLAY_TRACE(normMousePos.x);
        //DEBUG_OVERLAY_TRACE(normMousePos.y);

        v4 mouseClip = V4(normMousePos, -1.0f, 0.0f);

        camera->conf.viewMatrix = LookAtDirRH(camera->conf.position, camera->conf.front, V3(0.0f, 1.0f, 0.0f));
        camera->conf.projectionMatrix = PerspectiveOpenGLRH(camera->conf.fovDeg, camera->conf.aspectRatio, camera->conf.nearPlane, camera->conf.farPlane);
        camera->conf.invViewMatrix = camera->conf.viewMatrix;
        bool inv = Inverse(&camera->conf.invViewMatrix);
        SOKO_ASSERT(inv);
        camera->conf.invProjectionMatrix = camera->conf.projectionMatrix;
        inv = Inverse(&camera->conf.invProjectionMatrix);
        SOKO_ASSERT(inv);

        v4 mouseView = MulM4V4(camera->conf.invProjectionMatrix, mouseClip);
        mouseView = V4(mouseView.xy, -1.0f, 0.0f);
        DEBUG_OVERLAY_TRACE(mouseView);
        v3 mouseWorld = MulM4V4(camera->conf.invViewMatrix, mouseView).xyz;
        mouseWorld = Normalize(mouseWorld);
        camera->mouseRayRH = V3(mouseWorld.x, mouseWorld.y, mouseWorld.z);
        DEBUG_OVERLAY_TRACE(camera->mouseRayRH);
        DEBUG_OVERLAY_TRACE(camera->conf.front);
    }
}
