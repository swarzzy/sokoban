// NOTE:
// Maybe someday I should get rid of this crappy parser
// and use opengl functions for getting info about shaders and uniforms
// (glGetActiveAttrib, glGetActiveUniform) ???
#include <assert.h>
#include <vector>
#include <string>
#include <unordered_map>

#include <stdlib.h>

#include "../OfflineUtils.cpp"

#define INVALID_DEFAULT_CASE() assert(false)

#undef ERROR
#define ERROR(...) (printf(__VA_ARGS__), exit(EXIT_FAILURE))

global_variable i32x _IDENT_LEVEL = 0;
#define IDENT_PUSH() (_IDENT_LEVEL++)
#define IDENT_POP() (_IDENT_LEVEL--)
#define IDENT_RESET() (_IDENT_LEVEL = 0)
global_variable FILE* OutFile = 0;
// NOTE: Out line
#define L(format,...) do {assert(_IDENT_LEVEL >= 0); for (i32x i = 0; i < _IDENT_LEVEL; i++) fprintf(OutFile, "    "); fprintf(OutFile, CONCAT(format, "\n"), __VA_ARGS__);} while(false)
// NOTE: Out line no eol
#define O(format, ...) do {assert(_IDENT_LEVEL >= 0); for (i32x i = 0; i < _IDENT_LEVEL; i++) fprintf(OutFile, "    "); fprintf(OutFile, format, __VA_ARGS__);} while(false)
// NOTE: Append line
#define A(...) fprintf(OutFile, __VA_ARGS__)


enum class BuiltinType
{
    Unknown = 0,
    Bool,
    Float,
    Int,
    Vec2,
    Vec3,
    Vec4,
    Mat3,
    Mat4,
    sampler2D,
    samplerCube,
    sampler2DArray,
    sampler2DShadow
};

struct Uniform
{
    char* type;
    char* name;
    u32 arrayCount;
    i32 samplerSlot = -1;
    b32 isBuiltinType;
    BuiltinType builtinType;
    i32 userTypeIndex = -1;

    static const char* SamplerDecls[4];
};

const char* Uniform::SamplerDecls[4] = {
    "sampler2D",
    "samplerCube",
    "sampler2DArray",
    "sampler2DShadow"
};

const char* BuiltinTypeNames[] = {
    "bool",
    "float",
    "int",
    "vec2",
    "vec3",
    "vec4",
    "mat3",
    "mat4",
    "sampler2D",
    "samplerCube",
    "sampler2DArray",
    "sampler2DShadow"
};

struct VertexAttrib
{
    enum Type
    {
        Unknown = 0,
        Bool,
        Float,
        Int,
        Vec2,
        Vec3,
        Vec4,
        Mat3,
        Mat4
    } type;

    i32 location = -1;
    char* name;

    static const char* Types[(u32)Type::Mat4 + 1];
};

const char* VertexAttrib::Types[] = {
    "bool"
    "float",
    "int",
    "vec2",
    "vec3",
    "vec4",
    "mat3",
    "mat4",
};

struct StructMember
{
    char* name;
    char* typeName;
    u32 arrayCount;
    b32 isBuiltinType;
    i32 userTypeIndex = -1;
    BuiltinType builtinType;
};

struct Struct
{
    b32 used;
    const char* name;
    std::vector<StructMember> members;
};

struct ProgramConfig
{
    char* name;
    char* vertexName;
    char* fragmentName;
};

struct Shader
{
    enum Type {Unknown = 0, Vertex, Fragment} type;
    const char* source;
    const char* name;
    std::vector<Uniform> uniforms;
    std::vector<VertexAttrib> vertexAttribs;
    std::vector<Struct> userTypes;
};

struct Program
{
    ProgramConfig config;
    Shader* vertex;
    Shader* fragment;
};

constant char* DELIMETERS = " \f\n\r\t\v";
constant char* EOL_MARKERS = " \n\r";
constant char* WORD_PARSE_ERROR_MARKERS = ",;{} \f\n\r\t\v=+-";

char* EatUntilSpace(char* at)
{
    while (*at && !IsSpace(*at)) at++;
    return at;
}

