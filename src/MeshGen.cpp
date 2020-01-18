#include "MeshGen.h"

namespace soko
{
    inline void
    PushChunkMeshVertex(Level* level, ChunkMesh* mesh, v3 position, v3 normal, byte tileId)
    {
        if (!mesh->head)
        {
            mesh->head = level->mesherFreeList.Get(level->sessionArena);
            ZERO_STRUCT(ChunkMeshVertexBlock, mesh->head);
            mesh->tail = mesh->head;
            mesh->blockCount++;
            level->globalChunkMeshBlockCount++;
        }
        else if (mesh->head->at >= MESH_GEN_VERTEX_BLOCK_CAPACITY)
        {
            ChunkMeshVertexBlock* newBlock = level->mesherFreeList.Get(level->sessionArena);
            ZERO_STRUCT(ChunkMeshVertexBlock, newBlock);
            mesh->head->prevBlock = newBlock;
            newBlock->next = mesh->head;

            mesh->head = newBlock;
            mesh->blockCount++;
            level->globalChunkMeshBlockCount++;
        }
        // NOTE: Converting to right-handed system
        position.z = -position.z;
        normal.z = -normal.z;

        mesh->head->positions[mesh->head->at] = position;
        mesh->head->normals[mesh->head->at] = normal;
        mesh->head->tileIds[mesh->head->at] = tileId;
        mesh->head->at++;
        mesh->vertexCount++;
    }

    inline void
    PushChunkMeshQuad(Level* level, ChunkMesh* mesh,
                      v3 vtx0, v3 vtx1, v3 vtx2, v3 vtx3,
                      TileValue val)
    {
        v3 normal = Cross(vtx3 - vtx0, vtx1 - vtx0);

        PushChunkMeshVertex(level, mesh, vtx0, normal, val);
        PushChunkMeshVertex(level, mesh, vtx1, normal, val);
        PushChunkMeshVertex(level, mesh, vtx2, normal, val);
        PushChunkMeshVertex(level, mesh, vtx3, normal, val);

        mesh->quadCount++;
    }

    inline void
    BuildFace(Level* level, Chunk* chunk, ChunkMesh* outMesh, iv3 p, Direction dir)
    {

       const Tile* testTile = GetTileInChunk(chunk, p.x, p.y, p.z);
       SOKO_ASSERT(testTile);

        // NOTE: This stage works in left-handed coord system
        // and will be converted to right-handed in push vertex call
        v3 offset = V3((p.x * LEVEL_TILE_SIZE),
                       p.z * LEVEL_TILE_SIZE,
                       (p.y * LEVEL_TILE_SIZE));

        v3 min = offset - 0.5f * V3(LEVEL_TILE_SIZE);
        min.z = offset.z + 0.5f * LEVEL_TILE_SIZE;
        v3 max = offset + 0.5f * V3(LEVEL_TILE_SIZE);
        max.z = offset.z - 0.5f * LEVEL_TILE_SIZE;

        TileValue val = testTile->value;

        v3 vtx0 = V3(min.x, min.y, max.z);
        v3 vtx1 = V3(max.x, min.y, max.z);
        v3 vtx2 = V3(max.x, max.y, max.z);
        v3 vtx3 = V3(min.x, max.y, max.z);

        v3 vtx4 = V3(min.x, min.y, min.z);
        v3 vtx5 = V3(max.x, min.y, min.z);
        v3 vtx6 = V3(max.x, max.y, min.z);
        v3 vtx7 = V3(min.x, max.y, min.z);

        switch (dir)
        {
            CASE(Direction_North, PushChunkMeshQuad(level, outMesh, vtx5, vtx4, vtx7, vtx6, val));
            CASE(Direction_South, PushChunkMeshQuad(level, outMesh, vtx0, vtx1, vtx2, vtx3, val));
            CASE(Direction_West, PushChunkMeshQuad(level, outMesh, vtx4, vtx0, vtx3, vtx7, val));
            CASE(Direction_East, PushChunkMeshQuad(level, outMesh, vtx1, vtx5, vtx6, vtx2, val));
            CASE(Direction_Up, PushChunkMeshQuad(level, outMesh, vtx3, vtx2, vtx6, vtx7, val));
            CASE(Direction_Down, PushChunkMeshQuad(level, outMesh, vtx4, vtx5, vtx1, vtx0, val));
            default: {} break;
        }
    }

