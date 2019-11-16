#include "chip8.h"
#include <redasm/support/path.h>

Chip8Loader::Chip8Loader(): Loader() { }
AssemblerRequest Chip8Loader::assembler() const { return "chip8asm"; }

bool Chip8Loader::test(const LoadRequest &request) const
{
    if(Path::extIs(request.filePath(), "chip8"))
        return true;

    if(Path::extIs(request.filePath(), "ch8"))
        return true;

    if(Path::extIs(request.filePath(), "rom"))
        return true;

    return false;
}

void Chip8Loader::load()
{
    ldrdoc->segment("MEMORY", 0, 0x200, 0x1000, SegmentType::Code | SegmentType::Data);
    ldrdoc->entry(0x200);
}