char* Offset(char* at, u32x count)
{
    for (u32x i = 0 ; i < count; i++)
    {
        if (!*at) break;
        at++;
    }
    return at;
}

#define OffsetByLiteral(at, literal) Offset(at, (sizeof(literal) - 1))

bool OneOf(char c, const char* t)
{
    bool result = false;
    // TODO: Ensure this is compile time evaluated
    const auto size = StrLength(t);
    for (u32x i = 0; i < size; i++)
    {
        if (c == t[i])
        {
            result = true;
            break;
        }
    }
    return result;
}

bool ContainsOneOf(char* at, const char* chars)
{
    bool result = false;
    while(*at)
    {
        if (OneOf(*at, chars))
        {
            result = true;
            break;
        }
        at++;
    }
    return result;
}

char* EatUntilOneOf(char* at, const char* chars)
{
    while (*at && !OneOf(*at, chars)) at++;
    return at;
}

char* EatUntilOneOf(char* at, const char* chars1, const char* chars2)
{
    while (*at && !OneOf(*at, chars1) && !OneOf(*at, chars2)) at++;
    return at;
}

char* Advance1(char* at)
{
    if (*(at + 1)) at++;
    return at;
}

// NOTE: end is a one past last string character
char* ExtractString(char* beg, char* end)
{
    assert((uptr)beg < (uptr)end);
    auto size = end - beg + 1;
    char* string = (char*)malloc(size);
    memcpy(string, beg, size - 1);
    string[size - 1] = 0;
    return string;
}

Uniform FindNextUniform(Tokenizer* t)
{
    // TODO: Read comma-separated member declarations
    Uniform decl = {};
    while (*t->at)
    {
        t->at = EatSpace(t->at);
        if (Match(t->at, "uniform"))
        {
            t->at = OffsetByLiteral(t->at, "uniform");
            t->at = EatSpace(t->at);
            auto typeBegin = t->at;
            t->at = EatUntilSpace(t->at);
            decl.type = ExtractString(typeBegin, t->at);
            t->at = EatSpace(t->at);

            auto nameBegin = t->at;
            t->at = EatUntilOneOf(t->at, ";,[", DELIMETERS);
            auto nameEnd = t->at;
            decl.name = ExtractString(nameBegin, nameEnd);

            t->at = EatSpace(t->at);
            if (Match(t->at, '['))
            {
                decl.arrayCount = atoi(t->at + 1);
            }
            break;
        }
        else
        {
            t->at++;
        }
    }
    return decl;
}

VertexAttrib::Type MatchAttribType(Tokenizer* tok)
{
    auto type = VertexAttrib::Unknown;
    for (u32x i = 0; i < ArrayCount(VertexAttrib::Types); i++)
    {
        if (Match(tok->at, VertexAttrib::Types[i]))
        {
            type = (VertexAttrib::Type)(i + 1);
            tok->at += StrLength(VertexAttrib::Types[i]);
            break;
        }
    }
    return type;
}

void ParseVertexAttribTypeAndName(Tokenizer* t, VertexAttrib* attrib)
{
    t->at = EatSpace(t->at);
    attrib->type = MatchAttribType(t);
    t->at = EatSpace(t->at);
    auto begin = t->at;
    t->at = EatUntilOneOf(t->at, ";", DELIMETERS);
    attrib->name = ExtractString(begin, t->at);
}

