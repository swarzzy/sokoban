// Version 2.0 (10.11.2019)
// CHANGES:
// - Porting from aberration codebase
// - Cleanup in Utils.cpp -> OfflineUtils.cpp
// - Remove material info from aab mesh
// - Add tangents calculation
//
#define AB_PLATFORM_WINDOWS
#include <cstdint>
#include <cassert>

#include "FileFormats.h"
#include "ObjParser.cpp"

struct ABMesh {
    // UVs are optional
    // NOTE(V2.0) They are not optional!!!
    hpm::Vector3* vertices;
    hpm::Vector3* normals;
    hpm::Vector2* uv;

    u32 num_vertices;
    u32 num_indices;

    u32* indices;
    i32 material_index;

    // V2.0
    v3* tangents;
};

void FreeABMesh(ABMesh* mesh) {
    if (mesh->vertices) {
        free(mesh->vertices);
    }
    if (mesh->normals) {
        free(mesh->normals);
    }
    if (mesh->uv) {
        free(mesh->uv);
    }
    if (mesh->indices) {
        free(mesh->indices);
    }

    *mesh = {};
}

internal void
GenMeshTangents(ABMesh* mesh)
{
    mesh->tangents = (v3*)malloc(sizeof(v3) * mesh->num_vertices);
    assert(mesh->tangents);

    for (u32 index = 0; index < mesh->num_indices; index += 3)
    {
        assert((index + 2) < mesh->num_indices);
        v3 vt0 = mesh->vertices[mesh->indices[index]];
        v3 vt1 = mesh->vertices[mesh->indices[index + 1]];
        v3 vt2 = mesh->vertices[mesh->indices[index + 2]];

        v2 uv0 = mesh->uv[mesh->indices[index]];
        v2 uv1 = mesh->uv[mesh->indices[index + 1]];
        v2 uv2 = mesh->uv[mesh->indices[index + 2]];

        v3 e0 = vt1 - vt0;
        v3 e1 = vt2 - vt0;

        v2 dUV0 = uv1 - uv0;
        v2 dUV1 = uv2 - uv0;

        f32 f = 1.0f / (dUV0.u * dUV1.v - dUV1.u * dUV0.v);

        v3 tangent;
        tangent.x = f * (dUV1.v * e0.x - dUV0.v * e1.x);
        tangent.y = f * (dUV1.v * e0.y - dUV0.v * e1.y);
        tangent.z = f * (dUV1.v * e0.z - dUV0.v * e1.z);

        tangent = Normalize(tangent);

        mesh->tangents[index] = tangent;
        mesh->tangents[index + 1] = tangent;
        mesh->tangents[index + 2] = tangent;
    }
}


