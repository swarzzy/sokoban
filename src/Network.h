#pragma once

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
        char connectedPlayerName[PLAYER_NAME_LEN];
    };

#pragma pack(push, 1)
    struct NetMessageHeader
    {
        enum
        {
            ClientConnectMessage = 1,
            ClientLevelListMessage = 2,
            ClientPresenceMessage = 3,
            ServerConnectMessage = 4,
            ServerLevelListQueryMessage = 5,
            ServerPresenceMessage = 6,
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

    inline bool MessageValid(const NetMessageHeader* header)
    {
        bool result;
        result = (header->type > 0 && header->type <=6);
        return result;
    }
}
