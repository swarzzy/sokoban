#include "RenderGroup.h"
#include "Memory.h"

namespace soko
{
    RenderGroup*
    AllocateRenderGroup(AB::MemoryArena* arena,  u32 rbSize, u32 queueCapacity)
    {
        RenderGroup* group;
        group = PUSH_STRUCT(arena, RenderGroup);
        SOKO_ASSERT(group, "Failed to allocate render group");

        group->commandQueueCapacity = queueCapacity;
        SOKO_ASSERT((u64)sizeof(CommandQueueEntry) * (u64)queueCapacity <= 0xffffffff,
                    "RenderQueue size cannot be bigger than 4gb.");

        group->commandQueue = PUSH_ARRAY(arena, CommandQueueEntry, queueCapacity);
        SOKO_ASSERT(group->commandQueue, "Failed to allocate render queue");

        group->renderBufferSize = rbSize;
        group->renderBufferFree = rbSize;
        group->renderBuffer = (byte*)PUSH_SIZE(arena, rbSize);
        SOKO_ASSERT(group->renderBuffer, "Failed to allocate render buffer");

        group->renderBufferAt = group->renderBuffer;

        return group;
    }

    inline static void* _PushRenderData(RenderGroup* group, u32 size, u32 aligment, void* data)
    {
        u32 padding = 0;
        u32 useAligment = 0;
        byte* currentAt = group->renderBufferAt;

        if (aligment == 0)
        {
            useAligment = 1;
        }
        else
        {
            useAligment = aligment;
        }

        if ((uptr)currentAt % useAligment != 0)
        {
            // TODO: @Important: Check this padding calculation. It could be wrong
            padding = (useAligment - (uptr)currentAt % useAligment) % useAligment;
        }

        SOKO_ASSERT(size + padding < group->renderBufferFree,
                       "Not enough space in render buffer.");

        group->renderBufferAt += size + padding ;
        group->renderBufferFree -= size + padding;
        byte* nextAt = currentAt + padding;

        COPY_BYTES(size, nextAt, data);

        SOKO_ASSERT((uptr)nextAt % (uptr)useAligment == 0, "Wrong aligment");

        return (void*)nextAt;
    }

    static inline CommandQueueEntry*
    _PushCommandQueueEntry(RenderGroup* renderGroup, CommandQueueEntry* cmd)
    {
        SOKO_ASSERT(renderGroup->commandQueueAt < renderGroup->commandQueueCapacity);
        CommandQueueEntry* renderBucketDest =
            renderGroup->commandQueue + renderGroup->commandQueueAt;
        COPY_STRUCT(CommandQueueEntry, renderBucketDest, cmd);
        renderGroup->commandQueueAt++;
        return renderBucketDest;
    }

