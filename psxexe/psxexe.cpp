#include "psxexe.h"

PsxExeLoader::PsxExeLoader(): Loader() { }
AssemblerRequest PsxExeLoader::assembler() const { return ASSEMBLER_REQUEST("mips", "mips32le"); }

bool PsxExeLoader::test(const LoadRequest &request) const
{
    const auto* header = request.pointer<PsxExeHeader>();
    return !strncmp(header->id, PSXEXE_SIGNATURE, PSXEXE_SIGNATURE_SIZE);
}

void PsxExeLoader::load()
{
    this->signature("psyq");

    auto* header = this->pointer<PsxExeHeader>();

    if(header->t_addr > PSX_USER_RAM_START)
        ldrdoc->segment("RAM0", 0, PSX_USER_RAM_START, (header->t_addr - PSX_USER_RAM_START), Segment::T_Bss);

    ldrdoc->segment("TEXT", PSXEXE_TEXT_OFFSET, header->t_addr, header->t_size, Segment::T_Code | Segment::T_Data);

    if((header->t_addr + header->t_size) < PSX_USER_RAM_END)
        ldrdoc->segment("RAM1", 0, header->t_addr + header->t_size, PSX_USER_RAM_END - (header->t_addr + header->t_size), Segment::T_Bss);

    ldrdoc->entry(header->pc0);
}

REDASM_LOADER("PS-X Executable", "Dax", "MIT", 1)
REDASM_LOAD { psxexe.plugin = new PsxExeLoader(); return true; }
REDASM_UNLOAD { psxexe.plugin->release(); }
