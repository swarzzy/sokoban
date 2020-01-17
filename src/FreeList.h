#pragma once

namespace soko
{
    template<typename T>
    struct LinkedBlock
    {
        T* next;
    };

    template<typename T>
    struct FreeList
    {
        LinkedBlock<T>* head;
        u32 count;

        inline T* Get(MemoryArena* arena)
        {
            T* block = 0;
            if (this->head)
            {
                block = static_cast<T*>(this->head);
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

        inline void Push(LinkedBlock<T>* block)
        {
            block->next = static_cast<T*>(this->head);
            this->head = block;
            this->count++;
        }
    };
}
