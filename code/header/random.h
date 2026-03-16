// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

struct PCG_State {
    u64 state;
    u64 inc; 
};
static PCG_State global_pcg_state;


static u32 rand_u32();
static f32 rand_f32(f32 lo, f32 hi);
static f64 rand_f64(f64 lo, f64 hi);
static Vec3 rand_unit_vec3();