VertexAttrib FindNextAttrib(Tokenizer* t)
{
    VertexAttrib decl = {};
    while (*t->at)
    {
        t->at = EatSpace(t->at);
        // NOTE: Keyword "in" is not allowed to use as variable name.
        // This is true at leat on my nvidia driver. This makes parsing much easier
        // because we don't need to check cases when "in" used as variable name or smth.
        // TODO: Check this on glsl reference compiler
        auto foundIn = Match(t->at, "in") && OneOf(*(t->at + sizeof("in") - 1), DELIMETERS);
        if (foundIn)
        {
            t->at = OffsetByLiteral(t->at, "in");
            ParseVertexAttribTypeAndName(t, &decl);
            break;
        }
        if (Match(t->at, "layout"))
        {
            bool succesfullyParseLocation = false;
            t->at = OffsetByLiteral(t->at, "layout");
            t->at = EatSpace(t->at);
            if (*t->at == '(')
            {
                t->at = OffsetByLiteral(t->at, "(");
                t->at = EatSpace(t->at);
                if (Match(t->at, "location"))
                {
                    t->at = OffsetByLiteral(t->at, "location");
                    t->at = EatSpace(t->at);
                    if (Match(t->at, "="))
                    {
                        t->at = OffsetByLiteral(t->at, "=");
                        t->at = EatSpace(t->at);
                        decl.location = atoi(t->at);
                        t->at = EatUntilOneOf(t->at, ")");
                        t->at = OffsetByLiteral(t->at, ")");
                        t->at = EatSpace(t->at);
                        succesfullyParseLocation = true;
                    }
                }
            }
            if (succesfullyParseLocation)
            {
                if (Match(t->at, "in"))
                {
                    t->at = OffsetByLiteral(t->at, "in");
                    ParseVertexAttribTypeAndName(t, &decl);
                }
            }
            break;
        }
        else
        {
            t->at++;
        }
    }
    return decl;
}

Struct FindNextStruct(Tokenizer* t)
{
    // TODO: Read comma-separated member declarations
    Struct decl = {};
    while (*t->at)
    {
        t->at = EatSpace(t->at);
        if (Match(t->at, "struct"))
        {
            t->at = OffsetByLiteral(t->at, "struct");
            t->at = EatSpace(t->at);
            auto nameBegin = t->at;
            t->at = EatUntilOneOf(t->at, "{", DELIMETERS);
            decl.name = ExtractString(nameBegin, t->at);

            if (Match(t->at, "{"))
            {
                t->at = OffsetByLiteral(t->at, "{");
            }
            else
            {
                t->at = EatUntilOneOf(t->at, "{");
                t->at = OffsetByLiteral(t->at, "{");
            }
            t->at = EatSpace(t->at);

            while (true)
            {
                if (Match(t->at, '}')) goto end;
                StructMember member = {};
                auto typeNameBegin = t->at;
                // TODO: Eat by words
                t->at = EatUntilSpace(t->at);
                member.typeName = ExtractString(typeNameBegin, t->at);
                t->at = EatSpace(t->at);
                auto nameBegin = t->at;
                t->at = EatUntilOneOf(t->at, ";,[", DELIMETERS);
                member.name = ExtractString(nameBegin, t->at);
                if (Match(t->at, ";"))
                {
                    t->at = Advance1(t->at);
                }
                if (Match(t->at, ","))
                {
                    ERROR("Error: Comma separated struct member declarations are not supported.\n");
                }
                t->at = EatSpace(t->at);
                if (Match(t->at, '['))
                {
                    member.arrayCount = atoi(t->at + 1);
                    t->at = EatUntilOneOf(t->at, "]");
                    t->at = OffsetByLiteral(t->at, "]");
                    // TODO: Comma ceparated member declarations not allowen for now
                    t->at = Advance1(t->at);
                }
                t->at = EatSpace(t->at);
                decl.members.push_back(member);
            }
        }
        else
        {
            t->at++;
        }
    }
end:
    return decl;
}

BuiltinType GetUniformBuiltinType(const char* str)
{
    BuiltinType result = {};
    for (u32x i = 0; i < ArrayCount(BuiltinTypeNames); i++)
    {
        if (Match(str, BuiltinTypeNames[i]))
        {
            result = (BuiltinType)(i + 1);
            break;
        }
    }
    return result;
}

