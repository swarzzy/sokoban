#pragma warning(disable:4530)
#include "OfflineUtils.cpp"

#define HYPERMATH_IMPL
#include "hypermath.h"
#undef HYPERMATH_IMPL

#include <vector>

struct Mesh {
    u64 num_vertices;
    u64 num_normals;
    u64 num_uv;

    // NOTE(V2.0): They are should be the same
    u64 num_vertex_indices;
    u64 num_normal_indices;
    u64 num_uv_indices;

    // Theese all should be freed
    char* name;
    char* material_name;

    hpm::Vector3* vertices;
    hpm::Vector3* normals;
    hpm::Vector2* uv;

    u32* vertex_indices;
    u32* normal_indices;
    u32* uv_indices;
};

struct Material {
    char* name;
    char* diff_map_name;
    char* amb_map_name;
    char* spec_map_name;
    hpm::Vector3 ka;
    hpm::Vector3 kd;
    hpm::Vector3 ks;
    hpm::Vector3 ke;
    f32 shininess;
};

struct ParseVertexRet {
    hpm::Vector3 v;
    char* next;
};

inline static ParseVertexRet ParseVertex(const char* at) {  // after v
    char* next;
    f32 x = strtof(at, &next);
    assert(at != next);
    at = next;
    f32 y = strtof(at, &next);
    assert(at != next);
    at = next;
    f32 z = strtof(at, &next);
    assert(at != next);

    return { { x, y, z }, next };
}

struct ParseNormalRet {
    hpm::Vector3 v;
    char* next;
};

inline static ParseNormalRet ParseNormal(const char* at) {  // after v
    char* next;
    f32 x = strtof(at, &next);
    assert(at != next);
    at = next;
    f32 y = strtof(at, &next);
    assert(at != next);
    at = next;
    f32 z = strtof(at, &next);
    assert(at != next);

    auto result = hpm::Normalize(hpm::Vector3{ x, y, z });

    return { result, next };
}

struct ParseUVRet {
    hpm::Vector2 uv;
    char* next;
};

inline static ParseUVRet ParseUV(const char* at) {
    char* next;
    f32 u = strtof(at, &next);
    assert(at != next);
    at = next;
    f32 v = strtof(at, &next);
    assert(at != next);

    return { { u, v }, next };
}

enum class FaceIndexType : byte {
    Undefined = 0, Vertex, VertexNormal, VertexUV, VertexUVNormal
};

struct ParseFaceRet { u32 v[3];
    u32 uv[3];
    u32 n[3];
    const char* next;
};

inline static ParseFaceRet ParseFace(FaceIndexType type, const char* at) {  // after v

    char* next;
    ParseFaceRet result = {};

    for (u32 i = 0; i < 3; i++) {
        result.v[i] = strtoul(at, &next, 10) - 1;
        assert(at != next);
        at = next;
        if (type == FaceIndexType::VertexUV || type == FaceIndexType::VertexUVNormal) {
            while (*at == '/') at++;
            result.uv[i] = strtoul(at, &next, 10) - 1;
            assert(at != next);
            at = next;
        }
        if (type == FaceIndexType::VertexNormal || type == FaceIndexType::VertexUVNormal) {
            while (*at == '/') at++;
            result.n[i] = strtoul(at, &next, 10) - 1;
            assert(at != next);
            at = next;
        }
    }

    strtoul(at, &next, 10);
    assert(at == next); // Face has more than 3 vertices

    result.next = at;
    return result;
}

inline static FaceIndexType DefineFaceIndexType(const char* at) {
    FaceIndexType type = FaceIndexType::Undefined;
    while (*at == ' ') at++;
    u32 slash_counter = 0;
    b32 has_vt = false;
    while (*at != ' ' && *at != '\n') {
        if (slash_counter == 1 && !(*at == '/')) {
            has_vt = true;
        }
        if (*at == '/') {
            slash_counter++;
        }
        at++;
    }

    if (slash_counter == 0) {
        type = FaceIndexType::Vertex;
    }
    else if (slash_counter == 1) {
        type = FaceIndexType::VertexUV;
    }
    else if (slash_counter == 2) {
        if (has_vt) {
            type = FaceIndexType::VertexUVNormal;
        }
        else {
            type = FaceIndexType::VertexNormal;
        }
    }
    else {
        type = FaceIndexType::Undefined;
    }

    return type;
}

