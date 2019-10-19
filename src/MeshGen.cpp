#include "MeshGen.h"

namespace soko
{
    inline bool
    TileCanOcclude(Tile tile)
    {
        bool result = tile.value == TileValue_Wall;
        return result;
    }

    inline ChunkMeshVertexBlock*
    GetChunkMeshVertexBlock(ChunkMesh* mesh, AB::MemoryArena* arena)
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
            result->nextBlock = 0;
            result->prevBlock = 0;
            result->at = 0;
        }
        return result;
    }

    inline bool
    PushChunkMeshVertex(ChunkMesh* mesh, AB::MemoryArena* arena,
                        v3 position, v3 normal, byte ao, byte tileId)
    {
        bool result = 0;
        bool memoryIsAvailable = 1;
        if (!mesh->head)
        {
            mesh->head = GetChunkMeshVertexBlock(mesh, arena);
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
            ChunkMeshVertexBlock* newBlock = GetChunkMeshVertexBlock(mesh, arena);
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
    PushChunkMeshQuad(ChunkMesh* mesh, AB::MemoryArena* arena,
                      v3 vtx0, v3 vtx1, v3 vtx2, v3 vtx3,
                      u32 ao0, u32 ao1, u32 ao2, u32 ao3,
                      TileValue val)
    {
        v3 normal = Cross(vtx3 - vtx0, vtx1 - vtx0);
        bool result = false;
        byte ao = (byte)ao0 | (byte)(ao1 << 2) | (byte)(ao2 << 4) | (byte)(ao3 << 6);

        result = PushChunkMeshVertex(mesh, arena, vtx0, normal, ao, val) &&
            PushChunkMeshVertex(mesh, arena, vtx1, normal, ao, val) &&
            PushChunkMeshVertex(mesh, arena, vtx2, normal, ao, val) &&
            PushChunkMeshVertex(mesh, arena, vtx3, normal, ao, val);
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
    BuildFace(Chunk* chunk, ChunkMesh* outMesh, AB::MemoryArena* arena, iv3 p, Direction dir)
    {
        bool result = false;

        Tile testTile = GetTileInChunk(chunk, p.x, p.y, p.z);

        Tile c0Tile = GetTileInChunk(chunk, p.x - 1, p.y - 1, p.z - 1);
        Tile fdnTile = GetTileInChunk(chunk, p.x, p.y - 1, p.z - 1);
        Tile c1Tile = GetTileInChunk(chunk, p.x + 1, p.y - 1, p.z - 1);
        Tile flTile = GetTileInChunk(chunk, p.x - 1, p.y - 1, p.z);
        Tile frTile = GetTileInChunk(chunk, p.x + 1, p.y - 1, p.z);
        Tile c3Tile = GetTileInChunk(chunk, p.x - 1, p.y - 1, p.z + 1);
        Tile fupTile = GetTileInChunk(chunk, p.x, p.y - 1, p.z + 1);
        Tile c2Tile = GetTileInChunk(chunk, p.x + 1, p.y - 1, p.z + 1);

        Tile ldnTile = GetTileInChunk(chunk, p.x - 1, p.y, p.z - 1);
        Tile rdnTile = GetTileInChunk(chunk, p.x + 1, p.y, p.z - 1);
        Tile lupTile = GetTileInChunk(chunk, p.x - 1, p.y, p.z + 1);
        Tile rupTile = GetTileInChunk(chunk, p.x + 1, p.y, p.z + 1);

        Tile c4Tile = GetTileInChunk(chunk, p.x - 1, p.y + 1, p.z - 1);
        Tile bdnTile = GetTileInChunk(chunk, p.x, p.y + 1, p.z - 1);
        Tile c5Tile = GetTileInChunk(chunk, p.x + 1, p.y + 1, p.z - 1);
        Tile blTile = GetTileInChunk(chunk, p.x - 1, p.y + 1, p.z);
        Tile brTile = GetTileInChunk(chunk, p.x + 1, p.y + 1, p.z);
        Tile c7Tile = GetTileInChunk(chunk, p.x - 1, p.y + 1, p.z + 1);
        Tile bupTile = GetTileInChunk(chunk, p.x, p.y + 1, p.z + 1);
        Tile c6Tile = GetTileInChunk(chunk, p.x + 1, p.y + 1, p.z + 1);

        // NOTE: This stage works in left-handed coord system
        // and will be converted to right-handed in push vertex call
        v3 offset = V3((p.x * LEVEL_TILE_SIZE),
                       p.z * LEVEL_TILE_SIZE,
                       (p.y * LEVEL_TILE_SIZE));

        v3 min = offset - 0.5f * V3(LEVEL_TILE_SIZE);
        min.z = offset.z + 0.5f * LEVEL_TILE_SIZE;
        v3 max = offset + 0.5f * V3(LEVEL_TILE_SIZE);
        max.z = offset.z - 0.5f * LEVEL_TILE_SIZE;

        TileValue val = testTile.value;

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

            u32 ao5 = CalcVertexAO(TileCanOcclude(bdnTile), TileCanOcclude(brTile), TileCanOcclude(c5Tile));
            u32 ao6 = CalcVertexAO(TileCanOcclude(bupTile), TileCanOcclude(brTile), TileCanOcclude(c6Tile));
            u32 ao7 = CalcVertexAO(TileCanOcclude(bupTile), TileCanOcclude(blTile), TileCanOcclude(c7Tile));
            u32 ao4 = CalcVertexAO(TileCanOcclude(bdnTile), TileCanOcclude(blTile), TileCanOcclude(c4Tile));

            result = PushChunkMeshQuad(outMesh, arena, vtx5, vtx4, vtx7, vtx6, ao5, ao4, ao7, ao6, val);
        } break;
        case Direction_South:
        {
            u32 ao0 = CalcVertexAO(TileCanOcclude(fdnTile), TileCanOcclude(flTile), TileCanOcclude(c0Tile));
            u32 ao1 = CalcVertexAO(TileCanOcclude(fdnTile), TileCanOcclude(frTile), TileCanOcclude(c1Tile));
            u32 ao2 = CalcVertexAO(TileCanOcclude(fupTile), TileCanOcclude(frTile), TileCanOcclude(c2Tile));
            u32 ao3 = CalcVertexAO(TileCanOcclude(fupTile), TileCanOcclude(flTile), TileCanOcclude(c3Tile));

            result = PushChunkMeshQuad(outMesh, arena, vtx0, vtx1, vtx2, vtx3, ao0, ao1, ao2, ao3, val);
        } break;
        case Direction_West:
        {
            u32 ao4 = CalcVertexAO(TileCanOcclude(ldnTile), TileCanOcclude(blTile), TileCanOcclude(c4Tile));
            u32 ao7 = CalcVertexAO(TileCanOcclude(lupTile), TileCanOcclude(blTile), TileCanOcclude(c7Tile));
            u32 ao3 = CalcVertexAO(TileCanOcclude(lupTile), TileCanOcclude(flTile), TileCanOcclude(c3Tile));
            u32 ao0 = CalcVertexAO(TileCanOcclude(ldnTile), TileCanOcclude(flTile), TileCanOcclude(c0Tile));

            result = PushChunkMeshQuad(outMesh, arena, vtx4, vtx0, vtx3, vtx7, ao4, ao0, ao3, ao7, val);
        } break;
        case Direction_East:
        {
            u32 ao1 = CalcVertexAO(TileCanOcclude(frTile), TileCanOcclude(rdnTile), TileCanOcclude(c1Tile));
            u32 ao2 = CalcVertexAO(TileCanOcclude(frTile), TileCanOcclude(rupTile), TileCanOcclude(c2Tile));
            u32 ao6 = CalcVertexAO(TileCanOcclude(brTile), TileCanOcclude(rupTile), TileCanOcclude(c6Tile));
            u32 ao5 = CalcVertexAO(TileCanOcclude(brTile), TileCanOcclude(rdnTile), TileCanOcclude(c5Tile));

            result = PushChunkMeshQuad(outMesh, arena, vtx1, vtx5, vtx6, vtx2, ao1, ao5, ao6, ao2, val);

        } break;
        case Direction_Up:
        {
            u32 ao3 = CalcVertexAO(TileCanOcclude(fupTile), TileCanOcclude(lupTile), TileCanOcclude(c3Tile));
            u32 ao7 = CalcVertexAO(TileCanOcclude(bupTile), TileCanOcclude(lupTile), TileCanOcclude(c7Tile));
            u32 ao6 = CalcVertexAO(TileCanOcclude(bupTile), TileCanOcclude(rupTile), TileCanOcclude(c6Tile));
            u32 ao2 = CalcVertexAO(TileCanOcclude(fupTile), TileCanOcclude(rupTile), TileCanOcclude(c2Tile));

            result = PushChunkMeshQuad(outMesh, arena, vtx3, vtx2, vtx6, vtx7, ao3, ao2, ao6, ao7, val);

        } break;
        case Direction_Down:
        {
            u32 ao4 = CalcVertexAO(TileCanOcclude(bdnTile), TileCanOcclude(ldnTile), TileCanOcclude(c4Tile));
            u32 ao0 = CalcVertexAO(TileCanOcclude(fdnTile), TileCanOcclude(ldnTile), TileCanOcclude(c0Tile));
            u32 ao1 = CalcVertexAO(TileCanOcclude(fdnTile), TileCanOcclude(rdnTile), TileCanOcclude(c1Tile));
            u32 ao5 = CalcVertexAO(TileCanOcclude(bdnTile), TileCanOcclude(rdnTile), TileCanOcclude(c5Tile));

            result = PushChunkMeshQuad(outMesh, arena, vtx4, vtx5, vtx1, vtx0, ao4, ao5, ao1, ao0, val);
        } break;
        default: {} break;
        }
        return result;
    }

    internal bool
    GenChunkMesh(Chunk* chunk, ChunkMesh* outMesh, AB::MemoryArena* arena)
    {
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
                    Tile testTile = GetTileInChunk(chunk, tileX, tileY, tileZ);

                    if (IsTileOccupiedByTerrain(testTile))
                    {
                        Tile upTile = GetTileInChunk(chunk, tileX, tileY, tileZ + 1);
                        Tile dnTile = GetTileInChunk(chunk, tileX, tileY, tileZ - 1);
                        Tile lTile = GetTileInChunk(chunk, tileX - 1, tileY, tileZ);
                        Tile rTile = GetTileInChunk(chunk, tileX + 1, tileY, tileZ);
                        Tile fTile = GetTileInChunk(chunk, tileX, tileY - 1, tileZ);
                        Tile bTile = GetTileInChunk(chunk, tileX, tileY + 1, tileZ);

                        bool dnEmpty = (tileZ == 0) ? true : !TileCanOcclude(dnTile);
                        bool lEmpty  = (tileX == 0) ? true : !TileCanOcclude(lTile);
                        bool fEmpty  = (tileY == 0) ? true : !TileCanOcclude(fTile);
                        bool upEmpty = (tileZ == (CHUNK_DIM - 1)) ? true : !TileCanOcclude(upTile);
                        bool rEmpty  = (tileX == (CHUNK_DIM - 1)) ? true : !TileCanOcclude(rTile);
                        bool bEmpty  = (tileY == (CHUNK_DIM - 1)) ? true : !TileCanOcclude(bTile);

                        if (upEmpty)
                        {
                            result = result && BuildFace(chunk, outMesh, arena, IV3(tileX, tileY, tileZ), Direction_Up);
                        }
                        if (dnEmpty)
                        {
                            result = result && BuildFace(chunk, outMesh, arena, IV3(tileX, tileY, tileZ), Direction_Down);
                        }
                        if (rEmpty)
                        {
                            result = result && BuildFace(chunk, outMesh, arena, IV3(tileX, tileY, tileZ), Direction_East);
                        }
                        if (lEmpty)
                        {
                            result = result && BuildFace(chunk, outMesh, arena, IV3(tileX, tileY, tileZ), Direction_West);
                        }
                        if (fEmpty)
                        {
                            result = result && BuildFace(chunk, outMesh, arena, IV3(tileX, tileY, tileZ), Direction_South);
                        }
                        if (bEmpty)
                        {
                            result = result && BuildFace(chunk, outMesh, arena, IV3(tileX, tileY, tileZ), Direction_North);
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