ABMesh GenABMesh(Mesh* mesh, std::vector<Material>* material_stack) {
    ABMesh ab_mesh = {};
    assert(mesh->vertices != 0);
    assert(mesh->num_vertex_indices % 3 == 0); // Mesh has non triangle faces

    b32 generated_normals = false;

    if (!mesh->num_normals) {
        // FIXME: GenNormals now is not working properly.
        // Its whites out of memory boundaries somewhere
        // and there are actually bad algorithm (normals aren't  interpolated between tringles)
        // So just abort for now
        printf("Error. Mesh doesn't have normals.\n");
        assert(false);
        printf("File does not contains normals. Generating normals...\n");
        mesh->normals = (hpm::Vector3*)malloc(sizeof(hpm::Vector3) * mesh->num_vertex_indices);
        mesh->num_normals = GenNormals(mesh->vertices, mesh->num_vertices, mesh->vertex_indices, mesh->num_vertex_indices, mesh->normals);
        generated_normals = true;
    }

    b32 has_uv = mesh->uv != nullptr;

    hpm::Vector3* vertices = nullptr;
    hpm::Vector3* normals = nullptr;
    hpm::Vector2* uvs = nullptr;
    u32* indices = nullptr;

    u64 vertices_at = 0;
    u64 normals_at = 0;
    u64 uvs_at = 0;
    u64 indices_at = 0;

    vertices = (hpm::Vector3*)malloc(mesh->num_vertex_indices * sizeof(hpm::Vector3));
    indices = (u32*)malloc(mesh->num_vertex_indices * sizeof(u32));
    if (has_uv) {
        uvs = (hpm::Vector2*)malloc(mesh->num_vertex_indices * sizeof(hpm::Vector2));
    }

    if (generated_normals) {
        normals = mesh->normals;
        normals_at = mesh->num_normals;
    }
    else {
        normals = (hpm::Vector3*)malloc(mesh->num_vertex_indices * sizeof(hpm::Vector3));
    }

    for (u32 i = 0; i < mesh->num_vertex_indices; i += 3) {
        assert(mesh->vertex_indices[i + 2] < mesh->num_vertices); // Out of range
        if (mesh->uv_indices) {
            assert(mesh->uv_indices[i + 2] < mesh->num_uv); // Out of range
        }
        if (mesh->normal_indices) {
            assert(mesh->normal_indices[i + 2] < mesh->num_normals); // Out of range
        }

        vertices[vertices_at] = mesh->vertices[mesh->vertex_indices[i]];
        vertices[vertices_at + 1] = mesh->vertices[mesh->vertex_indices[i + 1]];
        vertices[vertices_at + 2] = mesh->vertices[mesh->vertex_indices[i + 2]];
        vertices_at += 3;

        if (has_uv) {
            uvs[uvs_at] = mesh->uv[mesh->uv_indices[i]];
            uvs[uvs_at + 1] = mesh->uv[mesh->uv_indices[i + 1]];
            uvs[uvs_at + 2] = mesh->uv[mesh->uv_indices[i + 2]];
            uvs_at += 3;
        }

        if (!generated_normals) {
            normals[normals_at] = mesh->normals[mesh->normal_indices[i]];
            normals[normals_at + 1] = mesh->normals[mesh->normal_indices[i + 1]];
            normals[normals_at + 2] = mesh->normals[mesh->normal_indices[i + 2]];
            normals_at += 3;
        }

        indices[indices_at] = i;
        indices[indices_at + 1] = i + 1;
        indices[indices_at + 2] = i + 2;
        indices_at += 3;
    }
# if 0
    i32 material_index = -1;
    if (mesh->material_name) { // has material
        // Fetch material
        for (u32 i = 0; i < material_stack->size(); i++) {
            if (strcmp(mesh->material_name, (*material_stack)[i].name) == 0) {
                material_index = i;
            }
        }
    }
#endif
    ab_mesh.vertices = vertices;
    ab_mesh.normals = normals;
    ab_mesh.uv = uvs;
    ab_mesh.indices = indices;
    ab_mesh.num_vertices = (u32)vertices_at;
    ab_mesh.num_indices = (u32)indices_at;
    //ab_mesh.material_index = material_index;

    GenMeshTangents(&ab_mesh);

    return ab_mesh;
}

