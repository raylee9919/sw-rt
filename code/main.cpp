// Copyright Seong Woo Lee. All Rights Reserved.

#include <vector>
#include <stack>

// .h
//
#include "header/base.h"
#include "header/win32.h"
#include "header/memory.h"
#include "header/simd.h"
#include "header/math.h"
#include "header/random.h"
#include "header/material.h"
#include "header/geometry.h"
#include "header/asset.h"
#include "header/hittable.h"
#include "header/entity.h"
#include "header/obj.h"
#include "header/rt.h"
#include "vendor/stb_image.h"

//.cpp
//
#include "source/win32.cpp"
#include "source/memory.cpp"
#include "source/simd.cpp"
#include "source/math.cpp"
#include "source/random.cpp"
#include "source/material.cpp"
#include "source/geometry.cpp"
#include "source/hittable.cpp"
#include "vendor/mikktspace.c"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ASSERT(x) 
#include "vendor/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBIW_ASSERT(x)
#include "vendor/stb_image_write.h"


static RT_State *state;


// Ray
//
Vec3 rt_cast_ray(std::vector<Hittable>& world, Ray& ray, f32 t_min, f32 t_max, int max_bounces)
{
    // @Todo: Has to modify the formula! Original formula already contains or doesn't contain some term for pdf? I don't know.
    //
    if (max_bounces < 0) {
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
        return sky_color;
    }

    PBR_Mat pbr = rec.pbr;

    // Albedo
    Vec3 albedo = Vec3(1.f);
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

    // @Temporary:
    // @Temporary:
    // @Temporary: If there's no normal texture, the sampled normal here is 
    // interpolated vertex normal which is in the local space of the mesh. 
    // So... applying TBN is unecessary...
#if 0
    Vec3 N = rec.vertex_normal;
    Vec3 T = normalize(rec.tangent.xyz);
    T = normalize(T - dot(T, N) * N);
    Vec3 B = normalize(rec.tangent.w * cross(N, T));
    Vec3 n = rec.sampled_normal;
    n = normalize(Vec3(dot(T,n), dot(B,n), dot(N,n)));
#else
    Vec3 n = rec.sampled_normal;
#endif

    // Get a new ray
    Vec3 new_dir;
    f32 pdf;
    {
        f32 r1 = rand_f32(0, 1);
        f32 r2 = rand_f32(0, 1);

        f32 phi = 2 * PI * r1;
        f32 x = cosf(phi) * sqrtf(r2);
        f32 y = sinf(phi) * sqrtf(r2);
        f32 z = sqrtf(1 - r2);
        
        Vec3 uniform_dir = Vec3(x, y, z);

        Vec3 w = n;
        Vec3 a = fabs(w.x) > 0.9f ? Vec3(0,1,0) : Vec3(1,0,0);
        Vec3 v = normalize(cross(w, a));
        Vec3 u = normalize(cross(w, v));

        new_dir = (uniform_dir.x * u) + (uniform_dir.y * v) + (uniform_dir.z * w);
        pdf = z * RECIPROCAL_PI;
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
    return emission + (brdf * ndotl * rt_cast_ray(world, new_ray, t_min, t_max, max_bounces - 1) / pdf) / 1;
}

void rt_work(void *param_)
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

                accumulated += rt_cast_ray(world, ray, 0.001f, 1e8, max_bounces) * weight;
            }

            // Replace NaN with zero.
            if (accumulated.x != accumulated.x) accumulated.x = 0.f;
            if (accumulated.y != accumulated.y) accumulated.y = 0.f;
            if (accumulated.z != accumulated.z) accumulated.z = 0.f;

            // @Todo: Correct gamma correction
            accumulated.x = powf(accumulated.x, 0.45454545f);
            accumulated.y = powf(accumulated.y, 0.45454545f);
            accumulated.z = powf(accumulated.z, 0.45454545f);

            u32 *out = (u32 *)(fb->data + r * fb->pitch + c * 4);
            *out = pack_rgba(Vec4(accumulated, 1.f));
        }
    }
}

