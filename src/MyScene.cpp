//
// Created by Niyaz on 2/22/20.
//

#include "MyScene.h"

int main() {
    MyScene *render = new MyScene(1920, 1080);
    int result = render->LoadGeometry("");
    if (result) {
        return result;
    }

    render->SetCamera(float3{-0.15f, -2.3f, -3.f}, float3{0.f, .3f, 1}, float3{0, .5f, -.4f});
    render->AddLight(new Light(float3{0, 0, -100.f}, float3{1.f, 1.f, 1.f}));
    render->Clear();
    render->DrawScene();
    result = render->Save("results/my_scene.png");
    return result;
}


int MyScene::LoadGeometry(std::string filename) {

    auto s1 = new MaterialSphere{
            float3{-1.5f, 0, 5}, 1
    };
    s1->SetEmisive(float3{0.3, 0.3, 0});
    s1->SetDiffuse(float3{0.3, 0.3, 0});

    auto s2 = new MaterialSphere{
            float3{1.5f, 0, 5}, 1
    };
    s2->SetReflectiveness(true);

    auto s3 = new MaterialSphere{
            float3{13.f, 0, 5}, 8
    };
    s3->SetReflectiveness(true);

    material_objects.push_back(s1);
    material_objects.push_back(s2);
    material_objects.push_back(s3);


    for (int a = 0; a < 360; a += 45) {
        float angle = M_PI * a / 180.f;

        float3 c{0, 0, 5};
        float r = 3;

        auto s = new MaterialSphere{
                float3{std::cos(angle) * r + c.x, c.y - r, std::sin(angle) * r + c.z}, 1
        };
        if (a % 90 == 0) {
            s->SetReflectiveness(true);
        } else {
            s->SetEmisive(float3{0, 0.3, 0.3});
            s->SetDiffuse(float3{0, 0.3, 0.3});
        }

        material_objects.push_back(s);

        s = new MaterialSphere{
                float3{std::cos(angle) * r + c.x, c.y + r, std::sin(angle) * r + c.z}, 1
        };
        if (a % 90 == 0) {
            s->SetEmisive(float3{0, 0.3, 0.3});
            s->SetDiffuse(float3{0, 0.3, 0.3});
        } else {
            s->SetReflectiveness(true);
        }

        material_objects.push_back(s);
    }

    return 0;
}


IntersectableData MaterialSphere::Intersect(const Ray &ray) const {
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

    if (min > 0) {
        auto t = min;
        auto norm = linalg::normalize(ray.position + ray.direction * t - center);
        return IntersectableData{t, norm};
    }

    // Put the normal as the coordinates...
    auto t = std::max(x0, x1);
    auto norm = linalg::normalize(ray.position + ray.direction * t - center);
    return IntersectableData{t, norm};
}

float3 MaterialSphere::GetNormal(float3 barycentric) const {
    return barycentric;
}

