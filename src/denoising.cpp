#include "denoising.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <time.h>
#include <omp.h>
#include <random>

Denoising::Denoising(short width, short height) : AABB(width, height)
{
    raytracing_depth = 16;
}

Denoising::~Denoising()
{
}

void Denoising::Clear() {
    AABB::Clear();
    history_buffer.resize(height * width);
}

Payload Denoising::Hit(const Ray& ray, const IntersectableData& data, const MaterialTriangle* triangle, const unsigned int max_raytrace_depth) const {
    if (max_raytrace_depth <= 0)
        return Miss(ray);

    if (triangle == nullptr)
        return Miss(ray);

    float3 color = triangle->emissive_color;

    if (color > float3{0, 0, 0}) {
        return Payload(color);
    }


    float3 X = ray.position + ray.direction * data.t;
    auto norm = triangle->GetNormal(data.baricentric);

    if (triangle->reflectiveness) {
        auto refl_dir = ray.direction - 2.f * linalg::dot(norm, ray.direction) * norm;
        Ray reflection{X, refl_dir}; // todo add a little offset to X: X + refl_dir * 0.0001

        return TraceRay(reflection, max_raytrace_depth - 1);
    }

    const int num_secondary_rays = 1;
    float3 lights_color;

    for (int i = 0; i < num_secondary_rays; i++) {
        float3 r_direction = blue_noise[GetRandom(omp_get_thread_num() + clock())];

        if (dot(norm, r_direction) <= 0)
            r_direction *= -1;

        Ray to_light = Ray{X, r_direction};

        Payload light = TraceRay(to_light, max_raytrace_depth - 1);

        lights_color += light.color * triangle->diffuse_color * std::max(0.f, linalg::dot(norm, to_light.direction));

        auto refl_dir = 2.f * linalg::dot(norm, to_light.direction) * norm - to_light.direction;
        lights_color += light.color * triangle->specular_color * std::pow(std::max(0.f, linalg::dot(ray.direction, refl_dir)), triangle->specular_exponent);
    }

    return Payload(color + lights_color / num_secondary_rays);
}

void Denoising::SetHistory(unsigned short x, unsigned short y, float3 color)
{
    history_buffer[y * width + x] = color;
}

float3 Denoising::GetHistory(unsigned short x, unsigned short y) const
{
    return history_buffer[y * width + x];
}


Payload Denoising::Miss(const Ray& ray) const
{
    return Payload{};
}

int Denoising::GetRandom(const int thread_num) const {
    static std::default_random_engine generator(thread_num);
    static std::uniform_int_distribution<int> distribution(0, 512 * 512);
    return distribution(generator);
}

void Denoising::DrawScene(int max_frame_number) {
    for (int i = 0; i < max_frame_number; i++) {
        std::cout << "Rendering " << i + 1 << " of " << max_frame_number << std::endl;
#pragma omp parallel for
        for (short x = 0; x < width; x++) {
#pragma omp parallel for
            for (short y = 0; y < height; y++) {
                auto payload = TraceRay(camera.GetCameraRay(x, y), raytracing_depth);
                SetHistory(x, y, payload.color + GetHistory(x, y));
            }
        }

        if (i%500 == 0) {
            for (int j = 0; j < frame_buffer.size(); j++)
                frame_buffer[j] = byte3{255*history_buffer[j] / (i+1)};
            Save("results/prom"+std::to_string(i)+".png");
        }
    }

    for (int i = 0; i < frame_buffer.size(); i++)
        frame_buffer[i] = byte3{255*history_buffer[i] / max_frame_number};
}

void Denoising::LoadBlueNoise(std::string file_name) {
    int width, height, channels;
    unsigned char *img = stbi_load(file_name.c_str(), &width, &height, &channels, 0);

    // Convert the reference to vector of colors
    for (int i = 0; i < width * height; i++) {
        float3 pixel{img[channels * i] / 128.f - 1, img[channels * i + 1] / 128.f - 1,
                     img[channels * i + 2] / 128.f - 1};
        blue_noise.push_back(pixel);
    }
}
