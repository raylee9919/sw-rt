// Copyright Seong Woo Lee. All Rights Reserved.

#include <math.h>

//
// Scalar
//
f32 lerp(f32 l, f32 r, f32 t) {
    return l + (r - l) * t;
}

f32 sqrt(f32 f) {
    // Don't do '_mm_store_ss' to get out float. Use '_mm_cvtss_f32'.
    return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(f)));
}

// @Study:
//         1. 16.17 FSQRT SQRTSS (https://www.agner.org/optimize/optimizing_assembly.pdf)
//         2. Newtonian Iteration (https://stackoverflow.com/questions/14752399/newton-raphson-with-sse2-can-someone-explain-me-these-3-lines)
f32 rsqrt(f32 f) {
    return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(f)));
}

//
// Vec2
//
Vec2::Vec2(f32 x_, f32 y_) {
    x = x_;
    y = y_;
}

Vec2 operator + (Vec2 l, Vec2 r) {
    return Vec2(l.x + r.x, l.y + r.y);
}

Vec2 operator - (Vec2 l, Vec2 r) {
    return Vec2(l.x - r.x, l.y - r.y);
}

Vec2 operator * (Vec2 v, f32 f) {
    v.x *= f;
    v.y *= f;
    return v;
}

Vec2 operator * (f32 f, Vec2 v) {
    return v * f;
}

Vec2 operator * (Vec2 l, Vec2 r) {
    return Vec2(l.x * r.x, l.y * r.y);
}

f32 dot(Vec2 l, Vec2 r) {
    return l.x * r.x + l.y * r.y;
}

//
// Vec3
//
Vec3::Vec3(f32 x_, f32 y_, f32 z_) {
    x = x_;
    y = y_;
    z = z_;
}
Vec3::Vec3(f32 f) {
    x = f;
    y = f;
    z = f;
}

Vec3 operator + (Vec3 l, Vec3 r) {
    return Vec3(l.x + r.x, l.y + r.y, l.z + r.z);
}

Vec3 operator - (Vec3 l, Vec3 r) {
    return Vec3(l.x - r.x, l.y - r.y, l.z - r.z);
}

Vec3 operator - (Vec3 v) {
    v.x *= -1.f;
    v.y *= -1.f;
    v.z *= -1.f;
    return v;
}

Vec3 operator * (Vec3 v, f32 f) {
    v.x *= f;
    v.y *= f;
    v.z *= f;
    return v;
}

Vec3 operator * (f32 f, Vec3 v) {
    return v * f;
}

Vec3 operator * (Vec3 l, Vec3 r) {
    return Vec3(l.x * r.x, l.y * r.y, l.z * r.z);
}

Vec3 operator / (Vec3 l, f32 r) {
    f32 rr = 1.f / r;
    return Vec3(l.x * rr, l.y * rr, l.z * rr);
}

f32 dot(Vec3 l, Vec3 r) {
    return l.x * r.x + l.y * r.y + l.z * r.z;
}

Vec3 cross(Vec3 l, Vec3 r) {
    Vec3 v;
    v.x = l.y * r.z - l.z * r.y;
    v.y = l.z * r.x - l.x * r.z;
    v.z = l.x * r.y - l.y * r.x;
    return v;
}

Vec3 normalize(Vec3 v) {
    f32 d = rsqrt(v.x*v.x + v.y*v.y + v.z*v.z);
    v.x *= d;
    v.y *= d;
    v.z *= d;
    return v;
}

Vec3 lerp(Vec3 l, Vec3 r, f32 t) {
    Vec3 v;
    v.x = lerp(l.x, r.x, t);
    v.y = lerp(l.y, r.y, t);
    v.z = lerp(l.z, r.z, t);
    return v;
}

Vec3 reflect(Vec3 in, Vec3 normal) 
{
    return in - 2.f*dot(in, normal)*normal;
}


//
// Vec4
//
Vec4::Vec4(Vec3 xyz, f32 w_) {
    sse = _mm_setr_ps(xyz.x, xyz.y, xyz.z, w_);
}

Vec4::Vec4(f32 r_, f32 g_, f32 b_, f32 a_) {
    sse = _mm_setr_ps(r_, g_, b_, a_);
}

Vec4::Vec4(f32 f) {
    sse = _mm_set1_ps(f);
}

Vec4 operator + (Vec4 l, Vec4 r) {
    Vec4 v;
    v.sse = _mm_add_ps(l.sse, r.sse);
    return v;
}

Vec4 operator - (Vec4 l, Vec4 r) {
    Vec4 v;
    v.sse = _mm_sub_ps(l.sse, r.sse);
    return v;
}

Vec4 operator * (Vec4 v, f32 f) {
    __m128 f_ = _mm_set1_ps(f);
    v.sse = _mm_mul_ps(v.sse, f_);
    return v;
}

Vec4 operator * (f32 f, Vec4 v) {
    return v * f;
}

Vec4 operator * (Vec4 l, Vec4 r) {
    Vec4 v;
    v.sse = _mm_mul_ps(l.sse, r.sse);
    return v;
}

