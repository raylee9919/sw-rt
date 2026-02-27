// Copyright Seong Woo Lee. All Rights Reserved.


u32 pcg32_rand()
{
    auto *rng = &global_pcg_state;
    u64 oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ULL + (rng->inc|1);
    u32 xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    u32 rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

f32 rand_f32(f32 lo, f32 hi)
{
    u32 u = pcg32_rand();
    f32 f = ((f32)u / (f32)(u32_max));
    return f*(hi - lo) + lo;
}

f64 rand_f64(f64 lo, f64 hi)
{
    u32 u = pcg32_rand();
    f64 f = ((f64)u / (f64)(u32_max));
    return f*(hi - lo) + lo;
}

Vec3 rand_unit_vec3()
{
    return normalize(Vec3(rand_f32(-1.f, 1.f), rand_f32(-1.f, 1.f), rand_f32(-1.f, 1.f)));
}
