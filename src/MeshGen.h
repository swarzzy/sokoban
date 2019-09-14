#pragma once

namespace soko
{
    struct Chunk;
    struct ChunkMeshVertexBlock
    {
        const_val u32 CAPACITY = 4096;
        ChunkMeshVertexBlock* nextBlock;
        ChunkMeshVertexBlock* prevBlock;
        u32 at;
        v3 positions[CAPACITY];
        v3 normals[CAPACITY];
        byte tileIds[CAPACITY];
    };


    struct ChunkMesh
    {
        u64 vertexCount;
        u64 quadCount;
        u32 blockCount;
        ChunkMeshVertexBlock* head;
        ChunkMeshVertexBlock* tail;
    };

    ChunkMesh GenChunkMesh(Chunk* chunk, AB::MemoryArena* arena);


}
