#include "assembler/chip8.h"
#include <rdapi/rdapi.h>
#include <filesystem>
#include <sstream>

static Chip8Assembler chip8assembler;

static bool decode(const RDAssemblerPlugin*, RDBufferView* view, RDInstruction* instruction) { return chip8assembler.decode(view, instruction); }
static void emulate(const RDAssemblerPlugin*, RDDisassembler* disassembler, const RDInstruction* instruction) { return chip8assembler.emulate(disassembler, instruction); }

static bool render(const RDAssemblerPlugin*, RDRenderItemParams* rip)
{
    if(!IS_TYPE(rip, RendererItemType_Operand)) return false;
    if(!IS_TYPE(rip->operand, OperandType_Register)) return false;

    if(rip->operand->u_data == CHIP8_REG_I)
        RDRendererItem_Push(rip->rendereritem, "i", "register_fg", nullptr);
    else if(rip->operand->u_data == CHIP8_REG_DT)
        RDRendererItem_Push(rip->rendereritem, "dt", "register_fg", nullptr);
    else if(rip->operand->u_data == CHIP8_REG_ST)
        RDRendererItem_Push(rip->rendereritem, "st", "register_fg", nullptr);
    else
    {
        std::stringstream ss;
        ss << ((rip->operand->u_data == CHIP8_REG_K) ? "k" : "v") << std::hex << rip->operand->reg;
        RDRendererItem_Push(rip->rendereritem, ss.str().c_str(), "register_fg", nullptr);
    }

    return true;
}

RDAssemblerPlugin* test(const RDLoaderPlugin*, const RDLoaderRequest* request)
{
    std::string ext = std::filesystem::path(request->filepath).extension();
    if((ext == ".chip8") || (ext == ".ch8") || (ext == ".rom")) return RDAssembler_Find("chip8asm");
    return nullptr;
}

static void load(RDLoaderPlugin*, RDLoader* loader)
{
    RDDocument* doc = RDLoader_GetDocument(loader);
    RDDocument_AddSegment(doc, "MEMORY", 0, 0x200, 0x1000, SegmentFlags_CodeData);
    RDDocument_SetEntry(doc, 0x200);
}

void redasm_entry()
{
    RD_PLUGIN_CREATE(RDAssemblerPlugin, chip8asm, "CHIP-8 Assembler");
    chip8asm.bits = 16;
    chip8asm.decode = &decode;
    chip8asm.emulate = &emulate;
    chip8asm.render = &render;

    RD_PLUGIN_CREATE(RDLoaderPlugin, chip8ldr, "CHIP-8");
    chip8ldr.test = &test;
    chip8ldr.load = &load;

    RDAssembler_Register(&chip8asm);
    RDLoader_Register(&chip8ldr);
}