    void
    RenderGroupPushCommand(RenderGroup* group, RenderCommandType type, void* data)
    {
        // NOTE: Makes shure there is no pending batches
        // before execute other commands
        SOKO_ASSERT(!(!((type == RENDER_COMMAND_PUSH_LINE_VERTEX) ||
                        (type == RENDER_COMMAND_DRAW_LINE_END)) &&
                      group->pendingLineBatchCommandHeader), 0);

        SOKO_ASSERT(!((type != RENDER_COMMAND_END_CHUNK_MESH_BATCH &&
                       type != RENDER_COMMAND_PUSH_CHUNK_MESH) &&
                      group->pendingChunkMeshBatchHeader));

        void* renderDataPtr = nullptr;
        CommandQueueEntry command = {};
        command.type = type;

        switch (type)
        {
        case RENDER_COMMAND_DRAW_MESH:
        {
            RenderCommandDrawMesh* renderData = (RenderCommandDrawMesh*)data;

            renderDataPtr = _PushRenderData(group,
                                            sizeof(RenderCommandDrawMesh),
                                            alignof(RenderCommandDrawMesh),
                                            data);

            uptr offset = (uptr)renderDataPtr - (uptr)group->renderBuffer;
            command.rbOffset = AB::SafeCastUptrU32(offset);
            _PushCommandQueueEntry(group, &command);
        }
        break;

        case RENDER_COMMAND_SET_DIR_LIGHT:
        {
            RenderCommandSetDirLight* renderData = (RenderCommandSetDirLight*)data;
            //group->dirLightEnabled = true;
            group->dirLight = renderData->light;
        } break;

        case RENDER_COMMAND_DRAW_LINE_BEGIN:
        {
            RenderCommandDrawLineBegin* renderData
                = (RenderCommandDrawLineBegin*)data;

            renderDataPtr =
                _PushRenderData(group,
                                sizeof(RenderCommandDrawLineBegin),
                                1,
                                data);

            uptr offset = (uptr)renderDataPtr - (uptr)group->renderBuffer;
            command.rbOffset = AB::SafeCastUptrU32(offset);
            CommandQueueEntry* entry = _PushCommandQueueEntry(group, &command);
            entry->instanceCount = 0;

            group->pendingLineBatchCommandHeader = entry;

        } break;

        case RENDER_COMMAND_PUSH_LINE_VERTEX:
        {
            RenderCommandPushLineVertex* renderData
                = (RenderCommandPushLineVertex*)data;

            renderDataPtr =
                _PushRenderData(group,
                                sizeof(RenderCommandPushLineVertex),
                                1,
                                (void*)renderData);
            group->pendingLineBatchCommandHeader->instanceCount++;
        } break;

        case RENDER_COMMAND_DRAW_LINE_END:
        {
            group->pendingLineBatchCommandHeader = 0;
        } break;

        case RENDER_COMMAND_BEGIN_CHUNK_MESH_BATCH:
        {
            command.rbOffset = 0;
            CommandQueueEntry* entry = _PushCommandQueueEntry(group, &command);
            entry->instanceCount = 0;

            group->pendingChunkMeshBatchHeader = entry;
        } break;

        case RENDER_COMMAND_PUSH_CHUNK_MESH:
        {
            auto renderData = (RenderCommandPushChunkMesh*)data;

            renderDataPtr = _PushRenderData(group,
                                            sizeof(RenderCommandPushChunkMesh),
                                            1, (void*)renderData);
            if (!group->pendingChunkMeshBatchHeader->instanceCount)
            {
                uptr offset = (uptr)renderDataPtr - (uptr)group->renderBuffer;
                group->pendingChunkMeshBatchHeader->rbOffset = AB::SafeCastUptrU32(offset);
            }
            group->pendingChunkMeshBatchHeader->instanceCount++;
        } break;

        case RENDER_COMMAND_END_CHUNK_MESH_BATCH:
        {
            group->pendingChunkMeshBatchHeader = 0;
        } break;

        INVALID_DEFAULT_CASE;
        }
    }

    void RenderGroupSetCamera(RenderGroup* group, const CameraConfig* config)
    {
        group->camera = config;
    }

    void RenderGroupResetQueue(RenderGroup* group)
    {
        group->commandQueueAt = 0;
        group->renderBufferAt = group->renderBuffer;
        group->renderBufferFree = group->renderBufferSize;
        //group->dirLightEnabled = false;
    }

