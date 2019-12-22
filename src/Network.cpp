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
}
