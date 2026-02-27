// Copyright Seong Woo Lee. All Rights Reserved.

AABB union(AABB l, AABB r)
{
    f32 min_x = min(l.min.x, r.min.x);
    f32 min_y = min(l.min.x, r.min.x);
    f32 min_z = min(l.min.x, r.min.x);

    f32 max_x = max(l.max.x, r.max.x);
    f32 max_y = max(l.max.x, r.max.x);
    f32 max_z = max(l.max.x, r.max.x);
}

f32 area(AABB box)
{
    Vec3 d = box.max - box.min;
    return 2.f * (d.x*d.y + d.y*d.z + d.z*d.x);
}

BVH* bvh_from_triangle_mesh(Triangle_Mesh* mesh)
{
    BVH* bvh = new bvh;
    memset(bvh, 0, sizeof(*bvh));

    assert(mesh_num % 3 == 0);
    u32 num_tri = mesh->num / 3;

    // Build bounding boxes from each triangle.
    //
    AABB* boxes = new AABB[num_tri];
    memset(boxes, 0, sizeof(AABB)*num_tri);

    for (u32 v = 0; v < mesh->num; v += 3) {
        Vec3 vert1 = mesh->vertices[v];
        Vec3 vert2 = mesh->vertices[v + 1];
        Vec3 vert3 = mesh->vertices[v + 2];

        f32 eps = 0.001f;
        f32 min_x = min(min(vert1.x, vert2.x), vert3.x) - eps;
        f32 min_y = min(min(vert1.y, vert2.y), vert3.y) - eps;
        f32 min_z = min(min(vert1.z, vert2.z), vert3.z) - eps;
        f32 max_x = max(max(vert1.x, vert2.x), vert3.x) + eps;
        f32 max_y = max(max(vert1.y, vert2.y), vert3.y) + eps;
        f32 max_z = max(max(vert1.z, vert2.z), vert3.z) + eps;

        AABB box;
        box.min = Vec3(min_x, min_y, min_z);
        box.max = Vec3(max_x, max_y, max_z);

        boxes[v] = box;
    }


    // Alloc max amount of nodes (2 * N - 1).
    //
    u32 max_nodes = 2*num_tri - 1;
    bvh->num_nodes = max_nodes;
    bvh->nodes = new BVH_Node[max_nodes];
    memset(bvh->nodes, 0, sizeof(BVH_Node)*max_nodes);
}
