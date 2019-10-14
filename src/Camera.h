#pragma once
#include "Platform.h"
#include "Level.h"

namespace soko
{
    struct FPCamera
    {
        CameraConfig conf;
        v3 targetPosition;
        v3 targetFront;
        b32 cursorCaptured;
        f32 moveSpeed;
        f32 moveSmooth;
        f32 rotateSpeed;
        f32 rotateSmooth;
        f32 pitch;
        f32 yaw;
    };

    struct Player;
    struct GameCamera
    {
        CameraConfig conf;
        f32 longitude;
        f32 latitude;
        f32 distance;
        v2 targetOrbit;
        f32 targetDistance;
        //v3 targetPos;
        f32 rotSpeed;
        f32 zoomSpeed;
        //f32 moveSpeed;
        //f32 moveFriction;
        //v3 velocity;
        f32 latSmooth;
        f32 longSmooth;
        f32 distSmooth;
        WorldPos worldPos;
        WorldPos targetWorldPos;
        f32 followSpeed;
    };

    internal void UpdateCamera(GameCamera* camera, const WorldPos* target);
    internal void UpdateCamera(FPCamera* camera);
}
