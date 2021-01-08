#include "assembler/chip8.h"
#include <rdapi/rdapi.h>
#include <filesystem>

const char* test(const RDLoaderRequest* request)
{
    auto ext = std::filesystem::path(request->filepath).extension();

    if((ext == ".chip8") || (ext == ".ch8") || (ext == ".rom"))
    {
        if((RDBuffer_Size(request->buffer) % 2) == 0)
            return "chip8asm";
    }

    return nullptr;
}

static bool load(RDContext* ctx)
{
    RDBuffer* buffer = RDContext_GetBuffer(ctx);
    RDDocument* doc = RDContext_GetDocument(ctx);

    RDDocument_AddSegment(doc, "STACK", 0, 0, 0x0040, SegmentFlags_Bss);
    RDDocument_AddSegment(doc, "SCRATCHPAD", 0, 0x0040, 0x000D, SegmentFlags_Bss);
    RDDocument_AddSegment(doc, "DISPLAY", 0, 0x0100, 0x0100, SegmentFlags_Bss);
    RDDocument_AddSegmentSize(doc, "PROGRAM", 0, 0x200, RDBuffer_Size(buffer), 0x1000 - 0x200, SegmentFlags_CodeData);
    RDDocument_SetEntry(doc, 0x200);
    return true;
}

void rdplugin_init(RDContext*, RDPluginModule* pm)
{
    RD_PLUGIN_ENTRY(RDEntryAssembler, chip8asm, "CHIP-8 Assembler");
    chip8asm.renderinstruction = &CHIP8::renderInstruction;
    chip8asm.emulate = &CHIP8::emulate;
    chip8asm.bits = 16;

    RD_PLUGIN_ENTRY(RDEntryLoader, chip8ldr, "CHIP-8");
    chip8ldr.test = &test;
    chip8ldr.load = &load;

    RDAssembler_Register(pm, &chip8asm);
    RDLoader_Register(pm, &chip8ldr);
}
