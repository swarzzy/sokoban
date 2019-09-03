#pragma once
#include "Platform.h"
#include "Renderer.h"
#include "Level.h"
#include "Camera.h"

// NOTE: For now assume that all msvc target devices are little-endian
#if defined (AB_COMPILER_MSVC)
#define SOKO_BYTE_ORDER AB_LITTLE_ENDIAN
#endif


namespace soko
{
    struct Mesh
    {
        u32 vertexCount;
        u32 normalCount;
        u32 uvCount;
        u32 indexCount;
        v3* vertices;
        v3* normals;
        v2* uvs;
        u32* indices;
        u32 gpuVertexBufferHandle;
        u32 gpuIndexBufferHandle;
    };

    struct Material
    {
        Texture diffMap;
        Texture specMap;
    };

    enum PlayerPendingMovementBit : u32
    {
        PENDING_MOVEMENT_BIT_FORWARD = 0x1,
        PENDING_MOVEMENT_BIT_BACKWARD = 0x2,
        PENDING_MOVEMENT_BIT_LEFT = 0x4,
        PENDING_MOVEMENT_BIT_RIGHT = 0x8
    };

    struct Player
    {
        Entity* e;
        Level* level;
        u32 inputFlags;
        bool reversed;
    };

    enum GameMode
    {
        GAME_MODE_MENU = 0,
        GAME_MODE_SINGLE,
        GAME_MODE_CLIENT,
        GAME_MODE_SERVER
    };

    struct GameState
    {
        static constexpr u32 MAX_PLAYERS = 2;
        AB::MemoryArena* memoryArena;
        AB::MemoryArena* tempArena;
        Renderer* renderer;
        RenderGroup* renderGroup;
        FPCamera debugCamera;
        GameCamera camera;
        b32 useDebugCamera;
        Mesh cubeMesh;
        Mesh plateMesh;
        Mesh portalMesh;
        Mesh spikesMesh;
        Mesh buttonMesh;
        Material tileMaterial;
        Material tilePlayerMaterial;
        Material tileBlockMaterial;
        Material redPlateMaterial;
        Material portalMaterial;
        Material spikesMaterial;
        Material buttonMaterial;
        u32 overlayCorner;
        Level level;
        u32 playerCount;
        Player players[MAX_PLAYERS];
        b32 platePressed;
        Player* controlledPlayer;
        Player* remotePlayer;
        u16 port;
        uptr socket;
        i32 ipOctets[4];
        u32 ipAddress;
        AB::NetAddress serverAddr;
        byte buffer[1024];
        i32 playerX;
        i32 playerY;
        u32 gameMode;
        b32 gameModeReadyToInit;
        b32 gameModeInitialized;
    };
}
