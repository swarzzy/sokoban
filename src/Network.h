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
        const_val u32 SLOTS_NUM = 2;
        const_val u32 SOCKET_BUFFER_SIZE = 1024;
        const_val u32 PLAYER_SLOT = 0;
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

    struct Client
    {
        uptr socket;
        AB::NetAddress serverAddr;
        i16 playerSlot;
        bool slotsOccupancy[Server::SLOTS_NUM];
        ClientSlot slots[Server::SLOTS_NUM];
        byte socketBuffer[Server::SOCKET_BUFFER_SIZE];
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
        // .. other players ...
    };

    struct ServerPlayerActionMsg
    {
        i16 slot;
    };
#pragma pack(pop)
}}
