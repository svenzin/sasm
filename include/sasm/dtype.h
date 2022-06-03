#pragma once

namespace sasm {

namespace dtype {

enum etype {
    any,
    u8, u16, u24, u32,
    i8, i16, i24, i32,
};

bool is_u8(int x);
bool is_u16(int x);
bool is_u24(int x);
bool is_u32(int x);

bool is_i8(int x);
bool is_i16(int x);
bool is_i24(int x);
bool is_i32(int x);

}

}
