#pragma once
namespace soko
{
#pragma pack(push, 1)

    enum PlayerSlot : byte
    {
        PlayerSlot_First,
        PlayerSlot_Second
    };

    struct PlayerAction
    {
        enum Action : byte
        {
            // NOTE: Movement action values should be same
            // as in Direction enum
            MoveNorth = 1,
            MoveSouth,
            MoveWest,
            MoveEast,
            MoveUp,
            MoveDown,
            ToggleInteractionMode
        } action;
        PlayerSlot slot;
    };
#pragma pack(pop)

    static_assert((byte)(PlayerAction::MoveNorth) == (byte)Direction_North);
    static_assert((byte)(PlayerAction::MoveSouth) == (byte)Direction_South);
    static_assert((byte)(PlayerAction::MoveWest) == (byte)Direction_West);
    static_assert((byte)(PlayerAction::MoveEast) == (byte)Direction_East);
    static_assert((byte)(PlayerAction::MoveUp) == (byte)Direction_Up);
    static_assert((byte)(PlayerAction::MoveDown) == (byte)Direction_Down);

    inline bool ActionIsMovement(PlayerAction action)
    {
        bool result = (action.action > 0 && action.action <= PlayerAction::MoveDown);
        return result;
    }

    template<u32 Size>
    struct PlayerActionBuffer
    {
        static constexpr u32 BufferSize = Size;
        u32 at;
        PlayerAction actions[Size];
    };

    internal void UpdatePlayer(Level* level, Entity* e);
}
