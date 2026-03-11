// Copyright Seong Woo Lee. All Rights Reserved.

// @Todo: 
// 1. Tangent Space
// 2. Sampling
//
#include <vector>
#include <stack>

// .h
//
#include "base.h"
#include "win32.h"
#include "memory.h"
#include "simd.h"
#include "string.h"
#include "math.h"
#include "random.h"
#include "material.h"
#include "geometry.h"
#include "asset.h"
#include "hittable.h"
#include "entity.h"
#include "obj.h"
#include "vendor/stb_image.h"

//.cpp
//
#include "win32.cpp"
#include "memory.cpp"
#include "simd.cpp"
#include "string.cpp"
#include "math.cpp"
#include "random.cpp"
#include "material.cpp"
#include "geometry.cpp"
#include "hittable.cpp"

#include "vendor/mikktspace.c"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ASSERT(x) 
#include "vendor/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBIW_ASSERT(x)
#include "vendor/stb_image_write.h"


struct Camera {
    Vec3 position;
    Vec3 dir;
    f32 focal_length;
    f32 aspect_ratio;
    f32 width;
    f32 height;
};

struct Framebuffer {
    u8 *data;
    int width;
    int height;
    int pitch;
};

struct State {
    Arena           *arena;

    int             num_samples;
    int             max_bounces;

    Framebuffer     framebuffer;
    Camera          camera;

    std::vector <Hittable> world;
};

static State *state;


// Ray
//
Vec3 cast_ray(std::vector<Hittable>& world, Ray& ray, f32 t_min, f32 t_max, int max_bounces)
{
    if (max_bounces <= 0) {
        return Vec3(0.f);
    }

    Hit_Record rec = {};
    rec.t = f32_max;
    for (auto& obj : world) {
        Hit_Record rec_local;
        if (rt_hit(obj, ray, t_min, t_max, &rec_local)) {
            // Closest hit
            if (rec_local.t < rec.t) {
                rec = rec_local;
            }
        }
    }

    // No hit.
    if (rec.t == f32_max) {
        Vec3 sky_color = lerp(Vec3(1.f), Vec3(0.5f, 0.7f, 1.0f), ray.direction.y*0.5f + 0.5f);
        //sky_color = Vec3(0.1f);
        return sky_color;
    }

    PBR_Mat pbr = rec.pbr;

    // Albedo
    Vec3 albedo = Vec3(0.f);
    if (pbr.albedo) {
        albedo = sample_rgb(pbr.albedo, rec.uv);
        // @Todo: Correct SRGB to Linear
        albedo.x = powf(albedo.x, 2.2f);
        albedo.y = powf(albedo.y, 2.2f);
        albedo.z = powf(albedo.z, 2.2f);
    }

    // Emission
    Vec3 emission = Vec3(0.f);
    if (pbr.emission) {
        emission = sample_rgb(pbr.emission, rec.uv);
    }

    // RM
    f32 roughness = 1.f, metallic = 0.f;
    if (pbr.rm) {
        Vec3 rm = sample_rgb(pbr.rm, rec.uv);
        roughness = rm.y;
        metallic  = rm.z;
    }

    Vec3 N = rec.vertex_normal;
    Vec3 T = normalize(rec.tangent.xyz);
    T = normalize(T - dot(T, N) * N);
    Vec3 B = normalize(rec.tangent.w * cross(N, T));
    Vec3 n = rec.sampled_normal;
    n = normalize(T*n.x + B*n.y + N*n.z);

    // Get a new ray
    Vec3 new_dir;
#if 0
    do {
        new_dir = rand_unit_vec3();
    } while(dot(new_dir, n) <= 0.f);
#else
    new_dir = n + rand_unit_vec3();
#endif
    if (new_dir.x < 1e-8 && new_dir.y < 1e-8 && new_dir.z < 1e-8) {
        new_dir = n;
    } else {
        new_dir = normalize(new_dir);
    }

    // Cook-Torrance BRDF
    //
    Vec3 p = rec.position;
    Vec3 v = -ray.direction;
    Vec3 l = new_dir;
    Vec3 h = normalize(v + l);

    f32 ndotl = max(dot(n, l), 0.f);
    f32 ndotv = max(dot(n, v), 0.f);
    f32 vdoth = max(dot(v, h), 0.f);
    f32 ndoth = max(dot(n, h), 0.f);
    f32 a = roughness * roughness;

    Vec3 f0 = Vec3(0.04f);
    f0 = lerp(f0, albedo, metallic);
    Vec3 schlick_fresnel = f0 + (Vec3(1.f) - f0) * powf(1.f - vdoth, 5.f);

    f32 a2 = a*a;
    f32 x = ndoth * ndoth * (a2 - 1.f) + 1.f;
    f32 ndf_ggx = (a2 * RECIPROCAL_PI) / (x*x);

    f32 k = a * 0.5f;
    f32 g1_l = ndotl / (ndotl * (1.f - k) + k);
    f32 g1_v = ndotv / (ndotv * (1.f - k) + k);
    f32 schlick_ggx = g1_l * g1_v;

    Vec3 specular_term = (schlick_fresnel * ndf_ggx * schlick_ggx) / (4.f * max(ndotl, 0.001f) * max(ndotv, 0.001f));

    Vec3 diffuse_term = albedo * RECIPROCAL_PI * (1.f - metallic) * (Vec3(1.f) - schlick_fresnel);

    Vec3 brdf = diffuse_term + specular_term;


    Ray new_ray(p + l * 1e-4f, new_dir);

    // Rendering equation
    return emission + brdf * ndotl * cast_ray(world, new_ray, t_min, t_max, max_bounces - 1);
}

