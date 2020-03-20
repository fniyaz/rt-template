#include "shadow_rays.h"

ShadowRays::ShadowRays(short width, short height): Lighting(width, height)
{
}

ShadowRays::~ShadowRays()
{
}

Payload ShadowRays::TraceRay(const Ray& ray, const unsigned int max_raytrace_depth) const
{
    if (max_raytrace_depth <= 0)
        return Miss(ray);

    float close_t = t_max;
    IntersectableData closest_data{t_max};
    MaterialTriangle *closest {nullptr};

    for (auto const &obj: material_objects) {
        auto data = obj->Intersect(ray);
        if (data.t <= close_t && data.t >= t_min) {
            close_t = data.t;
            closest_data = data;
            closest = obj;
        }
    }

    if (closest_data.t != t_max) {
        return Hit(ray, closest_data, closest, max_raytrace_depth - 1);
    }

    return Miss(ray);
}


Payload ShadowRays::Hit(const Ray& ray, const IntersectableData& data, const MaterialTriangle* triangle, const unsigned int max_raytrace_depth) const
{
    if (max_raytrace_depth <= 0)
        return Miss(ray);

    if (triangle == nullptr)
        return Miss(ray);

    float3 color = triangle->emissive_color;

    float3 X = ray.position + ray.direction * data.t;
    auto norm = triangle->GetNormal(data.baricentric);

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

float ShadowRays::TraceShadowRay(const Ray& ray, const float max_t) const
{
    for (auto const &obj: material_objects) {
        auto data = obj->Intersect(ray);
        if (data.t <= max_t && data.t >= t_min) {
            return data.t;
        }
    }

    return max_t;
}

