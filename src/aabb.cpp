#include "aabb.h"

//#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

AABB::AABB(short width, short height) :AntiAliasing(width, height)
{
}

AABB::~AABB()
{
}

int AABB::LoadGeometry(std::string filename)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn;
    std::string err;

    size_t delimeter = filename.find_last_of('/');
    auto dir = filename.substr(0, delimeter);

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str(), dir.c_str());
    if (!warn.empty()) {
        std::cout << warn << std::endl;
    }

    if (!err.empty()) {
        std::cerr << err << std::endl;
    }

    if (!ret) {
        exit(1);
    }

// Loop over shapes
    for (size_t s = 0; s < shapes.size(); s++) {
        // Loop over faces(polygon)

        Mesh mesh;
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            int fv = shapes[s].mesh.num_face_vertices[f];

            // Loop over vertices in the face.
            std::vector<Vertex> vert;
            for (size_t v = 0; v < fv; v++) {
                // access to vertex
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
                tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
                tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];

                if (idx.normal_index > 0) {
                    tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
                    tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
                    tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];
                    vert.emplace_back(float3{vx, vy, vz}, float3{nx, ny, nz});
                } else {
                    vert.emplace_back(float3{vx, vy, vz});
                }
            }
            index_offset += fv;

            MaterialTriangle materialTriangle(vert[0], vert[1], vert[2]);

            auto mat_ind = shapes[s].mesh.material_ids[f];
            auto mat = materials[mat_ind];
            materialTriangle.SetEmisive(float3{mat.emission});
            materialTriangle.SetAmbient(float3{mat.ambient});
            materialTriangle.SetDiffuse(float3{mat.diffuse});
            materialTriangle.SetSpecular(float3{mat.specular}, mat.shininess);
            materialTriangle.SetReflectiveness(mat.illum == 5);
            materialTriangle.SetReflectivenessAndTransparency(mat.illum == 7);
            materialTriangle.SetIor(mat.ior);


            mesh.AddTriangle(materialTriangle);
        }
        meshes.push_back(mesh);
    }


    return 0;
}

Payload AABB::TraceRay(const Ray& ray, const unsigned int max_raytrace_depth) const
{
    if (max_raytrace_depth <= 0)
        return Miss(ray);

    float close_t = t_max;
    IntersectableData closest_data{t_max};
    MaterialTriangle const *closest {nullptr};

    for (auto const &mesh: meshes) {
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

    if (closest_data.t != t_max) {
        return Hit(ray, closest_data, closest, max_raytrace_depth - 1);
    }

    return Miss(ray);
}

float AABB::TraceShadowRay(const Ray& ray, const float max_t) const
{
    for (auto const &mesh: meshes) {
        if (!mesh.AABBTest(ray))
            continue;

        for (auto &obj: mesh.Triangles()) {
            auto data = obj.Intersect(ray);
            if (data.t <= max_t && data.t >= t_min) {
                return data.t;
            }
        }
    }

    return max_t;
}

void Mesh::AddTriangle(const MaterialTriangle triangle)
{
    if (triangles.empty()) {
        aabb_min = triangle.a.position;
        aabb_min = triangle.a.position;
    }

    aabb_max = linalg::max(aabb_max, triangle.a.position);
    aabb_max = linalg::max(aabb_max, triangle.b.position);
    aabb_max = linalg::max(aabb_max, triangle.c.position);

    aabb_min = linalg::min(aabb_min, triangle.a.position);
    aabb_min = linalg::min(aabb_min, triangle.b.position);
    aabb_min = linalg::min(aabb_min, triangle.c.position);

    triangles.push_back(triangle);
}

bool Mesh::AABBTest(const Ray& ray) const
{
    float3 invRaydir = float3(1.0) / ray.direction;
    float3 t0 = (aabb_max - ray.position) * invRaydir;
    float3 t1 = (aabb_min - ray.position) * invRaydir;
    float3 tmin = min(t0, t1);
    float3 tmax = max(t0, t1);
    return maxelem(tmin) <= minelem(tmax);
}
