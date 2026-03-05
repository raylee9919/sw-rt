// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

struct Ray {
    Vec3 origin;
    Vec3 direction;

    explicit Ray() = default;
    explicit Ray(Vec3 o, Vec3 d);
};

struct AABB {
    Vec3 min;
    Vec3 max;

    explicit AABB() = default;
    explicit AABB(AABB a, AABB b);
};

struct Index_AABB {
    u32 index;
    AABB box;
};

struct BVH_Node {
    BVH_Node* left;
    BVH_Node* right;
    int index;
    AABB box;
};
