#include "psxbios.h"
#include <cstring>

#define PSX_BIOS_SIGNATURE_SIZE 0x10
#define PSX_BIOS_ENTRYPOINT     0xBFC00000
#define PSX_RAM_SIZE            0x00200000
#define PSX_BIOSDATA_START      "          Licensed"

const u32 PsxBiosLoader::BIOS_SIGNATURE_CRC32 = 0x096541CB;

const char* PsxBiosLoader::test(const RDLoaderPlugin*, const RDLoaderRequest* request)
{
    if(RDBuffer_Size(request->buffer) < PSX_BIOS_SIGNATURE_SIZE) return nullptr;
    if(RDBuffer_CRC32(request->buffer, 0, PSX_BIOS_SIGNATURE_SIZE) != PsxBiosLoader::BIOS_SIGNATURE_CRC32) return nullptr;
    return "mips32le";
}

bool PsxBiosLoader::load(RDLoaderPlugin*, RDLoader* loader)
{
    RDDocument* doc = RDLoader_GetDocument(loader);
    RDBuffer* b = RDLoader_GetBuffer(loader);
    size_t size = RDBuffer_Size(b);
    rd_offset res = RDBuffer_Find(b, reinterpret_cast<const u8*>(PSX_BIOSDATA_START), std::strlen(PSX_BIOSDATA_START));
    rd_location codesize = (res != RD_NPOS) ? res : size;

    RDDocument_AddSegment(doc, "RAM", 0, 0, PSX_RAM_SIZE, SegmentFlags_Bss);
    RDDocument_AddSegment(doc, "BIOSCODE", 0, PSX_BIOS_ENTRYPOINT, codesize, SegmentFlags_Code);

    if(res != RD_NPOS)
        RDDocument_AddSegment(doc, "BIOSDATA", codesize, PSX_BIOS_ENTRYPOINT + codesize, size - codesize, SegmentFlags_Data);

    RDDocument_SetEntry(doc, PSX_BIOS_ENTRYPOINT);
    return true;
}
