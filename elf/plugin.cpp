#include "elf.h"
#include <redasm/context.h>

REDASM_LOADER("ELF", "Dax", "MIT", 1)

REDASM_LOAD
{
    elf.plugin = new ElfLoader();
    return true;
}

REDASM_UNLOAD { elf.plugin->release(); }
