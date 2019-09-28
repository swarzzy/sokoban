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
                        v3 position, v3 normal, byte tileId)
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
            position.z *= -1.0f;
            mesh->head->positions[mesh->head->at] = position;
            mesh->head->normals[mesh->head->at] = normal;
            mesh->head->tileIds[mesh->head->at] = tileId;
            mesh->head->at++;
            mesh->vertexCount++;
            result = 1;
        }
        return result;
    }

    inline bool
    PushChunkMeshQuad(ChunkMesh* mesh, AB::MemoryArena* arena,
                      v3 vtx0, v3 vtx1, v3 vtx2, v3 vtx3, TileValue val)
    {
        v3 normal = Cross(vtx3 - vtx0, vtx1 - vtx0);
        bool result =
            PushChunkMeshVertex(mesh, arena, vtx0, normal, val) &&
            PushChunkMeshVertex(mesh, arena, vtx1, normal, val) &&
            PushChunkMeshVertex(mesh, arena, vtx2, normal, val) &&
            PushChunkMeshVertex(mesh, arena, vtx3, normal, val);
            mesh->quadCount++;
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
                    // TODO: Invalid coord define
                    u32 tileXMinusOne = tileX - 1;
                    u32 tileYMinusOne = tileY - 1;
                    u32 tileZMinusOne = tileZ - 1;
                    u32 tileXPlusOne = tileX + 1;
                    u32 tileYPlusOne = tileY + 1;
                    u32 tileZPlusOne = tileZ + 1;

                    Tile* testTile =
                        GetTileInChunk(chunk, tileX, tileY, tileZ);
                    Tile* upTile =
                        GetTileInChunk(chunk, tileX, tileY, tileZPlusOne);
                    Tile* dnTile =
                        GetTileInChunk(chunk, tileX, tileY, tileZMinusOne);
                    Tile* lTile =
                        GetTileInChunk(chunk, tileXMinusOne, tileY, tileZ);
                    Tile* rTile =
                        GetTileInChunk(chunk, tileXPlusOne, tileY, tileZ);
                    Tile* fTile =
                        GetTileInChunk(chunk, tileX, tileYPlusOne, tileZ);
                    Tile* bTile =
                        GetTileInChunk(chunk, tileX, tileYMinusOne, tileZ);

                    if (testTile && testTile->value)
                    {
                        v3 offset = V3((tileX * LEVEL_TILE_SIZE),
                                       tileZ * LEVEL_TILE_SIZE,
                                       (tileY * LEVEL_TILE_SIZE));

                        v3 min = offset - 0.5f * V3(LEVEL_TILE_SIZE);;
                        v3 max = offset + 0.5f * V3(LEVEL_TILE_SIZE);

                        TileValue val = testTile->value;

                        v3 vtx0 = V3(min.x, min.y, max.z);
                        v3 vtx1 = V3(max.x, min.y, max.z);
                        v3 vtx2 = V3(max.x, max.y, max.z);
                        v3 vtx3 = V3(min.x, max.y, max.z);

                        v3 vtx4 = V3(min.x, min.y, min.z);
                        v3 vtx5 = V3(max.x, min.y, min.z);
                        v3 vtx6 = V3(max.x, max.y, min.z);
                        v3 vtx7 = V3(min.x, max.y, min.z);

                        if (!TileNotEmpty(upTile))
                        {
                            result = result && PushChunkMeshQuad(outMesh, arena, vtx3, vtx7, vtx6, vtx2, val);
                        }
                        if (!TileNotEmpty(dnTile))
                        {
                            result = result && PushChunkMeshQuad(outMesh, arena, vtx4, vtx0, vtx1, vtx5, val);
                        }
                        if (!TileNotEmpty(rTile))
                        {
                            result = result && PushChunkMeshQuad(outMesh, arena, vtx1, vtx2, vtx6, vtx5, val);
                        }
                        if (!TileNotEmpty(lTile))
                        {
                            result = result && PushChunkMeshQuad(outMesh, arena, vtx4, vtx7, vtx3, vtx0, val);
                        }
                        if (!TileNotEmpty(fTile))
                        {
                            result = result && PushChunkMeshQuad(outMesh, arena, vtx0, vtx3, vtx2, vtx1, val);
                        }
                        if (!TileNotEmpty(bTile))
                        {
                            result = result && PushChunkMeshQuad(outMesh, arena, vtx5, vtx6, vtx7, vtx4, val);
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
