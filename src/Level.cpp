#include "Level.h"
#include "Memory.h"

namespace soko
{
	inline TileQuery
	GetTile(Level* level, i32 x, i32 y, i32 z, AB::MemoryArena* arena = nullptr)
	{
		TileQuery result = {};
		if (x > Level::MAX_DIM || x < Level::MIN_DIM ||
			y > Level::MAX_DIM || y < Level::MIN_DIM ||
			z > Level::MAX_DIM || z < Level::MIN_DIM)
		{
			result.result = TileQueryResult::OutOfBounds;
		}
		else
		{
			// TODO: Better hash
			u32 tileHash = (Abs(x) * 22 + Abs(y) * 12 + Abs(z) * 7) % Level::TILE_TABLE_SIZE;
			Tile* tile = level->tiles[tileHash];
			if (tile)
			{
				// TODO: Collapse to do-while
				if (tile->coord.x == x &&
					tile->coord.y == y &&
					tile->coord.z == z)
				{
					result.result = TileQueryResult::Found;
					result.tile = tile;
				}
				else
				{
					while (tile->nextTile)
					{
						Tile* nextTile = tile->nextTile;
						if (nextTile->coord.x == x &&
							nextTile->coord.y == y &&
							nextTile->coord.z == z)
						{
							result.result = TileQueryResult::Found;
							result.tile = nextTile;
							break;
						}
						else
						{
							tile = tile->nextTile;
						}
					}
				}
			}
			// NOTE: Allocation call always should be called with arena
			// Even if it takes space from freelist
			if (!result.tile && arena)
			{
				Tile* newTile = nullptr;
				if (level->tileFreeList)
				{
					newTile = level->tileFreeList;
					level->tileFreeList = newTile->nextTile;
					level->freeTileCount--;
					ZERO_STRUCT(Tile, newTile);
				}
				else
				{
					newTile = PUSH_STRUCT(arena, Tile);
				}

				if (!newTile)
				{
					result.result = TileQueryResult::AllocationError;
				}
				else
				{								
					if (!tile)
					{
						level->tiles[tileHash] = newTile;
					}
					else
					{
						tile->nextTile = newTile;
					}

					newTile->coord.x = x;
					newTile->coord.y = y;
					newTile->coord.z = z;
					level->tileCount++;

					result.result = TileQueryResult::Found;
					result.tile = newTile;

				}
			}
		}
		return result;
	}


	inline TileQuery
	GetTile(Level* level, v3i coord, AB::MemoryArena* arena = NULL)
	{
		TileQuery result = GetTile(level, coord.x, coord.y, coord.z, arena);
		return result;
	}

	void
	FreeTileIfEmpty(Level* level, Tile* tile)
	{
		if (tile->value == TILE_VALUE_NULL && !tile->firstEntity)
		{
			i32 x = tile->coord.x;
			i32 y = tile->coord.y;
			i32 z = tile->coord.z;
			// TODO: This is the same code as in GetTile
			// TODO: Better hash
			u32 tileHash = (Abs(x) * 22 + Abs(y) * 12 + Abs(z) * 7) % Level::TILE_TABLE_SIZE;
			Tile* currentTile = level->tiles[tileHash];
			bool found = false;
			if (currentTile)
			{
				// TODO: Collapse to do-while
				if (currentTile->coord.x == x &&
					currentTile->coord.y == y &&
					currentTile->coord.z == z)
				{
					level->tiles[tileHash] = currentTile->nextTile;
					found = true;
				}
				else
				{
					while (currentTile->nextTile)
					{
						Tile* nextTile = currentTile->nextTile;
						if (nextTile->coord.x == x &&
							nextTile->coord.y == y &&
							nextTile->coord.z == z)
						{
							currentTile->nextTile = tile->nextTile;
							found = true;
							break;
						}
						else
						{
							currentTile = currentTile->nextTile;
						}
					}
				}

			}
			SOKO_ASSERT(found);
			if (found)
			{
				tile->nextTile = level->tileFreeList;
				level->tileFreeList = tile;
				level->freeTileCount++;
				level->tileCount--;
			}
		}
	}