// TODO: This is very bag normals generation
// TODO: This now writes out of boundaries
static u32 GenNormals(hpm::Vector3* beg_vertices, u64 size_vertices, u32* beg_indices, u64 size_indices, hpm::Vector3* beg_normals) {
    u32 normals_at = 0;

    for (u32 i = 0; i < size_indices - 3; i += 3) {
        assert(i + 3 < size_indices);
        assert(beg_indices[i + 2] < size_vertices); // Out of range

        hpm::Vector3 first = hpm::SubV3V3(beg_vertices[beg_indices[i + 1]], beg_vertices[beg_indices[i]]);
        hpm::Vector3 second = hpm::SubV3V3(beg_vertices[beg_indices[i + 2]], beg_vertices[beg_indices[i]]);
        hpm::Vector3 normal = hpm::Normalize(hpm::Cross(first, second));
        beg_normals[normals_at] = normal;
        beg_normals[normals_at + 1] = normal;
        beg_normals[normals_at + 2] = normal;
        normals_at += 3;
    }
    return normals_at;
}

static u32 ExtractStringFromToEnd(char* out_buf, u32 out_size, const char* string) {
    const char* at = string;
    while (isspace((unsigned char)(*at))) at++;
    u32 buf_at = 0;
    while ((*at != '\n') && (*at != '\0')) {
        out_buf[buf_at] = (*at);
        buf_at++;
        if (buf_at == out_size - 1) {
            break;
        }
        at++;
    }
    out_buf[buf_at] = '\0';
    buf_at++;
    assert(buf_at < out_size);
    return buf_at;
}

struct ParseKRet {
    hpm::Vector3 K;
    const char* next;
};

inline static ParseKRet ParseK(const char* at) {  // after v
    char* next;
    f32 r = strtof(at, &next);
    assert(at != next);
    at = next;
    f32 g = strtof(at, &next);
    assert(at != next);
    at = next;
    f32 b = strtof(at, &next);
    assert(at != next);

    return { { r, g, b }, next };
}

enum class ObjTokenType : u32 {
    Unknown = 0,
    Comment,
    Object,
    Mtllib,
    Usemtl,
    Vertex,
    Normal,
    UV,
    Face
};

struct ObjToken {
    ObjTokenType type;
    union {
        hpm::Vector3 vertex;
        hpm::Vector3 normal;
        hpm::Vector2 uv;
        char* mtl_lib_filename;
        char* lib_name;
        char* object_name;
        struct {
            u32 v_ind[3];
            u32 n_ind[3];
            u32 uv_ind[3];
            FaceIndexType type;
        } face;
    } data;
};

static ObjToken TokenizeStringObj(const char* string) {
    ObjToken result = {};
    switch (string[0]) {
    case '#': {
        result.type = ObjTokenType::Comment;
    } break;
    case 'v': {
        if (string[1] == 't') {
            // uv
            auto[uv, next] = ParseUV(string + 2);
            result.type = ObjTokenType::UV;
            result.data.uv = uv;
        }
        else if (string[1] == 'n') {
            //normal
            auto[normal, next] = ParseNormal(string + 2);
            result.type = ObjTokenType::Normal;
            result.data.normal = normal;
        }
        else {
            // vertex
            auto[v, next] = ParseVertex(string + 1);
            result.type = ObjTokenType::Vertex;
            result.data.vertex = v;
        }
    } break;
    case 'f': {
        FaceIndexType type = DefineFaceIndexType(string + 1);
        auto[v_ind, uv_ind, n_ind, next] = ParseFace(type, string + 1);
        result.type = ObjTokenType::Face;
        memcpy(result.data.face.v_ind, v_ind, sizeof(u32) * 3);
        memcpy(result.data.face.n_ind, n_ind, sizeof(u32) * 3);
        memcpy(result.data.face.uv_ind, uv_ind, sizeof(u32) * 3);
        result.data.face.type = type;
    } break;
    case 'o': {
        char* obj_name = (char*)malloc(512);
        if (obj_name) {
            u32 obj_name_size = ExtractStringFromToEnd(obj_name, 512, string + 1);
            if (obj_name_size) {
                result.type = ObjTokenType::Object;
                result.data.object_name = obj_name;
            }
            else {
                printf("Object name too big.\n");
                assert(false);
            }
        }
        else {
            printf("malloc() failed.\n");
            assert(false);
        }
    } break;
    case 'm': {
        if (memcmp(string + 1, "tllib", 5) == 0) {
            char* mtl_name = (char*)malloc(512);
            if (mtl_name) {
                u32 mtl_name_size = ExtractStringFromToEnd(mtl_name, 512, string + 6);
                if (mtl_name_size) {
                    result.type = ObjTokenType::Mtllib;
                    result.data.mtl_lib_filename = mtl_name;
                }
                else {
                    printf("Mtl lib name too big.\n");
                    assert(false);
                }
            }
            else {
                printf("malloc() failed.\n");
                assert(false);
            }
        }
    } break;
    case 'u': {
        if (memcmp(string + 1, "semtl", 5) == 0) {
            char* lib_name = (char*)malloc(512);
            if (lib_name) {
                u32 lib_name_size = ExtractStringFromToEnd(lib_name, 512, string + 6);
                if (lib_name_size) {
                    result.type = ObjTokenType::Usemtl;
                    result.data.lib_name = lib_name;
                }
                else {
                    printf("Lib name too big.");
                    assert(false);
                }
            }
            else {
                printf("malloc() failed.\n");
                assert(false);
            }
        }
    } break;
    default: { result.type = ObjTokenType::Unknown; } break;
    }

    return result;
}

