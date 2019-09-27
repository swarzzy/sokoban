#include "Network.h"

namespace soko::net
{

    static void
    PushPlayerAction(InputBuffer* inputBuffer, PlayerAction action)
    {
        SOKO_ASSERT(inputBuffer->at <= InputBuffer::SIZE);
        *(inputBuffer->base + inputBuffer->at) = action;
        inputBuffer->at++;
    }

    void
    CollectPlayerInput(InputBuffer* inputBuffer)
    {
        if (JustPressed(AB::KEY_SPACE))
        {
            PushPlayerAction(inputBuffer, PlayerAction_ToggleInteractionMode);
        }

        if (JustPressed(AB::KEY_UP))
        {
            PushPlayerAction(inputBuffer, PlayerAction_MoveNorth);
        }

        if (JustPressed(AB::KEY_DOWN))
        {
            PushPlayerAction(inputBuffer, PlayerAction_MoveSouth);
        }

        if (JustPressed(AB::KEY_RIGHT))
        {
            PushPlayerAction(inputBuffer, PlayerAction_MoveEast);
        }

        if (JustPressed(AB::KEY_LEFT))
        {
            PushPlayerAction(inputBuffer, PlayerAction_MoveWest);
        }
    }

    bool
    InitializeServer(GameState* gameState, AB::MemoryArena* arena, i16 port)
    {
        bool result = false;
        uptr socket = NetCreateSocket();
        if (socket)
        {
            if (NetBindSocket(socket, port))
            {
                SOKO_ASSERT(!gameState->server);
                gameState->server = PUSH_STRUCT(arena, Server);
                if (gameState->server)
                {
                    Server* server = gameState->server;
                    server->socket = socket;
                    server->port = port;
                    result = true;
                }
            }
        }
        return result;
    }

    inline bool
    ServerAddPlayer(Server* server, Player* player, i16 slot, AB::NetAddress playerAddress)
    {
        bool result = false;
        if (slot >= 0 && slot < Server::SLOTS_NUM)
        {
            if (!server->slotsOccupancy[slot])
            {
                server->slotsOccupancy[slot] = true;
                server->slots[slot].player = player;
                server->slots[slot].address = playerAddress;
                result = true;
            }
        }
        return result;
    }

    inline i16
    ServerFindEmptySlot(const Server* server)
    {
        i16 result = -1;
        for (u32 i = 0; i < Server::SLOTS_NUM; i++)
        {
            if (!server->slotsOccupancy[i])
            {
                result = (i16)i;
                break;
            }
        }
        return result;
    }

    void
    ServerSendOutputMessages(GameState* gameState)
    {
        Server* server = gameState->server;
        byte* buffer = gameState->server->socketBuffer;
        for (u32 i = 0; i < Server::SLOTS_NUM; i++)
        {
            b32 slotOccupied = server->slotsOccupancy[i];
            ServerSlot* slot = server->slots + i;
            if (slotOccupied && slot->inputBuffer.at)
            {
                for (u32 inputIndex = 0;
                     inputIndex < slot->inputBuffer.at;
                     inputIndex++)
                {
                    PlayerAction action = (PlayerAction)slot->inputBuffer.base[inputIndex];
                    if (ActionIsMovement(action))
                    {
                        MoveEntity(gameState->level,
                                   slot->player->e,
                                   (Direction)action, gameState->memoryArena,
                                   slot->player->reversed);
                    }
                    else
                    {
                        switch (action)
                        {
                        case PlayerAction_ToggleInteractionMode:
                        {
                            slot->player->reversed = !slot->player->reversed;
                        } break;
                        INVALID_DEFAULT_CASE;
                        }
                    }
                }

                auto header = (ServerMsgHeader*)buffer;
                header->type = ServerMsg_PlayerAction;

                auto msg = (ServerPlayerActionMsg*)(buffer + sizeof(ServerMsgHeader));

                msg->slot = (i16)i;

                u32 inputOffset = sizeof(ServerMsgHeader) + sizeof(ServerPlayerActionMsg);
                // TODO: Check for buffer overflow
                COPY_BYTES(slot->inputBuffer.at, buffer + inputOffset, slot->inputBuffer.base);
                inputOffset += slot->inputBuffer.at;
                for (u32 sendIndex = 1; sendIndex < net::Server::SLOTS_NUM; sendIndex++)
                {
                    if (gameState->server->slotsOccupancy[sendIndex])
                    {
                        ServerSlot* sendSlot = server->slots + sendIndex;
                        auto[status, size] = NetSend(server->socket,
                                                     sendSlot->address,
                                                     buffer, inputOffset);
                        SOKO_ASSERT(status);
                    }
                }
                slot->inputBuffer.at = 0;
            }
        }
    }

