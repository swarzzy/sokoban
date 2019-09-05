#pragma once

namespace  soko { namespace net
{
    enum ClientMessage : byte
    {
        CLIENT_MESSAGE_JOIN,
        CLIENT_MESSAGE_LEAVE,
        CLIENT_MESSAGE_INPUT
    };

    enum ServerMessage : byte
    {
        SERVER_MESSAGE_JOIN_RESULT,
        SERVER_MESSAGE_ADD_PLAYER,
        SERVER_MESSAGE_INPUT
    };

#pragma pack(push, 1)
    struct ClientJoinMsg
    {

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
#pragma pack(pop)
}}
