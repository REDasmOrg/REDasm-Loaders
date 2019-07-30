#include "elf.h"
#include <redasm/context.h>

REDASM_LOADER("ELF Executable", "Dax", "MIT", 1)

REDASM_LOAD
{
    elf.plugin = new ElfLoader();
    return true;
}

REDASM_UNLOAD { elf.plugin->release(); }