ProgramConfig GetNextProgConfig(Tokenizer* t)
{
    ProgramConfig result = {};
    while (*t->at)
    {
        t->at = EatSpace(t->at);
        if (Match(t->at, "#program"))
        {
            t->at = OffsetByLiteral(t->at, "#program");
            t->at = EatSpace(t->at);

            auto nameBegin = t->at;
            result.name = t->at;
            t->at = EatUntilSpace(t->at);
            result.name = ExtractString(nameBegin, t->at);
            t->at = EatSpace(t->at);

            while (*t->at)
            {
                t->at = EatSpace(t->at);
                if (Match(t->at, "vertex"))
                {
                    t->at = OffsetByLiteral(t->at, "vertex");
                    t->at = EatUntilOneOf(t->at, ":");
                    t->at = OffsetByLiteral(t->at, ":");
                    t->at = EatSpace(t->at);

                    auto nameBegin = t->at;
                    t->at = EatUntilOneOf(t->at, DELIMETERS);
                    result.vertexName = ExtractString(nameBegin, t->at);
                    t->at = Advance1(t->at);
                }
                else if (Match(t->at, "fragment"))
                {
                    t->at = OffsetByLiteral(t->at, "fragment");
                    t->at = EatUntilOneOf(t->at, ":");
                    t->at = OffsetByLiteral(t->at, ":");
                    t->at = EatSpace(t->at);

                    auto nameBegin = t->at;
                    t->at = EatUntilOneOf(t->at, DELIMETERS);
                    result.fragmentName = ExtractString(nameBegin, t->at);
                    t->at = Advance1(t->at);
                }
                else if (Match(t->at, "#"))
                {
                    goto end;
                }
                else
                {
                    t->at++;
                }
            }
        }
        else
        {
            t->at++;
        }
    }
end:
    return result;
}

struct ShaderDecl
{
    Shader::Type type;
    char* name;
    char* path;
};

void ExtractShaderDeclarations(Tokenizer* t, std::vector<ShaderDecl>* decls)
{
    while (*t->at)
    {
        t->at = EatSpace(t->at);
        if (Match(t->at, "#shaders"))
        {
            t->at = OffsetByLiteral(t->at, "#shaders");
            t->at = EatSpace(t->at);

            while (*t->at)
            {
                ShaderDecl decl = {};
                t->at = EatSpace(t->at);
                if (Match(t->at, "#")) break;
                bool found = false;
                if (Match(t->at, "vertex"))
                {
                    decl.type = Shader::Vertex;
                    t->at = OffsetByLiteral(t->at, "vertex");
                    found = true;
                }
                else if (Match(t->at, "fragment"))
                {
                    decl.type = Shader::Fragment;
                    t->at = OffsetByLiteral(t->at, "fragment");
                    found = true;
                }
                if (found)
                {
                    t->at = EatUntilOneOf(t->at, ":");
                    t->at = OffsetByLiteral(t->at, ":");
                    t->at = EatSpace(t->at);
                    auto nameBegin = t->at;
                    t->at = EatUntilOneOf(t->at, DELIMETERS);
                    decl.name = ExtractString(nameBegin, t->at);
                    while (!Match(t->at, "->"))
                    {
                        if (!*t->at) break;
                        t->at++;
                    }
                    t->at = OffsetByLiteral(t->at, "->");
                    t->at = EatUntilOneOf(t->at, "\"");
                    t->at = OffsetByLiteral(t->at, "\"");
                    auto pathBegin = t->at;
                    t->at = EatUntilOneOf(t->at, "\"");
                    decl.path = ExtractString(pathBegin, t->at);
                    t->at = Advance1(t->at);
                    decls->push_back(decl);
                }
                else
                {
                    t->at++;
                }
            }
        }
        else
        {
            t->at++;
        }
    }
}

void ParseConfigFile(const char* filename, std::vector<ShaderDecl>* shaderDecls, std::vector<ProgramConfig>* programDecls)
{
    u32 size;
    auto source = ReadEntireFileAsText(filename, &size);
    if (!source)
    {
        ERROR("Error: failed to open config file %s\n", filename);
    }

    Tokenizer tokenzier = {};
    tokenzier.text = source;
    tokenzier.at = source;

    ExtractShaderDeclarations(&tokenzier, shaderDecls);

    tokenzier.at = source;

    for (auto& decl : *shaderDecls)
    {
        if (decl.type == Shader::Unknown)
        {
            ERROR("Error: Found unknown shader type\n");
        }
        if (!decl.name || !StrLength(decl.name))
        {
            ERROR("Error: Found shader declaration without a name\n");
        }

        if (!decl.path || !StrLength(decl.path))
        {
            ERROR("Error: Found shader declaration with no path specified\n");
        }
    }

    while (true)
    {
        auto conf = GetNextProgConfig(&tokenzier);

        if (!conf.name || !StrLength(conf.name))
        {
            ERROR("Error: Found program declaration without a name\n");
        }

        if (!conf.vertexName || !StrLength(conf.vertexName))
        {
            ERROR("Error: Found program declaration with no vertex path specified\n");
        }

        if (!conf.fragmentName || !StrLength(conf.fragmentName))
        {
            ERROR("Error: Found program declaration with no fragment path specified\n");
        }

        for (auto& it : *programDecls)
        {
            if (strcmp(it.name, conf.name) == 0)
            {
                ERROR("Error: Found two of more program declarations with the same name: %s\n", it.name);
            }
        }

        programDecls->emplace_back(conf);

        if (!*tokenzier.at) break;
    }
    //free(source);
}

