// Copyright Seong Woo Lee. All Rights Reserved.

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

struct RT_State {
    Arena           *arena;

    int             num_samples;
    int             max_bounces;

    Framebuffer     framebuffer;
    Camera          camera;

    std::vector <Hittable> world;
};

struct RT_Param {
    int min_x;
    int min_y;
    int max_x_past_one;
    int max_y_past_one;
};
