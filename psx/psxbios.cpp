#include "psxbios.h"
#include <cstring>

#define PSX_BIOS_SIGNATURE_SIZE 0x10
#define PSX_BIOS_KERNEL1        0xBFC00000
#define PSX_BIOS_KERNEL2        0xBFC10000
#define PSX_BIOS_BOOTMENU       0xBFC18000
#define PSX_BIOS_CHARACTERS     0xBFC64000

#define PSX_BIOS_ENTRYPOINT     0xBFC00000
#define PSX_RAM_SIZE            0x00200000
#define PSX_BIOSDATA_START      "          Licensed"

const u32 PsxBiosLoader::BIOS_SIGNATURE_CRC32 = 0x096541CB;

const char* PsxBiosLoader::test(const RDLoaderRequest* request)
{
    if(RDBuffer_Size(request->buffer) < PSX_BIOS_SIGNATURE_SIZE) return nullptr;
    if(RDBuffer_CRC32(request->buffer, 0, PSX_BIOS_SIGNATURE_SIZE) != PsxBiosLoader::BIOS_SIGNATURE_CRC32) return nullptr;
    return "mips32le";
}

bool PsxBiosLoader::load(RDContext*, RDLoader* loader)
{
    RDDocument* doc = RDLoader_GetDocument(loader);
    RDBuffer* b = RDLoader_GetBuffer(loader);

    RDDocument_AddSegment(doc, "RAM", 0, 0, PSX_RAM_SIZE, SegmentFlags_Bss);
    RDDocument_AddSegmentRange(doc, "KERNEL1", 0, PSX_BIOS_KERNEL1, 0xBFC10000, SegmentFlags_CodeData);
    RDDocument_AddSegmentRange(doc, "KERNEL2", 0x10000, PSX_BIOS_KERNEL2, 0xBFC18000, SegmentFlags_CodeData);
    RDDocument_AddSegmentRange(doc, "BOOTMENU", 0x18000, PSX_BIOS_BOOTMENU, 0xBFC64000, SegmentFlags_CodeData);
    RDDocument_AddSegmentRange(doc, "CHARACTERS", 0x64000, PSX_BIOS_CHARACTERS, PSX_BIOS_CHARACTERS + RDBuffer_Size(b), SegmentFlags_Data);

    PsxBiosLoader::parseROM(doc, loader);
    PsxBiosLoader::parseRAM(doc, b);
    RDDocument_SetEntry(doc, PSX_BIOS_ENTRYPOINT);
    return true;
}

void PsxBiosLoader::parseStrings(rd_address startaddress, const std::vector<std::string> strings, RDDocument* doc, RDLoader* ldr)
{
    u8* data = RD_AddrPointer(ldr, startaddress);
    if(!data) return;

    for(const std::string& s : strings)
    {
        size_t len = std::strlen(reinterpret_cast<char*>(data));
        auto loc = RD_AddressOf(ldr, data);
        if(!loc.valid) break;

        RDDocument_AddAsciiString(doc, loc.address, len, s.c_str());

        if(s != strings.back())
        {
            data += len;
            while(!(*data)) data++;
        }
    }
}

void PsxBiosLoader::parseROM(RDDocument* doc, RDLoader* ldr)
{
    PsxBiosLoader::parseStrings(0xBFC00108, { "kernelMaker", "versionString" }, doc, ldr);
    PsxBiosLoader::parseStrings(0xBFC7FF32, { "guiVersion", "copyrightString" }, doc, ldr);

    RDDocument_AddData(doc, 0xBFC00100, 4, "kernelDate");
    //u8* data = RD_AddrPointer(ldr, 0xBFC00100);
    //rd_log("Kernel Date: " + std::string(RD_FromBCD(data, sizeof(u32))));

    RDDocument_AddData(doc, 0xBFC00104, 4, "consoleType");
}

void PsxBiosLoader::parseRAM(RDDocument* doc, RDBuffer* b)
{

}