    void
    ServerPollInputMessages(GameState* gameState)
    {
        Server* server = gameState->server;
        while (true)
        {
            auto[rcStatus, rcSize, rcFrom] = NetRecieve(server->socket, server->socketBuffer,
                                                        net::Server::SOCKET_BUFFER_SIZE);
            if (rcStatus == AB::NetRecieveResult::Success && rcSize) // TODO: empty packets
            {
                byte* buffer = server->socketBuffer;
                auto inHeader = (ClientMsgHeader*)server->socketBuffer;
                auto outHeader = (ServerMsgHeader*)server->socketBuffer;
                u32 bufferAt = sizeof(ServerMsgHeader);
                switch (inHeader->type)
                {
                case net::ClientMsg_Join:
                {
                    i16 freeSlot = ServerFindEmptySlot(server);

                    outHeader->type = ServerMsg_JoinResult;

                    auto msg = (ServerJoinResultMsg*)(buffer + sizeof(ServerMsgHeader));
                    bufferAt += sizeof(ServerJoinResultMsg);

                    Player* player = 0;
                    bool slotInitialized = false;
                    if (freeSlot != -1)
                    {
                        // TODO: Aaargh!! Using static
                        static i32 playerCount = 0;
                        v3i coord = V3I(13 + playerCount, 13, 1);
                        playerCount++;

                        player = AddPlayer(gameState, coord, gameState->memoryArena);

                        if (player)
                        {
                            if (ServerAddPlayer(server, player, freeSlot, rcFrom))
                            {
                                msg->succeed = 1;
                                msg->newPlayer = {freeSlot, coord.x, coord.y, coord.z};

                                for (i16 otherSlot = 0;
                                     otherSlot < Server::SLOTS_NUM;
                                     otherSlot++)
                                {
                                    if (otherSlot != freeSlot)
                                    {
                                        if (gameState->server->slotsOccupancy[otherSlot])
                                        {
                                            ServerSlot* s = server->slots + otherSlot;
                                            auto data = (NewPlayerData*)(buffer + bufferAt);
                                            bufferAt += sizeof(NewPlayerData);

                                            data->slot = otherSlot;
                                            data->x = s->player->e->coord.x;
                                            data->y = s->player->e->coord.y;
                                            data->z = s->player->e->coord.z;
                                            msg->otherPlayersCount++;
                                        }
                                    }
                                }
                                slotInitialized = true;
                            }
                        }
                    }

                    if (!slotInitialized)
                    {
                        msg->succeed = 0;
                    }

                    auto[sndStatus, sndSize] =
                        NetSend(server->socket, rcFrom, buffer, bufferAt);

                    if (!sndStatus && slotInitialized)
                    {
                        DeletePlayer(gameState, player);
                        gameState->server->slotsOccupancy[freeSlot] = 0;
                    }

                    // NOTE: Recieving new player data to all other players
                    if (slotInitialized && sndStatus)
                    {
                        bufferAt = sizeof(ServerMsgHeader);
                        outHeader->type = ServerMsg_AddPlayer;
                        auto msg = (ServerAddPlayerMsg*)(buffer + bufferAt);
                        bufferAt += sizeof(ServerAddPlayerMsg);
                        msg->newPlayer = {freeSlot, player->e->coord.x, player->e->coord.y, player->e->coord.z};

                        for (u32 i = 0; i < Server::SLOTS_NUM; i++)
                        {
                            bool slotOccupied = server->slotsOccupancy[i];
                            if (slotOccupied && (i != freeSlot) && (i != Server::LOCAL_PLAYER_SLOT))
                            {
                                auto* s = server->slots + i;
                                auto[sndStatus, sndSize] =
                                    NetSend(server->socket, s->address, buffer, bufferAt);
                                // TODO: Make sure that all players get this info
                                // And revert all join stuff in not
                                SOKO_ASSERT(sndStatus);
                            }
                        }
                    }
                } break;

                case ClientMsg_Leave:
                {
                    i16 slot = inHeader->slot;
                    if (slot >= 0 && slot < Server::SLOTS_NUM)
                    {
                        if (server->slotsOccupancy[slot])
                        {
                            server->slotsOccupancy[slot] = 0;
                            DeletePlayer(gameState, server->slots[slot].player);

                            outHeader->type = ServerMsg_DeletePlayer;

                            auto msg = (ServerDeletePlayerMsg*)(buffer + bufferAt);
                            bufferAt += sizeof(ServerDeletePlayerMsg);

                            msg->slot = slot;

                            for (u32 i = 0; i < Server::SLOTS_NUM; i++)
                            {
                                if (i != slot &&
                                    i != Server::LOCAL_PLAYER_SLOT &&
                                    server->slotsOccupancy[i])
                                {
                                    ServerSlot* s = server->slots + i;
                                    auto[sndStatus, sndSize] =
                                        NetSend(server->socket, s->address, buffer, bufferAt);
                                    // TODO: Handle send error
                                    SOKO_ASSERT(sndStatus);
                                }
                            }
                        }
                    }
                } break;

                case ClientMsg_PlayerAction:
                {
                    if (inHeader->slot >= 0 && inHeader->slot < Server::SLOTS_NUM)
                    {
                        ServerSlot* slot = server->slots + inHeader->slot;
                        u32 recvInputSize = rcSize - sizeof(ClientMsgHeader);
                        // TODO: Handle case when recieved input is too big
                        if (recvInputSize > (InputBuffer::SIZE - slot->inputBuffer.at))
                        {
                            recvInputSize = InputBuffer::SIZE - slot->inputBuffer.at;
                        }
                        COPY_BYTES(recvInputSize,
                                   slot->inputBuffer.base + slot->inputBuffer.at,
                                   buffer + sizeof(ClientMsgHeader));

                        slot->inputBuffer.at += recvInputSize;
                    }
                } break;
                INVALID_DEFAULT_CASE;
                }
            }
            else if (rcStatus == AB::NetRecieveResult::Nothing)
            {
                break;
            }
            else
            {
                // TODO: Handle socket error (Maybe just disconnest)
                INVALID_CODE_PATH;
            }
        }

    }

