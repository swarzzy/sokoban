#pragma once

namespace soko
{
    struct GameMenu
    {
        static constexpr u32 LEVEL_PATH_BUFFER_SIZE = 256;
        char levelPathBuffer[LEVEL_PATH_BUFFER_SIZE];
        bool levelLoaded;
    };
}
