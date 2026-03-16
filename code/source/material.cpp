// Copyright Seong Woo Lee. All Rights Reserved.

u32 pack_rgba(Vec4 RGBA) 
{
    u32 R = (u32)(min(max(RGBA.r, 0.f), 1.f)*255.f + 0.5f);
    u32 G = (u32)(min(max(RGBA.g, 0.f), 1.f)*255.f + 0.5f);
    u32 B = (u32)(min(max(RGBA.b, 0.f), 1.f)*255.f + 0.5f);
    u32 A = (u32)(min(max(RGBA.a, 0.f), 1.f)*255.f + 0.5f);
    u32 RGBA8 = (R | (G<<8) | (B<<16) | (A<<24));
    return RGBA8;
}

Vec4 unpack_rgba(u32 RGBA8)
{
    f32 R = (f32)((RGBA8 & 0xff)) / 255.f;
    f32 G = (f32)((RGBA8 & 0xff00) >> 8) / 255.f;
    f32 B = (f32)((RGBA8 & 0xff0000) >> 16) / 255.f;
    f32 A = (f32)((RGBA8 & 0xff000000) >> 24) / 255.f;
    Vec4 RGBA = Vec4(R, G, B, A);
    return RGBA;
}

Texture *create_flat_color_texture(u32 width, u32 height, Vec3 color)
{
    Texture *tex = new Texture;

    tex->layout = TEXTURE_LAYOUT_RGB;
    tex->width  = width;
    tex->height = height;
    tex->pitch  = 3 * width;
    tex->data   = new u8[tex->pitch * height]; 

    for (u32 r = 0; r < height; ++r) {
        for (u32 c = 0; c < width; ++c) {
            u8 R = (u8)(color.x * 255.f + 0.5f);
            u8 G = (u8)(color.y * 255.f + 0.5f);
            u8 B = (u8)(color.z * 255.f + 0.5f);
            u8 *ptr = (u8 *)tex->data + r*tex->pitch + c*3;
            ptr[0] = R;
            ptr[1] = G;
            ptr[2] = B;
        }
    }

    return tex;
}

Texture *create_checker_texture(u32 width, u32 height)
{
    Texture *tex = new Texture;

    tex->layout = TEXTURE_LAYOUT_RGB;
    tex->width  = width;
    tex->height = height;
    tex->pitch  = 3 * width;
    tex->data   = new u8[tex->pitch * height]; 

    for (u32 r = 0; r < height; ++r) {
        for (u32 c = 0; c < width; ++c) {
            u8 color = 0xff;
            if ((r/128 + c/128) % 2 == 0) {
                color = 0x00;
            }

            u8 *ptr = (u8 *)tex->data + r*tex->pitch + c*3;
            ptr[0] = color;
            ptr[1] = color;
            ptr[2] = color;
        }
    }

    return tex;
}

Texture *create_texture_from_file(const char *file_path, Texture_Layout layout)
{
    int x = 0, y = 0, num_channels = 0;
    u8 *data = NULL;

    switch (layout) 
    {
        case TEXTURE_LAYOUT_RGB: {
            data = stbi_load(file_path, &x, &y, &num_channels, 0);
            assert(num_channels == 3);
        } break;

        default: {
            assert(!"Invalid default case.");
        } break;
    }

    Texture *tex = new Texture;
    {
        memset(tex, 0, sizeof(*tex));
        tex->layout = layout;
        tex->width  = x;
        tex->height = y;
        tex->pitch  = num_channels * x;
        tex->data   = data; 
    }

    return tex;
}

Vec3 to_rgb(u32 texel)
{
    f32 R = ((f32)(texel & 0xff) / 255.f);
    f32 G = ((f32)((texel & 0xff00) >> 8) / 255.f);
    f32 B = ((f32)((texel & 0xff0000) >> 16) / 255.f);
    Vec3 result = Vec3(R,G,B);
    return result;
}

Vec3 sample_rgb(Texture *tex, Vec2 uv)
{
    assert(tex->layout == TEXTURE_LAYOUT_RGB);
    int num_channels = 3;

    int w = (int)tex->width;
    int h = (int)tex->height;

    assert(w > 1 && h > 1);

    f32 ds = 1.f / (f32)(w - 1);
    f32 dt = 1.f / (f32)(h - 1);

    f32 s = uv.x - floorf(uv.x);
    f32 t = uv.y - floorf(uv.y);

    f32 fs = fmod(s, ds);
    f32 ft = fmod(t, dt);

    f32 s_min = s - fs;
    f32 t_min = t - ft;

    int r1 = max((int)(t_min * (f32)(h - 1)), 0);
    int c1 = max((int)(s_min * (f32)(w - 1)), 0);
    int r2 = min(r1 + 1, h - 1);
    int c2 = min(c1 + 1, w - 1);

    // 1 2
    // 3 4
    // @Robustness: Reading 4-bytes from this 3-channel texture.
    u32 tex1 = *(u32 *)((u8 *)tex->data + r1*tex->pitch + c1*num_channels);
    u32 tex2 = *(u32 *)((u8 *)tex->data + r1*tex->pitch + c2*num_channels);
    u32 tex3 = *(u32 *)((u8 *)tex->data + r2*tex->pitch + c1*num_channels);
    u32 tex4 = *(u32 *)((u8 *)tex->data + r2*tex->pitch + c2*num_channels);

    Vec3 smp1 = to_rgb(tex1);
    Vec3 smp2 = to_rgb(tex2);
    Vec3 smp3 = to_rgb(tex3);
    Vec3 smp4 = to_rgb(tex4);

    f32 tx = fs / ds;
    f32 ty = ft / dt;
    Vec3 result = lerp(lerp(smp1, smp2, tx), lerp(smp3, smp4, tx), ty);
    return result;
}

f32 sample_r(Texture *tex, Vec2 uv)
{
    assert(tex->layout == TEXTURE_LAYOUT_R);

    u32 w = tex->width;
    u32 h = tex->height;

    f32 s = uv.x - floorf(uv.x);
    f32 t = uv.y - floorf(uv.y);

    u32 r = (u32)(t * (f32)(h - 1) + 0.5f);
    u32 c = (u32)(s * (f32)(w - 1) + 0.5f);

    int num_channels = 1;

    u8 texel = *((u8 *)tex->data + r*tex->pitch + c*num_channels);
    f32 result = (f32)texel / 255.f;
    return result;
}