	inline u32
	AddEntity(Level* level, Entity entity, AB::MemoryArena* arena)
	{
		u32 id = 0;
		if (level->entityCount < Level::MAX_ENTITIES)
		{
			TileQuery query = GetTile(level, entity.coord, arena);
			if (query.result == TileQueryResult::Found)
			{
				Tile* tile = query.tile;
				if (tile->value != TILE_VALUE_WALL)
				{
					id = level->entityCount;
					level->entityCount++;
					level->entities[id] = entity;
					Tile entityTile = {};
					tile->value = TILE_VALUE_ENTITY;

					Entity* addedEntity = level->entities + id;
					addedEntity->nextEntityInTile = tile->firstEntity;
					addedEntity->prevEntityInTile = NULL;
					if (tile->firstEntity)
					{
						tile->firstEntity->prevEntityInTile = addedEntity;					
					}
					tile->firstEntity = addedEntity;
				}
			}				
		}
		return id;
	}

	inline Entity*
	GetEntity(Level* level, u32 id)
	{
		Entity* result = nullptr;
		if (id < Level::MAX_ENTITIES)
		{
			result = level->entities + id;
		}
		return result;
	}

	bool
	ChangeEntityLocation(Level* level, Entity* entity, v3i desiredCoord, AB::MemoryArena* arena)
	{
		bool result = false;
		TileQuery oldTileQuery = GetTile(level, entity->coord);
		SOKO_ASSERT(oldTileQuery.result == TileQueryResult::Found);
		Tile* oldTile = oldTileQuery.tile;
		
		TileQuery desiredTileQuery = GetTile(level, desiredCoord, arena);
		if (desiredTileQuery.result == TileQueryResult::Found)
		{
			Tile* desiredTile = desiredTileQuery.tile;
			bool tileIsFree = desiredTile->value != TILE_VALUE_WALL;
			// TODO: For all entities in tile (iterator)
			if (tileIsFree)
			{
				Entity* entityInTile = desiredTile->firstEntity;
				while(entityInTile)
				{
					// TODO: Entity type filtering
					tileIsFree = !entityInTile->type == ENTITY_TYPE_BLOCK;
					entityInTile = entityInTile->nextEntityInTile;
				}				
			}
			if (tileIsFree)
			{
				if (entity->prevEntityInTile)
				{
					entity->prevEntityInTile->nextEntityInTile = entity->nextEntityInTile;				
				}
				else
				{
					oldTile->firstEntity = entity->nextEntityInTile;
					if (oldTile->firstEntity)
					{
						oldTile->firstEntity->prevEntityInTile = NULL;
					}
				}

				if (!oldTile->firstEntity)
				{
					oldTile->value = TILE_VALUE_NULL;
				}
			
				desiredTile->value = TILE_VALUE_ENTITY;

				entity->nextEntityInTile = desiredTile->firstEntity;
				entity->prevEntityInTile = NULL;
				if (desiredTile->firstEntity)
				{
					desiredTile->firstEntity->prevEntityInTile = entity;
				}
				desiredTile->firstEntity = entity;
			
				entity->coord = desiredCoord;
				FreeTileIfEmpty(level, oldTile);
				result = true;
			}
		}
		return result;
	}

