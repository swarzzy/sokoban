// NOTE:
// Maybe someday I should get rid of this crappy parser
// and use opengl functions for getting info about shaders and uniforms
// (glGetActiveAttrib, glGetActiveUniform) ???
#include <assert.h>
#include <vector>

#include <stdlib.h>

#include "../OfflineUtils.cpp"

#undef ERROR
#define ERROR(...) (printf(__VA_ARGS__), exit(EXIT_FAILURE))

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
    sampler2DArray
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

    static const char* SamplerDecls[3];
};

const char* Uniform::SamplerDecls[3] = {
    "sampler2D",
    "samplerCube",
    "sampler2DArray"
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
    "sampler2DArray"
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
    char* vertexPath;
    char* fragmentPath;
};

struct Shader
{
    enum Type {Unknown = 0, Vertex, Fragment} type;
    const char* filename;
    std::vector<Uniform> uniforms;
    std::vector<VertexAttrib> vertexAttribs;
    std::vector<Struct> userTypes;
};

struct Program
{
    ProgramConfig config;
    Shader vertex;
    Shader fragment;
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
                    t->at = EatUntilOneOf(t->at, "\"");
                    t->at = OffsetByLiteral(t->at, "\"");
                    auto pathBegin = t->at;
                    t->at = EatUntilOneOf(t->at, "\"");
                    result.vertexPath = ExtractString(pathBegin, t->at);
                    t->at = Advance1(t->at);
                }
                else if (Match(t->at, "fragment"))
                {
                    t->at = OffsetByLiteral(t->at, "fragment");
                    t->at = EatUntilOneOf(t->at, ":");
                    t->at = EatUntilOneOf(t->at, "\"");
                    t->at = OffsetByLiteral(t->at, "\"");
                    auto pathBegin = t->at;
                    t->at = EatUntilOneOf(t->at, "\"");
                    result.fragmentPath = ExtractString(pathBegin, t->at);
                    t->at = Advance1(t->at);
                }
                else if (Match(t->at, "#program"))
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

void ParseConfigFile(const char* filename, std::vector<Program>* programs)
{
    u32 size;
    auto source = ReadEntireFileAsText(filename, &size);

    Tokenizer tokenzier = {};
    tokenzier.text = source;
    tokenzier.at = source;

    while (true)
    {
        auto conf = GetNextProgConfig(&tokenzier);

        if (!conf.name || !StrLength(conf.name))
        {
            ERROR("Error: Found program declaration without a name\n");
        }

        if (!conf.vertexPath || !StrLength(conf.vertexPath))
        {
            ERROR("Error: Found program declaration with no vertex path specified\n");
        }

        if (!conf.fragmentPath || !StrLength(conf.fragmentPath))
        {
            ERROR("Error: Found program declaration with no fragment path specified\n");
        }

        for (auto& it : *programs)
        {
            if (strcmp(it.config.name, conf.name) == 0)
            {
                ERROR("Error: Found two of more program declarations with the same name: %s\n", it.config.name);
            }
        }

        programs->emplace_back();
        programs->back().config = conf;

        if (!*tokenzier.at) break;
    }
    //free(source);
}

void ParseShaderFile(const char* filename, Shader::Type type, Shader* shader)
{
    u32 size;
    auto source = ReadEntireFileAsText(filename, &size);

    Tokenizer tokenizer = {};
    tokenizer.text = source;
    tokenizer.at = source;

    assert(type != Shader::Unknown);

    shader->type = type;
    shader->filename = filename;

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

int main(int argCount, char** args)
{
    //assert(argCount > 1);
    const char* configFileName = "../src/ShaderConfig.txt";

    std::vector<Program> programs;
    ParseConfigFile(configFileName, &programs);

    // TODO: Shader cache

    for (auto& it : programs)
    {
        ParseShaderFile(it.config.vertexPath, Shader::Vertex, &it.vertex);
        ParseShaderFile(it.config.fragmentPath, Shader::Fragment, &it.fragment);
    }

#if 0
    FILE* outFile = fopen("Shaders_Generated.cpp", "w");
    assert(outFile);
#undef OUT
#define OUTLN(...) do {for (i32x i = 0; i < identLevel; i++) fprintf(outFile, "\t"); fprintf(outFile, __VA_ARGS__);} while(false)
#define OUT(...) fprintf(outFile, __VA_ARGS__)
#define IDENT_PUSH() identLevel++
#define IDENT_POP() identLevel--
    i32x identLevel = 0;

    OUTLN("// This file was generated by a glsl preprocessor\n\n");
    OUTLN("namespace soko\n{\n");
    IDENT_PUSH();
    OUTLN("struct %s\n", shader.name);
    OUTLN("{\n");
    IDENT_PUSH();
    for (auto& type : shader.userTypes)
    {
        OUTLN("struct %s\n", type.name);
        OUTLN("{\n");
        IDENT_PUSH();
        for (auto& member : type.members)
        {
            if (member.isBuiltinType)
            {
                OUTLN("GLint %s", member.name);
                if (member.arrayCount > 0)
                {
                    OUT("[%d]", member.arrayCount);
                }
            }
            else
            {
                auto& type = shader.userTypes[member.userTypeIndex];
                OUTLN("%s %s", type.name, member.name);
                if (member.arrayCount > 0)
                {
                    OUT("[%d]", member.arrayCount);
                }
            }
            OUT(";\n");
        }
        IDENT_POP();
        OUTLN("};\n\n");
    }
    OUTLN("struct Uniforms\n");
    OUTLN("{\n");
    IDENT_PUSH();
    for (auto& it : shader.uniforms)
    {
        if (!it.isBuiltinType)
        {
            auto& type = shader.userTypes[it.userTypeIndex];
            OUTLN("%s %s", type.name, it.name);
        }
        else
        {
            OUTLN("GLint %s", it.name);
        }

        if (it.arrayCount > 0)
        {
            OUT("[%d];\n", it.arrayCount);
        }
        else
        {
            OUT(";\n");
        }
    }
    IDENT_POP();
    OUTLN("} uniforms;\n");
    if (shader.type == Shader::Vertex)
    {
        OUT("\n");
        OUTLN("enum class VertexAttribs : GLuint\n");
        OUTLN("{\n");
        IDENT_PUSH();
        for (auto& attrib : shader.vertexAttribs)
        {
            OUTLN("%s = %d,\n", attrib.name, attrib.location);
        }
        IDENT_POP();
        OUTLN("};\n");
    }
    IDENT_POP();
    OUTLN("};\n");
    OUT("\n");
    OUTLN("%s CompileShader_%s()\n", shader.name, shader.name);
    IDENT_POP();
    OUTLN("}");
#endif
}
