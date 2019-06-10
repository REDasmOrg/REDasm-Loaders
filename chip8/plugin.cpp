#include "assembler/chip8.h"
#include "loader/chip8.h"

REDASM_ASSEMBLER_ID(chip8asm, "CHIP-8 Assembler", "Dax", "MIT", 1)
REDASM_LOADER("CHIP-8 ROM", "Dax", "MIT", 1)

REDASM_LOAD
{
    chip8asm.plugin = new Chip8Assembler();
    chip8.plugin = new Chip8Loader();
    return true;
}

REDASM_UNLOAD
{
    chip8asm.plugin->release();
    chip8.plugin->release();
}
