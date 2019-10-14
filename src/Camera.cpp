#include "Camera.h"

namespace soko
{
    internal void
    InitCameras(GameCamera* camera, FPCamera* debugCamera, Player* targetPlayer)
    {
        CameraConfig conf = {};
        conf.position = V3(0.0f);
        conf.front = V3(0.0f, 0.0f, -1.0f);
        conf.fovDeg = 45.0f;
        conf.aspectRatio = 16.0f / 9.0f;
        conf.nearPlane = 0.1f;
        conf.farPlane = 100.0f;

        debugCamera->conf = conf;
        debugCamera->moveSpeed = 7.0f;
        debugCamera->rotateSpeed = 60.0f;
        debugCamera->moveSmooth = 0.8f;
        debugCamera->rotateSmooth = 0.45f;

        camera->conf = conf;
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
#if 1
        v3 relPos = Lerp(dist, V3(0.0f), GlobalAbsDeltaTime * camera->followSpeed);
        camera->worldPos = GetWorldPos(camera->targetWorldPos, relPos);
#else
        v3 acceleration = -dist;
        acceleration *= camera->moveSpeed / 2.5f; // 500.0f

        camera->moveFriction = 30.0f;
        f32 friction = camera->moveFriction; // 8.0f
        acceleration = acceleration - camera->velocity * friction;

        v3 movementDelta;
        movementDelta = 0.5f * acceleration *
            Square(GlobalAbsDeltaTime) +
            camera->velocity *
            GlobalAbsDeltaTime;

        camera->worldPos = GetWorldPos(camera->worldPos, movementDelta);
        camera->velocity += acceleration * GlobalAbsDeltaTime;
#endif

        //camera->worldOffset = -camera->targetPos;
        camera->conf.position = V3(x, y, z);
        camera->conf.front = -Normalize(V3(x, y, z));

#if 0
        v2 normMousePos;
        normMousePos.x = 2.0f * GlobalInput.mouseX - 1.0f;
        normMousePos.y = 2.0f * GlobalInput.mouseY - 1.0f;
        v4 mouseClip = V4(normMousePos, 1.0f, 0.0f);
        v4 mouseView = MulM4V4(camera->invProjection, mouseClip);
        mouseView = V4(mouseView.xy, 1.0f, 0.0f);
        v3 mouseWorld = MulM4V4(camera->invLookAt, mouseView).xyz;
        mouseWorld = Normalize(mouseWorld);
        camera->mouseRay = mouseWorld;
        camera->mouseRayWorld = FlipYZ(mouseWorld);
#endif
    }

}
