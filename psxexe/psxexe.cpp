#include "psxexe.h"

PsxExeLoader::PsxExeLoader(): Loader() { }
AssemblerRequest PsxExeLoader::assembler() const { return {"mips", "mips32le"}; }

bool PsxExeLoader::test(const LoadRequest &request) const
{
    const auto* header = request.pointer<PsxExeHeader>();
    return !strncmp(header->id, PSXEXE_SIGNATURE, PSXEXE_SIGNATURE_SIZE);
}

void PsxExeLoader::load()
{
    auto* header = this->pointer<PsxExeHeader>();

    if(header->t_addr > PSX_USER_RAM_START)
        this->document()->segment("RAM0", 0, PSX_USER_RAM_START, (header->t_addr - PSX_USER_RAM_START), SegmentType::Bss);

    this->document()->segment("TEXT", PSXEXE_TEXT_OFFSET, header->t_addr, header->t_size, SegmentType::Code | SegmentType::Data);

    if((header->t_addr + header->t_size) < PSX_USER_RAM_END)
        this->document()->segment("RAM1", 0, header->t_addr + header->t_size, PSX_USER_RAM_END - (header->t_addr + header->t_size), SegmentType::Bss);

    this->document()->entry(header->pc0);
}

REDASM_LOADER("PS-X Executable", "Dax", "MIT", 1)
REDASM_LOAD { psxexe.plugin = new PsxExeLoader(); return true; }
REDASM_UNLOAD { psxexe.plugin->release(); }
