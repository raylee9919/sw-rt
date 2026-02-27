// Copyright Seong Woo Lee. All Rights Reserved.

#include <vector>

// .h
//
#include "base.h"
#include "win32.h"
#include "string.h"
#include "math.h"
#include "random.h"
#include "material.h"
#include "geometry.h"
#include "hittable.h"
#include "entity.h"
#include "obj.h"

//.cpp
//
#include "win32.cpp"
#include "string.cpp"
#include "math.cpp"
#include "random.cpp"
#include "geometry.cpp"
#include "hittable.cpp"

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
    int             num_samples;
    int             max_bounces;

    Framebuffer     framebuffer;
    Camera          camera;

    std::vector<Hittable> world;
};

static State *state;


Material make_material_lambertian(Texture *albedo, Vec3 tint = Vec3(1.f)) 
{
    Material mat = {};
    mat.kind = MATERIAL_LAMBERTIAN;
    mat.lambertian.albedo = albedo;
    mat.lambertian.tint = tint;
    return mat;
}

Material make_material_metallic(Vec3 color_, f32 fuzz_ = 0.f) 
{
    Material mat = {};
    mat.kind = MATERIAL_METALLIC;
    mat.metallic.color = color_;
    mat.metallic.fuzz = fuzz_;
    return mat;
}

Material make_material_emission(Vec3 color) 
{
    Material mat = {};
    mat.kind = MATERIAL_EMISSION;
    mat.emission.color = color;
    return mat;
}

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

Texture *make_flat_color_texture(u32 width, u32 height, Vec4 color)
{
    Texture *tex = new Texture;

    tex->width  = width;
    tex->height = height;
    tex->pitch  = 4 * width;
    tex->data   = new u8[tex->pitch * height]; 

    for (u32 r = 0; r < height; ++r) {
        for (u32 c = 0; c < width; ++c) {
            *(u32 *)((u8 *)tex->data + r*tex->pitch + c*4) = pack_rgba(color);
        }
    }

    return tex;
}

Texture *make_checker_texture(u32 width, u32 height)
{
    Texture *tex = new Texture;

    tex->width  = width;
    tex->height = height;
    tex->pitch  = 4 * width;
    tex->data   = new u8[tex->pitch * height]; 

    for (u32 r = 0; r < height; ++r) {
        for (u32 c = 0; c < width; ++c) {
            Vec4 color = Vec4(1.f);
            if (((r/32) + (c/32)) % 2 == 0) {
                color = Vec4(0.2f, 0.2f, 0.2f, 1.f);
            }
            *(u32 *)((u8 *)tex->data + r*tex->pitch + c*4) = pack_rgba(color);
        }
    }

    return tex;
}

Vec3 sample(Texture *tex, Vec2 uv)
{
    u32 w = tex->width;
    u32 h = tex->height;

    // @Todo: Filtering
    u32 r = (u32)(uv.y * (f32)(h - 1) + 0.5f);
    u32 c = (u32)(uv.x * (f32)(w - 1) + 0.5f);

    u32 texel = *(u32 *)((u8 *)tex->data + r*tex->pitch + c*4);
    Vec3 result = unpack_rgba(texel).xyz;
    return result;
}


// Ray
//
struct Ray {
    Vec3 origin;
    Vec3 direction;

    Ray(Vec3 o, Vec3 d) {
        origin = o;
        direction = d;
    }
};

struct Hit_Record {
    f32 t;
    Vec3 position;
    Vec3 normal;
    Vec2 uv;
    Material material;
};

