#pragma once

#include <redasm/redasm.h>

struct ESPFirmwareHeader
{
    u8 magic;
    u8 segments;
    u8 flashmode;
    u8 flashsizefreq;
    u32 entrypoint;
};

struct ESPFirmwareSegment
{
    u32 address;
    u32 size;
};
