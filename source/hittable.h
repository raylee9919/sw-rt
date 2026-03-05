// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

struct Hittable;

enum Hittable_Kind {
    HITTABLE_INVALID = 0,
    HITTABLE_QUAD,
    HITTABLE_TRIANGLE,
    HITTABLE_BVH,
};

struct Hittable_Quad {
    Vec3 origin;
    Vec3 u, v;

    Vec3 normal;
    f32 d;
    Vec3 w;

    explicit Hittable_Quad() = default;
    explicit Hittable_Quad(Vec3 origin_, Vec3 u_, Vec3 v_);
};

struct Hittable_Tri {
    Vec3 origin;
    Vec3 u, v;

    Vec3 normal;
    f32 d;
    Vec3 w;

    explicit Hittable_Tri() = default;
    explicit Hittable_Tri(Vec3 origin_, Vec3 u_, Vec3 v_);
};

struct Hittable_BVH {
    BVH_Node *root;
    Hittable *primitives;
    int num;
};

struct Hittable {
    Hittable_Kind kind;
    Material material;

    union {
        Hittable_Quad  quad;
        Hittable_Tri   tri;
        Hittable_BVH   bvh;
    };
};

struct Hit_Record {
    f32 t;
    Vec3 position;
    Vec3 normal;
    Vec2 uv;
    Material material;
};

bool rt_hit(Hittable& hittable, Ray& ray, f32 t_min, f32 t_max, Hit_Record* rec_out);
