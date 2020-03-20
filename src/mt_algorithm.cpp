#include "mt_algorithm.h"


Sphere::Sphere(float3 center, float radius) :
	center(center), radius(radius)
{
}

Sphere::~Sphere()
{
}

IntersectableData Sphere::Intersect(const Ray& ray) const
{
    auto cp = center - ray.position;
    float a = linalg::dot(ray.direction, ray.direction);
    float b = -2 * linalg::dot(ray.direction, cp);
    float c = linalg::dot(cp, cp) - radius * radius;

    float dis = b * b - 4 * a * c;
    if (dis < 0)
        return -1;
    float x0 = (-b - sqrtf(dis)) / 2.f / a;
    float x1 = (-b + sqrtf(dis)) / 2.f / a;

    auto min = std::min(x0, x1);

    if (min > 0)
        return min;
    return std::max(x0, x1);
}



MTAlgorithm::MTAlgorithm(short width, short height) : RayGenerationApp(width, height)
{
}

MTAlgorithm::~MTAlgorithm()
{
}

int MTAlgorithm::LoadGeometry(std::string filename) {
    objects.push_back(new Sphere{float3{2, 0, -1}, 0.4f});
//    objects.push_back(new Sphere{float3{1, 0, 0}, 0.4f});
    objects.push_back(new Triangle{
            float3{-.5, -.5, -1},
            float3{.5, -.5, -1},
            float3{0, .5, -1}
    });
    return 0;
}

Payload MTAlgorithm::TraceRay(const Ray& ray, const unsigned int max_raytrace_depth) const {
    float close_t = t_max;
    IntersectableData closest_obj{t_max};

    for (auto const &obj: objects) {
        auto data = obj->Intersect(ray);
        if (data.t <= close_t && data.t >= t_min) {
            close_t = data.t;
            closest_obj = data;

        }
    }

    if (closest_obj.t != t_max) {
        return Hit(ray, closest_obj);
    }

    return Miss(ray);
}

Payload MTAlgorithm::Hit(const Ray& ray, const IntersectableData& data) const
{
    return Payload(data.baricentric);
}

Triangle::Triangle(Vertex a, Vertex b, Vertex c) :
	a(a), b(b), c(c) {
    ba = b.position - a.position;
    ca = c.position - a.position;
}

Triangle::Triangle() :
        Triangle(float3{0, 0, 0}, float3{0, 0, 0}, float3{0, 0, 0})
{
}

Triangle::~Triangle()
{
}

IntersectableData Triangle::Intersect(const Ray& ray) const {
    auto prev = linalg::cross(ray.direction, ca);
    float det = linalg::dot(ba, prev);

    if (std::abs(det) < 1e-8)
        return IntersectableData(-1);

    auto tvec = ray.position - a.position;
    float u = linalg::dot(tvec, prev) / det;

    if (u < 0 || u > 1)
        return IntersectableData(-1);

    auto qvec = linalg::cross(tvec, ba);
    float v = dot(ray.direction, qvec) / det;

    if (v < 0 || u + v > 1)
        return IntersectableData(-1);

    auto t = linalg::dot(ca, qvec) / det;


    return IntersectableData(t, float3{1 - u - v, u, v});
}
