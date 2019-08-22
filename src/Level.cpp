#include "Level.h"
#include "Memory.h"

namespace soko
{
	// NOTE: Using hash table for tiles might be incredibly slow!!!
	inline Tile*
	GetTile(Level* level, i32 x, i32 y, i32 z, AB::MemoryArena* arena = NULL)
	{
		Tile* result = NULL;
		// TODO: Better hash
		u32 tileHash = (Abs(x) * 22 + Abs(y) * 12 + Abs(z) * 7) % LEVEL_TILE_TABLE_SIZE;
		Tile* tile = level->tiles[tileHash];
		if (tile)
		{
			if (tile->coord.x == x &&
				tile->coord.y == y &&
				tile->coord.z == z)
			{
				result = tile;
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
						result = nextTile;
						break;
					}
					else
					{
						tile = tile->nextTile;
					}
				}
			}
		}

		if (!result && arena)
		{
			// NOTE: Cleared to zero
			Tile* newTile = PUSH_STRUCT(arena, Tile);
			SOKO_ASSERT(newTile);
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
			
			result = newTile;
		}
		return result;
	}

	inline Tile*
	GetTile(Level* level, v3i coord, AB::MemoryArena* arena = NULL)
	{
		Tile* result = GetTile(level, coord.x, coord.y, coord.z, arena);
		return result;
	}

	inline u32
	AddEntity(Level* level, Entity entity, AB::MemoryArena* arena)
	{
		u32 id = 0;
		if (level->entityCount < MAX_LEVEL_ENTITIES)
		{
			Tile* tile = GetTile(level, entity.coord, arena);
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
		return id;
	}

	bool
	ChangeEntityLocation(Level* level, Entity* entity, v3i desiredCoord, AB::MemoryArena* arena)
	{
		bool result = false;
		Tile* oldTile = GetTile(level, entity->coord);
		SOKO_ASSERT(oldTile);
		Tile* desiredTile = GetTile(level, desiredCoord, arena);
		if (desiredTile)
		{
			bool tileIsFree = !(bool)desiredTile->firstEntity;
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

		Tile* desiredTile = GetTile(level, desiredPos, arena);
		Tile* oldTile = GetTile(level, entity->coord);
		Tile* pushTile = GetTile(level, reverse ? revDesiredPos : desiredPos, arena);

		SOKO_ASSERT(oldTile);
		if (desiredTile && pushTile)
		{

			if (reverse)
			{
				result = ChangeEntityLocation(level, entity, desiredPos, arena);
			}
		
			if (pushTile->value != TILE_VALUE_WALL)
			{
				Entity* entityInTile = pushTile->firstEntity;
				bool recursive = (bool)depth;
				if (recursive)
				{
					while (entityInTile)
					{
						// NOTE: Resolve entity collision
						Entity* currentEntity = entityInTile;
						entityInTile = entityInTile->nextEntityInTile;
						MoveEntity(level, currentEntity, dir, arena, reverse, depth - 1);
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
					Tile* tile = GetTile(level, x, y, z);
					if (tile && tile->value == TILE_VALUE_WALL)
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
		for (u32 i = 0; i < level->entityCount; i++)
		{
			Entity* entity = level->entities + i;
			f32 xCoord = entity->coord.x * LEVEL_TILE_SIZE;
			f32 yCoord = entity->coord.z * LEVEL_TILE_SIZE;
			f32 zCoord = entity->coord.y * LEVEL_TILE_SIZE;
			v3 pos = V3(xCoord, yCoord, -zCoord);
			RenderCommandDrawMesh command = {};
			command.transform = Translation(pos);
			command.mesh = &gameState->cubeMesh;
			switch (entity->type)
			{
			case ENTITY_TYPE_BLOCK: { command.material = &gameState->tileBlockMaterial; } break;
			case ENTITY_TYPE_PLAYER: { command.material = &gameState->tilePlayerMaterial; } break;
			INVALID_DEFAULT_CASE;																								
			}
			RenderGroupPushCommand(gameState->renderGroup, RENDER_COMMAND_DRAW_MESH,
								   (void*)&command);

		}
	}

}
