#include "gba.h"
#include <cstring>
#include <cctype>

// https://www.reinterpretcast.com/writing-a-game-boy-advance-game
// https://problemkaputt.de/gbatek.htm

#define GBA_EWRAM_START_ADDR   0x02000000
#define GBA_EWRAM_SIZE         0x00030000
#define GBA_IWRAM_START_ADDR   0x03000000
#define GBA_IWRAM_SIZE         0x00007FFF
#define GBA_IOREG_START_ADDR   0x04000000
#define GBA_IOREG_SIZE         0x000003FF
#define GBA_PALETTE_START_ADDR 0x05000000
#define GBA_PALETTE_SIZE       0x000003FF
#define GBA_VRAM_START_ADDR    0x06000000
#define GBA_VRAM_SIZE          0x00017FFF
#define GBA_OAM_START_ADDR     0x07000000
#define GBA_OAM_SIZE           0x000003FF
#define GBA_ROM_START_ADDR     0x08000000
#define GBA_ROM_SIZE           0x02000000

#define GBA_SEGMENT_AREA(name) GBA_##name##_START_ADDR, GBA_##name##_SIZE

const char* GbaLoader::test(const RDLoaderPlugin*, const RDLoaderRequest* request)
{
    auto* header = reinterpret_cast<GbaRomHeader*>(RDBuffer_Data(request->buffer));
    if(header->fixed_val != 0x96) return nullptr;
    if(!GbaLoader::isUppercaseAscii(header->game_title, GBA_GAME_TITLE_SIZE)) return nullptr;
    if(!GbaLoader::isUppercaseAscii(header->game_code, GBA_GAME_CODE_SIZE)) return nullptr;
    if(!GbaLoader::isUppercaseAscii(header->maker_code, GBA_MAKER_CODE_SIZE)) return nullptr;
    if(header->header_checksum != GbaLoader::calculateChecksum(request->buffer)) return nullptr;

    return "arm32le";
}

bool GbaLoader::load(RDLoaderPlugin*, RDLoader* loader)
{
    RDDocument* doc = RDLoader_GetDocument(loader);
    RDBuffer* buffer = RDLoader_GetBuffer(loader);

    RDDocument_AddSegment(doc, "EWRAM", 0, GBA_SEGMENT_AREA(EWRAM), SegmentFlags_Bss);
    RDDocument_AddSegment(doc, "IWRAM", 0, GBA_SEGMENT_AREA(IWRAM), SegmentFlags_Bss);
    RDDocument_AddSegment(doc, "IOREG", 0, GBA_SEGMENT_AREA(IOREG), SegmentFlags_Bss);
    RDDocument_AddSegment(doc, "PALETTE", 0, GBA_SEGMENT_AREA(PALETTE), SegmentFlags_Bss);
    RDDocument_AddSegment(doc, "VRAM", 0, GBA_SEGMENT_AREA(VRAM), SegmentFlags_Bss);
    RDDocument_AddSegment(doc, "OAM", 0, GBA_SEGMENT_AREA(OAM), SegmentFlags_Bss);
    RDDocument_AddSegment(doc, "ROM", 0, GBA_ROM_START_ADDR, RDBuffer_Size(buffer), SegmentFlags_CodeData);
    RDDocument_SetEntry(doc, GbaLoader::getEP(buffer));
    return true;
}

bool GbaLoader::isUppercaseAscii(const char *s, size_t c)
{
    for(size_t i = 0; i < c; i++)
    {
        if(std::isupper(s[i]) || std::ispunct(s[i]) || std::isdigit(s[i]))
            continue;

        if(!s[i] && i) // Reached '\0'
            break;

        return false;
    }

    return true;
}

u32 GbaLoader::getEP(RDBuffer* buffer)
{
    u32 b = (*reinterpret_cast<u32*>(RDBuffer_Data(buffer)) & 0x00FFFFFF) << 2;
    return GBA_ROM_START_ADDR + (b + 8);
}

u8 GbaLoader::calculateChecksum(RDBuffer* buffer)
{
    u8 checksum = 0;
    u8* data = RDBuffer_Data(buffer);

    for(size_t i = 0xA0; i <= 0xBC; i++)
        checksum -= data[i];

    return checksum - 0x19;
}

void redasm_entry()
{
    RD_PLUGIN_CREATE(RDLoaderPlugin, gba, "GameBoy Advance ROM");
    gba.test = &GbaLoader::test;
    gba.load = &GbaLoader::load;
    RDLoader_Register(&gba);
}
