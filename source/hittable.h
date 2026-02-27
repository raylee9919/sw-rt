// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once


enum Hittable_Kind {
    HITTABLE_INVALID = 0,
    HITTABLE_SPHERE,
    HITTABLE_QUAD,
    HITTABLE_TRIANGLE,
    HITTABLE_BVH,
};

struct Hittable_Sphere {
    Vec3 center;
    f32 radius;
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
    Hittable_BVH *left;
    Hittable_BVH *right;
};

struct Hittable {
    Hittable_Kind kind;
    Material material;

    union {
        Hittable_Sphere     sphere;
        Hittable_Quad       quad;
        Hittable_Tri        tri;
        Hittable_BVH        bvh_node;
    };
};
