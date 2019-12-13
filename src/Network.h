#pragma once
namespace soko
{
#pragma pack(push, 1)
    struct NetMessageHeader
    {
        enum
        {
            ClientConnectMessage,
            ClientLevelListMessage
        } type;
    };

    struct ClientConnectMessage
    {
        NetMessageHeader header;
        char playerName[PLAYER_NAME_LEN];
    };

    struct ClientLevelListMessage
    {
        NetMessageHeader header;
        u32 numLevels;
        u64 firstGUID;
        // ... level GUIDs (u64) ...
    };
#pragma pack(pop)
}


#if 0
#include "NetMessages.h"

namespace  soko { namespace net
{
    constant u32 PLAYER_INPUT_BUFFER_SIZE = 128;

    struct InputBuffer
    {
        u32 at;
        PlayerAction base[PLAYER_INPUT_BUFFER_SIZE];
    };

    struct ServerSlot
    {
        Player* player;
        InputBuffer inputBuffer;
        AB::NetAddress address;
        f32 lastMsgTime;
    };

    constant u32 SERVER_SLOTS_NUM = 4;
    constant u32 SERVER_SOCKET_BUFFER_SIZE = 1024;
    constant u32 SERVER_LOCAL_PLAYER_SLOT = 0;

    struct Server
    {
        uptr socket;
        u16 port;
        bool slotsOccupancy[SERVER_SLOTS_NUM];
        ServerSlot slots[SERVER_SLOTS_NUM];
        byte socketBuffer[SERVER_SOCKET_BUFFER_SIZE];
        char levelName[SERVER_MAX_LEVEL_NAME_LEN];
    };

    struct ClientSlot
    {
        Player* player;
        InputBuffer inputBuffer;
    };

    enum ConnectionStatus
    {
        ConnectionStatus_None = 0,
        ConnectionStatus_Waiting,
        ConnectionStatus_Connected,
        ConnectionStatus_Error,
        ConnectionStatus_ServerIsFull
    };

    struct ServerStatus
    {
        u32 levelNameLen;
        char* levelName;
    };

    struct Client
    {
        ConnectionStatus connectionStatus;
        float joinTimeout;
        float waitingTime;
        uptr socket;
        AB::NetAddress serverAddr;
        i16 playerSlot;
        bool slotsOccupancy[SERVER_SLOTS_NUM];
        ClientSlot slots[SERVER_SLOTS_NUM];
        byte socketBuffer[SERVER_SOCKET_BUFFER_SIZE];
        char levelName[SERVER_MAX_LEVEL_NAME_LEN];
    };

}}
#endif