    internal void
    DrawAlignedBoxOutline(RenderGroup* renderGroup, v3 min, v3 max, v3 color, f32 lineWidth)
    {

        RenderCommandDrawLineBegin beginCommand = {};
        beginCommand.color = color;
        beginCommand.width = lineWidth;
        beginCommand.type = RENDER_LINE_TYPE_SEGMENTS;
        RenderGroupPushCommand(renderGroup,
                               RENDER_COMMAND_DRAW_LINE_BEGIN,
                               (void*)(&beginCommand));

        RenderCommandPushLineVertex v0Command = {};
        v0Command.vertex = min;
        RenderGroupPushCommand(renderGroup,
                               RENDER_COMMAND_PUSH_LINE_VERTEX,
                               (void*)(&v0Command));

        RenderCommandPushLineVertex v1Command = {};
        v1Command.vertex = V3(max.x, min.y, min.z);
        RenderGroupPushCommand(renderGroup,
                               RENDER_COMMAND_PUSH_LINE_VERTEX,
                               (void*)(&v1Command));
        RenderGroupPushCommand(renderGroup,
                               RENDER_COMMAND_PUSH_LINE_VERTEX,
                               (void*)(&v1Command));


        RenderCommandPushLineVertex v2Command = {};
        v2Command.vertex = V3(max.x, min.y, max.z);
        RenderGroupPushCommand(renderGroup,
                               RENDER_COMMAND_PUSH_LINE_VERTEX,
                               (void*)(&v2Command));
        RenderGroupPushCommand(renderGroup,
                               RENDER_COMMAND_PUSH_LINE_VERTEX,
                               (void*)(&v2Command));


        RenderCommandPushLineVertex v3Command = {};
        v3Command.vertex = V3(min.x, min.y, max.z);
        RenderGroupPushCommand(renderGroup,
                               RENDER_COMMAND_PUSH_LINE_VERTEX,
                               (void*)(&v3Command));
        RenderGroupPushCommand(renderGroup,
                               RENDER_COMMAND_PUSH_LINE_VERTEX,
                               (void*)(&v3Command));

        RenderGroupPushCommand(renderGroup,
                               RENDER_COMMAND_PUSH_LINE_VERTEX,
                               (void*)(&v0Command));

        //
        RenderCommandPushLineVertex v4Command = {};
        v4Command.vertex =V3(min.x, max.y, min.z);
        RenderGroupPushCommand(renderGroup,
                               RENDER_COMMAND_PUSH_LINE_VERTEX,
                               (void*)(&v4Command));

        RenderCommandPushLineVertex v5Command = {};
        v5Command.vertex = V3(max.x, max.y, min.z);
        RenderGroupPushCommand(renderGroup,
                               RENDER_COMMAND_PUSH_LINE_VERTEX,
                               (void*)(&v5Command));
        RenderGroupPushCommand(renderGroup,
                               RENDER_COMMAND_PUSH_LINE_VERTEX,
                               (void*)(&v5Command));


        RenderCommandPushLineVertex v6Command = {};
        v6Command.vertex = V3(max.x, max.y, max.z);
        RenderGroupPushCommand(renderGroup,
                               RENDER_COMMAND_PUSH_LINE_VERTEX,
                               (void*)(&v6Command));
        RenderGroupPushCommand(renderGroup,
                               RENDER_COMMAND_PUSH_LINE_VERTEX,
                               (void*)(&v6Command));


        RenderCommandPushLineVertex v7Command = {};
        v7Command.vertex = V3(min.x, max.y, max.z);
        RenderGroupPushCommand(renderGroup,
                               RENDER_COMMAND_PUSH_LINE_VERTEX,
                               (void*)(&v7Command));
        RenderGroupPushCommand(renderGroup,
                               RENDER_COMMAND_PUSH_LINE_VERTEX,
                               (void*)(&v7Command));

        RenderGroupPushCommand(renderGroup,
                               RENDER_COMMAND_PUSH_LINE_VERTEX,
                               (void*)(&v4Command));

        RenderGroupPushCommand(renderGroup,
                               RENDER_COMMAND_PUSH_LINE_VERTEX,
                               (void*)(&v0Command));
        RenderGroupPushCommand(renderGroup,
                               RENDER_COMMAND_PUSH_LINE_VERTEX,
                               (void*)(&v4Command));

        RenderGroupPushCommand(renderGroup,
                               RENDER_COMMAND_PUSH_LINE_VERTEX,
                               (void*)(&v1Command));
        RenderGroupPushCommand(renderGroup,
                               RENDER_COMMAND_PUSH_LINE_VERTEX,
                               (void*)(&v5Command));

        RenderGroupPushCommand(renderGroup,
                               RENDER_COMMAND_PUSH_LINE_VERTEX,
                               (void*)(&v2Command));
        RenderGroupPushCommand(renderGroup,
                               RENDER_COMMAND_PUSH_LINE_VERTEX,
                               (void*)(&v6Command));

        RenderGroupPushCommand(renderGroup,
                               RENDER_COMMAND_PUSH_LINE_VERTEX,
                               (void*)(&v3Command));
        RenderGroupPushCommand(renderGroup,
                               RENDER_COMMAND_PUSH_LINE_VERTEX,
                               (void*)(&v7Command));

        RenderGroupPushCommand(renderGroup,
                               RENDER_COMMAND_DRAW_LINE_END,
                               (void*)(0));
    }

    void
    DrawStraightLine(RenderGroup* renderGroup,
                     v3 begin, v3 end, v3 color, f32 lineWidth)
    {
        RenderCommandDrawLineBegin beginCommand = {};
        beginCommand.color = color;
        beginCommand.width = lineWidth;
        beginCommand.type = RENDER_LINE_TYPE_SEGMENTS;
        RenderGroupPushCommand(renderGroup,
                               RENDER_COMMAND_DRAW_LINE_BEGIN,
                               (void*)(&beginCommand));

        RenderCommandPushLineVertex v0Command = {};
        v0Command.vertex = begin;
        RenderGroupPushCommand(renderGroup,
                               RENDER_COMMAND_PUSH_LINE_VERTEX,
                               (void*)(&v0Command));

        RenderCommandPushLineVertex v1Command = {};
        v1Command.vertex = end;
        RenderGroupPushCommand(renderGroup,
                               RENDER_COMMAND_PUSH_LINE_VERTEX,
                               (void*)(&v1Command));

        RenderGroupPushCommand(renderGroup,
                               RENDER_COMMAND_DRAW_LINE_END,
                               (void*)(0));
    }
}
