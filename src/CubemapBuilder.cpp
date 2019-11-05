#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_HDR
#include "stb/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

#include "hypermath.h"
#include <assert.h>

const int SIDE_DIM = 512;
const f32 UV_STEP = 1.0f / SIDE_DIM;

struct Pixel
{
    f32 r, g, b;
};

int main(int ac, char** av)
{
    if (ac > 1)
    {
        const char* filename = av[1];
        if (filename)
        {
            int width, height, channels;
            Pixel* equirectImage = (Pixel*)stbi_loadf(filename, &width, &height, &channels, 3);
            assert(channels == 3);
            if (equirectImage)
            {
                Pixel* nx = (Pixel*)malloc(sizeof(Pixel) * SIDE_DIM * SIDE_DIM);

                for (int y = 0; y < SIDE_DIM; y++)
                {
                    for (int z = 0; z < SIDE_DIM; z++)
                    {
                        v3 dir = Normalize(V3(-1.0f, -UV_STEP * y, UV_STEP * z)) - V3(0.5f);
                        f32 r = Sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
                        f32 theta = Acos(dir.z / r);
                        f32 phi = Atan2(dir.y, dir.x);

                        v2 equirectUV = V2(phi / (2.0f * PI_32), theta / PI_32);
                        iv2 pixel = IV2((i32)(equirectUV.x * width), (i32)(equirectUV.y * height));
                        nx[z * SIDE_DIM + y] = equirectImage[pixel.y * width + pixel.x];
                    }
                }

                if(stbi_write_hdr("negX.hdr", SIDE_DIM, SIDE_DIM, channels, (float*)nx))
                {
                }
                else
                {
                    fprintf(stderr, "Failed to write image");
                }
            }
        }
    }
    return 0;
}