void FreeMesh(Mesh* mesh) {
    if (mesh->name) {
        free(mesh->name);
    }
    if (mesh->material_name) {
        free(mesh->material_name);
    }
    if (mesh->normals) {
        free(mesh->normals);
    }
    if (mesh->uv) {
        free(mesh->uv);
    }
    if (mesh->vertex_indices) {
        free(mesh->vertex_indices);
    }
    if (mesh->normal_indices) {
        free(mesh->normal_indices);
    }
    if (mesh->uv_indices) {
        free(mesh->uv_indices);
    }
    if (mesh->vertices) {
        free(mesh->vertices);
    }
    *mesh = {};
}

enum class MtlTokenType : u32 {
    Unknown = 0,
    Comment,
    Newmtl,
    DiffMap,
    AmbMap,
    SpecMap,
    Illum,
    Shininess,
    Ambient,
    Diffuse,
    Specular,
    Emission
};

struct MtlToken {
    MtlTokenType type;
    union {
        char* newmtl_name;
        char* diff_map_name;
        char* amb_map_name;
        char* spec_map_name;
        u32 illum;
        f32 shininess;
        hpm::Vector3 ambient;
        hpm::Vector3 diffuse;
        hpm::Vector3 specular;
        hpm::Vector3 emission;
    } data;
};

