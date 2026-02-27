// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

// There are two kinds of nodes in BVH - leaf node and internal node.
// Order of children doesn't matter in BVH.

struct AABB {
    Vec3 min;
    Vec3 max;
};

struct Triangle_Mesh {
    u32 num;
    Vec3 *vertices;
    Vec3 *normals;
};

struct BVH_Node {
    BVH_Node *left;
    BVH_Node *right;
};

struct BVH {
    BVH_Node* nodes;
    u32 num_nodes;
};

AABB union(AABB l, AABB r);
f32 area(AABB box);
