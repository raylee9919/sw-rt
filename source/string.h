// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

struct String {
    u8 *data;
    u32 length;

    explicit String() = default;
    explicit String(const char *cstr);
    explicit String(const char *cstr, u32 length_);
};
