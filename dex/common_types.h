#pragma once

#include <rdapi/types.h>

#define DALVIK_PACKEDSWITCH_IDENT  0x0100
#define DALVIK_SPARSESWITCH_IDENT  0x0200
#define DALVIK_FILLARRAYDATA_IDENT 0x0300
#define DALVIK_PACKEDSWITCH_NAME   "PackedSwitch"
#define DALVIK_SPARSESWITCH_NAME   "SparseSwitch"
#define DALVIK_FILLARRAYDATA_NAME  "FillArrayData"

#define DALVIK_PACKEDSWITCH_PATH  "/dex/PackedSwitchData"
#define DALVIK_SPARSESWITCH_PATH  "/dex/SparseSwitchData"
#define DALVIK_FILLARRAYDATA_PATH "/dex/FillArrayData"

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