bool hit(const Hittable& hittable, const Ray& ray, const f32 t_min, const f32 t_max, Hit_Record* rec_out) 
{
    switch (hittable.kind) {
        case HITTABLE_SPHERE: {
            auto& sphere = hittable.sphere;
            Vec3 oc = sphere.center - ray.origin;
            f32 a = dot(ray.direction, ray.direction);
            f32 b = dot(ray.direction, oc);
            f32 c = dot(oc, oc) - sphere.radius * sphere.radius;
            f32 discriminant = b*b - a*c;

            if (discriminant < 0.f) {
                return false;
            }

            f32 t = (b - sqrt(discriminant)) / a;
            if (t < t_min || t > t_max) {
                t = (b + sqrt(discriminant)) / a;
                if (t < t_min || t > t_max) {
                    return false;
                }
            }

            Vec3 hit_pos = ray.origin + ray.direction * t;

            Vec3 normal = normalize(hit_pos - sphere.center);
            if (dot(normal, ray.direction) > 0.f) {
                normal = -normal;
            }

            f32 theta = acosf(-hit_pos.y);
            f32 phi = atan2f(-hit_pos.z, hit_pos.x) + PI;
            Vec2 uv = Vec2(phi / (2*PI), theta / PI);

            rec_out->t = t;
            rec_out->position = hit_pos;
            rec_out->normal = normal;
            rec_out->uv = uv;
            rec_out->material = hittable.material;

            return true;
        } break;

        case HITTABLE_QUAD: {
            auto& quad = hittable.quad;

            f32 denom = dot(quad.normal, ray.direction);
            if (fabs(denom) < 1e-8) {
                return false;
            }

            f32 t = (quad.d - dot(quad.normal, ray.origin)) / denom;
            if (t < t_min || t > t_max) {
                return false;
            }

            Vec3 hit_pos = ray.origin + ray.direction * t;

            Vec3 p = hit_pos - quad.origin;
            f32 alpha = dot(quad.w, cross(p, quad.v));
            f32 beta  = dot(quad.w, cross(quad.u, p));

            if (!(alpha >= 0.f && alpha <= 1.f && beta >= 0.f && beta <= 1.f)) {
                return false;
            }

            Vec3 normal = quad.normal;
            if (dot(normal, ray.direction) > 0.f) {
                normal = -normal;
            }

            rec_out->t = t;
            rec_out->position = hit_pos;
            rec_out->normal = normal;
            rec_out->uv = Vec2(alpha, beta);
            rec_out->material = hittable.material;

            return true;
        } break;

        case HITTABLE_TRIANGLE: {
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

            Vec3 normal = tri.normal;
            if (dot(normal, ray.direction) > 0.f) {
                normal = -normal;
            }

            rec_out->t = t;
            rec_out->position = hit_pos;
            rec_out->normal = normal;
            rec_out->uv = Vec2(alpha, beta);
            rec_out->material = hittable.material;

            return true;
        } break;

        default: {
            assert(!"Invalid default case.");
            return false;
        } break;
    }
}

Vec3 cast_ray(std::vector<Hittable>& world, const Ray& ray, const f32 t_min, const f32 t_max, const int max_bounces)
{
    if (max_bounces <= 0) {
        return Vec3(0.f);
    }


    Hit_Record rec = {};
    rec.t = f32_max;
    for (auto& obj : world) {
        Hit_Record rec_local;
        if (hit(obj, ray, t_min, t_max, &rec_local)) {
            // Closest hit
            if (rec_local.t < rec.t) {
                rec = rec_local;
            }
        }
    }

    // No hit. Returns sky color.
    if (rec.t == f32_max) {
        //Vec3 bg_color = lerp(Vec3(1.f), Vec3(0.5f, 0.7f, 1.0f), ray.direction.y*0.5f + 0.5f);
        Vec3 bg_color = Vec3(0.0f, 0.0f, 0.0f);
        return bg_color;
    }

    // Bounce, reflectance.
    switch (rec.material.kind) {
        case MATERIAL_LAMBERTIAN: {
            auto mat = rec.material.lambertian;

            Vec3 r = ray.direction;
            Vec3 n = rec.normal;
            Vec3 new_dir = n + rand_unit_vec3();
            if (new_dir.x < 1e-8 && new_dir.y < 1e-8 && new_dir.z < 1e-8) {
                new_dir = n;
            } else {
                new_dir = normalize(new_dir);
            }
            Ray new_ray(rec.position, new_dir);

            Vec3 attenuation = sample(mat.albedo, rec.uv) * mat.tint;
            return attenuation * cast_ray(world, new_ray, t_min, t_max, max_bounces - 1);
        } break;
        
        case MATERIAL_METALLIC: {
            auto mat = rec.material.metallic;

            Vec3 new_dir = reflect(ray.direction, rec.normal) + rand_unit_vec3()*mat.fuzz;
            Ray new_ray(rec.position, new_dir);
            return mat.color * cast_ray(world, new_ray, t_min, t_max, max_bounces - 1);
        } break;

        case MATERIAL_EMISSION: {
            auto mat = rec.material.emission;

            //Vec3 new_dir = reflect(ray.direction, rec.normal);
            //Ray new_ray(rec.position, new_dir);
            //return mat.color + cast_ray(world, new_ray, t_min, t_max, max_bounces - 1);
            return mat.color;
        } break;

        default: {
            assert(!"Invalid default case.");
            return Vec3{};
        } break;
    }
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


            // @Todo: COLOR COLOR COLOR
            accumulated.x = sqrt(accumulated.x);
            accumulated.y = sqrt(accumulated.y);
            accumulated.z = sqrt(accumulated.z);

            u32 *out = (u32 *)(fb->data + r * fb->pitch + c * 4);
            *out = pack_rgba(Vec4(accumulated, 1.f));
        }
    }
}

