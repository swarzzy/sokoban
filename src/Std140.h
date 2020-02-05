#pragma once
#include "Platform.h"
#include "hypermath.h"

namespace soko::std140
{

#pragma pack(push, 1)
    union vec3
    {
        struct
        {
            float x, y, z, _unused;
        };
        float data[4];

        inline vec3() {}
        inline vec3(v3 v)
        {
            this->x = v.x;
            this->y = v.y;
            this->z = v.z;
        }
    };

    union vec4
    {
        struct
        {
            float x, y, z, w;
        };
        float data[4];

        inline vec4() {}
        inline vec4(v4 v)
        {
            this->x = v.x;
            this->y = v.y;
            this->z = v.z;
            this->w = v.w;
        }
    };

    union mat3
    {
        vec3 columns[3];
        float data[12];
        inline mat3() {}
        inline mat3(m3x3 m)
        {
            this->columns[0] = vec3(m.columns[0]);
            this->columns[1] = vec3(m.columns[1]);
            this->columns[2] = vec3(m.columns[2]);
        }
    };

    union mat4
    {
        vec4 columns[4];
        f32 data[16];
        inline mat4() {}
        inline mat4(m4x4 m)
        {
            memcpy(this->data, m.data, sizeof(float) * 16);
        }
    };

    template<typename T, u32 Size, u32 Stride>
    struct Array
    {
        template <u32 _Stride>
        struct Element
        {
            T value;
            _padby(_Stride - sizeof(T));
            static_assert((_Stride - sizeof(T)) > 0);
        };

        template<>
        struct Element<sizeof(T)>
        {
            T value;
        };

        Element<Stride> data[Size];

        inline T& operator[](uptr index) { return data[index].value; }
        inline const T& operator[](uptr index) const { return data[index].value; }
    };

#pragma pack(pop)
}