void ParseShaderFile(const char* filename, Shader::Type type, Shader* shader)
{
    u32 size;
    auto source = ReadEntireFileAsText(filename, &size);
    if (!source)
    {
        ERROR("Error: failed to open shader file %s\n", filename);
    }

    shader->source = source;

    Tokenizer tokenizer = {};
    tokenizer.text = source;
    tokenizer.at = source;

    assert(type != Shader::Unknown);

    shader->type = type;

    while (true)
    {
        auto uniform = FindNextUniform(&tokenizer);
        if (!*tokenizer.at) break;

        if (!uniform.type || !StrLength(uniform.type))
        {
            ERROR("Error: a uniform of unknown type\n");
        }
        if (!uniform.name || !StrLength(uniform.name))
        {
            ERROR("Error: Found a uniform without a name\n");
        }
        if (ContainsOneOf(uniform.name, WORD_PARSE_ERROR_MARKERS))
        {
            ERROR("Error: Failed to parse uniform name\n");
        }
        if (ContainsOneOf(uniform.type, WORD_PARSE_ERROR_MARKERS))
        {
            ERROR("Error: Failed to parse uniform type\n");
        }
        if (uniform.samplerSlot != -1 && uniform.arrayCount > 0)
        {
            ERROR("Error: Arrays of samplers are not supported\n");
        }

        shader->uniforms.push_back(uniform);
    }

    tokenizer.at = tokenizer.text;

    if (shader->type == Shader::Vertex)
    {
        while (true)
        {
            auto attrib = FindNextAttrib(&tokenizer);
            if (!*tokenizer.at) break;

            if (attrib.type == VertexAttrib::Type::Unknown)
            {
                ERROR("Error: Found a vertex attribute of unknown type\n");
            }
            if (!attrib.name || !StrLength(attrib.name))
            {
                ERROR("Error: Found a vertex attribute without a name\n");
            }
            if (ContainsOneOf(attrib.name, WORD_PARSE_ERROR_MARKERS))
            {
                ERROR("Error: Failed to parse a vertex attribute name\n");
            }

            shader->vertexAttribs.push_back(attrib);
        }
    }

    tokenizer.at = tokenizer.text;

    while (true)
    {
        auto userType = FindNextStruct(&tokenizer);
        if (!*tokenizer.at) break;

        if (!userType.name || !StrLength(userType.name))
        {
            ERROR("Error: Found a struct without a name\n");
        }

        if (!userType.members.size())
        {
            ERROR("Error: Struct can not have zero members\n");
        }

        for (auto& it : userType.members)
        {
            if (!it.name || !StrLength(it.name))
            {
                ERROR("Error: Found a struct member without a name\n");
            }
            if (!it.typeName || !StrLength(it.typeName))
            {
                ERROR("Error: Found a struct member with an unknown type\n");
            }

        }

        shader->userTypes.push_back(userType);
    }

    for (auto& type : shader->userTypes)
    {
        for (auto& member: type.members)
        {
            auto type = GetUniformBuiltinType(member.typeName);
            if (type != BuiltinType::Unknown)
            {
                member.isBuiltinType = true;
                member.builtinType = type;
            }
            else
            {
                // NOTE: User defined type
                for (u32x i = 0; i < shader->userTypes.size(); i++)
                {
                    if (Match(shader->userTypes[i].name, member.typeName))
                    {
                        shader->userTypes[i].used = true;
                        member.userTypeIndex = i;
                        break;
                    }
                }
                if (member.userTypeIndex == -1)
                {
                    ERROR("Error: Cannot find definition for type: %s of uniform %s\n", member.typeName, member.name);
                }
            }
        }
    }

    // TODO: Check that it less than max GL_TEXTUREX
    // NOTE: Samplers only available in fragment shaders
    u32x samplerCount = 0;

    for (auto& it : shader->uniforms)
    {
        for (u32x i = 0; i < ArrayCount(Uniform::SamplerDecls); i++)
        {
            if (Match(it.type, Uniform::SamplerDecls[i]))
            {
                it.samplerSlot = samplerCount;
                samplerCount++;
            }
        }
    }


    for (auto& it : shader->uniforms)
    {
        for (u32x i = 0; i < ArrayCount(Uniform::SamplerDecls); i++)
        {
            auto type = GetUniformBuiltinType(it.type);
            if (type != BuiltinType::Unknown)
            {
                it.isBuiltinType = true;
                it.builtinType = type;
            }
            else
            {
                // NOTE: User defined type
                for (u32x i = 0; i < shader->userTypes.size(); i++)
                {
                    if (Match(shader->userTypes[i].name, it.type))
                    {
                        shader->userTypes[i].used = true;
                        it.userTypeIndex = i;
                        break;
                    }
                }
                if (it.userTypeIndex == -1)
                {
                    ERROR("Error: Cannot find definition for type: %s of uniform %s\n", it.type, it.name);
                }
            }
        }
    }
    //free(source);
}

