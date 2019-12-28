#include "Network.h"

namespace soko
{
    template<u32 Size> internal void
    ServerUploadLocalActionBuffer(Server* server, PlayerActionBuffer<Size>* buffer)
    {
        // NOTE: Not sening but just copying actually
        u32 serverBufferFree = server->actionBuffer.BufferSize - server->actionBuffer.at;
        u32 copyCount = serverBufferFree < buffer->at ? serverBufferFree : buffer->at;
        u32 copySize = copyCount * sizeof(PlayerAction);
        Memcpy(server->actionBuffer.actions + server->actionBuffer.at,
               buffer->actions, copySize);
        server->actionBuffer.at += copyCount;
        buffer->at = 0;
        SOKO_ASSERT(server->actionBuffer.at <= server->actionBuffer.BufferSize);
    }

    template<u32 Size> internal void
    ServerDownloadLocalActionBuffer(Server* server, PlayerActionBuffer<Size>* buffer)
    {
        // NOTE: Not sening but just copying actually
        u32 bufferFree = buffer->BufferSize - buffer->at;
        u32 copyCount = bufferFree < server->actionBuffer.at ? bufferFree : server->actionBuffer.at;
        u32 copySize = copyCount * sizeof(PlayerAction);
        Memcpy(buffer->actions + buffer->at,
               server->actionBuffer.actions, copySize);
        buffer->at += copyCount;
    }

    template<u32 Size> internal void
    ServerSendActionBuffer(Server* server, PlayerActionBuffer<Size>* buffer)
    {
        if (buffer->at > 0)
        {
            // TODO: Stop wasting stack space
            const u32 messageSize = sizeof(ServerActionSequenceMessage) + (Size - 1) * sizeof(PlayerAction);
            byte messageMem[messageSize];
            ServerActionSequenceMessage* message = (ServerActionSequenceMessage*)messageMem;
            *message = {};
            u32 actualMessageSize = sizeof(ServerActionSequenceMessage) + (buffer->at * sizeof(PlayerAction)) - sizeof(PlayerAction);
            message->header.messageSize = actualMessageSize;
            message->actionCount = buffer->at;
            PlayerAction* actions = (PlayerAction*)(&message->firstAction);

            Memcpy(actions, buffer->actions, sizeof(PlayerAction) * buffer->at);

            auto result = NetSend(server->connectionSocket, (void*)(message), actualMessageSize, {});
            SOKO_LOG_SERVER_SEND(result, ServerActionSequenceMessage);

            //buffer->at = 0;
        }
    }

    template<u32 Size> internal void
    PrintActionBuffer(PlayerActionBuffer<Size>* buffer)
    {
        if (buffer->at)
        {
            PrintString("ActionBuffer contents:\n");
            for (u32 i = 0; i < buffer->at; i++)
            {
                auto action = buffer->actions[i];
                const char* slotStr = action.slot == PlayerSlot_First ? "First" : "Second";
                PrintString("Action: %u8, %s\n", (u8)action.action, slotStr);
            }
        }
    }


    template<u32 Size> internal void
    ClientSendActionBuffer(Client* client, PlayerActionBuffer<Size>* buffer)
    {
        if (buffer->at > 0)
        {
            // TODO: Stop wasting stack space
            const u32 messageSize = sizeof(ClientActionSequenceMessage) + Size - 1;
            byte messageMem[messageSize];
            ClientActionSequenceMessage* message = (ClientActionSequenceMessage*)messageMem;
            *message = {};
            u32 actualMessageSize = sizeof(ClientActionSequenceMessage) + (buffer->at * sizeof(byte)) - 1;
            message->header.messageSize = actualMessageSize;
            message->actionCount = buffer->at;
            byte* actions = &message->firstAction;

            for (u32 i = 0; i < buffer->at; i++)
            {
                actions[i] = buffer->actions[i].action;
            }

            auto result = NetSend(client->socket, (void*)(message), actualMessageSize, {});
            SOKO_LOG_CLIENT_SEND(result, ClientActionSequenceMessage);

            buffer->at = 0;
        }
    }

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
        server->connectedPlayerName[0] = 0;
        server->playerConnected = false;
    }

    internal void
    ShutdownServer(Server* server)
    {
        if (server->listenSocket.handle)
        {
            NetCloseSocket(server->listenSocket);
        }
        if (server->connectionSocket.handle)
        {
            NetCloseSocket(server->connectionSocket);
        }
        *server = {};
    }

    internal void
    ClientDisconnectFromServer(Client* client)
    {
        NetCloseSocket(client->socket);
        *client = {};
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
        if (session->server->state == Server::Connected)
        {
            auto recieveResult = BeginReadIncomingMessages(session->server->connectionSocket, session->socketBuffer);
            // TODO: Handle errors and normal disconnects differently
            if (recieveResult.status == NetRecieveResult::ConnectionClosed ||
                recieveResult.status == NetRecieveResult::ConnectionReset ||
                recieveResult.status == NetRecieveResult::Error)
            {
                EndReadIncomingMessages(session->socketBuffer);
                ServerDisconnectPlayer(session->server);
                ShutdownServer(session->server);
                DeleteEntity(session->level, session->secondPlayer);
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
                        case NetMessageHeader::ClientActionSequenceMessage:
                        {
                            SOKO_LOG_SERVER_RECV(ClientActionSequenceMessage);
                            auto* message = (ClientActionSequenceMessage*)header;

                            for (u32 i = 0; i < message->actionCount; i++)
                            {
                                PlayerAction::Action action = (PlayerAction::Action)((&message->firstAction)[i]);
                                if(!PushPlayerAction(&session->server->actionBuffer, { action, PlayerSlot_Second } ))
                                {
                                    SOKO_INFO("Server: input buffer overflow");
                                    break;
                                }
                            }

                        } break;
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

            //PrintActionBuffer(&session->server->actionBuffer);
            ServerSendActionBuffer(session->server, &session->server->actionBuffer);


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
            if (session->server->connectionTimer < 0.0f)
            {
                ServerDisconnectPlayer(session->server);
                ShutdownServer(session->server);
                DeleteEntity(session->level, session->secondPlayer);
            }
        }
    }

    internal void
    SessionUpdateClient(GameSession* session)
    {
        if (session->client->connectionEstablished)
        {
            auto recieveResult = BeginReadIncomingMessages(session->client->socket, session->socketBuffer);
            // TODO: Handle errors and normal disconnect differently
            if (recieveResult.status == NetRecieveResult::ConnectionClosed ||
                recieveResult.status == NetRecieveResult::ConnectionReset ||
                recieveResult.status == NetRecieveResult::Error)
            {
                EndReadIncomingMessages(session->socketBuffer);
                ClientDisconnectFromServer(session->client);
                DeleteEntity(session->level, session->firstPlayer);
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
                        case NetMessageHeader::ServerActionSequenceMessage:
                        {
                            SOKO_LOG_CLIENT_RECV(ServerActionSequenceMessage);
                            auto* message = (ServerActionSequenceMessage*)header;

                            for (u32 i = 0; i < message->actionCount; i++)
                            {
                                PlayerAction* action = (PlayerAction*)((&message->firstAction) + i);
                                if(!PushPlayerAction(&session->playerActionBuffer, *action))
                                {
                                    SOKO_INFO("Client: input buffer overflow");
                                    break;
                                }
                            }

                        } break;
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
            if (session->client->connectionTimer < 0.0f)
            {
                ClientDisconnectFromServer(session->client);
                DeleteEntity(session->level, session->firstPlayer);
            }
        }
    }
}
