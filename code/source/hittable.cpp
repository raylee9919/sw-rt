// Copyright Seong Woo Lee. All Rights Reserved.


Hittable_Tri::Hittable_Tri(Vec3 origin_, Vec3 u_, Vec3 v_)
{
    origin = origin_;
    u = u_;
    v = v_;
    Vec3 n = cross(u, v);
    normal = normalize(n);
    d = dot(normal, origin);
    w = n / dot(n, n);
}

bool rt_hit_tri(Hittable& hittable, Ray& ray, f32 t_min, f32 t_max, Hit_Record* rec_out)
{
    auto& tri = hittable.tri;

    f32 denom = dot(tri.normal, ray.direction);
    if (fabs(denom) < 1e-8) {
        return false;
    }

    f32 t = (tri.d - dot(tri.normal, ray.origin)) / denom;
    if (t < t_min || t > t_max) {
        return false;
    }

    Vec3 hit_pos = ray.origin + ray.direction * t;

    Vec3 p = hit_pos - tri.origin;
    f32 alpha = dot(tri.w, cross(p, tri.v));
    f32 beta  = dot(tri.w, cross(tri.u, p));

    if (!(alpha >= 0.f && alpha <= 1.f && beta >= 0.f && beta <= 1.f && (alpha + beta <= 1.f))) {
        return false;
    }

    // Tex coords
    Vec2 st1 = tri.st[0];
    Vec2 st2 = tri.st[1];
    Vec2 st3 = tri.st[2];
    Vec2 st = st1 * (1.f - alpha - beta) + st2 * alpha + st3 * beta;

    // Tangents
    Vec4 t1 = tri.tangents[0];
    Vec4 t2 = tri.tangents[1];
    Vec4 t3 = tri.tangents[2];
    Vec4 tangent = t1 * (1.f - alpha - beta) + t2 * alpha + t3 * beta;

    // Normal
    // @Todo: Move this to shader.
    Vec3 n1 = tri.normals[0];
    Vec3 n2 = tri.normals[1];
    Vec3 n3 = tri.normals[2];
    Vec3 vertex_normal  = normalize(n1 * (1.f - alpha - beta) + n2 * alpha + n3 * beta);
    // @Temporary
    Vec3 sampled_normal = vertex_normal;
    if (hittable.pbr.normal) {
        sampled_normal = sample_rgb(hittable.pbr.normal, st) * 2.f - Vec3(1.f);
    }
    // @Temporary
    //if (dot(normal, ray.direction) > 0.f) {
    //    normal = -normal;
    //}

    // Write
    rec_out->t              = t;
    rec_out->position       = hit_pos;
    rec_out->vertex_normal  = vertex_normal;
    rec_out->sampled_normal = sampled_normal;
    rec_out->uv             = st;
    rec_out->pbr            = hittable.pbr;
    rec_out->tangent        = tangent;

    return true;
}

bool rt_hit_bvh(Hittable& hittable, Ray& ray, f32 t_min, f32 t_max, Hit_Record *rec_out)
{
    auto& bvh = hittable.bvh;
    Hittable *primitives = bvh.primitives;

    bool hit = false;

    std::stack <BVH_Node *> stk;
    stk.push(bvh.root);

    f32 min_t = f32_max;

    while (!stk.empty()) {
        auto *node = stk.top();
        stk.pop();

        if (!ray_aabb_intersect(ray, node->box)) {
            continue;
        }

        if (is_leaf(node)) {
            // @Temporary: Closest hit
            //
            Hit_Record rec;
            bool did_hit = rt_hit(primitives[node->index], ray, t_min, t_max, &rec);
            if (did_hit && (rec.t < min_t)) {
                min_t = rec.t;
                *rec_out = rec;
                hit = true;
            }
        } else {
            stk.push(node->left);
            if (node->left != node->right) {
                stk.push(node->right);
            }
        }
    }

    return hit;
}

bool rt_hit(Hittable& hittable, Ray& ray, f32 t_min, f32 t_max, Hit_Record* rec_out) 
{
    switch (hittable.kind) {
        case HITTABLE_TRIANGLE: return rt_hit_tri(hittable, ray, t_min, t_max, rec_out);
        case HITTABLE_BVH:      return rt_hit_bvh(hittable, ray, t_min, t_max, rec_out);

        default: {
            assert(!"Invalid default case.");
            return false;
        } break;
    }
}
