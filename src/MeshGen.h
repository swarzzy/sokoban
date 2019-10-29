#pragma once

namespace soko
{
    struct Chunk;

    constant u32 CHUNK_MESH_VERTEX_BLOCK_CAPACITY = 4096;

#pragma pack(push, 1)
    struct ChunkMeshVertex
    {
        v3 pos;
        v3 normal;
        byte tileId;
        byte AO;
    };
#pragma pack(pop)

    struct ChunkMeshVertexBlock
    {
        ChunkMeshVertexBlock* nextBlock;
        ChunkMeshVertexBlock* prevBlock;
        u32 at;
        v3 positions[CHUNK_MESH_VERTEX_BLOCK_CAPACITY];
        v3 normals[CHUNK_MESH_VERTEX_BLOCK_CAPACITY];
        byte tileIds[CHUNK_MESH_VERTEX_BLOCK_CAPACITY];
        byte AO[CHUNK_MESH_VERTEX_BLOCK_CAPACITY];
    };

    struct ChunkMesh
    {
        u64 vertexCount;
        u64 quadCount;
        u32 blockCount;
        ChunkMeshVertexBlock* head;
        ChunkMeshVertexBlock* tail;
        ChunkMeshVertexBlock* free;
    };

    struct Level;
    internal bool GenChunkMesh(Level* level, Chunk* chunk, ChunkMesh* outMesh, AB::MemoryArena* arena);
}