Vec4& operator += (Vec4& l, Vec4 r) {
    l.sse = _mm_add_ps(l.sse, r.sse);
    return l;
}

Vec4& operator *= (Vec4& v, f32 f) {
    v = v * f;
    return v;
}

f32 dot(Vec4 l, Vec4 r) {
    // @Todo: SIMD
    return l.x * r.x + l.y * r.y + l.z * r.z + l.w * r.w;
}

Vec4 lerp(Vec4 l, Vec4 r, f32 t) {
    Vec4 v;
    __m128 d_ = _mm_sub_ps(r.sse, l.sse);
    __m128 t_ = _mm_set1_ps(t);
    v.sse = _mm_add_ps(l.sse, _mm_mul_ps(d_, t_));
    return v;
}


//
// Mat4
//
Mat4 operator * (Mat4 l, Mat4 r) {
    // @Todo: SIMD
    Mat4 m = {};
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            for (int k = 0; k < 4; ++k) {
                m.e[i][j] += l.e[i][k] * r.e[k][j];
            }
        }
    }
    return m;
}

Vec4 operator * (Mat4 l, Vec4 r) {
    // @Todo: SIMD
    Vec4 v;
    v.x = l._11 * r.x + l._12 * r.y + l._13 * r.z + l._14 * r.w;
    v.y = l._21 * r.x + l._22 * r.y + l._23 * r.z + l._24 * r.w;
    v.z = l._31 * r.x + l._32 * r.y + l._33 * r.z + l._34 * r.w;
    v.w = l._41 * r.x + l._42 * r.y + l._43 * r.z + l._44 * r.w;
    return v;
}

Mat4 identity() {
    Mat4 m;
    m.rows[0] = Vec4(1, 0, 0, 0);
    m.rows[1] = Vec4(0, 1, 0, 0);
    m.rows[2] = Vec4(0, 0, 1, 0);
    m.rows[3] = Vec4(0, 0, 0, 1);
    return m;
}

Mat4 look_at_lh(Vec3 eye, Vec3 focus, Vec3 up) 
{
    Vec3 z = normalize(focus - eye);
    Vec3 x = normalize(cross(z, up));
    Vec3 y = cross(x, z);

    Mat4 m;
    m._11 = x.x;
    m._12 = x.y;
    m._13 = x.z;
    m._14 = -dot(eye, x);

    m._21 = y.x;
    m._22 = y.y;
    m._23 = y.z;
    m._24 = -dot(eye, y);

    m._31 = z.x;
    m._32 = z.y;
    m._33 = z.z;
    m._34 = -dot(eye, z);
    
    m._41 = 0.f;
    m._42 = 0.f;
    m._43 = 0.f;
    m._44 = 1.f;
    return m;
}

// @Todo: Correct?

Mat4 perspective(f32 fov, f32 aspect_ratio, f32 near_z, f32 far_z) 
{
    Mat4 m;
    f32 a = aspect_ratio;
    f32 f = 1.f / tanf(fov * 0.5f);
    f32 fmn = 1.f / (far_z - near_z);
    m.rows[0] = Vec4(f, 0.f, 0.f, 0.f);
    m.rows[1] = Vec4(0.f, a*f, 0.f, 0.f);
    m.rows[2] = Vec4(0.f, 0.f, far_z * fmn, -(far_z * near_z) * fmn);
    m.rows[3] = Vec4(0.f, 0.f, 1.f, 0.);
    return m;
}

Mat4 x_rotation(f32 a) {
    f32 c = cosf(a);
    f32 s = sinf(a);
    Mat4 r = {
        1,  0,  0,  0,
        0,  c, -s,  0,
        0,  s,  c,  0,
        0,  0,  0,  1
    };
    return r;
}

Mat4 y_rotation(f32 a) {
    f32 c = cosf(a);
    f32 s = sinf(a);
    Mat4 r = {
        c,  0,  s,  0,
        0,  1,  0,  0,
        -s,  0,  c,  0,
        0,  0,  0,  1
    };
    return r;
}

Mat4 z_rotation(f32 a) {
    f32 c = cosf(a);
    f32 s = sinf(a);
    Mat4 r = {
        c, -s,  0,  0,
        s,  c,  0,  0,
        0,  0,  1,  0,
        0,  0,  0,  1
    };
    return r;
}

Vec3 to_barycentric(Vec2 p, Vec2 a, Vec2 b, Vec2 c) {
    Vec2 v1 = b - a;
    Vec2 v2 = c - a; 
    Vec2 v3 = p - a;
    f32 d00 = dot(v1, v1);
    f32 d01 = dot(v1, v2);
    f32 d11 = dot(v2, v2);
    f32 d20 = dot(v3, v1);
    f32 d21 = dot(v3, v2);
    f32 denom = d00 * d11 - d01 * d01;
    f32 v = (d11 * d20 - d01 * d21) / denom;
    f32 w = (d00 * d21 - d01 * d20) / denom;
    f32 u = 1.0f - v - w;
    return Vec3(u, v, w);
}
