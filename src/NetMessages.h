#pragma once

namespace soko
{
    enum ClientMsgType : byte
    {
        ClientMsg_QueryState,
        ClientMsg_Join,
        ClientMsg_Leave,
        ClientMsg_PlayerAction
    };

    enum ServerMsgType : byte
    {
        ServerMsg_State,
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

    struct ClientMsgQueryState
    {
    };

    constant u32 SERVER_MAX_LEVEL_NAME_LEN = 256;

    struct ServerMsgState
    {
        b32 hasAvailableSlot;
        u32 levelNameStrLen;

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

    struct ServerAddPlayerMsg
    {
        NewPlayerData newPlayer;
    };

    struct ServerDeletePlayerMsg
    {
        i16 slot;
    };
#pragma pack(pop)

}
