// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include <stdint.h>
#include <immintrin.h>

typedef int8_t      s8;  
typedef int16_t     s16; 
typedef int32_t     s32; 
typedef int64_t     s64; 
typedef uint8_t     u8;  
typedef uint16_t    u16; 
typedef uint32_t    u32; 
typedef uint64_t    u64; 
typedef s8          b8;
typedef s16         b16;
typedef s32         b32;
typedef float       f32; 
typedef double      f64; 


#define internal static

#define CONCAT(A, B) A##B
#define CONCAT2(A, B) CONCAT(A, B)

#undef assert
#ifdef _WIN32
#  ifdef _DEBUG
#    define assert(exp) if (!(exp)) { __debugbreak(); }
#  else
#    define assert(exp) if (!(exp)) { *(volatile int*)0 = 0; }
#  endif
#else
#  define assert(exp) if (!(exp)) { *(volatile int*)0 = 0; }
#endif

#define array_count(arr) (sizeof(arr)/sizeof(arr[0]))

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define clamp(val, lo, hi) min(max((val), (lo)), (hi))

// Defer
//
template <typename F>
struct Scope_Exit {
    Scope_Exit(F f) : f(f) {}
    ~Scope_Exit() { f(); }
    F f;
};
template <typename F>
Scope_Exit<F> scope_exit_make(F f) {
    return Scope_Exit<F>(f);
};
#define defer(code) \
    auto CONCAT2(scope_exit_, __LINE__) = scope_exit_make([=](){code;})


const f32 f32_max = 3.402823e+38f;
const u32 u32_max = 0xffffffff;