    // TODO: Store client and server in transient slorage in order to delete
    // when it no longer needed

    bool InitializeClient(GameState* gameState)
    {
        bool result = false;
        if (!gameState->client)
        {
            gameState->client = PUSH_STRUCT(gameState->memoryArena, net::Client);
            if (gameState->client)
            {
                uptr socket = NetCreateSocket();
                if (socket)
                {
                    gameState->client->joinTimeout = 15.0f;
                    gameState->client->socket = socket;
                    result = true;
                }
            }
        }
        return result;
    }

    ConnectionStatus
    ClientTryToConnect(GameState* gameState, AB::NetAddress serverAddr = {})
    {
        ConnectionStatus result = ConnectionStatus_None;
        Client* client = gameState->client;
        byte* buffer = client->socketBuffer;

        ConnectionStatus currentStatus = client->connectionStatus;

        if (currentStatus == ConnectionStatus_None)
        {
            u32 bufferAt = 0;
            client->serverAddr = serverAddr;
            buffer[bufferAt] = net::ClientMsg_Join;
            bufferAt += 1;

            auto[sndStatus, sndSize] = NetSend(client->socket, serverAddr,
                                               buffer, bufferAt);
            if (sndStatus)
            {
                client->waitingTime = client->joinTimeout;
                result = ConnectionStatus_Waiting;
            }
        }

        if (client->waitingTime)
        {
            client->waitingTime -= GlobalAbsDeltaTime;

            auto[recvStatus, recvSize, recvFrom] =
                NetRecieve(client->socket, buffer, net::Server::SOCKET_BUFFER_SIZE);
            if (recvStatus == AB::NetRecieveResult::Success && recvSize)
            {
                if (buffer[0] == net::ServerMsg_JoinResult)
                {
                    auto msg = (net::ServerJoinResultMsg*)(buffer + 1);
                    u32 bufferAt = sizeof(net::ServerJoinResultMsg) + 1;
                    if (msg->succeed)
                    {
                        v3i playerCoord = V3I(msg->newPlayer.x, msg->newPlayer.y, msg->newPlayer.z);
                        Player* player = AddPlayer(gameState, playerCoord, gameState->memoryArena);
                        if (player)
                        {
                            gameState->controlledPlayer = player;
                            gameState->client->playerSlot = msg->newPlayer.slot;

                            // TODO: Check for overflow
                            gameState->client->slotsOccupancy[gameState->client->playerSlot] = 1;
                            gameState->client->slots[gameState->client->playerSlot].player = player;

                            bufferAt = sizeof(net::ServerJoinResultMsg) + 1;

                            i32 levelNameLength = msg->levelNameLength;
                            char* levelName = (char*)(buffer + bufferAt);
                            bufferAt += levelNameLength;
                            // TODO Clenup on level name size constants
                            wchar_t nameBuffer[CLIENT_LEVEL_NAME_LEN];
                            mbstowcs(nameBuffer, levelName, CLIENT_LEVEL_NAME_LEN);
                            if (DebugGetFileSize(nameBuffer))
                            {
                                COPY_BYTES(sizeof(wchar_t) * CLIENT_LEVEL_NAME_LEN, client->levelName, nameBuffer);
                                while (bufferAt < recvSize)
                                {
                                    auto nextPlayer = (net::NewPlayerData*)(buffer + bufferAt);
                                    bufferAt += sizeof(net::NewPlayerData);

                                    v3i coord = V3I(nextPlayer->x, nextPlayer->y, nextPlayer->z);
                                    Player* player = AddPlayer(gameState, coord, gameState->memoryArena);
                                    SOKO_ASSERT(player);
                                    // TODO: Check for overflow
                                    gameState->client->slotsOccupancy[nextPlayer->slot] = 1;
                                    gameState->client->slots[nextPlayer->slot].player = player;
                                }
                                result = ConnectionStatus_Connected;
                            }
                        }
                    }
                }
            }
            else if (recvStatus == AB::NetRecieveResult::Nothing)
            {
                result = ConnectionStatus_Waiting;
            }
            else
            {
                result = ConnectionStatus_None;
                client->serverAddr;
                // TODO: Log error
            }
        }
        else
        {
            client->serverAddr;
            // TODO: Log timeout
        }
        return result;
    }
}
