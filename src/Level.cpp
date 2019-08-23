#include "Level.h"
#include "Memory.h"

namespace soko
{
	inline v3i DirToUnitOffset(Direction dir)
	{
		v3i result = {};
		switch (dir)
		{
		case DIRECTION_NORTH: { result = V3I(0, 1, 0); } break;
		case DIRECTION_SOUTH: { result = V3I(0, -1, 0); } break;
		case DIRECTION_WEST:  { result = V3I(1, 0, 0); } break;
		case DIRECTION_EAST:  { result = V3I(-1, 0, 0); } break;
		case DIRECTION_UP:    { result = V3I(0, 0, 1); } break;
		case DIRECTION_DOWN:  { result = V3I(0, 0, -1); } break;
			INVALID_DEFAULT_CASE;			
		}
		return result;
	}

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
		if (tile->value == TILE_VALUE_EMPTY && !tile->entityList.first)
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

					Entity* addedEntity = level->entities + id;
					addedEntity->id = id;
					addedEntity->nextEntityInTile = tile->entityList.first;
					addedEntity->prevEntityInTile = NULL;
					if (tile->entityList.first)
					{
						tile->entityList.first->prevEntityInTile = addedEntity;					
					}
					tile->entityList.first = addedEntity;
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
	ChangeEntityLocation(Level* level, Entity* entity, v3i desiredCoord, AB::MemoryArena* arena);
	
	void UpdateEntitiesInTile(Level* level, Tile* tile, AB::MemoryArena* arena)
	{
		for (Entity& entity : tile->entityList)
		{
			switch (entity.type)
			{
			case ENTITY_TYPE_PLATE:
			{
				level->platePressed = false;
				for (Entity& e : tile->entityList)
				{
					if (&e != &entity)
					{
						if (e.type == ENTITY_TYPE_BLOCK)
						{
							level->platePressed = true;
						}
					}
				}
			} break;
			case ENTITY_TYPE_PORTAL:
			{
				for (Entity& e : tile->entityList)
				{
					if (&e != &entity)
					{
						if (IsSet(e, ENTITY_FLAG_MOVABLE))
						{
							// NOTE: No need in flag if teleporting
							// not no the same tile where portal located
#if 0
							if (IsSet(e, ENTITY_FLAG_JUST_TELEPORTED))
							{
								UnsetFlag(e, ENTITY_FLAG_JUST_TELEPORTED);
							}
							else
							{
								SetFlag(e, ENTITY_FLAG_JUST_TELEPORTED);
								v3i newCoord = GetEntity(level, entity.bindedPortalID)->coord + DirToUnitOffset(entity.portalDirection);
								bool teleported = ChangeEntityLocation(level, &e, newCoord, arena);
								if (!teleported)
								{
									UnsetFlag(e, ENTITY_FLAG_JUST_TELEPORTED);								
								}
							}
#endif
							SetFlag(e, ENTITY_FLAG_JUST_TELEPORTED);
							v3i newCoord = GetEntity(level, entity.bindedPortalID)->coord + DirToUnitOffset(entity.portalDirection);
							bool teleported = ChangeEntityLocation(level, &e, newCoord, arena);
	
						}
					}
				}
			} break;
			default: {} break;
			}			
		}
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
				for (Entity& entityInTile : desiredTile->entityList)
				{
					if (IsSet(entityInTile, ENTITY_FLAG_COLLIDES))
					{
						tileIsFree = false;
						break;
					}					
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
					oldTile->entityList.first = entity->nextEntityInTile;
					if (oldTile->entityList.first)
					{
						oldTile->entityList.first->prevEntityInTile = NULL;
					}
				}

				entity->nextEntityInTile = desiredTile->entityList.first;
				entity->prevEntityInTile = NULL;
				if (desiredTile->entityList.first)
				{
					desiredTile->entityList.first->prevEntityInTile = entity;
				}
				desiredTile->entityList.first = entity;
				entity->coord = desiredCoord;
				
				UpdateEntitiesInTile(level, oldTile, arena);
				UpdateEntitiesInTile(level, desiredTile, arena);
			
				FreeTileIfEmpty(level, oldTile);
				result = true;
			}
		}
		return result;
	}

	bool
	MoveEntity(Level* level, Entity* entity, Direction dir, AB::MemoryArena* arena, bool reverse = false, u32 depth = 2)
	{
		bool result = false;
		v3i desiredPos = entity->coord;
		desiredPos += DirToUnitOffset(dir);
		v3i revDesiredPos = entity->coord;
		revDesiredPos -= DirToUnitOffset(dir);
		
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
				Entity* entityInTile = pushTile->entityList.first;
				bool recursive = (bool)depth;
				if (recursive)
				{
					for (Entity& e : pushTile->entityList)
					{
						if (IsSet(&e, ENTITY_FLAG_MOVABLE) &&
							IsSet(&e, ENTITY_FLAG_COLLIDES))
						{
							// NOTE: Resolve entity collision
							MoveEntity(level, &e, dir, arena, reverse, depth - 1);							
						}						
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
						f32 xCoord = x * Level::TILE_SIZE;
						f32 yCoord = z * Level::TILE_SIZE;
						f32 zCoord = y * Level::TILE_SIZE;
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
			f32 xCoord = entity->coord.x * Level::TILE_SIZE;
			f32 yCoord = entity->coord.z * Level::TILE_SIZE;
			f32 zCoord = entity->coord.y * Level::TILE_SIZE;
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
