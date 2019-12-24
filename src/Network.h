#pragma once
#include "Player.h"

#define SOKO_LOG_CLIENT_SEND(result, messageType) if (!result.status) SOKO_INFO("Client: failed to send <%s> message", #messageType); else SOKO_INFO("Client: sent <%s> message", #messageType)
#define SOKO_LOG_SERVER_SEND(result, messageType) if (!result.status) SOKO_INFO("Server: failed to send <%s> message", #messageType); else SOKO_INFO("Server: sent <%s> message", #messageType)

#define SOKO_LOG_CLIENT_RECV(messageType) SOKO_INFO("Client: recieve <%s> message", #messageType)
#define SOKO_LOG_SERVER_RECV(messageType) SOKO_INFO("Server: recieve <%s> message", #messageType)

namespace soko
{
    struct SocketBuffer
    {
        // TODO: We don't need THAT big buffer
        u32 at;
        u32 end;
        byte buffer[MEGABYTES(1)];
    };

    struct Server
    {
        enum
        {
            NotInitialized = 0,
            Listening,
            Connected
        } state;
        u16 port;
        Socket listenSocket;
        Socket connectionSocket;
        f32 presenceTimer;
        f32 connectionTimer;
        // TODO: For debug only
        b32 stopSendPresenceMessages;
        // NOTE: Set to true when connect message recieved
        b32 playerConnected;
        char connectedPlayerName[PLAYER_NAME_LEN];
        PlayerActionBuffer<1024> actionBuffer;
    };

    struct Client
    {
        Socket socket;
        f32 connectionTimeout;
        b32 connectionEstablished;
        f32 presenceTimer;
        f32 connectionTimer;
        // TODO: For debug only!!!
        b32 stopSendPresenceMessages;
        u64 levelGUID;
        char connectedPlayerName[PLAYER_NAME_LEN];
    };

#pragma pack(push, 1)
    struct NetMessageHeader
    {
        enum
        {
            InvalidMessage = 0,
            ClientConnectMessage,
            ClientLevelListMessage,
            ClientPresenceMessage,
            ClientActionSequenceMessage,
            ServerConnectMessage,
            ServerLevelListQueryMessage,
            ServerPresenceMessage,
            ServerEnterLevelMessage,
            ServerActionSequenceMessage,
            _NumMessageTypes
        } type;
        u32 messageSize;
    };

    struct ClientConnectMessage
    {
        NetMessageHeader header = { NetMessageHeader::ClientConnectMessage, sizeof(ClientConnectMessage)};;
        char playerName[PLAYER_NAME_LEN];
    };

    // NOTE: Size should be specified manually
    struct ClientLevelListMessage
    {
        NetMessageHeader header = { NetMessageHeader::ClientLevelListMessage, 0};
        u32 numLevels;
        u64 firstGUID;
        // ... level GUIDs (u64) ...
    };

    struct ClientActionSequenceMessage
    {
        NetMessageHeader header = { NetMessageHeader::ClientActionSequenceMessage, 0};
        u32 actionCount;
        byte firstAction;
        // ... Actions ...
    };

    struct ServerActionSequenceMessage
    {
        NetMessageHeader header = { NetMessageHeader::ServerActionSequenceMessage, 0};
        u32 actionCount;
        PlayerAction firstAction;
        // ... Actions ...
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

    struct ServerEnterLevelMessage
    {
        NetMessageHeader header = { NetMessageHeader::ServerEnterLevelMessage, sizeof(ServerEnterLevelMessage)};
        u64 levelGUID;
    };
#pragma pack(pop)

    inline bool MessageValid(const NetMessageHeader* header)
    {
        bool result;
        result = (header->type > NetMessageHeader::InvalidMessage && header->type < NetMessageHeader::_NumMessageTypes);
        return result;
    }
}