void OutUserType(const Struct* type, const Shader* shader)
{
    L("struct %s", type->name);
    L("{");
    IDENT_PUSH();
    for (auto& member : type->members)
    {
        if (member.isBuiltinType)
        {
            O("GLint %s", member.name);
            if (member.arrayCount > 0)
            {
                A("[%d]", member.arrayCount);
            }
        }
        else
        {
            auto subType = &shader->userTypes[member.userTypeIndex];
            O("%s %s", subType->name, member.name);
            if (member.arrayCount > 0)
            {
                A("[%d]", member.arrayCount);
            }
        }
        A(";\n");
    }
    IDENT_POP();
    L("};\n");
}

void OutUniform(const Uniform* uni, const Shader* shader)
{
    if (!uni->isBuiltinType)
    {
        auto* type = &shader->userTypes[uni->userTypeIndex];
        O("%s %s", type->name, uni->name);
    }
    else
    {
        O("GLint %s", uni->name);
    }

    if (uni->arrayCount > 0)
    {
        A("[%d];\n", uni->arrayCount);
    }
    else
    {
        A(";\n");
    }
}

void OutShader(const Shader* shader)
{
    switch (shader->type)
    {
        CASE(Shader::Vertex, L("struct VertexShader"));
        CASE(Shader::Fragment, L("struct FragmentShader"));
        INVALID_DEFAULT_CASE();
    };
    L("{");
    IDENT_PUSH();
    for (auto& type : shader->userTypes)
    {
        OutUserType(&type, shader);
    }
    L("struct Uniforms");
    L("{");
    IDENT_PUSH();
    for (auto& it : shader->uniforms)
    {
        OutUniform(&it, shader);
    }
    IDENT_POP();
    L("} uniforms;\n");

    switch(shader->type)
    {
    case Shader::Vertex:
    {
        L("struct VertexAttribs");
        L("{");
        IDENT_PUSH();
        for (auto& attrib : shader->vertexAttribs)
        {
            L("GLuint %s = %d;", attrib.name, attrib.location);
        }
        IDENT_POP();
        L("} vertexAttribs;\n");
    } break;
    case Shader::Fragment:
    {
        L("struct Samplers");
        L("{");
        IDENT_PUSH();
        for (auto& it : shader->uniforms)
        {
            if (it.samplerSlot != -1)
            {
                L("Sampler %s = { %d, GL_TEXTURE%d };", it.name, it.samplerSlot, it.samplerSlot);
            }
        }
        IDENT_POP();
        L("} samplers;\n");
    } break;
    INVALID_DEFAULT_CASE();
    };
    IDENT_POP();
    O("} ");
    switch (shader->type)
    {
        CASE(Shader::Vertex, A("vertex;\n\n"));
        CASE(Shader::Fragment, A("fragment;\n\n"));
        INVALID_DEFAULT_CASE();
    };

}