    internal void
    GenChunkMesh(Level* level, Chunk* chunk, ChunkMesh* outMesh)
    {
        auto beginTime = GetTimeStamp();

        if (outMesh->vertexCount)
        {
            SOKO_ASSERT(outMesh->head);
            SOKO_ASSERT(outMesh->tail);
            // TODO: @Speed: Stop freeing all blocks before generate
            ChunkMeshVertexBlock* block = outMesh->head;
            while (true)
            {
                auto next = block->next;

                level->mesherFreeList.Push(block);
                level->globalChunkMeshBlockCount--;

                if (!next) break;
                block = next;
            }

            Memset(outMesh, 0, sizeof(ChunkMesh));
        }

        for (u32 tileZ = 0; tileZ < CHUNK_DIM; tileZ++)
        {
            for (u32 tileY = 0; tileY < CHUNK_DIM; tileY++)
            {
                for (u32 tileX = 0; tileX < CHUNK_DIM; tileX++)
                {
                    const Tile* testTile = GetTileInChunk(chunk, tileX, tileY, tileZ);

                    if (TileIsTerrain(testTile))
                    {
                        // NOTE: Using unsigned int wrapping here.
                        // Wrapping is UB according to C++ spec
                        // But it seems that on most compilers
                        // (including msvc, clang, gcc)
                        // Wrapping has well defined behavior
                        const Tile* upTile = GetTileInChunk(chunk, tileX, tileY, tileZ + 1);
                        const Tile* dnTile = GetTileInChunk(chunk, tileX, tileY, tileZ - 1);
                        const Tile* lTile = GetTileInChunk(chunk, tileX - 1, tileY, tileZ);
                        const Tile* rTile = GetTileInChunk(chunk, tileX + 1, tileY, tileZ);
                        const Tile* fTile = GetTileInChunk(chunk, tileX, tileY - 1, tileZ);
                        const Tile* bTile = GetTileInChunk(chunk, tileX, tileY + 1, tileZ);

                        bool dnEmpty = (tileZ == 0) ? true : !TileIsTerrain(dnTile);
                        bool lEmpty  = (tileX == 0) ? true : !TileIsTerrain(lTile);
                        bool fEmpty  = (tileY == 0) ? true : !TileIsTerrain(fTile);
                        bool upEmpty = (tileZ == (CHUNK_DIM - 1)) ? true : !TileIsTerrain(upTile);
                        bool rEmpty  = (tileX == (CHUNK_DIM - 1)) ? true : !TileIsTerrain(rTile);
                        bool bEmpty  = (tileY == (CHUNK_DIM - 1)) ? true : !TileIsTerrain(bTile);

                        if (upEmpty) BuildFace(level, chunk, outMesh, IV3(tileX, tileY, tileZ), Direction_Up);
                        if (dnEmpty) BuildFace(level, chunk, outMesh, IV3(tileX, tileY, tileZ), Direction_Down);
                        if (rEmpty) BuildFace(level, chunk, outMesh, IV3(tileX, tileY, tileZ), Direction_East);
                        if (lEmpty) BuildFace(level, chunk, outMesh, IV3(tileX, tileY, tileZ), Direction_West);
                        if (fEmpty) BuildFace(level, chunk, outMesh, IV3(tileX, tileY, tileZ), Direction_South);
                        if (bEmpty) BuildFace(level, chunk, outMesh, IV3(tileX, tileY, tileZ), Direction_North);
                    }
                }
            }
        }
        auto timeElapsed = GetTimeStamp() - beginTime;
        SOKO_INFO("Generated mesh for chunk (x: %i32, y:%i32, z:%i32). Time: %f64 us",
                  chunk->coord.x, chunk->coord.y, chunk->coord.z, timeElapsed);
    }
}