static MtlToken TokenizeStringMtl(const char* string) {
    MtlToken result = {};
    switch (string[0]) {
    case '#': {
        result.type = MtlTokenType::Comment;
    } break;
    case 'n': {
        if (memcmp(string + 1, "ewmtl", 5) == 0) {
            char* mtl_name = (char*)malloc(512);
            if (mtl_name) {
                u32 mtl_name_size = ExtractStringFromToEnd(mtl_name, 512, string + 6);
                if (mtl_name_size) {
                    result.type = MtlTokenType::Newmtl;
                    result.data.newmtl_name = mtl_name;
                }
                else {
                    printf("Materal name is too big.\n");
                    assert(false);
                }
            }
            else {
                printf("malloc() failed.\n");
                assert(false);
            }
        }
    } break;
    case 'm': {
        if (memcmp(string + 1, "ap_Kd", 5) == 0) {
            char* diff_map_name = (char*)malloc(512);
            if (diff_map_name) {
                u32 diff_map_name_size = ExtractStringFromToEnd(diff_map_name, 512, string + 6);
                if (diff_map_name_size) {
                    result.type = MtlTokenType::DiffMap;
                    result.data.diff_map_name = diff_map_name;
                }
                else {
                    printf("Diffuse map name is too big.\n");
                    assert(false);
                }
            }
            else {
                printf("malloc() failed.\n");
                assert(false);
            }
        }
        else if (memcmp(string + 1, "ap_Ka", 5) == 0) {
            char* amb_map_name = (char*)malloc(512);
            if (amb_map_name) {
                u32 amb_map_name_size = ExtractStringFromToEnd(amb_map_name, 512, string + 6);
                if (amb_map_name_size) {
                    result.type = MtlTokenType::AmbMap;
                    result.data.amb_map_name = amb_map_name;
                }
                else {
                    printf("Ambient map name is too big.\n");
                    assert(false);
                }
            }
            else {
                printf("malloc() failed.\n");
                assert(false);
            }
        }
        else if (memcmp(string + 1, "ap_Ks", 5) == 0) {
            char* spec_map_name = (char*)malloc(512);
            if (spec_map_name) {
                u32 spec_map_name_size = ExtractStringFromToEnd(spec_map_name, 512, string + 6);
                if (spec_map_name_size) {
                    result.type = MtlTokenType::SpecMap;
                    result.data.spec_map_name = spec_map_name;
                }
                else {
                    printf("Specular map name is too big.\n");
                    assert(false);
                }
            }
            else {
                printf("malloc() failed.\n");
                assert(false);
            }
        }
        else {
            printf("Warning: unexpected MTL directive.\n");
        }
    } break;
    case 'i': {
        if (memcmp(string + 1, "llum", 4) == 0) {
            char* next = nullptr;
            u32 illum = strtol(string + 5, &next, 10);
            assert((string + 5) != next);

            result.type = MtlTokenType::Illum;
            result.data.illum = illum;
        }
        else {
            printf("Warning: unexpected MTL directive.\n");
        }
    } break;
    case 'N': {
        if (string[1] == 's') {
            char* next = nullptr;
            f32 shin = strtof(string + 2, &next);
            assert((string + 2) != next);

            result.type = MtlTokenType::Shininess;
            result.data.shininess = shin;
        }
        else {
            printf("Warning: unexpected MTL directive.\n");
        }
    } break;
    case 'K': {
        switch (string[1]) {
        case 'a': {
            auto[amb_color, next] = ParseK(string + 2);
            result.type = MtlTokenType::Ambient;
            result.data.ambient = amb_color;
        } break;
        case 'd': {
            auto[dif_color, next] = ParseK(string + 2);
            result.type = MtlTokenType::Diffuse;
            result.data.diffuse = dif_color;
        } break;
        case 's': {
            auto[spc_color, next] = ParseK(string + 2);
            result.type = MtlTokenType::Specular;
            result.data.specular = spc_color;
        } break;
        case 'e': {
            auto[em_color, next] = ParseK(string + 2);
            result.type = MtlTokenType::Emission;
            result.data.emission = em_color;
        } break;
        default: {
            //printf("Failed to parse mtl argument.Unknown argument");
        }
        }
    } break;
    default: {
        //printf("Failed to parse mtl argument.Unknown argument");
    } break;
    }

    return  result;
}

struct MtlParsingState {
    Material material;
    b32 in_material;
};

static void HandleMtlToken(MtlParsingState* state, const MtlToken* tok, std::vector<Material>* material_stack) {
    switch (tok->type) {
    case MtlTokenType::Comment: {
        // do nothing
    } break;
    case MtlTokenType::Illum: {
        if (state->in_material) {
            assert(tok->data.illum == 2); // Only Blinn-phong supported
        }
    } break;
    case MtlTokenType::Ambient: {
        if (state->in_material) {
            state->material.ka = tok->data.ambient;
        }
    } break;
    case MtlTokenType::Diffuse: {
        if (state->in_material) {
            state->material.kd = tok->data.diffuse;
        }
    } break;
    case MtlTokenType::Specular: {
        if (state->in_material) {
            state->material.ks = tok->data.specular;
        }
    } break;
    case MtlTokenType::Emission: {
        if (state->in_material) {
            state->material.ke = tok->data.emission;
        }
    } break;
    case MtlTokenType::Shininess: {
        if (state->in_material) {
            state->material.shininess = tok->data.shininess;
        }
    } break;
    case MtlTokenType::AmbMap: {
        if (state->in_material) {
            state->material.amb_map_name = tok->data.amb_map_name;
        }
    } break;
    case MtlTokenType::DiffMap: {
        if (state->in_material) {
            state->material.diff_map_name = tok->data.diff_map_name;
        }
    } break;
    case MtlTokenType::SpecMap: {
        if (state->in_material) {
            state->material.spec_map_name= tok->data.spec_map_name;
        }
    } break;
    case MtlTokenType::Newmtl: {
        if (!(state->in_material)) {
            state->in_material = true;
            state->material.name = tok->data.newmtl_name;
        }
        else {
            material_stack->push_back(state->material);
            state->material = {};
            state->material.name = tok->data.newmtl_name;
        }
    } break;
    case MtlTokenType::Unknown: {
        //printf("Unknown mtl directive\n");
    } break;
    default: {
        //printf("Unknown mtl directive\n");
    } break;
    }
}

static void FinalizeMtl(MtlParsingState* state, std::vector<Material>* material_stack) {
    material_stack->push_back(state->material);
}