struct RT_Param {
    int min_x;
    int min_y;
    int max_x_past_one;
    int max_y_past_one;
};

void raycast_work(void *param_)
{
    RT_Param *param = (RT_Param *)param_;

    // Camera
    Vec3 eye = state->camera.position;
    Vec3 dir = state->camera.dir;
    f32 focal_length = state->camera.focal_length;
    f32 vp_w = state->camera.width;
    f32 vp_h = state->camera.height;

    Vec3 left = normalize(cross(Vec3(0.f, 1.f, 0.f), dir));
    Vec3 up = normalize(cross(dir, left));

    Vec3 cen = eye + dir * focal_length;
    Vec3 left_top = cen + left*vp_w*0.5f + up*vp_h*0.5f;

    // RT
    int num_samples = state->num_samples;
    int max_bounces = state->max_bounces;

    // World
    auto& world = state->world;

    // Framebuffer
    auto *fb = &state->framebuffer;

    f32 dx = state->camera.width  / (f32)fb->width;
    f32 dy = state->camera.height / (f32)fb->height;


    for (int r = param->min_y; r < param->max_y_past_one; ++r) {
        for (int c = param->min_x; c < param->max_x_past_one; ++c) {
            Vec3 right = -dx*left;
            Vec3 down  = -dy*up;
            Vec3 px_cen = left_top + ((f32)c + 0.5f)*right + ((f32)r + 0.5f)*down;


            Vec3 accumulated = Vec3(0);
            f32 weight = 1.f / (f32)num_samples;
            for (int i = 0; i < num_samples; ++i) {
                Vec3 px_sample = px_cen + rand_f32(-1.f, 1.f)*0.5f*right + rand_f32(-1.f, 1.f)*0.5f*down;
                Ray ray = Ray(eye, normalize(px_sample - eye));

                accumulated = accumulated + weight * cast_ray(world, ray, 0.001f, 1e8, max_bounces);
            }

            // Replace NaN with zero.
            if (accumulated.x != accumulated.x) accumulated.x = 0.f;
            if (accumulated.y != accumulated.y) accumulated.y = 0.f;
            if (accumulated.z != accumulated.z) accumulated.z = 0.f;

            // @Todo: Correct gamma correction
            accumulated.x = powf(accumulated.x, 1.f/2.2f);
            accumulated.y = powf(accumulated.y, 1.f/2.2f);
            accumulated.z = powf(accumulated.z, 1.f/2.2f);

            u32 *out = (u32 *)(fb->data + r * fb->pitch + c * 4);
            *out = pack_rgba(Vec4(accumulated, 1.f));
        }
    }
}

