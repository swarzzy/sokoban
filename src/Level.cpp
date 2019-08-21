#include "Level.h"

namespace soko
{
	inline Tile*
	GetTile(Level* level, u32 x, u32 y, u32 z)
	{
		SOKO_ASSERT(x < level->xDim);
		SOKO_ASSERT(y < level->yDim);
		SOKO_ASSERT(z < level->zDim);

		u32 offset = z * level->xDim * level->yDim + y * level->xDim + x;
		Tile* result = level->tiles + offset;
		return result;
	}

	inline Tile*
	GetTile(Level* level, v3u coord)
	{
		Tile* result = GetTile(level, coord.x, coord.y, coord.z);
		return result;
	}

	inline u32
	AddEntity(Level* level, Entity entity)
	{
		u32 id = 0;
		if (level->entityCount < MAX_LEVEL_ENTITIES)
		{
			Tile* tile = GetTile(level, entity.coord);
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
	MoveEntity(Level* level, Entity* entity, MovementDir dir, u32 depth = 2)
	{
		bool result = false;
		v3u desiredPos = entity->coord;
		// TODO: Use ints as coords
		switch (dir)
		{
		case MOVE_DIR_FORWARD: { desiredPos += V3U(0, 1, 0); } break;
		case MOVE_DIR_BACK:    { desiredPos -= V3U(0, 1, 0); } break;
		case MOVE_DIR_RIGHT:   { desiredPos += V3U(1, 0, 0); } break;
		case MOVE_DIR_LEFT:    { desiredPos -= V3U(1, 0, 0); } break;
			INVALID_DEFAULT_CASE;
		}
		
		Tile* desiredTile = GetTile(level, desiredPos);
		Tile* oldTile = GetTile(level, entity->coord);
		if (desiredTile->value != TILE_VALUE_WALL)
		{
			Entity* entityInTile = desiredTile->firstEntity;
			bool tileIsFree = !(bool)entityInTile;
			bool recursive = (bool)depth;
			if (recursive)
			{
				while (entityInTile)
				{
					// NOTE: Resolve entity collision
					Entity* currentEntity = entityInTile;
					entityInTile = entityInTile->nextEntityInTile;
					tileIsFree = MoveEntity(level, currentEntity, dir, depth - 1);
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
			
				entity->coord = desiredPos;
				result = true;				
			}
		}
		return result;
	}

	void
	DrawLevel(Level* level, GameState* gameState)
	{
		for (u32 x = 0; x < level->xDim; x++)
		{
			for (u32 y = 0; y < level->yDim; y++)
			{
				for (u32 z = 0; z < level->zDim; z++)
				{
					Tile* tile = GetTile(level, x, y, z);
					if (tile->value == TILE_VALUE_WALL)
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
