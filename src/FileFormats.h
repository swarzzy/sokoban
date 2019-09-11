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
    const u32 AAB_FILE_VERSION = 0;

#pragma pack(push, 1)
    struct AABMeshMaterialProperties
    {
        v3 ka;
        v3 kd;
        v3 ks;
        v3 ke;
        f32 shininess;
    };

    struct AABMeshHeader
    {
        u32 magicValue;
        u32 version;
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
        u64 materialNameOffset;     // Both are zero if there is no material
        u64 materialDiffBitmapNameOffset;
        u64 materialSpecBitmapNameOffset;
        u64 materialPropertiesOffset;
    };

    struct AABLevelHeader
    {
        u32 magicValue;
        u32 version;
        u64 assetSize;
        u32 assetType;
        u32 chunkCount;
        u64 firstChunkOffset;
    };
#pragma pack (pop)


}
