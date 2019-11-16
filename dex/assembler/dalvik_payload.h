#pragma once

#include <redasm/redasm.h>
#include <redasm/libs/visit_struct/visit_struct.hpp>

#define DALVIK_PACKED_SWITCH_IDENT   0x0100
#define DALVIK_SPARSE_SWITCH_IDENT   0x0200
#define DALVIK_FILL_ARRAY_DATA_IDENT 0x0300

struct DalvikPackedSwitchPayload {
    u16 ident, size;
    s32 first_key;
    u32 targets[1];
};

struct DalvikSparseSwitchPayload {
    u16 ident, size;
    u32 keys[1];
    //u32 targets[1];
};

struct DalvikFillArrayDataPayload {
    u16 ident, element_width;
    u32 size;
    u8 data[1];
};

VISITABLE_STRUCT(DalvikPackedSwitchPayload, ident, size, first_key);
VISITABLE_STRUCT(DalvikSparseSwitchPayload, ident, size);
VISITABLE_STRUCT(DalvikFillArrayDataPayload, ident, element_width, size);
