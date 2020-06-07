#pragma once

#include <rdapi/rdapi.h>

struct ESP8266RomHeader1
{
    u8 magic, segments;
    u8 flashmode, flashsizefreq;
    u32 entrypoint;
};

struct ESP8266RomHeader2
{
    u8 magic, segments;
    u8 flashmode, flashsizefreq;
    u32 entrypoint;
    u8 unused[4];
    u32 size;
};

struct ESPSegment
{
    u32 address;
    u32 size;
};