int main() 
{
    Win32_State *win32_state = new Win32_State;
    win32_init(win32_state);

    stbi_set_flip_vertically_on_load(true);

    // Create textures.
    //
    //Texture *white_tex = make_flat_color_texture(1024, 1024, Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    Texture *helmet_albedo    = create_texture_from_file("data/DamagedHelmetAlbedo.jpg", TEXTURE_LAYOUT_RGB);
    Texture *helmet_normal    = create_texture_from_file("data/DamagedHelmetNormal.jpg", TEXTURE_LAYOUT_RGB);
    Texture *helmet_emission  = create_texture_from_file("data/DamagedHelmetEmission.jpg", TEXTURE_LAYOUT_RGB);
    Texture *helmet_rm        = create_texture_from_file("data/DamagedHelmetRM.jpg", TEXTURE_LAYOUT_RGB); // @Temporary

    Entity *entity = new Entity;
    memset(entity, 0, sizeof(*entity));
    load_and_parse_obj(String("data/DamagedHelmet.obj"), &entity->mesh, &entity->aabb);

    auto *mesh = &entity->mesh;
    u32 num_tri = mesh->num / 3;

    Vec3 translation = Vec3(0.f, 2.f, -1.f);

    Hittable *triangles = new Hittable[num_tri];
    memset(triangles, 0, num_tri * sizeof(triangles[0]));
    for (u32 i = 0; i < mesh->num; i += 3) {
        Vec3 vert1 = translation + mesh->vertices[i];
        Vec3 vert2 = translation + mesh->vertices[i + 1];
        Vec3 vert3 = translation + mesh->vertices[i + 2];
        Vec2 uv1 = mesh->uvs[i];
        Vec2 uv2 = mesh->uvs[i + 1];
        Vec2 uv3 = mesh->uvs[i + 2];
        Hittable tri = {};
        {
            tri.kind            = HITTABLE_TRIANGLE;
            tri.pbr.albedo      = helmet_albedo;
            tri.pbr.normal      = helmet_normal;
            tri.pbr.emission    = helmet_emission;
            tri.pbr.rm          = helmet_rm;
            tri.tri             = Hittable_Tri(vert1, vert2 - vert1, vert3 - vert1);
            tri.tri.st[0]       = uv1;
            tri.tri.st[1]       = uv2;
            tri.tri.st[2]       = uv3;
            tri.tri.tangents[0] = mesh->tangents[i];
            tri.tri.tangents[1] = mesh->tangents[i + 1];
            tri.tri.tangents[2] = mesh->tangents[i + 2];
        }
        triangles[i / 3] = tri;
    }

    AABB *boxes = new AABB[num_tri];
    for (u32 i = 0; i < mesh->num; i += 3) {
        Vec3 vert1 = translation + mesh->vertices[i];
        Vec3 vert2 = translation + mesh->vertices[i + 1];
        Vec3 vert3 = translation + mesh->vertices[i + 2];
        f32 eps = 0.00001f;
        f32 min_x = min(min(vert1.x, vert2.x), vert3.x) - eps;
        f32 min_y = min(min(vert1.y, vert2.y), vert3.y) - eps;
        f32 min_z = min(min(vert1.z, vert2.z), vert3.z) - eps;
        f32 max_x = max(max(vert1.x, vert2.x), vert3.x) + eps;
        f32 max_y = max(max(vert1.y, vert2.y), vert3.y) + eps;
        f32 max_z = max(max(vert1.z, vert2.z), vert3.z) + eps;
        u32 index = i / 3;
        boxes[index].min.x = min_x;
        boxes[index].min.y = min_y;
        boxes[index].min.z = min_z;
        boxes[index].max.x = max_x;
        boxes[index].max.y = max_y;
        boxes[index].max.z = max_z;
    }
    BVH_Node *bvh_root = create_bvh(boxes, num_tri);

    Hittable hit_bvh = {};
    {
        hit_bvh.kind           = HITTABLE_BVH;
        hit_bvh.bvh.root       = bvh_root;
        hit_bvh.bvh.primitives = triangles;
        hit_bvh.bvh.num        = num_tri;
    }



    {
        Arena* arena = arena_alloc();
        state = arena_push<State>(arena);
        state->arena = arena;
        {
            state->num_samples = 64; 
            state->max_bounces = 2;

            auto *buf = &state->framebuffer;
            {
                buf->width  = 640;
                buf->height = 640;
                buf->pitch  = buf->width * 4;
                buf->data   = arena_push<u8>(state->arena, buf->pitch * buf->height, CACHE_LINE);
            }

            {
                auto *c = &state->camera;
                c->position     = Vec3(-0.9f, 2.0f, 0.8f);
                c->dir          = normalize(Vec3(0.f, 2.f, -1.0f) - c->position);
                c->focal_length = 1.5f;
                c->aspect_ratio = (f32)buf->width / (f32)buf->height;
                c->width        = 3.f;
                c->height       = c->width / c->aspect_ratio;
            }
        }
    }

    auto& world = state->world;
    world.push_back(hit_bvh);



    {
        printf("Numer of samples: %d\n", state->num_samples);
        printf("Max bounces: %d\n", state->max_bounces);
        printf("Resolution: %dx%d\n", state->framebuffer.width, state->framebuffer.height);
    }

    {
        auto *buf = &state->framebuffer;
        int w = buf->width;
        int h = buf->height;

#if 1
        int tile_width  = 64;
        int tile_height = 64;
#else
        int tile_width  = 300000;
        int tile_height = 300000;
#endif
        int num_tiles_x = (w + tile_width  - 1) / tile_width;
        int num_tiles_y = (h + tile_height - 1) / tile_height;
        int num_tiles = num_tiles_x * num_tiles_y;

        printf("Tile size: %dx%d\n", tile_width, tile_height);
        printf("Numer of tiles: %d\n", num_tiles);

        u64 begin = ReadOSTimer();
        printf("Begin raytracing...\n");
        for (int tile_y = 0; tile_y < num_tiles_y; ++tile_y) {
            for (int tile_x = 0; tile_x < num_tiles_x; ++tile_x) {
                RT_Param *param = new RT_Param;
                {
                    param->min_x = tile_x * tile_width;
                    param->min_y = tile_y * tile_height;
                    param->max_x_past_one = min((tile_x + 1) * tile_width, w);
                    param->max_y_past_one = min((tile_y + 1) * tile_height, h);
                }
                AddWork(&win32_state->work_queue, (Work_Callback *)raycast_work, param);
            }
        }
        CompleteAllWork(&win32_state->work_queue);

        u64 end = ReadOSTimer();
        f32 elapsed = (f32)(end - begin) * win32_state->rec_qpc_freq;
        printf("Rendering took %.6f seconds.\n", elapsed);
    }



    // Write
    //
    {
        auto *buf = &state->framebuffer;
        // @Temporary
        if (stbi_write_bmp("C:\\dev\\swl\\sw-rt\\image.bmp", buf->width, buf->height, 4, buf->data) == 0) {
            fprintf(stderr, "Failed to write.\n");
            return -1;
        }
    }


    fprintf(stderr, "***SUCCESSFUL!***\n");
    return 0;
}
