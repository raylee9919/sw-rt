// Copyright Seong Woo Lee. All Rights Reserved.

Ray::Ray(Vec3 o, Vec3 d)
{
    origin = o;
    direction = d;
}

AABB::AABB(AABB a, AABB b)
{
    min.x = min(a.min.x, b.min.x);
    min.y = min(a.min.y, b.min.y);
    min.z = min(a.min.z, b.min.z);

    max.x = max(a.max.x, b.max.x);
    max.y = max(a.max.y, b.max.y);
    max.z = max(a.max.z, b.max.z);
}

int bvh_cmp(const void *l, const void *r)
{
    // Sort AABBs along the random axis.
    //
    int axis = 0;

    Index_AABB *a = (Index_AABB *)l;
    Index_AABB *b = (Index_AABB *)r;
    if (a->box.min.e[axis] > b->box.min.e[axis]) return  1;
    if (a->box.min.e[axis] < b->box.min.e[axis]) return -1;
    return 0;
}

BVH_Node *create_bvh_recursive(Index_AABB* boxes, int start, int len)
{
    qsort(boxes + start, len, sizeof(boxes[0]), bvh_cmp);

    BVH_Node *node = new BVH_Node;

    if (len == 1) {
        node->left  = NULL;
        node->right = NULL;
        node->index = boxes[start].index;
        node->box   = boxes[start].box;
        return node;
    } 

    int n = len / 2;
    node->left  = create_bvh_recursive(boxes, start, n);
    node->right = create_bvh_recursive(boxes, start + n, len - n);
    node->index = -1;
    node->box   = AABB(node->left->box, node->right->box);
    return node;
}

BVH_Node *create_bvh(AABB *boxes, int num)
{
    Index_AABB *tmp = new Index_AABB[num];
    defer(delete [] tmp);
    for (int i = 0; i < num; ++i) {
        tmp[i].index = i;
        tmp[i].box   = boxes[i];
    }

    BVH_Node* root = create_bvh_recursive(tmp, 0, num);

    return root;
}

bool is_leaf(BVH_Node *node)
{
    return node->index != -1;
}

bool ray_aabb_intersect(Ray& ray, AABB& box)
{
    Vec3 inv_dir(
        ray.direction.x == 0.0f ? std::numeric_limits<float>::infinity() : 1.0f / ray.direction.x,
        ray.direction.y == 0.0f ? std::numeric_limits<float>::infinity() : 1.0f / ray.direction.y,
        ray.direction.z == 0.0f ? std::numeric_limits<float>::infinity() : 1.0f / ray.direction.z
    );

    float tmin = (box.min.x - ray.origin.x) * inv_dir.x;
    float tmax = (box.max.x - ray.origin.x) * inv_dir.x;

    if (tmin > tmax) std::swap(tmin, tmax);

    float tymin = (box.min.y - ray.origin.y) * inv_dir.y;
    float tymax = (box.max.y - ray.origin.y) * inv_dir.y;

    if (tymin > tymax) std::swap(tymin, tymax);

    if (tmin > tymax || tymin > tmax)
        return false;

    tmin = max(tmin, tymin);
    tmax = min(tmax, tymax);

    float tzmin = (box.min.z - ray.origin.z) * inv_dir.z;
    float tzmax = (box.max.z - ray.origin.z) * inv_dir.z;

    if (tzmin > tzmax) std::swap(tzmin, tzmax);

    if (tmin > tzmax || tzmin > tmax)
        return false;

    tmin = max(tmin, tzmin);
    tmax = min(tmax, tzmax);

    // For rays (not line segments), we usually want tmax >= 0
    // Some implementations also require tmin >= 0 (ray starts outside or on surface)
    return tmax >= 0.0f && tmin <= tmax;
}
