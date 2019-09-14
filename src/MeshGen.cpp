#include "MeshGen.h"

namespace soko
{
    static inline bool
    TileNotEmpty(const Tile* tile)
    {
        bool result = tile && (bool)tile->value;
        return result;
    }

    static inline void
    PushChunkMeshVertex(ChunkMesh* mesh, AB::MemoryArena* arena,
                        v3 position, v3 normal, byte tileId)
    {
        if (!mesh->head)
        {
            mesh->head = PUSH_STRUCT(arena, ChunkMeshVertexBlock);
            ZERO_STRUCT(ChunkMeshVertexBlock, mesh->head);
            mesh->tail = mesh->head;
            SOKO_ASSERT(mesh->head);
            mesh->blockCount++;
        }
        else if (mesh->head->at >= ChunkMeshVertexBlock::CAPACITY)
        {
            ChunkMeshVertexBlock* newBlock = PUSH_STRUCT(arena, ChunkMeshVertexBlock);
            SOKO_ASSERT(newBlock);
            ZERO_STRUCT(ChunkMeshVertexBlock, newBlock);
            mesh->head->prevBlock = newBlock;
            newBlock->nextBlock = mesh->head;

            mesh->head = newBlock;
            mesh->blockCount++;
        }

        // NOTE: Converting to right-handed system
        position.z *= -1.0f;
        mesh->head->positions[mesh->head->at] = position;
        mesh->head->normals[mesh->head->at] = normal;
        mesh->head->tileIds[mesh->head->at] = tileId;
        mesh->head->at++;
        mesh->vertexCount++;
    }

    static inline void
    PushChunkMeshQuad(ChunkMesh* mesh, AB::MemoryArena* arena,
                      v3 vtx0, v3 vtx1, v3 vtx2, v3 vtx3, TileValue val)
    {
        v3 normal = Cross(vtx3 - vtx0, vtx1 - vtx0);
        PushChunkMeshVertex(mesh, arena, vtx0, normal, val);
        PushChunkMeshVertex(mesh, arena, vtx1, normal, val);
        PushChunkMeshVertex(mesh, arena, vtx2, normal, val);
        PushChunkMeshVertex(mesh, arena, vtx3, normal, val);
        mesh->quadCount++;
    }

    ChunkMesh
    GenChunkMesh(Chunk* chunk, AB::MemoryArena* arena)
    {
        ChunkMesh mesh = {};
        for (u32 tileZ = 0; tileZ < Chunk::DIM; tileZ++)
        {
            for (u32 tileY = 0; tileY < Chunk::DIM; tileY++)
            {
                for (u32 tileX = 0; tileX < Chunk::DIM; tileX++)
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
                        v3 offset = V3((tileX * Level::TILE_SIZE),
                                       tileZ * Level::TILE_SIZE,
                                       (tileY * Level::TILE_SIZE));

                        v3 min = offset - 0.5f * V3(Level::TILE_SIZE);;
                        v3 max = offset + 0.5f * V3(Level::TILE_SIZE);

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
                            PushChunkMeshQuad(&mesh, arena, vtx3, vtx7, vtx6, vtx2, val);
                        }
                        if (!TileNotEmpty(dnTile))
                        {
                            PushChunkMeshQuad(&mesh, arena, vtx4, vtx0, vtx1, vtx5, val);
                        }
                        if (!TileNotEmpty(rTile))
                        {
                            PushChunkMeshQuad(&mesh, arena, vtx1, vtx2, vtx6, vtx5, val);
                        }
                        if (!TileNotEmpty(lTile))
                        {
                            PushChunkMeshQuad(&mesh, arena, vtx4, vtx7, vtx3, vtx0, val);
                        }
                        if (!TileNotEmpty(fTile))
                        {
                            PushChunkMeshQuad(&mesh, arena, vtx0, vtx3, vtx2, vtx1, val);
                        }
                        if (!TileNotEmpty(bTile))
                        {
                            PushChunkMeshQuad(&mesh, arena, vtx5, vtx6, vtx7, vtx4, val);
                        }
                    }
                }
            }
        }
        return mesh;
    }
}
