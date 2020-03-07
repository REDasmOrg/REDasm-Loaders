#include "chip8.h"
#include <redasm/support/filesystem.h>

Chip8Loader::Chip8Loader(): Loader() { }
AssemblerRequest Chip8Loader::assembler() const { return "chip8asm"; }

bool Chip8Loader::test(const LoadRequest &request) const
{
    if(FS::Path(request.filePath()).ext() == ".chip8") return true;
    if(FS::Path(request.filePath()).ext() == ".ch8") return true;
    if(FS::Path(request.filePath()).ext() == "rom") return true;
    return false;
}

void Chip8Loader::load()
{
    ldrdoc->segment("MEMORY", 0, 0x200, 0x1000, Segment::T_Code | Segment::T_Data);
    ldrdoc->entry(0x200);
}
