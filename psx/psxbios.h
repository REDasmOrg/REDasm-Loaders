#pragma once

#include <rdapi/rdapi.h>
#include <vector>
#include <array>

class PsxBiosLoader
{
    public:
        PsxBiosLoader() = delete;
        static const char* test(const RDLoaderRequest* request);
        static bool load(RDContext*, RDLoader* loader);

    private:
        static void parseStrings(rd_address startaddress, const std::vector<std::string> strings, RDDocument* doc, RDLoader* ldr);
        static void parseROM(RDDocument* doc, RDLoader* ldr);
        static void parseRAM(RDDocument* doc, RDBuffer* b);

    private:
        static const u32 BIOS_SIGNATURE_CRC32;
};
