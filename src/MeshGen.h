#pragma once
#include "FreeList.h"

namespace soko
{
    struct Chunk;

#pragma pack(push, 1)
    struct ChunkMeshVertex
    {
        v3 pos;
        v3 normal;
        byte tileId;
    };
#pragma pack(pop)

    struct ChunkMeshVertexBlock : LinkedBlock<ChunkMeshVertexBlock>
    {
        ChunkMeshVertexBlock* prevBlock;
        u32 at;
        v3 positions[MESH_GEN_VERTEX_BLOCK_CAPACITY];
        v3 normals[MESH_GEN_VERTEX_BLOCK_CAPACITY];
        byte tileIds[MESH_GEN_VERTEX_BLOCK_CAPACITY];
    };

    struct ChunkMesh
    {
        u32 vertexCount;
        u32 quadCount;
        u32 blockCount;
        ChunkMeshVertexBlock* head;
        ChunkMeshVertexBlock* tail;
    };

    struct Level;
    void GenChunkMesh(Level* level, Chunk* chunk, ChunkMesh* outMesh);
}
