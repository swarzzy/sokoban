#include "Network.h"

namespace soko::net
{
    static void
    PushPlayerAction(ClientInput* inputBuffer, PlayerAction action)
    {
        SOKO_ASSERT(inputBuffer->bufferAt <= ClientInput::BUFFER_SIZE);
        *(inputBuffer->buffer + inputBuffer->bufferAt) = (byte)action;
        inputBuffer->bufferAt++;
    }

    void
    CollectPlayerInput(ClientInput* inputBuffer)
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

}