void LoadMTL(const char* path, std::vector<Material>* material_stack) {
    u32 size;
    char* data = ReadEntireFileAsText(path, &size);
    if (data) {
        data[size - 1] = '\0';
        MtlParsingState parsing_state = {};

        char* at = data;
        if (*at != '\n') {
            MtlToken tok = TokenizeStringMtl(at);
            HandleMtlToken(&parsing_state, &tok, material_stack);
        }

        while (*at != '\0') {
            if (*at == '\n') {
                MtlToken tok = TokenizeStringMtl(at + 1);
                HandleMtlToken(&parsing_state, &tok, material_stack);
            }
            at++;
        }

        FinalizeMtl(&parsing_state, material_stack);

        FreeFileMemory(data);
    }
    else {
        printf("Failed to read mtl library: %s", path);
    }
}

// This is not POD! Do not memset it to 0
struct ObjParsingState {
    u32 global_vertex_index_offset;
    u32 global_normal_index_offset;
    u32 global_uv_index_offset;

    std::vector<hpm::Vector3> vertices;
    std::vector<hpm::Vector3> normals;
    std::vector<hpm::Vector2> uvs;
    std::vector<u32> vertex_indices;
    std::vector<u32> normal_indices;
    std::vector<u32> uv_indices;
    char* name;
    char* material_name;
    b32 default_group;
};

static void ResetObjParsingState(ObjParsingState* state) {
    state->global_vertex_index_offset += SafeTruncateU64U32(state->vertices.size());
    state->global_normal_index_offset += SafeTruncateU64U32(state->normals.size());
    state->global_uv_index_offset += SafeTruncateU64U32(state->uvs.size());

    state->vertices.clear();
    state->normals.clear();
    state->uvs.clear();
    state->vertex_indices.clear();
    state->normal_indices.clear();
    state->uv_indices.clear();
    state->name = nullptr;
    state->material_name = nullptr;
}

static void PushMesh(ObjParsingState* state, std::vector<Mesh>* mesh_stack) {
    Mesh mesh = {};
    mesh.num_vertices = state->vertices.size();
    mesh.num_normals = state->normals.size();
    mesh.num_uv = state->uvs.size();

    mesh.num_vertex_indices = state->vertex_indices.size();
    mesh.num_normal_indices = state->normal_indices.size();
    mesh.num_uv_indices = state->uv_indices.size();

    mesh.name = state->name;
    mesh.material_name = state->material_name;

    if (mesh.num_vertices) {
        mesh.vertices = (hpm::Vector3*)malloc(sizeof(hpm::Vector3) * mesh.num_vertices);
        assert(mesh.vertices); // malloc() failed
        memcpy(mesh.vertices, state->vertices.data(), mesh.num_vertices * sizeof(hpm::Vector3));
    }

    if (mesh.num_normals) {
        mesh.normals = (hpm::Vector3*)malloc(sizeof(hpm::Vector3) * mesh.num_normals);
        assert(mesh.normals); // malloc() failed
        memcpy(mesh.normals, state->normals.data(), mesh.num_normals * sizeof(hpm::Vector3));
    }

    if (mesh.num_uv) {
        mesh.uv = (hpm::Vector2*)malloc(sizeof(hpm::Vector2) * mesh.num_uv);
        assert(mesh.uv); // malloc() failed
        memcpy(mesh.uv, state->uvs.data(), mesh.num_uv * sizeof(hpm::Vector2));
    }

    if (mesh.num_vertex_indices) {
        mesh.vertex_indices = (u32*)malloc(sizeof(u32) * mesh.num_vertex_indices);
        assert(mesh.vertex_indices); // malloc() failed
        memcpy(mesh.vertex_indices, state->vertex_indices.data(), mesh.num_vertex_indices * sizeof(u32));
    }

    if (mesh.num_normal_indices) {
        mesh.normal_indices = (u32*)malloc(sizeof(u32) * mesh.num_normal_indices);
        assert(mesh.normal_indices); // malloc() failed
        memcpy(mesh.normal_indices, state->normal_indices.data(), mesh.num_normal_indices * sizeof(u32));
    }

    if (mesh.num_uv_indices) {
        mesh.uv_indices = (u32*)malloc(sizeof(u32) * mesh.num_uv_indices);
        assert(mesh.uv_indices); // malloc() failed
        memcpy(mesh.uv_indices, state->uv_indices.data(), mesh.num_uv_indices * sizeof(u32));
    }

    ResetObjParsingState(state);
    mesh_stack->push_back(mesh);
}

