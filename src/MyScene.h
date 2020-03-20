//
// Created by Niyaz on 2/22/20.
//

#ifndef RAYTRACING_MYSCENE_H
#define RAYTRACING_MYSCENE_H


#include "anti_aliasing.h"

class MyScene : public AntiAliasing {
public:
    MyScene(short width, short height) : AntiAliasing(width, height) {
        raytracing_depth = 10;
    };

    ~MyScene() override = default;

    Payload Miss(Ray const &r) const override {

        if (r.direction.y*0 >= 0) {
            float yr = (1 - std::abs(r.direction.y));
            float xr = 0;

            if (r.direction.x < 0)
                xr = (1 - std::abs(r.direction.z));
            return Payload{float3{1.f * xr * yr, 0.3f * xr * yr, 0}};
        }


        auto pos = r.position + r.direction * 1000;

        if (((int(pos.x) % 1000 + 1000) % 1000 - 500)
//            * ((int(pos.y) % 1000 + 1000) % 1000 - 500)
            * ((int(pos.z) % 1000 + 1000) % 1000 - 500) > 0)
        {
            return Payload{{0, 0.5f, 0}};
        }

        return Payload{{0, 0, 0.5f}};
    }

    int LoadGeometry(std::string filename) override;
};


// God forgive me for that hierarchy
class MaterialSphere : public MaterialTriangle {

public:
    MaterialSphere(float3 center, float radius) : center(center), radius(radius) {};

    IntersectableData Intersect(const Ray &ray) const override;

    float3 GetNormal(float3 barycentric) const override;

private:
    float3 center;
    float radius;
};


#endif //RAYTRACING_MYSCENE_H
