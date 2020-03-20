#include "anti_aliasing.h"

AntiAliasing::AntiAliasing(short width, short height) :Refraction(width, height)
{
}

AntiAliasing::~AntiAliasing()
{
}

void AntiAliasing::DrawScene()
{
    camera.SetRenderTargetSize(width*2, height*2);

    #pragma omp parallel for
    for (short x = 0; x < width; x++) {
        #pragma omp parallel for
        for (short y = 0; y < height; y++) {
            auto payload0 = TraceRay(camera.GetCameraRay(2*x + 1, 2*y), raytracing_depth).color;
            auto payload1 = TraceRay(camera.GetCameraRay(2*x, 2*y + 1), raytracing_depth).color;
            auto payload2 = TraceRay(camera.GetCameraRay(2*x + 1, 2*y + 1), raytracing_depth).color;
            auto payload3 = TraceRay(camera.GetCameraRay(2*x, 2*y), raytracing_depth).color;

            SetPixel(x, y, (payload0 + payload1 + payload2 + payload3) / 4);
        }
        std::cout << x << std::endl;
    }
}