Hittable *create_triangles_from_mesh(Triangle_Mesh *mesh, Mat4 transform,
                                     Texture *albedo, Texture *normal, Texture *rm, Texture *emission)
{
    u32 num_tri = mesh->num / 3;

    Hittable *triangles = (Hittable *)os_heap_alloc(sizeof(Hittable) * num_tri);
    for (u32 i = 0; i < mesh->num; i += 3) {
        Vec3 vert1 = (transform * Vec4(mesh->vertices[i + 0], 1)).xyz;
        Vec3 vert2 = (transform * Vec4(mesh->vertices[i + 1], 1)).xyz;
        Vec3 vert3 = (transform * Vec4(mesh->vertices[i + 2], 1)).xyz;
        Vec2 uv1 = mesh->uvs[i];
        Vec2 uv2 = mesh->uvs[i + 1];
        Vec2 uv3 = mesh->uvs[i + 2];
        Hittable tri = {};
        {
            tri.kind            = HITTABLE_TRIANGLE;
            tri.pbr.albedo      = albedo;
            tri.pbr.normal      = normal;
            tri.pbr.emission    = emission;
            tri.pbr.rm          = rm;
            tri.tri             = Hittable_Tri(vert1, vert2 - vert1, vert3 - vert1);
            tri.tri.st[0]       = uv1;
            tri.tri.st[1]       = uv2;
            tri.tri.st[2]       = uv3;
            tri.tri.normals[0]  = (transform * Vec4(mesh->normals[i + 0], 0)).xyz;
            tri.tri.normals[1]  = (transform * Vec4(mesh->normals[i + 1], 0)).xyz;
            tri.tri.normals[2]  = (transform * Vec4(mesh->normals[i + 2], 0)).xyz;
            tri.tri.tangents[0] = Vec4((transform * Vec4(mesh->tangents[i + 0].xyz, 0)).xyz, mesh->tangents[i + 0].w);
            tri.tri.tangents[1] = Vec4((transform * Vec4(mesh->tangents[i + 1].xyz, 0)).xyz, mesh->tangents[i + 1].w);
            tri.tri.tangents[2] = Vec4((transform * Vec4(mesh->tangents[i + 2].xyz, 0)).xyz, mesh->tangents[i + 2].w);
        }
        triangles[i / 3] = tri;
    }

    return triangles;
}

