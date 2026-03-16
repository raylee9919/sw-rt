// Copyright Seong Woo Lee. All Rights Reserved.


Arena* arena_alloc(u64 size)
{
    u64 header_size = sizeof(Arena);
    u64 alloc_size = align_up_pow2(header_size + size, 4096); // @Temporary

    u8* ptr = (u8 *)os_page_alloc(alloc_size);

    Arena* arena = (Arena*)ptr;
    arena->next      = NULL;
    arena->prev      = NULL;
    arena->base      = ptr + header_size;
    arena->reserved  = alloc_size - header_size;
    arena->used      = 0;

    return arena;
}

u8* arena_push_size(Arena* arena, u64 size, u64 alignment)
{
    Arena* cur = arena;

    u8* aligned_ptr = (u8*)align_up_pow2((u64)(arena->base + arena->used), alignment);
    if (aligned_ptr + size >= arena->base + arena->reserved) {
        cur = arena_alloc(align_up_pow2(sizeof(Arena), alignment) - sizeof(Arena) + size);
        cur->prev = arena;
        arena->next = cur;
        aligned_ptr = (u8*)align_up_pow2((u64)(cur->base + cur->used), alignment);
    }

    cur->used += size;
    memset(aligned_ptr, 0, sizeof(size));

    return aligned_ptr;
}

template <typename T>
T* arena_push(Arena* arena, u32 num, u64 alignment)
{
    return (T*)arena_push_size(arena, sizeof(T)*num, alignment);
}
