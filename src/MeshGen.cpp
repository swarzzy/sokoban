#include "MeshGen.h"

namespace soko
{
    inline bool
    TileNotEmpty(const Tile* tile)
    {
        bool result = tile && (bool)tile->value;
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
            mesh->head = PUSH_STRUCT(arena, ChunkMeshVertexBlock);
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
            ChunkMeshVertexBlock* newBlock = PUSH_STRUCT(arena, ChunkMeshVertexBlock);
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
    BuildFace(Chunk* chunk, ChunkMesh* outMesh, AB::MemoryArena* arena, v3i p, Direction dir)
    {
        bool result = false;

        Tile* testTile = GetTileInChunk(chunk, p.x, p.y, p.z);

        Tile* c0Tile = GetTileInChunk(chunk, p.x - 1, p.y - 1, p.z - 1);
        Tile* fdnTile = GetTileInChunk(chunk, p.x, p.y - 1, p.z - 1);
        Tile* c1Tile = GetTileInChunk(chunk, p.x + 1, p.y - 1, p.z - 1);
        Tile* flTile = GetTileInChunk(chunk, p.x - 1, p.y - 1, p.z);
        Tile* frTile = GetTileInChunk(chunk, p.x + 1, p.y - 1, p.z);
        Tile* c3Tile = GetTileInChunk(chunk, p.x - 1, p.y - 1, p.z + 1);
        Tile* fupTile = GetTileInChunk(chunk, p.x, p.y - 1, p.z + 1);
        Tile* c2Tile = GetTileInChunk(chunk, p.x + 1, p.y - 1, p.z + 1);

        Tile* ldnTile = GetTileInChunk(chunk, p.x - 1, p.y, p.z - 1);
        Tile* rdnTile = GetTileInChunk(chunk, p.x + 1, p.y, p.z - 1);
        Tile* lupTile = GetTileInChunk(chunk, p.x - 1, p.y, p.z + 1);
        Tile* rupTile = GetTileInChunk(chunk, p.x + 1, p.y, p.z + 1);

        Tile* c4Tile = GetTileInChunk(chunk, p.x - 1, p.y + 1, p.z - 1);
        Tile* bdnTile = GetTileInChunk(chunk, p.x, p.y + 1, p.z - 1);
        Tile* c5Tile = GetTileInChunk(chunk, p.x + 1, p.y + 1, p.z - 1);
        Tile* blTile = GetTileInChunk(chunk, p.x - 1, p.y + 1, p.z);
        Tile* brTile = GetTileInChunk(chunk, p.x + 1, p.y + 1, p.z);
        Tile* c7Tile = GetTileInChunk(chunk, p.x - 1, p.y + 1, p.z + 1);
        Tile* bupTile = GetTileInChunk(chunk, p.x, p.y + 1, p.z + 1);
        Tile* c6Tile = GetTileInChunk(chunk, p.x + 1, p.y + 1, p.z + 1);

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

            u32 ao5 = CalcVertexAO(TileNotEmpty(bdnTile), TileNotEmpty(brTile), TileNotEmpty(c5Tile));
            u32 ao6 = CalcVertexAO(TileNotEmpty(bupTile), TileNotEmpty(brTile), TileNotEmpty(c6Tile));
            u32 ao7 = CalcVertexAO(TileNotEmpty(bupTile), TileNotEmpty(blTile), TileNotEmpty(c7Tile));
            u32 ao4 = CalcVertexAO(TileNotEmpty(bdnTile), TileNotEmpty(blTile), TileNotEmpty(c4Tile));

            result = PushChunkMeshQuad(outMesh, arena, vtx5, vtx4, vtx7, vtx6, ao5, ao4, ao7, ao6, val);
        } break;
        case Direction_South:
        {
            u32 ao0 = CalcVertexAO(TileNotEmpty(fdnTile), TileNotEmpty(flTile), TileNotEmpty(c0Tile));
            u32 ao1 = CalcVertexAO(TileNotEmpty(fdnTile), TileNotEmpty(frTile), TileNotEmpty(c1Tile));
            u32 ao2 = CalcVertexAO(TileNotEmpty(fupTile), TileNotEmpty(frTile), TileNotEmpty(c2Tile));
            u32 ao3 = CalcVertexAO(TileNotEmpty(fupTile), TileNotEmpty(flTile), TileNotEmpty(c3Tile));

            result = PushChunkMeshQuad(outMesh, arena, vtx0, vtx1, vtx2, vtx3, ao0, ao1, ao2, ao3, val);
        } break;
        case Direction_West:
        {
            u32 ao4 = CalcVertexAO(TileNotEmpty(ldnTile), TileNotEmpty(blTile), TileNotEmpty(c4Tile));
            u32 ao7 = CalcVertexAO(TileNotEmpty(lupTile), TileNotEmpty(blTile), TileNotEmpty(c7Tile));
            u32 ao3 = CalcVertexAO(TileNotEmpty(lupTile), TileNotEmpty(flTile), TileNotEmpty(c3Tile));
            u32 ao0 = CalcVertexAO(TileNotEmpty(ldnTile), TileNotEmpty(flTile), TileNotEmpty(c0Tile));

            result = PushChunkMeshQuad(outMesh, arena, vtx4, vtx0, vtx3, vtx7, ao4, ao0, ao3, ao7, val);
        } break;
        case Direction_East:
        {
            u32 ao1 = CalcVertexAO(TileNotEmpty(frTile), TileNotEmpty(rdnTile), TileNotEmpty(c1Tile));
            u32 ao2 = CalcVertexAO(TileNotEmpty(frTile), TileNotEmpty(rupTile), TileNotEmpty(c2Tile));
            u32 ao6 = CalcVertexAO(TileNotEmpty(brTile), TileNotEmpty(rupTile), TileNotEmpty(c6Tile));
            u32 ao5 = CalcVertexAO(TileNotEmpty(brTile), TileNotEmpty(rdnTile), TileNotEmpty(c5Tile));

            result = PushChunkMeshQuad(outMesh, arena, vtx1, vtx5, vtx6, vtx2, ao1, ao5, ao6, ao2, val);

        } break;
        case Direction_Up:
        {
            u32 ao3 = CalcVertexAO(TileNotEmpty(fupTile), TileNotEmpty(lupTile), TileNotEmpty(c3Tile));
            u32 ao7 = CalcVertexAO(TileNotEmpty(bupTile), TileNotEmpty(lupTile), TileNotEmpty(c7Tile));
            u32 ao6 = CalcVertexAO(TileNotEmpty(bupTile), TileNotEmpty(rupTile), TileNotEmpty(c6Tile));
            u32 ao2 = CalcVertexAO(TileNotEmpty(fupTile), TileNotEmpty(rupTile), TileNotEmpty(c2Tile));

            result = PushChunkMeshQuad(outMesh, arena, vtx3, vtx2, vtx6, vtx7, ao3, ao2, ao6, ao7, val);

        } break;
        case Direction_Down:
        {
            u32 ao4 = CalcVertexAO(TileNotEmpty(bdnTile), TileNotEmpty(ldnTile), TileNotEmpty(c4Tile));
            u32 ao0 = CalcVertexAO(TileNotEmpty(fdnTile), TileNotEmpty(ldnTile), TileNotEmpty(c0Tile));
            u32 ao1 = CalcVertexAO(TileNotEmpty(fdnTile), TileNotEmpty(rdnTile), TileNotEmpty(c1Tile));
            u32 ao5 = CalcVertexAO(TileNotEmpty(bdnTile), TileNotEmpty(rdnTile), TileNotEmpty(c5Tile));

            result = PushChunkMeshQuad(outMesh, arena, vtx4, vtx5, vtx1, vtx0, ao4, ao5, ao1, ao0, val);
        } break;
        default: {} break;
        }
        return result;
    }

    internal bool
    GenChunkMesh(Chunk* chunk, ChunkMesh* outMesh, AB::MemoryArena* arena)
    {
        bool result = 1;
        ZERO_STRUCT(ChunkMesh, outMesh);
        for (u32 tileZ = 0; tileZ < CHUNK_DIM; tileZ++)
        {
            for (u32 tileY = 0; tileY < CHUNK_DIM; tileY++)
            {
                for (u32 tileX = 0; tileX < CHUNK_DIM; tileX++)
                {
                    Tile* testTile = GetTileInChunk(chunk, tileX, tileY, tileZ);

                    if (testTile && testTile->value)
                    {
                        Tile* upTile = GetTileInChunk(chunk, tileX, tileY, tileZ + 1);
                        Tile* dnTile = GetTileInChunk(chunk, tileX, tileY, tileZ - 1);
                        Tile* lTile = GetTileInChunk(chunk, tileX - 1, tileY, tileZ);
                        Tile* rTile = GetTileInChunk(chunk, tileX + 1, tileY, tileZ);
                        Tile* fTile = GetTileInChunk(chunk, tileX, tileY + 1, tileZ);
                        Tile* bTile = GetTileInChunk(chunk, tileX, tileY - 1, tileZ);

                        if (!TileNotEmpty(upTile))
                        {
                            result = result && BuildFace(chunk, outMesh, arena, V3I(tileX, tileY, tileZ), Direction_Up);
                        }
                        if (!TileNotEmpty(dnTile))
                        {
                            result = result && BuildFace(chunk, outMesh, arena, V3I(tileX, tileY, tileZ), Direction_Down);
                        }
                        if (!TileNotEmpty(rTile))
                        {
                            result = result && BuildFace(chunk, outMesh, arena, V3I(tileX, tileY, tileZ), Direction_East);
                        }
                        if (!TileNotEmpty(lTile))
                        {
                            result = result && BuildFace(chunk, outMesh, arena, V3I(tileX, tileY, tileZ), Direction_West);
                        }
                        if (!TileNotEmpty(fTile))
                        {
                            result = result && BuildFace(chunk, outMesh, arena, V3I(tileX, tileY, tileZ), Direction_South);
                        }
                        if (!TileNotEmpty(bTile))
                        {
                            result = result && BuildFace(chunk, outMesh, arena, V3I(tileX, tileY, tileZ), Direction_North);
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
        return result;
    }
}
