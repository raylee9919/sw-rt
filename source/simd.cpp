// Copyright Seong Woo Lee. All Rights Reserved.

f32x4 operator + (f32x4 l, f32x4 r)
{
    return _mm_add_ps(l, r);
}

f32x4 operator - (f32x4 l, f32x4 r)
{
    return _mm_sub_ps(l, r);
}

f32x4 operator * (f32x4 l, f32x4 r)
{
    return _mm_mul_ps(l, r);
}

f32x4 operator / (f32x4 l, f32x4 r)
{
    return _mm_div_ps(l, r);
}

f32x4 operator == (f32x4 l, f32x4 r)
{
    return _mm_cmpeq_ps(l, r);
}

f32x4 operator < (f32x4 l, f32x4 r)
{
    return _mm_cmplt_ps(l, r);
}

f32x4 operator <= (f32x4 l, f32x4 r)
{
    return _mm_cmple_ps(l, r);
}

f32x4 operator > (f32x4 l, f32x4 r)
{
    return _mm_cmpgt_ps(l, r);
}

f32x4 operator >= (f32x4 l, f32x4 r)
{
    return _mm_cmpge_ps(l, r);
}
