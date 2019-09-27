#pragma once

namespace  soko { namespace net
{
    struct InputBuffer
    {
        static constexpr u32 SIZE = 128;
        u32 at;
        PlayerAction base[SIZE];
    };

    struct ServerSlot
    {
        Player* player;
        InputBuffer inputBuffer;
        AB::NetAddress address;
        f32 lastMsgTime;
    };

    struct Server
    {
        const_val u32 SLOTS_NUM = 4;
        const_val u32 SOCKET_BUFFER_SIZE = 1024;
        const_val u32 LOCAL_PLAYER_SLOT = 0;
        uptr socket;
        u16 port;
        bool slotsOccupancy[SLOTS_NUM];
        ServerSlot slots[SLOTS_NUM];
        byte socketBuffer[SOCKET_BUFFER_SIZE];
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
        ConnectionStatus_Connected
    };

    // TODO: Cleanup on this constants and level names and stuff
    constexpr u32 CLIENT_LEVEL_NAME_LEN = 256;

    struct Client
    {
        ConnectionStatus connectionStatus;
        float joinTimeout;
        float waitingTime;
        uptr socket;
        AB::NetAddress serverAddr;
        i16 playerSlot;
        bool slotsOccupancy[Server::SLOTS_NUM];
        ClientSlot slots[Server::SLOTS_NUM];
        byte socketBuffer[Server::SOCKET_BUFFER_SIZE];
        wchar_t levelName[CLIENT_LEVEL_NAME_LEN];
    };

    enum ClientMsgType : byte
    {
        ClientMsg_Join,
        ClientMsg_Leave,
        ClientMsg_PlayerAction
    };

    enum ServerMsgType : byte
    {
        ServerMsg_JoinResult,
        ServerMsg_AddPlayer,
        ServerMsg_DeletePlayer,
        ServerMsg_PlayerAction
    };

#pragma pack(push, 1)

    struct ClientMsgHeader
    {
        ClientMsgType type;
        i16 slot;
    };

    struct ServerMsgHeader
    {
        ServerMsgType type;
    };

    struct NewPlayerData
    {
        i16 slot;
        i32 x;
        i32 y;
        i32 z;
    };

    struct ServerJoinResultMsg
    {
        u8 succeed;
        u8 otherPlayersCount;
        NewPlayerData newPlayer;
        u16 levelNameLength;
        // .. level filename ...
        // .. other players ...
    };

    struct ServerPlayerActionMsg
    {
        i16 slot;
    };

    struct ServerAddPlayerMsg
    {
        NewPlayerData newPlayer;
    };

    struct ServerDeletePlayerMsg
    {
        i16 slot;
    };
#pragma pack(pop)
}}
