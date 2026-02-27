// Copyright Seong Woo Lee. All Rights Reserved.

String::String(const char *cstr)
{
    u32 len = 0;
    for (char *c = (char *)cstr; *c; ++c) {
        len++;
    }

    data = (u8 *)cstr;
    length = len;
}

String::String(const char *cstr, u32 length_)
{
    data = (u8 *)cstr;
    length = length_;
}
