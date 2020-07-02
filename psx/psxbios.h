#pragma once

#include <rdapi/rdapi.h>
#include <array>

#define PSX_BIOS_SIGNATURE_SIZE 0x10

class PsxBiosLoader
{
    public:
        PsxBiosLoader() = delete;
        static const char* test(const RDLoaderPlugin*, const RDLoaderRequest* request);
        static bool load(RDLoaderPlugin*, RDLoader* loader);

    private:
        static const std::array<u8, PSX_BIOS_SIGNATURE_SIZE> BIOS_SIGNATURE;
};
