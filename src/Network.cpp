#include "Network.h"

namespace soko::net
{

    static void
    PushPlayerAction(InputBuffer* inputBuffer, PlayerAction action)
    {
        SOKO_ASSERT(inputBuffer->at <= PLAYER_INPUT_BUFFER_SIZE);
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

    internal Server*
    InitializeServer(AB::MemoryArena* arena, i16 port)
    {
        Server* result = 0;
        uptr socket = NetCreateSocket();
        if (socket)
        {
            if (NetBindSocket(socket, port))
            {
                result = PUSH_STRUCT(arena, Server);
                if (result)
                {
                    result->socket = socket;
                    result->port = port;
                }
            }
        }
        return result;
    }

    inline bool
    ServerAddPlayer(Server* server, Player* player, i16 slot, AB::NetAddress playerAddress)
    {
        bool result = false;
        if (slot >= 0 && slot < SERVER_SLOTS_NUM)
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
        for (u32 i = 0; i < SERVER_SLOTS_NUM; i++)
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
        for (u32 i = 0; i < SERVER_SLOTS_NUM; i++)
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
                for (u32 sendIndex = 1; sendIndex < net::SERVER_SLOTS_NUM; sendIndex++)
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
                                                        net::SERVER_SOCKET_BUFFER_SIZE);
            if (rcStatus == AB::NetRecieveResult::Success && rcSize) // TODO: empty packets
            {
                byte* buffer = server->socketBuffer;
                auto inHeader = (ClientMsgHeader*)server->socketBuffer;
                auto outHeader = (ServerMsgHeader*)server->socketBuffer;
                u32 bufferAt = sizeof(ServerMsgHeader);
                switch (inHeader->type)
                {
                case ClientMsg_QueryState:
                {
                    outHeader->type = ServerMsg_State;
                    auto msg = (ServerMsgState*)(buffer + sizeof(ServerMsgHeader));
                    msg->hasAvailableSlot = ServerFindEmptySlot(server);
                    msg->levelNameStrLen = SERVER_MAX_LEVEL_NAME_LEN;
                    void* name = (byte*)msg + sizeof(ServerMsgState);
                    bufferAt += sizeof(ServerMsgState);
                    COPY_BYTES(SERVER_MAX_LEVEL_NAME_LEN, name, server->levelName);
                    bufferAt += SERVER_MAX_LEVEL_NAME_LEN;

                    auto[sndStatus, sndSize] =
                        NetSend(server->socket, rcFrom, buffer, bufferAt);
                    SOKO_ASSERT(sndStatus);

                    // TODO: check for errors
                } break;
                case ClientMsg_Join:
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
                                     otherSlot < SERVER_SLOTS_NUM;
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

                        for (u32 i = 0; i < SERVER_SLOTS_NUM; i++)
                        {
                            bool slotOccupied = server->slotsOccupancy[i];
                            if (slotOccupied && (i != freeSlot) && (i != SERVER_LOCAL_PLAYER_SLOT))
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
                    if (slot >= 0 && slot < SERVER_SLOTS_NUM)
                    {
                        if (server->slotsOccupancy[slot])
                        {
                            server->slotsOccupancy[slot] = 0;
                            DeletePlayer(gameState, server->slots[slot].player);

                            outHeader->type = ServerMsg_DeletePlayer;

                            auto msg = (ServerDeletePlayerMsg*)(buffer + bufferAt);
                            bufferAt += sizeof(ServerDeletePlayerMsg);

                            msg->slot = slot;

                            for (u32 i = 0; i < SERVER_SLOTS_NUM; i++)
                            {
                                if (i != slot &&
                                    i != SERVER_LOCAL_PLAYER_SLOT &&
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
                    if (inHeader->slot >= 0 && inHeader->slot < SERVER_SLOTS_NUM)
                    {
                        ServerSlot* slot = server->slots + inHeader->slot;
                        u32 recvInputSize = rcSize - sizeof(ClientMsgHeader);
                        // TODO: Handle case when recieved input is too big
                        if (recvInputSize > (PLAYER_INPUT_BUFFER_SIZE - slot->inputBuffer.at))
                        {
                            recvInputSize = PLAYER_INPUT_BUFFER_SIZE - slot->inputBuffer.at;
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

    internal Client*
    InitializeClient(AB::MemoryArena* arena)
    {
        Client* result = 0;
        Client* client = PUSH_STRUCT(arena, Client);
        if (client)
        {
            uptr socket = NetCreateSocket();
            if (socket)
            {
                client->joinTimeout = 15.0f;
                client->socket = socket;
                result = client;
            }
        }
        return result;
    }

    inline bool
    SendServerStateQuery(uptr socket, AB::NetAddress serverAddr)
    {
        bool result;
        ClientMsgHeader msg = {};
        msg.type = ClientMsg_QueryState;
        msg.slot = -1;

        auto[sndStatus, sndSize] = NetSend(socket, serverAddr,
                                           (void*)&msg, sizeof(ClientMsgHeader));
        result = sndStatus;
        return result;
    }

    internal i32
    ClientWaitForServerState(Socket socket, void* buffer, u32 bufferSize)
    {
        i32 result = 0;
        auto[recvStatus, recvSize, recvFrom] =
            NetRecieve(socket, buffer, bufferSize);
        if (recvStatus == AB::NetRecieveResult::Success && recvSize)
        {
            auto header = (ServerMsgHeader*)buffer;
            if (header->type == ServerMsg_State)
            {
                result = 1;
            }
        }
        else if (recvStatus == AB::NetRecieveResult::Nothing)
        {
        }
        else
        {
            result = -1;
        }
        return result;
    }

    inline bool
    ClientSendConnectionQuery(uptr socket, AB::NetAddress serverAddr)
    {
        bool result;
        ClientMsgHeader msg = {};
        msg.type = ClientMsg_Join;
        msg.slot = -1;

        auto[sndStatus, sndSize] = NetSend(socket, serverAddr,
                                           (void*)&msg, sizeof(ClientMsgHeader));
        result = sndStatus;
        return result;
    }

    internal i32
    ClientWaitForConnectionResult(Socket socket, void* buffer, u32 bufferSize)
    {
        i32 result = 0;
        auto[recvStatus, recvSize, recvFrom] =
            NetRecieve(socket, buffer, bufferSize);
        if (recvStatus == AB::NetRecieveResult::Success && recvSize)
        {
            auto header = (ServerMsgHeader*)buffer;
            if (header->type == ServerMsg_JoinResult)
            {
                result = recvSize;
            }
        }
        else if (recvStatus == AB::NetRecieveResult::Nothing)
        {
        }
        else
        {
            result = -1;
        }
        return result;
    }

    internal bool
    ClientEstablishConnection(net::Client* client, ServerJoinResultMsg* msg, u32 messageSize, GameState* gameState)
    {
        bool result = 0;
        byte* buffer = client->socketBuffer;
        u32 bufferAt = 0;
        if (msg->succeed)
        {
            v3i playerCoord = V3I(msg->newPlayer.x, msg->newPlayer.y, msg->newPlayer.z);
            Player* player = AddPlayer(gameState, playerCoord, gameState->memoryArena);
            if (player)
            {
                gameState->controlledPlayer = player;
                client->playerSlot = msg->newPlayer.slot;

                // TODO: Check for overflow
                client->slotsOccupancy[client->playerSlot] = 1;
                client->slots[client->playerSlot].player = player;

                bufferAt = sizeof(ServerJoinResultMsg) + 1;
                while (bufferAt < messageSize)
                {
                    auto nextPlayer = (NewPlayerData*)(buffer + bufferAt);
                    bufferAt += sizeof(NewPlayerData);

                    v3i coord = V3I(nextPlayer->x, nextPlayer->y, nextPlayer->z);
                    Player* player = AddPlayer(gameState, coord, gameState->memoryArena);
                    SOKO_ASSERT(player);
                    // TODO: Check for overflow
                    client->slotsOccupancy[nextPlayer->slot] = 1;
                    client->slots[nextPlayer->slot].player = player;
                }
                result = 1;
            }
        }
        return result;
    }
#if 0
    ClientConnectionResult
    ClientTryToConnect(GameState* gameState, AB::MemoryArena* tempArena, AB::NetAddress serverAddr = {})
    {
        ClientConnectionResult result = {};
        Client* client = gameState->client;
        byte* buffer = client->socketBuffer;

        ConnectionStatus currentStatus = client->connectionStatus;

        if (currentStatus == ConnectionStatus_None)
        {
            u32 bufferAt = 0;
            client->serverAddr = serverAddr;
            auto msg = (ClientMsgHeader*)(buffer);
            msg->type = ClientMsg_QueryState;
            msg->slot = -1;
            bufferAt += sizeof(ClientMsgHeader);

            auto[sndStatus, sndSize] = NetSend(client->socket, serverAddr,
                                               buffer, bufferAt);
            if (sndStatus)
            {
                client->waitingTime = client->joinTimeout;
                result.status = ConnectionStatus_Waiting;
            }
        }

        if (client->waitingTime)
        {
            client->waitingTime -= GlobalAbsDeltaTime;

            auto[recvStatus, recvSize, recvFrom] =
                NetRecieve(client->socket, buffer, net::SERVER_SOCKET_BUFFER_SIZE);
            if (recvStatus == AB::NetRecieveResult::Success && recvSize)
            {
                auto header = (ServerMsgHeader*)buffer;
                if (header->type == ServerMsg_State)
                {
                    auto stateMsg = (ServerMsgState*)(buffer + sizeof(ServerMsgHeader));
                    auto levelName = (char*)stateMsg + sizeof(ServerMsgState);
                    SOKO_ASSERT(stateMsg.levelNameStrLen <= SERVER_MAX_LEVEL_NAME_LEN);
                    if (stateMsg->hasAvailableSlot)
                    {
                        result.levelName = PUSH_SIZE(tempArena, stateMsg->levelNameStrLen);
                        if (result.levelName)
                        {
                            result.status = ConnectionStatus_Connected;
                            CORY_BYTES(stateMsg->levelNameStrLen, result.levelName, levelName);
                        }
                    }
                }

                if (buffer[0] == ServerMsg_JoinResult)
                {
                    auto msg = (ServerJoinResultMsg*)(buffer + 1);
                    u32 bufferAt = sizeof(ServerJoinResultMsg) + 1;
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

                            bufferAt = sizeof(ServerJoinResultMsg) + 1;

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
                                    auto nextPlayer = (NewPlayerData*)(buffer + bufferAt);
                                    bufferAt += sizeof(NewPlayerData);

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
#endif
}
