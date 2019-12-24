#pragma once
namespace soko
{
    enum PlayerSlot
    {
        PlayerSlot_First,
        PlayerSlot_Second
    };

    enum PlayerAction : byte
    {
        // NOTE: Movement action values should be same
        // as in Direction enum
        PlayerAction_MoveNorth = 1,
        PlayerAction_MoveSouth,
        PlayerAction_MoveWest,
        PlayerAction_MoveEast,
        PlayerAction_MoveUp,
        PlayerAction_MoveDown,
        PlayerAction_ToggleInteractionMode
    };

    static_assert((byte)PlayerAction_MoveNorth == (byte)Direction_North);
    static_assert((byte)PlayerAction_MoveSouth == (byte)Direction_South);
    static_assert((byte)PlayerAction_MoveWest == (byte)Direction_West);
    static_assert((byte)PlayerAction_MoveEast == (byte)Direction_East);
    static_assert((byte)PlayerAction_MoveUp == (byte)Direction_Up);
    static_assert((byte)PlayerAction_MoveDown == (byte)Direction_Down);

    inline bool ActionIsMovement(PlayerAction action)
    {
        bool result = (action > 0 && action <= PlayerAction_MoveDown);
        return result;
    }

    struct PlayerActionBuffer
    {
        PlayerSlot slot;
        u32 at;
        PlayerAction actions[256];
    };

    internal void UpdatePlayer(Level* level, Entity* e);
}
