#include "MeshGen.h"

namespace soko
{
    inline ChunkMeshVertexBlock*
    GetChunkMeshVertexBlock(Level* level, ChunkMesh* mesh, AB::MemoryArena* arena)
    {
        ChunkMeshVertexBlock* result = 0;
        if (mesh->free)
        {
            result = mesh->free;
            mesh->free = result->nextBlock;
        }
        else
        {
            result = PUSH_STRUCT(arena, ChunkMeshVertexBlock);
        }
        if (result)
        {
            level->globalChunkMeshBlockCount++;
            result->nextBlock = 0;
            result->prevBlock = 0;
            result->at = 0;
        }
        return result;
    }

    inline bool
    PushChunkMeshVertex(Level* level, ChunkMesh* mesh, AB::MemoryArena* arena,
                        v3 position, v3 normal, byte ao, byte tileId)
    {
        bool result = 0;
        bool memoryIsAvailable = 1;
        if (!mesh->head)
        {
            mesh->head = GetChunkMeshVertexBlock(level, mesh, arena);
            if (mesh->head)
            {
                ZERO_STRUCT(ChunkMeshVertexBlock, mesh->head);
                mesh->tail = mesh->head;
                mesh->blockCount++;
                memoryIsAvailable = 1;
            }
            else
            {
                memoryIsAvailable = 0;
            }
        }
        else if (mesh->head->at >= CHUNK_MESH_VERTEX_BLOCK_CAPACITY)
        {
            ChunkMeshVertexBlock* newBlock = GetChunkMeshVertexBlock(level, mesh, arena);
            if (newBlock)
            {
                ZERO_STRUCT(ChunkMeshVertexBlock, newBlock);
                mesh->head->prevBlock = newBlock;
                newBlock->nextBlock = mesh->head;

                mesh->head = newBlock;
                mesh->blockCount++;
                memoryIsAvailable = 1;
            }
            else
            {
                memoryIsAvailable = 0;
            }
        }

        if (memoryIsAvailable)
        {
            // NOTE: Converting to right-handed system
            position.z = -position.z;
            normal.z = -normal.z;

            mesh->head->positions[mesh->head->at] = position;
            mesh->head->normals[mesh->head->at] = normal;
            mesh->head->tileIds[mesh->head->at] = tileId;
            mesh->head->AO[mesh->head->at] = ao;
            mesh->head->at++;
            mesh->vertexCount++;
            result = 1;
        }
        return result;
    }

    inline bool
    PushChunkMeshQuad(Level* level, ChunkMesh* mesh, AB::MemoryArena* arena,
                      v3 vtx0, v3 vtx1, v3 vtx2, v3 vtx3,
                      u32 ao0, u32 ao1, u32 ao2, u32 ao3,
                      TileValue val)
    {
        v3 normal = Cross(vtx3 - vtx0, vtx1 - vtx0);
        bool result = false;
        byte ao = (byte)ao0 | (byte)(ao1 << 2) | (byte)(ao2 << 4) | (byte)(ao3 << 6);

        result = PushChunkMeshVertex(level, mesh, arena, vtx0, normal, ao, val) &&
            PushChunkMeshVertex(level, mesh, arena, vtx1, normal, ao, val) &&
            PushChunkMeshVertex(level, mesh, arena, vtx2, normal, ao, val) &&
            PushChunkMeshVertex(level, mesh, arena, vtx3, normal, ao, val);
            mesh->quadCount++;
            return result;
    }

    inline u32
    CalcVertexAO(bool side0, bool side1, bool corner)
    {
        static_assert((int)true == 1);
        u32 result = 0;

        if (!(side0 && side1))
        {
            result = 3 - (u32)side0 - (u32)side1 - (u32)corner;
        }
        return result;
    }

