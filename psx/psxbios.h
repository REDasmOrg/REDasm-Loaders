#pragma once

#include <rdapi/rdapi.h>
#include <vector>
#include <array>

class PsxBiosLoader
{
    public:
        PsxBiosLoader() = delete;
        static const char* test(const RDLoaderRequest* request);
        static bool load(RDContext*ctx);

    private:
        static void parseStrings(rd_address startaddress, const std::vector<std::string> strings, RDDocument* doc, RDContext* ctx);
        static void parseROM(RDDocument* doc, RDContext* ctx);
        static void parseRAM(RDDocument* doc, RDBuffer* b);

    private:
        static const u32 BIOS_SIGNATURE_CRC32;
};