	bool
	MoveEntity(Level* level, Entity* entity, MovementDir dir, AB::MemoryArena* arena, bool reverse = false, u32 depth = 2)
	{
		bool result = false;
		v3i desiredPos = entity->coord;
		v3i revDesiredPos = entity->coord;
		MovementDir revDir;
		switch (dir)
		{
		case MOVE_DIR_FORWARD: { desiredPos += V3I(0, 1, 0); revDesiredPos -= V3I(0, 1, 0); revDir = MOVE_DIR_BACK; } break;
		case MOVE_DIR_BACK:    { desiredPos -= V3I(0, 1, 0); revDesiredPos += V3I(0, 1, 0); revDir = MOVE_DIR_FORWARD; } break;
		case MOVE_DIR_RIGHT:   { desiredPos += V3I(1, 0, 0); revDesiredPos -= V3I(1, 0, 0); revDir = MOVE_DIR_LEFT; } break;
		case MOVE_DIR_LEFT:    { desiredPos -= V3I(1, 0, 0); revDesiredPos += V3I(1, 0, 0); revDir = MOVE_DIR_RIGHT; } break;
		INVALID_DEFAULT_CASE;
		}

		auto[desRes, desiredTile] = GetTile(level, desiredPos, arena);
		auto[oldRes, oldTile] = GetTile(level, entity->coord);
		auto[pushRes, pushTile] = GetTile(level, reverse ? revDesiredPos : desiredPos);

		SOKO_ASSERT(oldTile);
		if (desiredTile)
		{
			if (reverse)
			{
				result = ChangeEntityLocation(level, entity, desiredPos, arena);
			}
		
			if (pushRes == TileQueryResult::Found && pushTile->value != TILE_VALUE_WALL)
			{
				Entity* entityInTile = pushTile->firstEntity;
				bool recursive = (bool)depth;
				if (recursive)
				{
					while (entityInTile)
					{
						if (entityInTile->type == ENTITY_TYPE_BLOCK)
						{
							// NOTE: Resolve entity collision
							Entity* currentEntity = entityInTile;
							MoveEntity(level, currentEntity, dir, arena, reverse, depth - 1);							
						}
						entityInTile = entityInTile->nextEntityInTile;
					}
				}
			}

			if (!reverse)
			{
				result = ChangeEntityLocation(level, entity, desiredPos, arena);
			}
		}
		return result;
	}

	void
	DrawLevel(Level* level, GameState* gameState)
	{
		// TODO: Sparseness
		for (u32 x = 0; x < level->xDim; x++)
		{
			for (u32 y = 0; y < level->yDim; y++)
			{
				for (u32 z = 0; z < level->zDim; z++)
				{
					auto[queryResult, tile] = GetTile(level, x, y, z);
					if (queryResult == TileQueryResult::Found && tile->value == TILE_VALUE_WALL)
					{
						f32 xCoord = x * LEVEL_TILE_SIZE;
						f32 yCoord = z * LEVEL_TILE_SIZE;
						f32 zCoord = y * LEVEL_TILE_SIZE;
						v3 pos = V3(xCoord, yCoord, -zCoord);
						RenderCommandDrawMesh command = {};
						command.transform = Translation(pos);
						command.mesh = &gameState->cubeMesh;
						command.material = &gameState->tileMaterial;
						RenderGroupPushCommand(gameState->renderGroup, RENDER_COMMAND_DRAW_MESH,
											   (void*)&command);

					}
				}
			}
		}
	}

	void
	DrawEntities(Level* level, GameState* gameState)
	{
		for (u32 i = 1; i < level->entityCount; i++)
		{
			// TODO: Entity iterator?
			Entity* entity = level->entities + i;
			f32 xCoord = entity->coord.x * LEVEL_TILE_SIZE;
			f32 yCoord = entity->coord.z * LEVEL_TILE_SIZE;
			f32 zCoord = entity->coord.y * LEVEL_TILE_SIZE;
			v3 pos = V3(xCoord, yCoord, -zCoord);
			RenderCommandDrawMesh command = {};
			command.transform = Translation(pos);
			SOKO_ASSERT(entity->mesh);
			SOKO_ASSERT(entity->material);
			command.mesh = entity->mesh;
			command.material = entity->material;
			RenderGroupPushCommand(gameState->renderGroup, RENDER_COMMAND_DRAW_MESH,
								   (void*)&command);

		}
	}

}
