#include <algorithm>
#include "bvh.h"


BVH::BVH(short width, short height) :AABB(width, height)
{
}

BVH::~BVH()
{
}

bool cmp(const Mesh& a, const Mesh& b)
{
    return a.aabb_max.y < b.aabb_max.y;
}

void BVH::BuildBVH()
{
    std::sort(meshes.begin(), meshes.end(), cmp);
    auto middle = meshes.begin();
    std::advance(middle, 2);

    TLAS bot;
    for(auto i = meshes.begin(); i != middle; i++) {
        bot.AddMesh(*i);
    }
    TLAS top;
    for (auto i = middle; i != meshes.end(); i++) {
        bot.AddMesh(*i);
    }
    tlases.push_back(top);
    tlases.push_back(bot);
}

Payload BVH::TraceRay(const Ray& ray, const unsigned int max_raytrace_depth) const
{
    if (max_raytrace_depth <= 0)
        return Miss(ray);

    float close_t = t_max;
    IntersectableData closest_data{t_max};
    MaterialTriangle const *closest {nullptr};

    for (auto const &tlas: tlases) {
        if (!tlas.AABBTest(ray))
            continue;

        for (auto const &mesh: tlas.GetMeshes()) {
            if (!mesh.AABBTest(ray))
                continue;

            for (auto &obj: mesh.Triangles()) {
                auto data = obj.Intersect(ray);
                if (data.t <= close_t && data.t >= t_min) {
                    close_t = data.t;
                    closest_data = data;
                    closest = &obj;
                }
            }
        }
    }

    if (closest_data.t != t_max) {
        return Hit(ray, closest_data, closest, max_raytrace_depth - 1);
    }

    return Miss(ray);
}

float BVH::TraceShadowRay(const Ray& ray, const float max_t) const
{
    for (auto const &tlas: tlases) {
        if (!tlas.AABBTest(ray))
            continue;

        for (auto const &mesh: tlas.GetMeshes()) {
            if (!mesh.AABBTest(ray))
                continue;

            for (auto &obj: mesh.Triangles()) {
                auto data = obj.Intersect(ray);
                if (data.t <= max_t && data.t >= t_min) {
                    return data.t;
                }
            }
        }
    }

    return max_t;
}

bool TLAS::AABBTest(const Ray& ray) const
{
    float3 invRaydir = float3(1.0) / ray.direction;
    float3 t0 = (aabb_max - ray.position) * invRaydir;
    float3 t1 = (aabb_min - ray.position) * invRaydir;
    float3 tmin = min(t0, t1);
    float3 tmax = max(t0, t1);
    return maxelem(tmin) <= minelem(tmax);
}

void TLAS::AddMesh(const Mesh mesh)
{
    if (meshes.empty()) {
        aabb_min = mesh.aabb_min;
        aabb_max = mesh.aabb_max;
    } else {
        aabb_max = linalg::max (aabb_max, mesh.aabb_max);
        aabb_min = linalg::min(aabb_min, mesh.aabb_min);
    }

    meshes.push_back(mesh);
}