Hittable create_quad(Vec3 origin, Vec3 u, Vec3 v, Material mat)
{
    Hittable quad;
    quad.kind = HITTABLE_QUAD;
    quad.material = mat;
    quad.quad = Hittable_Quad(origin, u, v);
    return quad;
}

Hittable create_tri(Vec3 origin, Vec3 u, Vec3 v, Material mat)
{
    Hittable tri;
    tri.kind = HITTABLE_TRIANGLE;
    tri.material = mat;
    tri.tri = Hittable_Tri(origin, u, v);
    return tri;
}

int main() 
{
    Win32_State *win32_state = new Win32_State;
    win32_init(win32_state);

    Entity *entity = new Entity;
    memset(entity, 0, sizeof(*entity));
    load_and_parse_obj(String("C:\\dev\\swl\\sw-rt\\monkey.obj"), &entity->mesh, &entity->aabb);


    auto* mesh = &entity->mesh;
    BVH* bl_bvh = bvh_from_triangle_mesh(mesh);
    {
    }

    state = new State;
    {
        state->num_samples = 1; 
        state->max_bounces = 1;

        auto *buf = &state->framebuffer;
        {
            buf->width  = 800;
            buf->height = 800;
            buf->pitch = buf->width * 4;
            buf->data = new u8[buf->pitch * buf->height];
            memset(buf->data, 0, buf->pitch * buf->height);
        }

        {
            auto *c = &state->camera;
            c->position = Vec3(0.f, 4.9f, 5.f);
            c->dir = normalize(Vec3(0.f, 5.f, 0.f) - c->position);
            c->focal_length = 1.5f;
            c->aspect_ratio = (f32)buf->width / (f32)buf->height;
            c->width = 3.f;
            c->height = c->width / c->aspect_ratio;
        }
    }


    Texture *white_tex   = make_flat_color_texture(1024, 1024, Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    Texture *checker_tex = make_checker_texture(1024, 1024);

    // Cornell box.
    //
    auto& world = state->world;
    world.push_back(create_quad(Vec3(-5.f, 10.f, -5.f), Vec3(0.f, 0.f, 10.f), Vec3(0.f, -10.f, 0.f), make_material_lambertian(white_tex, Vec3(0.0f, 1.0f, 0.0f)))); // left green
    world.push_back(create_quad(Vec3( 5.f, 10.f, -5.f), Vec3(0.f,-10.f, 0.f), Vec3(0.f, 0.f, 10.f), make_material_lambertian(white_tex, Vec3(1.0f, 0.0f, 0.0f)))); // right red
    world.push_back(create_quad(Vec3(-5.f, 10.f, -5.f), Vec3(10.f, 0.f, 0.f), Vec3(0.f, 0.f, 10.f), make_material_lambertian(white_tex))); // top
    world.push_back(create_quad(Vec3(-5.f, 0.f, -5.f), Vec3(0.f, 0.f, 10.f), Vec3(10.f, 0.f, 0.f), make_material_lambertian(white_tex))); // bottom
    world.push_back(create_quad(Vec3(-5.f, 0.f, -5.f), Vec3(10.f, 0.f, 0.f), Vec3(0.f, 10.f, 0.f), make_material_lambertian(white_tex))); // back
    world.push_back(create_quad(Vec3(-5.f, 0.f, 5.f), Vec3(10.f, 0.f, 0.f), Vec3(0.f, 10.f, 0.f), make_material_lambertian(white_tex))); // front
    world.push_back(create_quad(Vec3(-2.0f, 9.995f, -2.0f), Vec3(4.f, 0.f, 0.f), Vec3(0.f, 0.f, 4.f), make_material_emission(Vec3(1.f)))); // light


    for (u32 i = 0; i < mesh->num; i += 3) {
        f32 scale = 1.f;
        Vec3 vert1 = Vec3(0,2,-1) + scale * mesh->vertices[i];
        Vec3 vert2 = Vec3(0,2,-1) + scale * mesh->vertices[i+1];
        Vec3 vert3 = Vec3(0,2,-1) + scale * mesh->vertices[i+2];
        Hittable tri = {};
        tri.kind = HITTABLE_TRIANGLE;
        tri.material = make_material_lambertian(white_tex, Vec3(0.3f, 0.3f, 1.0f));
        tri.tri = Hittable_Tri(vert1, vert2 - vert1, vert3 - vert1);
        world.push_back(tri);
    }





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
        int tile_width  = 30000;
        int tile_height = 30000;
#endif
        int num_tiles_x = (w + tile_width  - 1) / tile_width;
        int num_tiles_y = (h + tile_height - 1) / tile_height;
        int num_tiles = num_tiles_x * num_tiles_y;

        printf("Tile size: %dx%d\n", tile_width, tile_height);
        printf("Numer of tiles: %d\n", num_tiles);

        u64 begin = ReadOSTimer();
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