AABB *create_boxes_from_mesh(Triangle_Mesh *mesh, Mat4 transform)
{
    u32 num_tri = mesh->num / 3;

    AABB *boxes = (AABB *)os_heap_alloc(sizeof(AABB) * num_tri);

    for (u32 i = 0; i < mesh->num; i += 3) {
        Vec3 vert1 = (transform * Vec4(mesh->vertices[i + 0], 1)).xyz;
        Vec3 vert2 = (transform * Vec4(mesh->vertices[i + 1], 1)).xyz;
        Vec3 vert3 = (transform * Vec4(mesh->vertices[i + 2], 1)).xyz;
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

    return boxes;
}

Hittable create_hittable_bvh(Triangle_Mesh *mesh, Mat4 transform,
                             Texture *albedo, Texture *normal, Texture *rm, Texture *emission)
{
    const int num_tri = mesh->num / 3;
    const int num_box = num_tri;

    Hittable *triangles = create_triangles_from_mesh(mesh, transform, albedo, normal, rm, emission);
    AABB *boxes         = create_boxes_from_mesh(mesh, transform);
    BVH_Node *bvh_root  = create_bvh(boxes, num_box);

    Hittable h = {};
    {
        h.kind           = HITTABLE_BVH;
        h.bvh.root       = bvh_root;
        h.bvh.primitives = triangles;
        h.bvh.num        = num_tri;
    }

    return h;
}

Entity *entity_alloc()
{
    return (Entity *)os_heap_alloc(sizeof(Entity));
}

int main() 
{
    OS_State *win32_state = (OS_State *)os_heap_alloc(sizeof(OS_State));
    os_init(win32_state);

    stbi_set_flip_vertically_on_load(true);

    // Init state
    //
    {
        Arena* arena = arena_alloc();
        state = arena_push<RT_State>(arena);
        state->arena = arena;
        {
            state->num_samples = 1024; 
            state->max_bounces = 2;

            auto *buf = &state->framebuffer;
            {
                buf->width  = 480;
                buf->height = 480;
                buf->pitch  = buf->width * 4;
                buf->data   = arena_push<u8>(state->arena, buf->pitch * buf->height, CACHE_LINE);
            }

            {
                auto *cam = &state->camera;
                cam->position     = Vec3(0.0f, 1.0f, 4.8f);
                cam->dir          = normalize(Vec3(0.0f, 1.0f, -1.0f) - cam->position);
                cam->focal_length = 1.5f;
                cam->aspect_ratio = (f32)buf->width / (f32)buf->height;
                cam->width        = 3.f;
                cam->height       = cam->width / cam->aspect_ratio;
            }
        }
    }


    // Create textures.
    //
    Texture *checker_tex     = create_checker_texture(1024, 1024);
    Texture *white_tex       = create_flat_color_texture(1024, 1024, Vec3(1.0f, 1.0f, 1.0f));
    Texture *black_tex       = create_flat_color_texture(1024, 1024, Vec3(0.0f, 0.0f, 0.0f));
    Texture *red_tex         = create_flat_color_texture(1024, 1024, Vec3(1.0f, 0.0f, 0.0f));
    Texture *green_tex       = create_flat_color_texture(1024, 1024, Vec3(0.0f, 1.0f, 0.0f));
    Texture *blue_tex        = create_flat_color_texture(1024, 1024, Vec3(0.0f, 0.0f, 1.0f));
    Texture *helmet_albedo   = create_texture_from_file("data/DamagedHelmetAlbedo.jpg", TEXTURE_LAYOUT_RGB);
    Texture *helmet_normal   = create_texture_from_file("data/DamagedHelmetNormal.jpg", TEXTURE_LAYOUT_RGB);
    Texture *helmet_emission = create_texture_from_file("data/DamagedHelmetEmission.jpg", TEXTURE_LAYOUT_RGB);
    Texture *helmet_rm       = create_texture_from_file("data/DamagedHelmetRM.jpg", TEXTURE_LAYOUT_RGB); // @Temporary: We are extracing roughness and metallic by hand in the shader.

    // Create meshes.
    //
    Triangle_Mesh *helmet_mesh = (Triangle_Mesh *)os_heap_alloc(sizeof(Triangle_Mesh));
    load_obj("data/DamagedHelmet.obj", helmet_mesh);

    Triangle_Mesh *knight_mesh = (Triangle_Mesh *)os_heap_alloc(sizeof(Triangle_Mesh));
    load_obj("data/knight.obj", knight_mesh);

    Triangle_Mesh *plane_mesh = (Triangle_Mesh *)os_heap_alloc(sizeof(Triangle_Mesh));
    load_obj("data/plane.obj", plane_mesh);

    // Create my scene.
    //
    {
        auto& world = state->world;

        // Knight.
        //
        {
            Entity *knight = entity_alloc();
            knight->mesh = knight_mesh;
            Mat4 transform = translation(0.0f, -3.0f, -2.f) * scale(1.0f);
            Hittable knight_bvh = create_hittable_bvh(knight->mesh, transform, NULL, NULL, black_tex, NULL);
            world.push_back(knight_bvh);
        }

        // Cornell box.
        //
        const f32 sz = 10.f;
        const f32 t = sz * 0.5f;
        { // Light
            Entity *e = entity_alloc();
            e->mesh = plane_mesh;
            Mat4 m = translation(0, t-0.001f, 0) * x_rotation(PI * 0.5f) * scale(sz*0.3f);
            Hittable bvh = create_hittable_bvh(e->mesh, m, NULL, NULL, NULL, white_tex);
            world.push_back(bvh);
        }

        { // Top
            Entity *e = entity_alloc();
            e->mesh = plane_mesh;
            Mat4 m = translation(0, t, 0) * x_rotation(PI * 0.5f) * scale(sz);
            Hittable bvh = create_hittable_bvh(e->mesh, m, NULL, NULL, NULL, NULL);
            world.push_back(bvh);
        }

        { // Bottom
            Entity *e = entity_alloc();
            e->mesh = plane_mesh;
            Mat4 m = translation(0, -t, 0) * x_rotation(-PI * 0.5f) * scale(sz);
            Hittable bvh = create_hittable_bvh(e->mesh, m, checker_tex, NULL, NULL, NULL);
            world.push_back(bvh);
        }

        { // Back
            Entity *e = entity_alloc();
            e->mesh = plane_mesh;
            Mat4 m = translation(0, 0, -t) * scale(sz);
            Hittable bvh = create_hittable_bvh(e->mesh, m, NULL, NULL, NULL, NULL);
            world.push_back(bvh);
        }

        { // Front
            Entity *e = entity_alloc();
            e->mesh = plane_mesh;
            Mat4 m = translation(0, 0, t) * y_rotation(PI) * scale(sz);
            Hittable bvh = create_hittable_bvh(e->mesh, m, NULL, NULL, NULL, NULL);
            world.push_back(bvh);
        }

        { // Left
            Entity *e = entity_alloc();
            e->mesh = plane_mesh;
            Mat4 m = translation(-t, 0, 0) * y_rotation(PI*0.5f) * scale(sz);
            Hittable bvh = create_hittable_bvh(e->mesh, m, red_tex, NULL, NULL, NULL);
            world.push_back(bvh);
        }

        { // Right
            Entity *e = entity_alloc();
            e->mesh = plane_mesh;
            Mat4 m = translation(t, 0, 0) * y_rotation(-PI*0.5f) * scale(sz);
            Hittable bvh = create_hittable_bvh(e->mesh, m, green_tex, NULL, NULL, NULL);
            world.push_back(bvh);
        }
    }


    // Do the work.
    //
    {
        printf("numer of samples: %d\n", state->num_samples);
        printf("max bounces: %d\n", state->max_bounces);
        printf("resolution: %dx%d\n", state->framebuffer.width, state->framebuffer.height);
    }

    {
        auto *buf = &state->framebuffer;
        int w = buf->width;
        int h = buf->height;

        int tile_width  = 64;
        int tile_height = 64;
        int num_tiles_x = (w + tile_width  - 1) / tile_width;
        int num_tiles_y = (h + tile_height - 1) / tile_height;
        int num_tiles = num_tiles_x * num_tiles_y;

        printf("tile size: %dx%d\n", tile_width, tile_height);
        printf("numer of tiles: %d\n", num_tiles);

        u64 begin = os_qpc();
        printf("begin raytracing...\n");
        for (int tile_y = 0; tile_y < num_tiles_y; ++tile_y) {
            for (int tile_x = 0; tile_x < num_tiles_x; ++tile_x) {
                RT_Param *param = (RT_Param *)os_heap_alloc(sizeof(RT_Param));
                {
                    param->min_x = tile_x * tile_width;
                    param->min_y = tile_y * tile_height;
                    param->max_x_past_one = min((tile_x + 1) * tile_width, w);
                    param->max_y_past_one = min((tile_y + 1) * tile_height, h);
                }
                AddWork(&win32_state->work_queue, (Work_Callback *)rt_work, param);
            }
        }
        CompleteAllWork(&win32_state->work_queue);

        u64 end = os_qpc();
        f32 elapsed = (f32)(end - begin) * win32_state->rcp_qpc_freq;
        printf("rendering took %.6f seconds.\n", elapsed);
    }



    // Write
    //
    auto *buf = &state->framebuffer;
    // @Temporary
    if (stbi_write_bmp("C:\\dev\\swl\\sw-rt\\image.bmp", buf->width, buf->height, 4, buf->data) == 0) {
        fprintf(stderr, "failed to write.\n");
        return -1;
    }


    fprintf(stderr, "***SUCCESSFUL!***\n");
    return 0;
}