    inline bool
    BuildFace(Level* level, Chunk* chunk, ChunkMesh* outMesh, AB::MemoryArena* arena, iv3 p, Direction dir)
    {
        bool result = false;

       const Tile* testTile = GetTileInChunk(chunk, p.x, p.y, p.z);
       SOKO_ASSERT(testTile);

       const Tile* c0Tile = GetTileInChunk(chunk, p.x - 1, p.y - 1, p.z - 1);
       const Tile* fdnTile = GetTileInChunk(chunk, p.x, p.y - 1, p.z - 1);
       const Tile* c1Tile = GetTileInChunk(chunk, p.x + 1, p.y - 1, p.z - 1);
       const Tile* flTile = GetTileInChunk(chunk, p.x - 1, p.y - 1, p.z);
       const Tile* frTile = GetTileInChunk(chunk, p.x + 1, p.y - 1, p.z);
       const Tile* c3Tile = GetTileInChunk(chunk, p.x - 1, p.y - 1, p.z + 1);
       const Tile* fupTile = GetTileInChunk(chunk, p.x, p.y - 1, p.z + 1);
       const Tile* c2Tile = GetTileInChunk(chunk, p.x + 1, p.y - 1, p.z + 1);

       const Tile* ldnTile = GetTileInChunk(chunk, p.x - 1, p.y, p.z - 1);
       const Tile* rdnTile = GetTileInChunk(chunk, p.x + 1, p.y, p.z - 1);
       const Tile* lupTile = GetTileInChunk(chunk, p.x - 1, p.y, p.z + 1);
       const Tile* rupTile = GetTileInChunk(chunk, p.x + 1, p.y, p.z + 1);

       const Tile* c4Tile = GetTileInChunk(chunk, p.x - 1, p.y + 1, p.z - 1);
       const Tile* bdnTile = GetTileInChunk(chunk, p.x, p.y + 1, p.z - 1);
       const Tile* c5Tile = GetTileInChunk(chunk, p.x + 1, p.y + 1, p.z - 1);
       const Tile* blTile = GetTileInChunk(chunk, p.x - 1, p.y + 1, p.z);
       const Tile* brTile = GetTileInChunk(chunk, p.x + 1, p.y + 1, p.z);
       const Tile* c7Tile = GetTileInChunk(chunk, p.x - 1, p.y + 1, p.z + 1);
       const Tile* bupTile = GetTileInChunk(chunk, p.x, p.y + 1, p.z + 1);
       const Tile* c6Tile = GetTileInChunk(chunk, p.x + 1, p.y + 1, p.z + 1);

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
        case Direction_North:
        {
            u32 ao5 = CalcVertexAO(TileIsTerrain(bdnTile), TileIsTerrain(brTile), TileIsTerrain(c5Tile));
            u32 ao6 = CalcVertexAO(TileIsTerrain(bupTile), TileIsTerrain(brTile), TileIsTerrain(c6Tile));
            u32 ao7 = CalcVertexAO(TileIsTerrain(bupTile), TileIsTerrain(blTile), TileIsTerrain(c7Tile));
            u32 ao4 = CalcVertexAO(TileIsTerrain(bdnTile), TileIsTerrain(blTile), TileIsTerrain(c4Tile));

            result = PushChunkMeshQuad(level, outMesh, arena, vtx5, vtx4, vtx7, vtx6, ao5, ao4, ao7, ao6, val);
        } break;
        case Direction_South:
        {
            u32 ao0 = CalcVertexAO(TileIsTerrain(fdnTile), TileIsTerrain(flTile), TileIsTerrain(c0Tile));
            u32 ao1 = CalcVertexAO(TileIsTerrain(fdnTile), TileIsTerrain(frTile), TileIsTerrain(c1Tile));
            u32 ao2 = CalcVertexAO(TileIsTerrain(fupTile), TileIsTerrain(frTile), TileIsTerrain(c2Tile));
            u32 ao3 = CalcVertexAO(TileIsTerrain(fupTile), TileIsTerrain(flTile), TileIsTerrain(c3Tile));

            result = PushChunkMeshQuad(level, outMesh, arena, vtx0, vtx1, vtx2, vtx3, ao0, ao1, ao2, ao3, val);
        } break;
        case Direction_West:
        {
            u32 ao4 = CalcVertexAO(TileIsTerrain(ldnTile), TileIsTerrain(blTile), TileIsTerrain(c4Tile));
            u32 ao7 = CalcVertexAO(TileIsTerrain(lupTile), TileIsTerrain(blTile), TileIsTerrain(c7Tile));
            u32 ao3 = CalcVertexAO(TileIsTerrain(lupTile), TileIsTerrain(flTile), TileIsTerrain(c3Tile));
            u32 ao0 = CalcVertexAO(TileIsTerrain(ldnTile), TileIsTerrain(flTile), TileIsTerrain(c0Tile));

            result = PushChunkMeshQuad(level, outMesh, arena, vtx4, vtx0, vtx3, vtx7, ao4, ao0, ao3, ao7, val);
        } break;
        case Direction_East:
        {
            u32 ao1 = CalcVertexAO(TileIsTerrain(frTile), TileIsTerrain(rdnTile), TileIsTerrain(c1Tile));
            u32 ao2 = CalcVertexAO(TileIsTerrain(frTile), TileIsTerrain(rupTile), TileIsTerrain(c2Tile));
            u32 ao6 = CalcVertexAO(TileIsTerrain(brTile), TileIsTerrain(rupTile), TileIsTerrain(c6Tile));
            u32 ao5 = CalcVertexAO(TileIsTerrain(brTile), TileIsTerrain(rdnTile), TileIsTerrain(c5Tile));

            result = PushChunkMeshQuad(level, outMesh, arena, vtx1, vtx5, vtx6, vtx2, ao1, ao5, ao6, ao2, val);

        } break;
        case Direction_Up:
        {
            u32 ao3 = CalcVertexAO(TileIsTerrain(fupTile), TileIsTerrain(lupTile), TileIsTerrain(c3Tile));
            u32 ao7 = CalcVertexAO(TileIsTerrain(bupTile), TileIsTerrain(lupTile), TileIsTerrain(c7Tile));
            u32 ao6 = CalcVertexAO(TileIsTerrain(bupTile), TileIsTerrain(rupTile), TileIsTerrain(c6Tile));
            u32 ao2 = CalcVertexAO(TileIsTerrain(fupTile), TileIsTerrain(rupTile), TileIsTerrain(c2Tile));

            result = PushChunkMeshQuad(level, outMesh, arena, vtx3, vtx2, vtx6, vtx7, ao3, ao2, ao6, ao7, val);

        } break;
        case Direction_Down:
        {
            u32 ao4 = CalcVertexAO(TileIsTerrain(bdnTile), TileIsTerrain(ldnTile), TileIsTerrain(c4Tile));
            u32 ao0 = CalcVertexAO(TileIsTerrain(fdnTile), TileIsTerrain(ldnTile), TileIsTerrain(c0Tile));
            u32 ao1 = CalcVertexAO(TileIsTerrain(fdnTile), TileIsTerrain(rdnTile), TileIsTerrain(c1Tile));
            u32 ao5 = CalcVertexAO(TileIsTerrain(bdnTile), TileIsTerrain(rdnTile), TileIsTerrain(c5Tile));

            result = PushChunkMeshQuad(level, outMesh, arena, vtx4, vtx5, vtx1, vtx0, ao4, ao5, ao1, ao0, val);
        } break;
        default: {} break;
        }
        return result;
    }

    internal bool
    GenChunkMesh(Level* level, Chunk* chunk, ChunkMesh* outMesh)
    {
        AB::MemoryArena* arena = level->sessionArena;
        bool result = true;
        i64 beginTime = GetTimeStamp();

        if (outMesh->vertexCount)
        {
            SOKO_ASSERT(outMesh->head);
            SOKO_ASSERT(outMesh->tail);
            // TODO: @Speed: Stop freeing all blocks before generate
            ChunkMeshVertexBlock* block = outMesh->head;
            while (true)
            {
                auto nextBlock = block->nextBlock;
                block->nextBlock = outMesh->free;
                outMesh->free = block;
                // TODO: Global freelidt for all chunks
                level->globalChunkMeshBlockCount--;

                if (block == outMesh->tail) break;

                block = nextBlock;
            }

            outMesh->vertexCount = 0;
            outMesh->quadCount = 0;
            outMesh->blockCount = 0;
            outMesh->head = 0;
            outMesh->tail = 0;
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

                        if (upEmpty)
                        {
                            result = result && BuildFace(level, chunk, outMesh, arena, IV3(tileX, tileY, tileZ), Direction_Up);
                        }
                        if (dnEmpty)
                        {
                            result = result && BuildFace(level, chunk, outMesh, arena, IV3(tileX, tileY, tileZ), Direction_Down);
                        }
                        if (rEmpty)
                        {
                            result = result && BuildFace(level, chunk, outMesh, arena, IV3(tileX, tileY, tileZ), Direction_East);
                        }
                        if (lEmpty)
                        {
                            result = result && BuildFace(level, chunk, outMesh, arena, IV3(tileX, tileY, tileZ), Direction_West);
                        }
                        if (fEmpty)
                        {
                            result = result && BuildFace(level, chunk, outMesh, arena, IV3(tileX, tileY, tileZ), Direction_South);
                        }
                        if (bEmpty)
                        {
                            result = result && BuildFace(level, chunk, outMesh, arena, IV3(tileX, tileY, tileZ), Direction_North);
                        }
                    }

                    if (!result)
                    {
                        goto loopEnd;
                    }
                }
            }
        }
    loopEnd:
        i64 timeElapsed = GetTimeStamp() - beginTime;
        SOKO_INFO("Generated mesh for chunk (x: %i32, y:%i32, z:%i32). Time: %i64 us",
                  chunk->coord.x, chunk->coord.y, chunk->coord.z, timeElapsed);

        return result;
    }
}
