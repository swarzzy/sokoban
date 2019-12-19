#pragma once

#define SOKO_LOG_CLIENT_SEND(result, messageType) if (!result.status) SOKO_INFO("Client: failed to send <%s> message", #messageType); else SOKO_INFO("Client: sent <%s> message", #messageType)
#define SOKO_LOG_SERVER_SEND(result, messageType) if (!result.status) SOKO_INFO("Server: failed to send <%s> message", #messageType); else SOKO_INFO("Server: sent <%s> message", #messageType)

#define SOKO_LOG_CLIENT_RECV(messageType) SOKO_INFO("Client: recieve <%s> message", #messageType)
#define SOKO_LOG_SERVER_RECV(messageType) SOKO_INFO("Server: recieve <%s> message", #messageType)

namespace soko
{
#pragma pack(push, 1)
    struct NetMessageHeader
    {
        enum
        {
            ClientConnectMessage,
            ClientLevelListMessage,
            ClientPresenceMessage,
            ServerConnectMessage,
            ServerLevelListQueryMessage,
            ServerPresenceMessage,
        } type;
        u32 messageSize;
    };

    struct ClientConnectMessage
    {
        NetMessageHeader header = { NetMessageHeader::ClientConnectMessage, sizeof(ClientConnectMessage)};;
        char playerName[PLAYER_NAME_LEN];
    };

    struct ClientLevelListMessage
    {
        NetMessageHeader header = { NetMessageHeader::ClientLevelListMessage, sizeof(ClientLevelListMessage)};
        u32 numLevels;
        u64 firstGUID;
        // ... level GUIDs (u64) ...
    };

    struct ServerConnectMessage
    {
        NetMessageHeader header = { NetMessageHeader::ServerConnectMessage, sizeof(ServerConnectMessage)};
        char playerName[PLAYER_NAME_LEN];
    };

    struct ServerLevelListQueryMessage
    {
        NetMessageHeader header = { NetMessageHeader::ServerLevelListQueryMessage, sizeof(ServerLevelListQueryMessage)};
    };

    struct ServerPresenceMessage
    {
        NetMessageHeader header = { NetMessageHeader::ServerPresenceMessage, sizeof(ServerPresenceMessage)};
    };

    struct ClientPresenceMessage
    {
        NetMessageHeader header = { NetMessageHeader::ClientPresenceMessage, sizeof(ClientPresenceMessage)};
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
