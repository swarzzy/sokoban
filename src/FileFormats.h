#pragma once
#include "Platform.h"
#include "hypermath.h"

namespace AB
{
    enum AABFileType : u32
    {
        AAB_FILE_TYPE_MESH = 0x01020304,
        AAB_FILE_TYPE_LEVEL = 0x10203040
    };

    const u32 AAB_FILE_MAGIC_VALUE = 0xaabaabaa;
    const u32 AAB_MESH_FILE_VERSION = 2;
    const u32 AAB_LEVEL_FILE_VERSION = 1;

#pragma pack(push, 1)
    struct AABMeshMaterialProperties
    {
        v3 ka;
        v3 kd;
        v3 ks;
        v3 ke;
        f32 shininess;
    };

    struct AABMeshHeaderV1
    {
        u32 magicValue;
        u32 version = 1;
        u64 assetSize;
        u32 assetType;
        u32 verticesCount;
        u32 normalsCount;
        u32 uvsCount;
        u32 indicesCount;
        u64 verticesOffset;
        u64 normalsOffset;
        u64 uvsOffset;
        u64 indicesOffset;
        u64 materialNameOffset;
        // NOTE: Both are zero if there is no material
        u64 materialDiffBitmapNameOffset;
        u64 materialSpecBitmapNameOffset;
        u64 materialPropertiesOffset;
    };

    struct AABMeshHeaderV2
    {
        u32 magicValue;
        u32 version = 2;
        u64 assetSize;
        u32 assetType;
        u32 vertexCount;
        u32 indexCount;
        u64 vertexOffset;
        u64 normalsOffset;
        u64 uvOffset;
        u64 indicesOffset;
        u64 tangentsOffset;
    };

    struct AABLevelHeader
    {
        u32 magicValue;
        u32 version;
        u64 assetSize;
        u32 assetType;
        u32 chunkCount;
        u32 chunkMeshBlockCount;
        u32 entityCount;
        u64 firstChunkOffset;
        u64 firstEntityOffset;
        // TODO: Is that possible that iv3 would have different alignment
        // on any of supported platform?
        iv3 firstPlayerSpawnPos;
        iv3 secondPlayerSpawnPos;
        b32 hasSecondPlayer;
    };
#pragma pack (pop)


}
