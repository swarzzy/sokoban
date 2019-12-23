#include "Network.h"

namespace soko
{
    internal void
    InitServer(Server* server)
    {
        SOKO_ASSERT(server->state == Server::NotInitialized);
        SOKO_ASSERT(!server->listenSocket.handle);

        Socket listenSocket = NetCreateSocket(SocketType_TCP);
        if (listenSocket.handle)
        {
            u16 port = NetBindSocket(listenSocket);
            if (port)
            {
                if (NetListen(listenSocket, 4))
                {
                    server->port = port;
                    server->listenSocket = listenSocket;
                    server->state = Server::Listening;
                }
                else
                {
                    SOKO_INFO("Failed to listen socket");
                    NetCloseSocket(listenSocket);
                }
            }
            else
            {
                SOKO_INFO("Failed to bind socket");
                NetCloseSocket(listenSocket);
            }
        }
    }

    internal void
    ServerDisconnectPlayer(Server* server)
    {
        server->state = Server::Listening;
        NetCloseSocket(server->connectionSocket);
        server->connectionSocket = {};
        // TODO: Disconnect player
        server->connectedPlayerName[0] = 0;
        server->playerConnected = false;
    }

    internal void
    ShutdownServer(Server* server)
    {
        if (server->listenSocket.handle)
        {
            NetCloseSocket(server->listenSocket);
            server->listenSocket = {};
        }
        if (server->connectionSocket.handle)
        {
            NetCloseSocket(server->connectionSocket);
            server->connectionSocket = {};
        }
        server->state = Server::NotInitialized;
        server->connectedPlayerName[0] = 0;
        server->port = 0;
    }

    internal void
    ClientDisconnectFromServer(Client* client)
    {
        NetCloseSocket(client->socket);
        client->socket = {};
        // TODO: Disconnect player
        client->connectionEstablished = false;
        client->connectedPlayerName[0] = 0;
    }

    inline NetRecieveResult
    BeginReadIncomingMessages(Socket socket, SocketBuffer* buffer)
    {
        auto result = NetRecieve(socket, buffer->buffer, ArrayCount(buffer->buffer));
        buffer->end = result.bytesRecieved;
        buffer->at = 0;
        return result;
    }

    inline NetMessageHeader*
    GetNextIncomingMessage(SocketBuffer* buffer)
    {
        NetMessageHeader* result = 0;
        if ((buffer->end - buffer->at) >= sizeof(NetMessageHeader))
        {
            result = (NetMessageHeader*)(buffer->buffer + buffer->at);
            if (MessageValid(result))
            {
                u32 messageSize = result->messageSize;
                buffer->at += messageSize;
            }
        }
        return result;
    }

    inline void
    EndReadIncomingMessages(SocketBuffer* buffer)
    {
        buffer->at = 0;
        buffer->end = 0;
    }

    internal void
    SessionUpdateServer(GameSession* session)
    {
        auto recieveResult = BeginReadIncomingMessages(session->server->connectionSocket, session->socketBuffer);
        // TODO: Handle errors and normal disconnects differently
        if (recieveResult.status == NetRecieveResult::ConnectionClosed ||
            recieveResult.status == NetRecieveResult::ConnectionReset ||
            recieveResult.status == NetRecieveResult::Error)
        {
            EndReadIncomingMessages(session->socketBuffer);
            ServerDisconnectPlayer(session->server);
        }
        else if (recieveResult.status == NetRecieveResult::Success)
        {
            NetMessageHeader* header;
            do
            {
                header = GetNextIncomingMessage(session->socketBuffer);
                if (header)
                {
                    // TODO: Not reset timer if message is broken
                    session->server->connectionTimer = CONNECTION_TIMEOUT;
                    switch (header->type)
                    {
                    case NetMessageHeader::ClientPresenceMessage:
                    {
                        SOKO_LOG_SERVER_RECV(ClientPresenceMessage);
                    } break;
                    default: { SOKO_INFO("Server recieved broken message"); } break;
                    }
                }
            }
            while (header);
        }
        EndReadIncomingMessages(session->socketBuffer);

        // NOTE: Send presence message
        // TODO: For debug only!!!
        if (JustPressed(KEY_P))
        {
            session->server->stopSendPresenceMessages = !session->server->stopSendPresenceMessages;
        }
        if (!session->server->stopSendPresenceMessages)
        {
            session->server->presenceTimer -= GlobalAbsDeltaTime;
            if (session->server->presenceTimer < 0.0f)
            {
                session->server->presenceTimer = PRESENCE_MESSAGE_TIMEOUT;
                ServerPresenceMessage message = {};
                auto result = NetSend(session->server->connectionSocket, (void*)(&message), sizeof(message), {});
                SOKO_LOG_SERVER_SEND(result, ServerPresenceMessage);
            }
        }
        session->server->connectionTimer -= GlobalAbsDeltaTime;
        // TODO: Disconnect if timer is out
    }

    internal void
    SessionUpdateClient(GameSession* session)
    {
        auto recieveResult = BeginReadIncomingMessages(session->client->socket, session->socketBuffer);
        // TODO: Handle errors and normal disconnect differently
        if (recieveResult.status == NetRecieveResult::ConnectionClosed ||
            recieveResult.status == NetRecieveResult::ConnectionReset ||
            recieveResult.status == NetRecieveResult::Error)
        {
            ClientDisconnectFromServer(session->client);
            EndReadIncomingMessages(session->socketBuffer);
        }
        else if (recieveResult.status == NetRecieveResult::Success)
        {
            NetMessageHeader* header;
            do
            {
                header = GetNextIncomingMessage(session->socketBuffer);
                if (header)
                {
                    // TODO: Not reset the timer if message are broken
                    session->client->connectionTimer = CONNECTION_TIMEOUT;
                    switch (header->type)
                    {
                    case NetMessageHeader::ServerPresenceMessage:
                    {
                        SOKO_LOG_CLIENT_RECV(ServerPresenceMessage);
                    } break;
                    default: { SOKO_INFO("Client: recieved broken message"); } break;
                    }
                }
            }
            while (header);
        }

        EndReadIncomingMessages(session->socketBuffer);

        // NOTE: Send presence message
        // TODO: For debug only!!!
        if (JustPressed(KEY_P))
        {
            session->client->stopSendPresenceMessages = !session->client->stopSendPresenceMessages;
        }
        if (!session->client->stopSendPresenceMessages)
        {
            session->client->presenceTimer -= GlobalAbsDeltaTime;
            if (session->client->presenceTimer < 0.0f)
            {
                session->client->presenceTimer = PRESENCE_MESSAGE_TIMEOUT;
                ClientPresenceMessage message = {};
                auto result = NetSend(session->client->socket, (void*)(&message), sizeof(message), {});
                SOKO_LOG_CLIENT_SEND(result, ClientPresenceMessage);
            }
        }

        session->client->connectionTimer -= GlobalAbsDeltaTime;
    }
}
