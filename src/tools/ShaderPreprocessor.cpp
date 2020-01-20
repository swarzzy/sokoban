#include <assert.h>
#include <string>
#include <vector>
#include <algorithm>

#include "../OfflineUtils.cpp"


struct Uniform
{
    char* type;
    char* name;
    i32 samplerSlot = -1;

    static const char* SamplerDecls[3];
};

const char* Uniform::SamplerDecls[3] = {
    "sampler2D",
    "samplerCube",
    "sampler2DArray"
};

struct VertexAttrib
{
    enum Type
    {
        Unknown = 0,
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
    "float",
    "int",
    "vec2",
    "vec3",
    "vec4",
    "mat3",
    "mat4",
};


struct Shader
{
    enum Type {Unknown = 0, Vertex, Fragment} type;
    char* name;
    std::vector<Uniform> uniforms;
    std::vector<VertexAttrib> vertexAttribs;
};

constant char* DELIMETERS = " \f\n\r\t\v";
constant char* WORD_PARSE_ERROR_MARKERS = ",;{} \f\n\r\t\v=+-";

char* EatUntilSpace(char* at)
{
    while (*at && !IsSpace(*at)) at++;
    return at;
}

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

struct ShaderDecl
{
    Shader::Type type;
    char* name;
};

ShaderDecl FindTypeDecl(Tokenizer* t)
{
    ShaderDecl decl = {};
    while (*t->at)
    {
        t->at = EatSpace(t->at);
        if (*t->at == '#')
        {
            u32x offset = 0;
            if (MatchStrings(t->at, "#vertex"))
            {
                decl.type = Shader::Vertex;
                offset = sizeof("#vertex") - 1;
            }
            else if (MatchStrings(t->at, "#fragment"))
            {
                decl.type = Shader::Fragment;
                offset = sizeof("#fragment") - 1;
            }

            if (offset)
            {
                t->at += offset;
                t->at = EatSpace(t->at);
                decl.name = t->at;
                t->at = EatUntilSpace(t->at);
                *t->at = 0;
                t->at = Advance1(t->at);
                break;
            }
        }
    }
    return decl;
}

Uniform FindNextUniform(Tokenizer* t)
{
    // TODO: Read comma-separated uniform declarations
    Uniform decl = {};
    while (*t->at)
    {
        t->at = EatSpace(t->at);
        if (MatchStrings(t->at, "uniform"))
        {
            t->at += sizeof("uniform") - 1;
            t->at = EatSpace(t->at);
            decl.type = t->at;
            t->at = EatUntilSpace(t->at);
            *t->at = 0;
            t->at = Advance1(t->at);
            t->at = EatSpace(t->at);
            decl.name = t->at;
            t->at = EatUntilOneOf(t->at, ";,", DELIMETERS);
            *t->at = 0;
            t->at = Advance1(t->at);
            break;
        }
        t->at++;
    }
    return decl;
}

VertexAttrib::Type MatchAttribType(Tokenizer* tok)
{
    auto type = VertexAttrib::Unknown;
    for (u32x i = 0; i < ArrayCount(VertexAttrib::Types); i++)
    {
        if (MatchStrings(tok->at, VertexAttrib::Types[i]))
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
    attrib->name = t->at;
    t->at = EatUntilOneOf(t->at, ";", DELIMETERS);
    *t->at = 0;
    t->at = Advance1(t->at);
}

char* OffsetUnsafe(char* at, u32x count)
{
    at += count;
    return at;
}

#define OffsetByLiteral(at, literal) OffsetUnsafe(at, (sizeof(literal) - 1))

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
        auto foundIn = MatchStrings(t->at, "in") && OneOf(*(t->at + sizeof("in") - 1), DELIMETERS);
        if (foundIn)
        {
            t->at = OffsetByLiteral(t->at, "in");
            ParseVertexAttribTypeAndName(t, &decl);
            break;
        }
        if (MatchStrings(t->at, "layout"))
        {
            bool succesfullyParseLocation = false;
            t->at = OffsetByLiteral(t->at, "layout");
            t->at = EatSpace(t->at);
            if (*t->at == '(')
            {
                t->at = OffsetByLiteral(t->at, "(");
                t->at = EatSpace(t->at);
                if (MatchStrings(t->at, "location"))
                {
                    t->at = OffsetByLiteral(t->at, "location");
                    t->at = EatSpace(t->at);
                    if (MatchStrings(t->at, "="))
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
                if (MatchStrings(t->at, "in"))
                {
                    t->at = OffsetByLiteral(t->at, "in");
                    ParseVertexAttribTypeAndName(t, &decl);
                }
            }
            break;
        }
        t->at++;
    }
    return decl;
}

int main(int argCount, char** args)
{
    //assert(argCount > 1);
    const char* filename = "../src/TestShader.glsl";

    u32 size;
    auto shaderSorce = ReadEntireFileAsText(filename, &size);

    Tokenizer uniformTokenizer = {};
    uniformTokenizer.text = shaderSorce;
    uniformTokenizer.at = shaderSorce;

    auto attribsTokenizer = uniformTokenizer.Clone();

    Shader shader = {};

    auto decl = FindTypeDecl(&uniformTokenizer);

    if (decl.type == Shader::Unknown)
    {
        printf("Error: Unknown shader type\n");
        return -1;
    }
    if (!StrLength(decl.name))
    {
        printf("Error: Found a shader without a name\n");
        return -1;
    }

    shader.type = decl.type;
    shader.name = decl.name;

    while (true)
    {
        auto uniform = FindNextUniform(&uniformTokenizer);
        if (!*uniformTokenizer.at) break;

        if (!StrLength(uniform.type))
        {
            printf("Error: a uniform of unknown type\n");
            return -1;
        }
        if (!StrLength(uniform.name))
        {
            printf("Error: Found a uniform without a name\n");
            return -1;
        }
        if (ContainsOneOf(uniform.name, WORD_PARSE_ERROR_MARKERS))
        {
            printf("Error: Failed to parse uniform name\n");
            return -1;
        }
        if (ContainsOneOf(uniform.type, WORD_PARSE_ERROR_MARKERS))
        {
            printf("Error: Failed to parse uniform type\n");
            return -1;
        }

        shader.uniforms.push_back(uniform);
    }

    // TODO: Check that it less than max GL_TEXTUREX
    u32x samplerCount = 0;

    foreach (shader.uniforms)
    {
        for (u32x i = 0; i < ArrayCount(Uniform::SamplerDecls); i++)
        {
            if (MatchStrings(it.type, Uniform::SamplerDecls[i]))
            {
                it.samplerSlot = samplerCount;
                samplerCount++;
            }
        }
    }

    if (shader.type == Shader::Vertex)
    {
        while (true)
        {
            auto attrib = FindNextAttrib(&attribsTokenizer);
            if (!*attribsTokenizer.at) break;

            if (attrib.type == VertexAttrib::Type::Unknown)
            {
                printf("Error: Found a vertex attribute of unknown type\n");
                return -1;
            }
            if (!StrLength(attrib.name))
            {
                printf("Error: Found a vertex attribute without a name\n");
                return -1;
            }
            if (ContainsOneOf(attrib.name, WORD_PARSE_ERROR_MARKERS))
            {
                printf("Error: Failed to parse a vertex attribute name\n");
                return -1;
            }

            shader.vertexAttribs.push_back(attrib);
        }
    }

    int asa = 0;


#if 0

    auto shaderSource = std::string(_shaderSrc);

    Shader shader = {};


    const char* typeDirectives[] = {
        "#vertex",
        "#fragment"
    };

    const Shader::Type typeIDs[] = {
        Shader::Vertex,
        Shader::Fragment
    };

    StrPos at;

    for (u32 i = 0; i < ArrayCount(typeDirectives); i++)
    {
        auto index = shaderSource.find(typeDirectives[i]);
        if (index != std::string::npos)
        {
            at = i;
            shader.type = typeIDs[i];
            break;
        }
    }

    assert(shader.type);

    shader.name = GetNextWord(shaderSource, &at);

    StrPos index = at;
    do
    {
        index = shaderSource.find("uniform", index);
        if (index == std::string::npos) break;

        index = NextWord(shaderSource, index);

        auto type = ParseType(shaderSource, &index);
        if (type == Type::Unknown)
        {
            printf("Error: Found uniform of unknown type.\n");
            continue;
        }

        index = NextWord(shaderSource, index);

        while (true)
        {
            Uniform uniform;

            index = EatSpace(shaderSource, index);

            auto nameEnd = shaderSource.find_first_of(",;", index);
            auto nameLen = nameEnd - index;
            if (index == std::string::npos || !nameLen)
            {
                PANIC("Error: Unexpected end of file.");
            }
            auto nameStr = shaderSource.substr(index, nameLen);

            while (isspace((unsigned char)(nameStr[nameStr.length() - 1]))) nameStr.pop_back();

            uniform.type = type;
            uniform.name =  nameStr;

            shader.uniforms.push_back(std::move(uniform));

            index = nameEnd;
            if (nameEnd == std::string::npos) break;
            if (shaderSource[nameEnd] == ';') break;
            nameEnd++;
            if (nameEnd >= shaderSource.length())
            {
                index = std::string::npos;
                break;
            }
            index = nameEnd;
        }
        if (index == std::string::npos) break;
    } while (true);
#endif
}
