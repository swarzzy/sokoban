#pragma once
#include "Platform.h"
#include "Renderer.h"
#include "Level.h"
#include "Camera.h"
#include "GameMenu.h"

// NOTE: For now assume that all msvc target devices are little-endian
#if defined (AB_COMPILER_MSVC)
#define SOKO_BYTE_ORDER AB_LITTLE_ENDIAN
#endif

namespace soko::net
{
    struct Server;
    struct Client;
}

namespace soko
{

    enum PlayerAction : byte
    {
        // NOTE: Movement action values should be same
        // as in Direction enum
        PlayerAction_MoveNorth = 1,
        PlayerAction_MoveSouth,
        PlayerAction_MoveWest,
        PlayerAction_MoveEast,
        PlayerAction_MoveUp,
        PlayerAction_MoveDown,
        PlayerAction_ToggleInteractionMode,
    };

    inline bool ActionIsMovement(PlayerAction action)
    {
        bool result = (byte)action >= PlayerAction_MoveNorth && (byte)action <= PlayerAction_MoveDown;
        return result;
    }


    static_assert((byte)PlayerAction_MoveNorth == (byte)DIRECTION_NORTH);
    static_assert((byte)PlayerAction_MoveSouth == (byte)DIRECTION_SOUTH);
    static_assert((byte)PlayerAction_MoveWest == (byte)DIRECTION_WEST);
    static_assert((byte)PlayerAction_MoveEast == (byte)DIRECTION_EAST);
    static_assert((byte)PlayerAction_MoveUp == (byte)DIRECTION_UP);
    static_assert((byte)PlayerAction_MoveDown == (byte)DIRECTION_DOWN);

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

    struct Player
    {
        Entity* e;
        Level* level;
        u32 inputFlags;
        bool reversed;
    };

    struct ClientInput
    {
        static constexpr u32 BUFFER_SIZE = 512;
        u32 bufferAt;
        byte buffer[BUFFER_SIZE];
    };

    struct PlayerSlot
    {
        b32 used;
        Player* player;
        ClientInput input;
    };

    struct GameState
    {
        GameMode globalGameMode;
        GameMenu mainMenu;
        GameSession session;

        static constexpr u32 MAX_PLAYERS = 4;
        AB::MemoryArena* memoryArena;
        AB::MemoryArena* tempArena;
        Renderer* renderer;
        RenderGroup* renderGroup;
        FPCamera debugCamera;
        GameCamera camera;
        b32 useDebugCamera;
        Material materials[_EntityMaterial_Count];
        Mesh meshes[_EntityMesh_Count];
        u32 overlayCorner;
        bool playersOccupancy[MAX_PLAYERS];
        Player players[MAX_PLAYERS];
        b32 platePressed;
        Player* controlledPlayer;
    };
}
