// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#define PI 3.1415926535897932385f
#define RECIPROCAL_PI 0.31830988618f

union Vec2 {
    struct{ f32 x, y; };
    f32 e[2];

    explicit Vec2() = default;
    explicit Vec2(f32 x_, f32 y_);
};

union Vec3 {
    struct{
        union {
            Vec2 xy;
            struct { f32 x, y; };
        };
        f32 z; 
    };
    f32 e[3];

    explicit Vec3() = default;
    explicit Vec3(f32 x_, f32 y_, f32 z_);
    explicit Vec3(f32 f);
};

union Vec4 {
    struct { f32 r, g, b, a; };
    struct {
        union {
            Vec3 xyz;
            struct {
                union {
                    Vec2 xy;
                    struct { f32 x, y; };
                };
                f32 z;
            };
        };
        f32 w;
    };
    f32 e[4];
    __m128 sse;

    explicit Vec4() = default;
    explicit Vec4(Vec3 xyz, f32 w_);
    explicit Vec4(f32 r_, f32 g_, f32 b_, f32 a_);
    explicit Vec4(f32 f);
};

union Mat4 {
    struct {
        f32 _11, _12, _13, _14;
        f32 _21, _22, _23, _24;
        f32 _31, _32, _33, _34;
        f32 _41, _42, _43, _44;
    };
    f32 e[4][4];
    Vec4 rows[4];
};



f32 lerp(f32 l, f32 r, f32 t);
f32 rsqrt(f32 f);

Vec2 operator + (Vec2 l, Vec2 r);
Vec2 operator - (Vec2 l, Vec2 r);
Vec2 operator * (Vec2 v, f32 f);
Vec2 operator * (f32 f, Vec2 v);
Vec2 operator * (Vec2 l, Vec2 r);
f32 dot(Vec2 l, Vec2 r);

Vec3 operator + (Vec3 l, Vec3 r);
Vec3& operator += (Vec3& l, Vec3 r);
Vec3 operator - (Vec3 l, Vec3 r);
Vec3& operator - (Vec3& v, f32 f);
Vec3 operator * (Vec3 v, f32 f);
Vec3 operator * (f32 f, Vec3 v);
Vec3 operator * (Vec3 l, Vec3 r);
Vec3 operator / (Vec3 l, f32 r);
f32 dot(Vec3 l, Vec3 r);
Vec3 cross(Vec3 l, Vec3 r);
Vec3 normalize(Vec3 v);
Vec3 lerp(Vec3 l, Vec3 r, f32 t);
Vec3 reflect(Vec3 in, Vec3 normal);


Vec4 operator + (Vec4 l, Vec4 r);
Vec4 operator - (Vec4 l, Vec4 r);
Vec4 operator * (Vec4 v, f32 f);
Vec4 operator * (f32 f, Vec4 v);
Vec4 operator * (Vec4 l, Vec4 r);
Vec4& operator *= (Vec4& v, f32 f);
f32 dot(Vec4 l, Vec4 r);
Vec4 lerp(Vec4 l, Vec4 r, f32 t);


Mat4 operator * (Mat4 l, Mat4 r);
Vec4 operator * (Mat4 l, Vec4 r);
Mat4 identity();
Mat4 look_at(Vec3 eye, Vec3 focus, Vec3 up);
Mat4 perspective(f32 fov, f32 aspect_ratio, f32 near_z, f32 far_z);
Mat4 x_rotation(f32 a);
Mat4 y_rotation(f32 a);
Mat4 z_rotation(f32 a);
Vec3 to_barycentric(Vec2 p, Vec2 a, Vec2 b, Vec2 c);
