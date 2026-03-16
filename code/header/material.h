// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

enum Texture_Layout {
    TEXTURE_LAYOUT_INVALID  = 0,
    TEXTURE_LAYOUT_RGBA,
    TEXTURE_LAYOUT_RGB,
    TEXTURE_LAYOUT_R,
};

struct Texture {
    Texture_Layout layout;
    u8 *data;
    u32 width;
    u32 height;
    u32 pitch;
};

struct PBR_Mat {
    Texture *emission;
    Texture *albedo;
    Texture *normal;
    Texture *rm; // @Temporary
};

Vec3 sample_rgb(Texture *tex, Vec2 uv);

u32 pack_rgba(Vec4 RGBA); 
Vec4 unpack_rgba(u32 RGBA8);
Texture *create_flat_color_texture(u32 width, u32 height, Vec3 color);
Texture *create_texture_from_file(const char *file_path);
