#include "refraction.h"

#include <cmath>

Refraction::Refraction(short width, short height) :Reflection(width, height)
{
    this->raytracing_depth = 15;
}

Refraction::~Refraction()
{
}

Payload Refraction::Hit(const Ray &ray, const IntersectableData &data, const MaterialTriangle *triangle,
                        const unsigned int max_raytrace_depth) const {
    if (max_raytrace_depth <= 0)
        return Miss(ray);

    if (triangle == nullptr)
        return Miss(ray);

    float3 color = triangle->emissive_color;

    float3 X = ray.position + ray.direction * data.t;
    auto norm = triangle->GetNormal(data.baricentric);

    if (triangle->reflectiveness) {
        auto refl_dir = ray.direction - 2.f * linalg::dot(norm, ray.direction) * norm;
        Ray reflection{X, refl_dir}; // todo add a little offset to X: X + refl_dir * 0.0001

        return TraceRay(reflection, max_raytrace_depth - 1);
    }

    if (triangle->reflectiveness_and_transparency) {
        float kr;
        float cosi = std::max(-1.f, std::min(1.f, dot(ray.direction, norm)));
        float etai = 1.f;
        float etat = triangle->ior;
        if (cosi > 0) {
            std::swap(etai, etat);
        }
        float sint = etai / etat * sqrtf(std::max(0.f, 1 - cosi * cosi));
        if (sint >= 1) {
            kr = 1;
        } else {
            float cost = sqrtf(std::max(0.f, 1 - sint * sint));
            float cosi_abs = std::fabs(cosi);
            float Rs = ((etat * cosi_abs) - (etai * cost)) / ((etat * cosi_abs) + (etai * cost));
            float Rp = ((etai * cosi_abs) - (etat * cost)) / ((etai * cosi_abs) + (etat * cost));
            kr = (Rs * Rs + Rp * Rp) / 2;
        }

        bool outside = dot(ray.direction, norm) < 0;
        auto bias = 0.001f * norm;
        float3 refraction_color;

        if (kr < 1) {
            float cosi_abs = std::fabs(cosi);
            float eta = etai / etat;
            float k = 1 - eta * eta * (1 - cosi_abs * cosi_abs);
            float3 refr_dir{0, 0, 0};
            if (k > 0) {
                refr_dir = eta * ray.direction + (eta * cosi_abs - sqrtf(k)) * norm;
            }
            Ray refraction_ray{outside ? X - bias : X + bias, refr_dir};

            refraction_color = TraceRay(refraction_ray, max_raytrace_depth - 1).color;
        }

        auto refl_dir = ray.direction - 2.f * linalg::dot(norm, ray.direction) * norm;
        Ray reflection{outside ? X + bias : X - bias, refl_dir};
        auto reflection_color =  TraceRay(reflection, max_raytrace_depth - 1).color;

        return Payload(reflection_color * kr + refraction_color * (1 - kr));
    }

    for (auto const &light: lights) {
        Ray to_light = Ray{X, light->position - X};

        auto to_light_dist = linalg::length(light->position - X);
        auto shadow_ray_t = TraceShadowRay(to_light, to_light_dist);

        if (std::abs(shadow_ray_t - to_light_dist) > t_min)
            continue;

        color += light->color * triangle->diffuse_color * std::max(0.f, linalg::dot(norm, to_light.direction));

        auto refl_dir = 2.f * linalg::dot(norm, to_light.direction) * norm - to_light.direction;
        color += light->color * triangle->specular_color *
                 std::pow(std::max(0.f, linalg::dot(ray.direction, refl_dir)), triangle->specular_exponent);
    }

//    return Payload(linalg::abs(data.baricentric));
    return Payload(color);
}
