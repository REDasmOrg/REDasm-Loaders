#pragma once

#define ULP_MAGIC         0x00706c75

#include <rdapi/rdapi.h>

struct ULPHeader {
    u32 magic;
    u16 textoffset;
    u16 textsize;
    u16 datasize;
    u16 bsssize;
};
