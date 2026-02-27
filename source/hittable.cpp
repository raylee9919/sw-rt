// Copyright Seong Woo Lee. All Rights Reserved.


Hittable_Quad::Hittable_Quad(Vec3 origin_, Vec3 u_, Vec3 v_)
{
    origin = origin_;
    u = u_;
    v = v_;
    Vec3 n = cross(u, v);
    normal = normalize(n);
    d = dot(normal, origin);
    w = n / dot(n, n);
}

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