using namespace std::literals::string_literals;
void OutUniformAccessorInternal(Shader* shader, Struct* type, const char* prefix, std::string& buffer)
{
    const char* shaderType;
    switch(shader->type)
    {
        CASE(Shader::Vertex, shaderType = "vertex");
        CASE(Shader::Fragment, shaderType = "fragment");
        INVALID_DEFAULT_CASE();
    }

    for (auto& member : type->members)
    {
        if (member.isBuiltinType)
        {
            if (member.arrayCount == 0)
            {
                O("%s", prefix);
                A("%s.%s = glGetUniformLocation(handle, \"%s.%s\");\n", buffer.c_str(), member.name, buffer.c_str(), member.name);
            }
            else
            {
                for (u32x i = 0; i < member.arrayCount; i++)
                {
                    O("%s", prefix);
                    A("%s.%s[%d] = glGetUniformLocation(handle, \"%s.%s[%d]\");\n", buffer.c_str(), member.name, i, buffer.c_str(), member.name, i);
                }
            }
        }
        else
        {
            if (member.arrayCount == 0)
            {
                std::string str = buffer;
                str += "."s + member.name;
                auto newType = &shader->userTypes[member.userTypeIndex];
                OutUniformAccessorInternal(shader, newType, prefix, str);
            }
            else
            {
                for (u32x i = 0; i < member.arrayCount; i++)
                {
                    std::string str = buffer;
                    str += "."s + member.name + "["s + std::to_string(i) + "]"s;
                    auto newType = &shader->userTypes[member.userTypeIndex];
                    OutUniformAccessorInternal(shader, newType, prefix, str);
                }
            }
        }
    }
}


void OutUniformAccessor(Shader* shader, Uniform* uni)
{
    const char* shaderType;
    switch(shader->type)
    {
        CASE(Shader::Vertex, shaderType = "vertex");
        CASE(Shader::Fragment, shaderType = "fragment");
        INVALID_DEFAULT_CASE();
    }
    if (uni->isBuiltinType)
    {
        if (uni->arrayCount == 0)
        {
            L("result.%s.uniforms.%s = glGetUniformLocation(handle, \"%s\");", shaderType, uni->name, uni->name);
        }
        else
        {
            for (u32x i = 0; i < uni->arrayCount; i++)
            {
                L("result.%s.uniforms.%s[%d] = glGetUniformLocation(handle, \"%s[%d]\");", shaderType, uni->name, i,  uni->name, i);
            }
        }
    }
    else
    {
        std::string buffer;
        std::string prefix;
        auto type = &shader->userTypes[uni->userTypeIndex];
        if (uni->arrayCount == 0)
        {
            prefix = "result."s + shaderType + ".uniforms."s;
            buffer = uni->name;
            OutUniformAccessorInternal(shader, type, prefix.c_str(), buffer);
        }
        else
        {
            prefix = "result."s + shaderType + ".uniforms."s;
            for (u32x i = 0; i < uni->arrayCount; i++)
            {
                buffer = uni->name + "["s + std::to_string(i) + "]"s;
                OutUniformAccessorInternal(shader, type, prefix.c_str(), buffer);
            }
        }
    }
}