# if 0
// NOTE: This is crappy temporary solution
void WriteAABMesh(const char* file_name, ABMesh* mesh, std::vector<Material>* material_stack) {
    // TODO: Temporary: writing matrial to mesh asset
    u64 material_name_size = 0;
    u64 material_props_size = 0;

    u64 material_diff_name_size = 0;
    char* material_diff_map_name = nullptr;

    u64 material_spec_name_size = 0;
    char* material_spec_map_name = nullptr;
    Material* material = {};
    if (mesh->material_index != -1) { // has material
        material = &(*material_stack)[mesh->material_index];
        material_name_size = strlen(material->name) + 1;
        if (material->spec_map_name) {
            material_spec_map_name = material->spec_map_name;
            material_spec_name_size = strlen(material->spec_map_name) + 1;
        }
        // TODO: For now its assumed that amb and diff map is always the same texture
        if (material->amb_map_name) {
            material_diff_name_size = strlen(material->amb_map_name) + 1;
            material_diff_map_name = material->amb_map_name;
        }
        else if (material->diff_map_name) {
            material_diff_name_size = strlen(material->diff_map_name) + 1;
            material_diff_map_name = material->diff_map_name;
        }
    }
    material_props_size = sizeof(AB::AABMeshMaterialProperties);


    b32 has_uv = mesh->uv != nullptr;
    u32 num_uv = has_uv ? mesh->num_vertices : 0;

    u64 data_size = sizeof(hpm::Vector3) * mesh->num_vertices
        + sizeof(hpm::Vector2) * num_uv
        + sizeof(hpm::Vector3) * mesh->num_vertices
        + sizeof(u32) * mesh->num_indices
        + material_name_size
        + material_props_size
        + material_diff_name_size
        + material_spec_name_size;

    u64 file_size = data_size + sizeof(AB::AABMeshHeader);

    void* file_buffer = malloc(file_size);
    assert(file_buffer); // malloc failed

    AB::AABMeshHeader* header_ptr = (AB::AABMeshHeader*)file_buffer;
    header_ptr->magicValue = AB::AAB_FILE_MAGIC_VALUE;
    header_ptr->version = AB::AAB_FILE_VERSION;
    header_ptr->assetSize = data_size;
    header_ptr->assetType = AB::AAB_FILE_TYPE_MESH;
    header_ptr->verticesCount = mesh->num_vertices;
    header_ptr->normalsCount = mesh->num_vertices;
    header_ptr->uvsCount = num_uv;
    header_ptr->indicesCount = mesh->num_indices;

    void* vertices_ptr = (void*)(header_ptr + 1);
    u64 vertices_size = sizeof(hpm::Vector3) * mesh->num_vertices;
    memcpy(vertices_ptr, mesh->vertices, vertices_size);


    void* normals_ptr = (void*)((byte*)vertices_ptr + vertices_size);
    u64 normals_size = sizeof(hpm::Vector3) * mesh->num_vertices;
    memcpy(normals_ptr, mesh->normals, normals_size);

    void* uvs_ptr = (void*)((byte*)normals_ptr + normals_size);
    u64 uvs_size = sizeof(hpm::Vector2) * num_uv;
    if (uvs_size) {
        memcpy(uvs_ptr, mesh->uv, uvs_size);
    }

    void* indices_ptr = (void*)((byte*)uvs_ptr + uvs_size);
    u64 indices_size = sizeof(u32) * mesh->num_indices;
    memcpy(indices_ptr, mesh->indices, indices_size);
#if 0
    if (mesh->material_index != -1) {
        // TODO: Strings in the middle of file can break aligment.
        // Should write strings at the end of file or introduce padding
        void* mat_ptr = (void*)((byte*)indices_ptr + indices_size);
        memcpy(mat_ptr, material->name, material_name_size);
        mat_ptr = (void*)((byte*)mat_ptr + material_name_size);

        AB::AABMeshMaterialProperties aab_material = {};
        aab_material.k_a = material->ka;
        aab_material.k_d = material->kd;
        aab_material.k_s = material->ks;
        aab_material.k_e = material->ke;
        aab_material.shininess = material->shininess;

        memcpy(mat_ptr, &aab_material, sizeof(aab_material));

        if (material_diff_name_size) {
            mat_ptr = (void*)((byte*)mat_ptr + material_props_size);
            memcpy(mat_ptr, material_diff_map_name, material_diff_name_size);
        }

        if (material_spec_name_size) {
            mat_ptr = (void*)((byte*)mat_ptr + material_diff_name_size);
            memcpy(mat_ptr, material_spec_map_name, material_spec_name_size);
        }
    }
#endif
    header_ptr->verticesOffset = sizeof(AB::AABMeshHeader);
    header_ptr->normalsOffset = header_ptr->verticesOffset + vertices_size;
    header_ptr->uvsOffset = header_ptr->normalsOffset + normals_size;
    header_ptr->indicesOffset = header_ptr->uvsOffset + uvs_size;
#if 0
    if (mesh->material_index != -1) {
        header_ptr->material_name_offset = header_ptr->indices_offset + indices_size;
        header_ptr->material_properties_offset = header_ptr->material_name_offset + material_name_size;
        if (material_diff_map_name) {
            header_ptr->material_diff_bitmap_name_offset = header_ptr->material_properties_offset + material_props_size;
        }
        else {
            header_ptr->material_diff_bitmap_name_offset = 0;
        }

        if (material_spec_map_name) {
            header_ptr->material_spec_bitmap_name_offset = header_ptr->material_diff_bitmap_name_offset + material_diff_name_size;
        } else {
            header_ptr->material_spec_bitmap_name_offset = 0;
        }
    }
    else {
        header_ptr->material_name_offset = 0;
        header_ptr->material_properties_offset = 0;
        header_ptr->material_diff_bitmap_name_offset = 0;
        header_ptr->material_spec_bitmap_name_offset = 0;
    }
#endif
    assert(file_size < 0xffffffff); // Can`t write bigger than 4gb
    b32 result = WriteFile(file_name, file_buffer, (u32)file_size);
    assert(result); // Failed to write file

    free(file_buffer);
}
#endif

