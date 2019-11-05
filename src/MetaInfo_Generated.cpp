#include "MetaInfo_Generated.h"

namespace soko { namespace meta {

internal MetaInfo*
InitMetaInfo(AB::MemoryArena* arena)
{
    MetaInfo* info = PUSH_STRUCT(arena, MetaInfo);
    SOKO_ASSERT(info);
    AddEntry_EntityType(&info->EntityType, 0, EntityType_Block, "EntityType_Block");
    AddEntry_EntityType(&info->EntityType, 1, EntityType_Player, "EntityType_Player");
    AddEntry_EntityType(&info->EntityType, 2, EntityType_Plate, "EntityType_Plate");
    AddEntry_EntityType(&info->EntityType, 3, EntityType_Portal, "EntityType_Portal");
    AddEntry_EntityType(&info->EntityType, 4, EntityType_Spikes, "EntityType_Spikes");
    AddEntry_EntityType(&info->EntityType, 5, EntityType_Button, "EntityType_Button");
    AddEntry_EntityType(&info->EntityType, 6, EntityType_Spawner, "EntityType_Spawner");
    AddEntry_EntityFlags(&info->EntityFlags, 0, EntityFlag_Collides, "EntityFlag_Collides");
    AddEntry_EntityFlags(&info->EntityFlags, 1, EntityFlag_Movable, "EntityFlag_Movable");
    AddEntry_EntityFlags(&info->EntityFlags, 2, EntityFlag_JustTeleported, "EntityFlag_JustTeleported");
    AddEntry_EntityFlags(&info->EntityFlags, 3, EntityFlag_Player, "EntityFlag_Player");
    AddEntry_EntityMesh(&info->EntityMesh, 0, EntityMesh_Cube, "EntityMesh_Cube");
    AddEntry_EntityMesh(&info->EntityMesh, 1, EntityMesh_Plate, "EntityMesh_Plate");
    AddEntry_EntityMesh(&info->EntityMesh, 2, EntityMesh_Portal, "EntityMesh_Portal");
    AddEntry_EntityMesh(&info->EntityMesh, 3, EntityMesh_Spikes, "EntityMesh_Spikes");
    AddEntry_EntityMesh(&info->EntityMesh, 4, EntityMesh_Button, "EntityMesh_Button");
    AddEntry_EntityMesh(&info->EntityMesh, 5, EntityMesh_Sphere, "EntityMesh_Sphere");
    AddEntry_EntityMaterial(&info->EntityMaterial, 0, EntityMaterial_Tile, "EntityMaterial_Tile");
    AddEntry_EntityMaterial(&info->EntityMaterial, 1, EntityMaterial_Player, "EntityMaterial_Player");
    AddEntry_EntityMaterial(&info->EntityMaterial, 2, EntityMaterial_Block, "EntityMaterial_Block");
    AddEntry_EntityMaterial(&info->EntityMaterial, 3, EntityMaterial_RedPlate, "EntityMaterial_RedPlate");
    AddEntry_EntityMaterial(&info->EntityMaterial, 4, EntityMaterial_Portal, "EntityMaterial_Portal");
    AddEntry_EntityMaterial(&info->EntityMaterial, 5, EntityMaterial_Spikes, "EntityMaterial_Spikes");
    AddEntry_EntityMaterial(&info->EntityMaterial, 6, EntityMaterial_Button, "EntityMaterial_Button");
    AddEntry_EntityMaterial(&info->EntityMaterial, 7, EntityMaterial_PbrMetal, "EntityMaterial_PbrMetal");
    AddEntry_Direction(&info->Direction, 0, Direction_Invalid, "Direction_Invalid");
    AddEntry_Direction(&info->Direction, 1, Direction_North, "Direction_North");
    AddEntry_Direction(&info->Direction, 2, Direction_South, "Direction_South");
    AddEntry_Direction(&info->Direction, 3, Direction_West, "Direction_West");
    AddEntry_Direction(&info->Direction, 4, Direction_East, "Direction_East");
    AddEntry_Direction(&info->Direction, 5, Direction_Up, "Direction_Up");
    AddEntry_Direction(&info->Direction, 6, Direction_Down, "Direction_Down");
    AddEntry_TileValue(&info->TileValue, 0, TileValue_TileNotExist, "TileValue_TileNotExist");
    AddEntry_TileValue(&info->TileValue, 1, TileValue_Empty, "TileValue_Empty");
    AddEntry_TileValue(&info->TileValue, 2, TileValue_Wall, "TileValue_Wall");
    AddEntry_TileValue(&info->TileValue, 3, TileValue_Stone, "TileValue_Stone");
    AddEntry_TileValue(&info->TileValue, 4, TileValue_Grass, "TileValue_Grass");

    return info;
}
}}