static void HandleObjToken(ObjParsingState* state, const char* obj_dir, ObjToken* tok,
                           std::vector<Mesh>* mesh_stack, std::vector<Material>* material_stack) {

    switch (tok->type) {
    case ObjTokenType::Vertex: {
        state->vertices.push_back(tok->data.vertex);
    } break;
    case ObjTokenType::Normal: {
        state->normals.push_back(tok->data.normal);
    } break;
    case ObjTokenType::UV: {
        state->uvs.push_back(tok->data.uv);
    } break;
    case ObjTokenType::Comment: {
        // Do nothing
    } break;
    case ObjTokenType::Object: {
        if (state->default_group) {
            assert(state->vertices.size() == 0); // Data has no group
            state->default_group = false;
            free(state->name);
            state->name = tok->data.object_name;
        }
        else {
            PushMesh(state, mesh_stack);
            state->name = tok->data.object_name;
        }
    } break;
    case ObjTokenType::Face: {
        auto type = tok->data.face.type;
        state->vertex_indices.push_back(tok->data.face.v_ind[0] - state->global_vertex_index_offset);
        state->vertex_indices.push_back(tok->data.face.v_ind[1] - state->global_vertex_index_offset);
        state->vertex_indices.push_back(tok->data.face.v_ind[2] - state->global_vertex_index_offset);

        if (type == FaceIndexType::VertexNormal || type == FaceIndexType::VertexUVNormal) {
            state->normal_indices.push_back(tok->data.face.n_ind[0] - state->global_normal_index_offset);
            state->normal_indices.push_back(tok->data.face.n_ind[1] - state->global_normal_index_offset);
            state->normal_indices.push_back(tok->data.face.n_ind[2] - state->global_normal_index_offset);
        }

        if (type == FaceIndexType::VertexUV || type == FaceIndexType::VertexUVNormal) {
            state->uv_indices.push_back(tok->data.face.uv_ind[0] - state->global_uv_index_offset);
            state->uv_indices.push_back(tok->data.face.uv_ind[1] - state->global_uv_index_offset);
            state->uv_indices.push_back(tok->data.face.uv_ind[2] - state->global_uv_index_offset);
        }
    } break;
    case ObjTokenType::Mtllib: {
        auto fname_len = strlen(tok->data.mtl_lib_filename) + 1;
        auto dir_len = strlen(obj_dir) + 1;
        char* full_mtl_path = (char*)malloc(fname_len + dir_len);
        memcpy(full_mtl_path, obj_dir, dir_len + 1);
        strcat(full_mtl_path, tok->data.mtl_lib_filename);

        LoadMTL(full_mtl_path, material_stack);

        free(full_mtl_path);
    } break;
    case ObjTokenType::Usemtl: {
        if (state->material_name) {
            printf("Warninig: material %s overwrited by %s\n", state->material_name, tok->data.lib_name);
        }
        state->material_name = tok->data.lib_name;
    } break;
    default: {} break;
    }
}

void ParseOBJ(const char* file_path, std::vector<Mesh>* mesh_stack, std::vector<Material>* material_stack) {
    u32 size;
    char* data = ReadEntireFileAsText(file_path, &size);
    if (data) {
        constexpr u32 file_dir_sz = 512;
        char dir[file_dir_sz];
        u32 written;
        bool success = GetDirectory(file_path, dir, file_dir_sz, &written);

        if (success) {
            // TODO: move that to function. Because it's refactoring is a hell now
            ObjParsingState state = ObjParsingState();
            state.name = (char*)malloc(strlen("unnamed") + 1);
            memcpy(state.name, "unnamed", strlen("unnamed") + 1);

            state.default_group = true;
            state.material_name = nullptr;

            char* at = data;
            if (*at != '\n') {
                ObjToken tok = TokenizeStringObj(at);
                HandleObjToken(&state, file_path, &tok, mesh_stack, material_stack);
            }

            while (*at != '\0') {
                if (*at == '\n') {
                    ObjToken tok = TokenizeStringObj(at + 1);
                    HandleObjToken(&state, dir, &tok, mesh_stack, material_stack);
                }
                at++;
            }
            PushMesh(&state, mesh_stack);
        }
        else {
            printf("Failed to read file: %s", file_path);
        }
        FreeFileMemory(data);
    }
    else {
        printf("Internal error."); // failed to get directory
    }
}