// NOTE: This is crappy temporary solution
void WriteAABMeshV2(const char* file_name, ABMesh* mesh, std::vector<Material>* material_stack)
{
    u32 num_uv = mesh->num_vertices;

    u64 data_size =
        sizeof(hpm::Vector3) * mesh->num_vertices
        + sizeof(hpm::Vector2) * num_uv
        + sizeof(hpm::Vector3) * mesh->num_vertices
        + sizeof(u32) * mesh->num_indices
        + sizeof(hpm::Vector3) * mesh->num_vertices; // tangents

    u64 file_size = data_size + sizeof(AB::AABMeshHeaderV2);

    void* file_buffer = malloc(file_size);
    assert(file_buffer); // malloc failed

    AB::AABMeshHeaderV2* header_ptr = (AB::AABMeshHeaderV2*)file_buffer;
    header_ptr->magicValue = AB::AAB_FILE_MAGIC_VALUE;
    header_ptr->version = AB::AAB_MESH_FILE_VERSION;
    header_ptr->assetSize = data_size;
    header_ptr->assetType = AB::AAB_MESH_FILE_VERSION;
    header_ptr->vertexCount = mesh->num_vertices;
    header_ptr->indexCount = mesh->num_indices;

    void* vertices_ptr = (void*)(header_ptr + 1);
    u64 vertices_size = sizeof(hpm::Vector3) * mesh->num_vertices;
    memcpy(vertices_ptr, mesh->vertices, vertices_size);

    void* normals_ptr = (void*)((byte*)vertices_ptr + vertices_size);
    u64 normals_size = sizeof(hpm::Vector3) * mesh->num_vertices;
    memcpy(normals_ptr, mesh->normals, normals_size);

    void* uvs_ptr = (void*)((byte*)normals_ptr + normals_size);
    u64 uvs_size = sizeof(hpm::Vector2) * num_uv;
    memcpy(uvs_ptr, mesh->uv, uvs_size);

    void* tangents_ptr = (void*)((byte*)uvs_ptr + uvs_size);
    u64 tangents_size = sizeof(v3) * mesh->num_vertices;
    memcpy(tangents_ptr, mesh->tangents, tangents_size);

    void* indices_ptr = (void*)((byte*)tangents_ptr + tangents_size);
    u64 indices_size = sizeof(u32) * mesh->num_indices;
    memcpy(indices_ptr, mesh->indices, indices_size);

    header_ptr->vertexOffset = sizeof(AB::AABMeshHeaderV2);
    header_ptr->normalsOffset = header_ptr->vertexOffset + vertices_size;
    header_ptr->uvOffset = header_ptr->normalsOffset + normals_size;
    header_ptr->tangentsOffset = header_ptr->uvOffset + uvs_size;
    header_ptr->indicesOffset = header_ptr->tangentsOffset + tangents_size;

    assert(file_size < 0xffffffff); // Can`t write bigger than 4gb
    b32 result = WriteFile(file_name, file_buffer, (u32)file_size);
    assert(result); // Failed to write file

    free(file_buffer);
}


int main(int argc, char** argv) {
    if (argc > 1) {
        constexpr u32 file_dir_sz = 512;
        char file_dir[file_dir_sz];
        u32 written;
        bool success = GetDirectory(argv[1], file_dir, file_dir_sz, &written);

        if (success) {
            std::vector<Mesh> mesh_stack;
            std::vector<Material> material_stack;

            ParseOBJ(argv[1], &mesh_stack, &material_stack);
            for (u32 i = 0; i < mesh_stack.size(); i++) {
                ABMesh mesh = GenABMesh(&(mesh_stack[i]), &material_stack);
                char* file_name = (char*)malloc(strlen(mesh_stack[i].name) + 4 + 2);
                strcpy(file_name, mesh_stack[i].name);
                strcat(file_name, ".aab");
                WriteAABMeshV2(file_name, &mesh, &material_stack);
                free(file_name);
                FreeMesh(&(mesh_stack[i]));
                FreeABMesh(&mesh);

            }
            //VertexData vertex_data = ParseOBJ((char*)data, size, file_dir);
            //WriteAABMesh(vertex_data);
        } else {
            printf("Too long file path.\n");
        }
    } else {
        printf("No input.\n");
    }
    return 0;
}
