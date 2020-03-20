#include "lighting.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <algorithm>

Lighting::Lighting(short width, short height) : MTAlgorithm(width, height)
{
}

Lighting::~Lighting()
{
}

int Lighting::LoadGeometry(std::string filename)
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

            auto materialTriangle = new MaterialTriangle(vert[0], vert[1], vert[2]);

            auto mat_ind = shapes[s].mesh.material_ids[f];
            auto mat = materials[mat_ind];
            materialTriangle->SetEmisive(float3{mat.emission});
            materialTriangle->SetAmbient(float3{mat.ambient});
            materialTriangle->SetDiffuse(float3{mat.diffuse});
            materialTriangle->SetSpecular(float3{mat.specular}, mat.shininess);
            materialTriangle->SetReflectiveness(mat.illum == 5);
            materialTriangle->SetReflectivenessAndTransparency(mat.illum == 7);
            materialTriangle->SetIor(mat.ior);


            material_objects.push_back(materialTriangle);
        }
    }


    return 0;
}

void Lighting::AddLight(Light* light)
{
    lights.push_back(light);
}

Payload Lighting::TraceRay(const Ray& ray, const unsigned int max_raytrace_depth) const {
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

    if (closest_data.t < t_max) {
        return Hit(ray, closest_data, closest);
    }

    return Miss(ray);
}


Payload Lighting::Hit(const Ray& ray, const IntersectableData& data, const MaterialTriangle* triangle) const
{
    if (triangle == nullptr)
        return Miss(ray);

    float3 color = triangle->emissive_color;

    float3 X = ray.position + ray.direction * data.t;
    auto norm = triangle->GetNormal(data.baricentric);

    //return Payload(norm);

    for (auto const &light: lights) {
        Ray to_light = Ray(X, light->position - X);

        color += light->color * triangle->diffuse_color * std::max(0.f, linalg::dot(norm, to_light.direction));

        auto refl_dir = 2.f * linalg::dot(norm, to_light.direction) * norm - to_light.direction;
        color += light->color * triangle->specular_color * std::pow(std::max(0.f, linalg::dot(ray.direction, refl_dir)), triangle->specular_exponent);
    }

    return Payload(color);
}

float3 MaterialTriangle::GetNormal(float3 barycentric) const {
    if (linalg::length(a.normal) > 0 && linalg::length(b.normal) > 0 && linalg::length(c.normal) > 0)
        return
                linalg::normalize(a.normal * barycentric.x +
                b.normal * barycentric.y +
                c.normal * barycentric.z);

    return geo_normal;
}
