#include "gba.h"
#include "analyzers/crt0_analyzer.h"
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

const char* GbaLoader::test(const RDLoaderRequest* request)
{
    auto* header = reinterpret_cast<GbaRomHeader*>(RDBuffer_Data(request->buffer));
    if(header->fixed_val != 0x96) return nullptr;
    if(!GbaLoader::isUppercaseAscii(header->game_title, GBA_GAME_TITLE_SIZE)) return nullptr;
    if(!GbaLoader::isUppercaseAscii(header->game_code, GBA_GAME_CODE_SIZE)) return nullptr;
    if(!GbaLoader::isUppercaseAscii(header->maker_code, GBA_MAKER_CODE_SIZE)) return nullptr;
    if(header->header_checksum != GbaLoader::calculateChecksum(request->buffer)) return nullptr;

    return "arm32le";
}

bool GbaLoader::load(RDContext* ctx)
{
    auto* header = reinterpret_cast<GbaRomHeader*>(RDContext_GetBufferData(ctx));
    RDDocument* doc = RDContext_GetDocument(ctx);

    RDDocument_SetSegment(doc, "EWRAM", 0, GBA_SEGMENT_AREA(EWRAM), SegmentFlags_Bss);
    RDDocument_SetSegment(doc, "IWRAM", 0, GBA_SEGMENT_AREA(IWRAM), SegmentFlags_Bss);
    RDDocument_SetSegment(doc, "IOREG", 0, GBA_SEGMENT_AREA(IOREG), SegmentFlags_Bss);
    RDDocument_SetSegment(doc, "PALETTE", 0, GBA_SEGMENT_AREA(PALETTE), SegmentFlags_Bss);
    RDDocument_SetSegment(doc, "VRAM", 0, GBA_SEGMENT_AREA(VRAM), SegmentFlags_Bss);
    RDDocument_SetSegment(doc, "OAM", 0, GBA_SEGMENT_AREA(OAM), SegmentFlags_Bss);
    RDDocument_SetSegment(doc, "ROM", 0, GBA_ROM_START_ADDR, RDContext_GetBufferSize(ctx), SegmentFlags_CodeData);
    RDDocument_SetEntry(doc, GbaLoader::parseAddress(header->entry_point));

    GbaLoader::createHeaderType(ctx);

    std::string gametitle{header->game_title}, gamecode{header->game_code};
    if(!gametitle.empty() && !gamecode.empty()) rd_log("Loaded: '" + gametitle + "', code '" + gamecode + "'");
    return true;
}

void GbaLoader::createHeaderType(RDContext* ctx)
{
    rd_ptr<RDType> gbaheader(RDType_CreateStructure("GBAHeader"));
    RDStructure_Append(gbaheader.get(), RDType_CreateInt(4, false), "entry_point");
    RDStructure_Append(gbaheader.get(), RDType_CreateArray(GBA_NINTENDO_LOGO_SIZE, RDType_CreateInt(1, false)), "nintendo_logo");
    RDStructure_Append(gbaheader.get(), RDType_CreateAsciiString(GBA_GAME_TITLE_SIZE), "game_title");
    RDStructure_Append(gbaheader.get(), RDType_CreateAsciiString(GBA_GAME_CODE_SIZE), "game_code");
    RDStructure_Append(gbaheader.get(), RDType_CreateAsciiString(GBA_MAKER_CODE_SIZE), "maker_code");
    RDStructure_Append(gbaheader.get(), RDType_CreateInt(1, false), "fixed_val");
    RDStructure_Append(gbaheader.get(), RDType_CreateInt(1, false), "main_unit_code");
    RDStructure_Append(gbaheader.get(), RDType_CreateInt(1, false), "device_type");
    RDStructure_Append(gbaheader.get(), RDType_CreateArray(7, RDType_CreateInt(1, false)), "reserved_area");
    RDStructure_Append(gbaheader.get(), RDType_CreateInt(1, false), "software_version");
    RDStructure_Append(gbaheader.get(), RDType_CreateArray(2, RDType_CreateInt(1, false)), "reserved_area_2");
    RDStructure_Append(gbaheader.get(), RDType_CreateInt(4, false), "ram_entry_point");
    RDStructure_Append(gbaheader.get(), RDType_CreateInt(1, false), "boot_mode");
    RDStructure_Append(gbaheader.get(), RDType_CreateInt(1, false), "slave_id");
    RDStructure_Append(gbaheader.get(), RDType_CreateArray(26, RDType_CreateInt(1, false)), "unused");
    RDStructure_Append(gbaheader.get(), RDType_CreateInt(4, false), "joybus_entry_point");

    //RDDocument* doc = RDContext_GetDocument(ctx);
    //RDDocument_SetType(doc, GBA_ROM_START_ADDR, gbaheader.get());
}

bool GbaLoader::isUppercaseAscii(const char *s, size_t c)
{
    for(size_t i = 0; s[i] && i < c; i++)
    {
        if(std::isupper(s[i]) || std::ispunct(s[i]) || std::isdigit(s[i]) || std::isspace(s[i]))
            continue;

        return false;
    }

    return true;
}

u8 GbaLoader::calculateChecksum(RDBuffer* buffer)
{
    u8 checksum = 0;
    u8* data = RDBuffer_Data(buffer);

    for(size_t i = 0xA0; i <= 0xBC; i++)
        checksum -= data[i];

    return checksum - 0x19;
}

u32 GbaLoader::parseAddress(u32 opcode)
{
    u32 b = (opcode & 0x00FFFFFF) << 2;
    return GBA_ROM_START_ADDR + (b + 8);
}

void rdplugin_init(RDContext*, RDPluginModule* pm)
{
    RD_PLUGIN_ENTRY(RDEntryLoader, gba, "GameBoy Advance ROM");
    gba.test = &GbaLoader::test;
    gba.load = &GbaLoader::load;
    RDLoader_Register(pm, &gba);

    RD_PLUGIN_ENTRY(RDEntryAnalyzer, gba_crt0, "Analyze CRT0");
    gba_crt0.description = "Analyze CRT0 Instructions";
    gba_crt0.flags = AnalyzerFlags_Selected | AnalyzerFlags_RunOnce;
    gba_crt0.order = 1000;

    gba_crt0.isenabled = [](const RDContext* ctx) -> bool {
        auto* loader = RDContext_GetLoader(ctx);
        return loader && RD_StrEquals(RDLoader_GetId(loader), "gba");
    };

    gba_crt0.execute = [](RDContext* ctx) {
        Crt0Analyzer crt0a(ctx);
        crt0a.execute();
    };

    RDAnalyzer_Register(pm, &gba_crt0);
}
