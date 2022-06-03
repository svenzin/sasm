#pragma once

namespace sasm {

namespace dtype {

bool is_u8(int x) { return (0 <= x) && (x <= 0xFF); }
bool is_u16(int x) { return (0 <= x) && (x <= 0xFFFF); }
bool is_u24(int x) { return (0 <= x) && (x <= 0xFFFFFF); }
bool is_u32(int x) { return (0 <= x) && (x <= 0xFFFFFFFF); }

bool is_i8(int x) { return (-0x80 <= x) && (x <= 0x7F); }
bool is_i16(int x) { return (-0x8000 <= x) && (x <= 0x7FFF); }
bool is_i24(int x) { return (-0x800000 <= x) && (x <= 0x7FFFFF); }
bool is_i32(int x) { return (-0x7FFFFFFF - 1 <= x) && (x <= 0x7FFFFFFF); }

}

}
