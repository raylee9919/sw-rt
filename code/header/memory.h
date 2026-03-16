// Copyright Seong Woo Lee. All Rights Reserved.

struct Arena {
    Arena   *next;
    Arena   *prev;
    u8      *base;
    u64     reserved;
    u64     used;
};



Arena* arena_alloc(u64 size = 0);

u8* arena_push_size(Arena* arena, u64 size, u64 alignment = 1);

template <typename T>
T* arena_push(Arena* arena, u32 num = 1, u64 alignment = alignof(T));
