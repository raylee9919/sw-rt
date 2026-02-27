// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

// Texture
//
struct Texture {
    u8 *data;
    u32 width;
    u32 height;
    u32 pitch;
};

// Material
//
enum Material_Kind {
    MATERIAL_INVALID = 0,
    MATERIAL_LAMBERTIAN, // f(l,v) = Cdiff
    MATERIAL_METALLIC,
    MATERIAL_EMISSION,
};

struct Material_Lambertian {
    Texture *albedo;
    Vec3 tint;
};

struct Material_Metallic {
    Vec3 color;
    f32 fuzz;
};

struct Material_Emission {
    Vec3 color;
};

struct Material {
    Material_Kind kind;
    union {
        Material_Lambertian lambertian;
        Material_Metallic   metallic;
        Material_Emission   emission;
    };
};

