#include "assembler/dalvik.h"
#include "loader/dex.h"

REDASM_ASSEMBLER_ID(dalvik, "Dalvik Assembler", "Dax", "MIT", 1)
REDASM_LOADER("Dalvik Executable", "Dax", "MIT", 1)

REDASM_LOAD
{
    dalvik.plugin = new DalvikAssembler();
    dex.plugin = new DexLoader();
    return true;
}

REDASM_UNLOAD
{
    dalvik.plugin->release();
    dex.plugin->release();
}
