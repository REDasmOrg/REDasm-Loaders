#include "psxbios.h"
#include <cstring>

#define PSX_BIOS_ENTRYPOINT 0xBFC00000
#define PSX_RAM_SIZE        0x00200000
#define PSX_BIOSDATA_START  "          Licensed"

const std::array<u8, PSX_BIOS_SIGNATURE_SIZE> PsxBiosLoader::BIOS_SIGNATURE = {
    0x13, 0x00, 0x08, 0x3C, 0x3F, 0x24, 0x08, 0x35,
    0x80, 0x1F, 0x01, 0x3C, 0x10, 0x10, 0x28, 0xAC,
};

const char* PsxBiosLoader::test(const RDLoaderPlugin*, const RDLoaderRequest* request)
{
    if(RDBuffer_Size(request->buffer) < PsxBiosLoader::BIOS_SIGNATURE.size()) return nullptr;

    u8* data = RDBuffer_Data(request->buffer);
    if(std::memcmp(data, PsxBiosLoader::BIOS_SIGNATURE.data(), PsxBiosLoader::BIOS_SIGNATURE.size())) return nullptr;

    return "mips32le";
}

bool PsxBiosLoader::load(RDLoaderPlugin*, RDLoader* loader)
{
    RDDocument* doc = RDLoader_GetDocument(loader);
    RDBuffer* b = RDLoader_GetBuffer(loader);
    u8* data = RDBuffer_Data(b);
    size_t size = RDBuffer_Size(b);
    const u8* res = RD_FindBytes(data, size, reinterpret_cast<const u8*>(PSX_BIOSDATA_START), std::strlen(PSX_BIOSDATA_START));

    rd_location codesize = res ?  (res - data) : size;

    RDDocument_AddSegment(doc, "RAM", 0, 0, PSX_RAM_SIZE, SegmentFlags_Bss);
    RDDocument_AddSegment(doc, "BIOSCODE", 0, PSX_BIOS_ENTRYPOINT, codesize, SegmentFlags_Code);

    if(res)
        RDDocument_AddSegment(doc, "BIOSDATA", codesize, PSX_BIOS_ENTRYPOINT + codesize, size - codesize, SegmentFlags_Data);

    RDDocument_SetEntry(doc, PSX_BIOS_ENTRYPOINT);
    return true;
}
