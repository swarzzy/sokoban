#pragma once
#include "Platform.h"
#include "Level.h"

namespace soko
{
    // NOTE: This is now used only in camera and sim regions
    struct WorldPos
    {
        iv3 tile;
        v3 offset;
    };

    inline WorldPos MakeWorldPos(iv3 tile) { return {tile, {}}; }
    inline WorldPos MakeWorldPos(i32 x, i32 y, i32 z) { return {{x, y, z}, {}}; }

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
        f32 rotSpeed;
        f32 zoomSpeed;
        f32 latSmooth;
        f32 longSmooth;
        f32 distSmooth;
        // NOTE: Current interpolated camera world position
        WorldPos worldPos;
        // NOTE: worldPos interpolation destination
        WorldPos targetWorldPos;
        f32 followSpeed;

        v3 mouseRayRH;
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
        // NOTE: Position of a point on camera looks
        WorldPos worldPos;
        v3 frameAcceleration;
        i32 frameScrollOffset;

        v3 mouseRayRH;
    };
}
