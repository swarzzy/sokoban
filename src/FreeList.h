#pragma once

namespace soko
{
    // NOTE: Requires "next" ptr in T
    template<typename T>
    struct FreeList
    {
        T* head;
        u32 count;

        inline T* Get(MemoryArena* arena)
        {
            T* block = 0;
            if (this->head)
            {
                block = this->head;
                this->head = block->next;
                this->count--;
                block->next = 0;
            }
            if (!block)
            {
                // NOTE: Assumed mem cleared to zero
                block = PUSH_STRUCT(arena, T);
                SOKO_ASSERT(block);
            }
            return block;
        }

        inline void Push(T* block)
        {
            block->next = this->head;
            this->head = block;
            this->count++;
        }
    };
}
