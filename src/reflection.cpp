#include "reflection.h"

Reflection::Reflection(short width, short height) :ShadowRays(width, height)
{
}

Reflection::~Reflection()
{
}

Payload Reflection::Hit(const Ray& ray, const IntersectableData& data, const MaterialTriangle* triangle, const unsigned int raytrace_depth) const
{
    if (raytrace_depth <= 0)
        return Miss(ray);

    if (triangle == nullptr)
        return Miss(ray);

    float3 color = triangle->emissive_color;

    float3 X = ray.position + ray.direction * data.t;
    auto norm = triangle->GetNormal(data.baricentric);

    if (triangle->reflectiveness) {
        auto refl_dir = ray.direction - 2.f * linalg::dot(norm, ray.direction) * norm;
        Ray reflection{X, refl_dir};

        return TraceRay(reflection, raytrace_depth-1);
    }

    for (auto const &light: lights) {
        Ray to_light = Ray{X, light->position - X};

        auto to_light_dist = linalg::length(light->position - X);
        auto shadow_ray_t = TraceShadowRay(to_light, to_light_dist);

        if (std::abs(shadow_ray_t - to_light_dist) > t_min)
            continue;

        color += light->color * triangle->diffuse_color * std::max(0.f, linalg::dot(norm, to_light.direction));

        auto refl_dir = 2.f * linalg::dot(norm, to_light.direction) * norm - to_light.direction;
        color += light->color * triangle->specular_color * std::pow(std::max(0.f, linalg::dot(ray.direction, refl_dir)), triangle->specular_exponent);
    }

    return Payload(color);
}
