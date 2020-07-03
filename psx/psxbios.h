#pragma once

#include <rdapi/rdapi.h>
#include <array>

class PsxBiosLoader
{
    public:
        PsxBiosLoader() = delete;
        static const char* test(const RDLoaderPlugin*, const RDLoaderRequest* request);
        static bool load(RDLoaderPlugin*, RDLoader* loader);

    private:
        static const u32 BIOS_SIGNATURE_CRC32;
};
