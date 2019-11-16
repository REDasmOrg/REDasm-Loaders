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

GbaLoader::GbaLoader(): Loader() { }
AssemblerRequest GbaLoader::assembler() const { return ASSEMBLER_REQUEST("arm", "armthumb"); }

bool GbaLoader::test(const LoadRequest &request) const
{
    const auto* header = request.pointer<GbaRomHeader>();

    if(header->fixed_val != 0x96)
        return false;

    if(!GbaLoader::isUppercaseAscii(header->game_title, GBA_GAME_TITLE_SIZE))
        return false;

    if(!GbaLoader::isUppercaseAscii(header->game_code, GBA_GAME_CODE_SIZE))
        return false;

    if(!GbaLoader::isUppercaseAscii(header->maker_code, GBA_MAKER_CODE_SIZE))
        return false;

    return header->header_checksum == GbaLoader::calculateChecksum(request.view());
}

void GbaLoader::load()
{
    ldrdoc->segment("EWRAM", 0, GBA_SEGMENT_AREA(EWRAM), SegmentType::Bss);
    ldrdoc->segment("IWRAM", 0, GBA_SEGMENT_AREA(IWRAM), SegmentType::Bss);
    ldrdoc->segment("IOREG", 0, GBA_SEGMENT_AREA(IOREG), SegmentType::Bss);
    ldrdoc->segment("PALETTE", 0, GBA_SEGMENT_AREA(PALETTE), SegmentType::Bss);
    ldrdoc->segment("VRAM", 0, GBA_SEGMENT_AREA(VRAM), SegmentType::Bss);
    ldrdoc->segment("OAM", 0, GBA_SEGMENT_AREA(OAM), SegmentType::Bss);
    ldrdoc->segment("ROM", 0, GBA_ROM_START_ADDR, this->buffer()->size(), SegmentType::Code | SegmentType::Data);
    ldrdoc->entry(this->getEP());
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

u32 GbaLoader::getEP() const
{
    u32 b = (static_cast<u32>(this->view()) & 0x00FFFFFF) << 2;
    return GBA_ROM_START_ADDR + (b + 8);
}

u8 GbaLoader::calculateChecksum(const BufferView &view)
{
    u8 checksum = 0;

    for(size_t i = 0xA0; i <= 0xBC; i++)
        checksum -= view[i];

    return checksum - 0x19;
}

REDASM_LOADER("GameBoy Advance ROM", "Dax", "MIT", 1)
REDASM_LOAD { gbadvance.plugin = new GbaLoader(); return true; }
REDASM_UNLOAD { gbadvance.plugin->release(); }