int main(int argCount, char** args)
{
    assert(argCount > 1);
    const char* configFileName = args[1];

    std::vector<ShaderDecl> shaderDecls;
    std::vector<ProgramConfig> programDecls;
    ParseConfigFile(configFileName, &shaderDecls, &programDecls);

    std::unordered_map<std::string, Shader*> shaders;

    // TODO: Test for name collisions

    for (auto& it : shaderDecls)
    {
        Shader* shader = new Shader();
        shader->name = it.name;
        switch(it.type)
        {
            CASE(Shader::Vertex, ParseShaderFile(it.path, Shader::Vertex, shader));
            CASE(Shader::Fragment, ParseShaderFile(it.path, Shader::Fragment, shader));
            INVALID_DEFAULT_CASE();
        }
        shaders[std::string(shader->name)] = shader;
    }

    std::vector<Program> programs;
    for (auto& it : programDecls)
    {
        Program prog = {};
        prog.config = it;
        programs.push_back(std::move(prog));
    }

    for (auto& it : programs)
    {
        std::string vertexName = it.config.vertexName;
        std::string fragmentName = it.config.fragmentName;

        auto resultVertex = shaders.find(vertexName);
        if (resultVertex == shaders.end())
        {
            ERROR("Error: Program uses undeclared shader\n");
        }
        it.vertex = resultVertex->second;

        auto resultFragment = shaders.find(fragmentName);
        if (resultFragment == shaders.end())
        {
            ERROR("Error: Program uses undeclared shader\n");
        }
        it.fragment = resultFragment->second;
    }

    // TODO: Deal with CRLF and LF stuff!
    OutFile = fopen("Shaders_Generated.cpp", "wb");
    assert(OutFile);
    IDENT_RESET();
    L("// Thise file was generated by a glsl preprocessor\n");
    L("namespace soko");
    L("{");
    IDENT_PUSH();

    L("struct ShaderInfo");
    L("{");
    IDENT_PUSH();

    for (auto& prog : programs)
    {
        L("struct Info_%s", prog.config.name);
        L("{");
        IDENT_PUSH();
        L("static const char* VertexSource;");
        L("static const char* FragmentSource;");
        L("");
        L("GLuint handle;");
        L("");
        OutShader(prog.vertex);
        OutShader(prog.fragment);
        IDENT_POP();
        L("} %s;\n", prog.config.name);
    }

    IDENT_POP();
    L("};\n");

    for (auto& prog : programs)
    {
        L("ShaderInfo::Info_%s CompileProgram_%s()", prog.config.name, prog.config.name);
        L("{");
        IDENT_PUSH();
        L("ShaderInfo::Info_%s result = {};", prog.config.name);
        L("auto handle = CreateProgram(ShaderInfo::Info_%s::VertexSource, ShaderInfo::Info_%s::FragmentSource);", prog.config.name, prog.config.name);
        L("if (handle)");
        L("{");
        IDENT_PUSH();
        L("result.handle = handle;");
        L("// NOTE: Assign vertex shader uniforms");
        for (auto& it : prog.vertex->uniforms)
        {
            OutUniformAccessor(prog.vertex, &it);
        }
        A("\n");
        L("// NOTE: Assign fragment shader uniforms");
        for (auto& it : prog.fragment->uniforms)
        {
            OutUniformAccessor(prog.fragment, &it);
        }

        A("\n");
        L("//NOTE: Setting samplers");
        L("glUseProgram(handle);");
        for (auto& it : prog.fragment->uniforms)
        {
            if (it.samplerSlot >= 0)
            {
                L("glUniform1i(result.fragment.uniforms.%s, (GLint)result.fragment.samplers.%s.sampler);", it.name, it.name);
            }
        }
        L("glUseProgram(0);");
        IDENT_POP();
        L("}");
        L("return result;");
        IDENT_POP();
        L("}\n");
    }

    L("ShaderInfo LoadShaders()");
    L("{");
    IDENT_PUSH();
    L("ShaderInfo info = {};");
    for (auto& it : programs)
    {
        L("info.%s = CompileProgram_%s();", it.config.name, it.config.name);
        L("if (!info.%s.handle) SOKO_WARN(\"ShaderManager: Failed to load shader program %s\");", it.config.name, it.config.name);
    }
    L("return info;");
    IDENT_POP();
    L("}");
    L("\n");

    L("void UnloadShaders(ShaderInfo* info)");
    L("{");
    IDENT_PUSH();
    L("glFinish();");
    for (auto& it : programs)
    {
        L("if (info->%s.handle)", it.config.name);
        L("{");
        IDENT_PUSH();
        L("glDeleteProgram(info->%s.handle);", it.config.name);
        L("info->%s.handle = 0;", it.config.name);
        IDENT_POP();
        L("}");
    }
    IDENT_POP();
    L("}\n");

    for (auto& prog : programs)
    {
        O("const char* ShaderInfo::Info_%s::VertexSource = R\"(", prog.config.name);
        A("%s", prog.vertex->source);
        A(")\";\n\n");

        O("const char* ShaderInfo::Info_%s::FragmentSource = R\"(", prog.config.name);
        A("%s", prog.fragment->source);
        A(")\";\n\n");
    }
    IDENT_POP();
    L("}");
}
