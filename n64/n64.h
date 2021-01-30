#pragma once

#include <rdapi/rdapi.h>
#include "n64_header.h"

class N64Loader
{
    public:
        N64Loader() = delete;
        static const char* test(const RDLoaderRequest* request);
        static bool load(RDContext* ctx);

    public:
        static bool checkMediaType(const N64RomHeader* header);
        static bool checkCountryCode(const N64RomHeader* header);
        static bool checkChecksum(const N64RomHeader *header, const RDBufferView* view);

    private:
        static bool getBootcodeAndSeed(const N64RomHeader* header, u32* bootcode, u32* seed);
        static u32 calculateChecksum(const N64RomHeader *header, const RDBufferView* view, u32 *crc);
        static u32 getCICVersion(const N64RomHeader *header);
        static u32 getEP(const N64RomHeader *header);
};
