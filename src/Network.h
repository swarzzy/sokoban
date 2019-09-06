#pragma once

namespace  soko { namespace net
{
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
