#include "chip8_printer.h"
#include "chip8_registers.h"

Chip8Printer::Chip8Printer(Disassembler *disassembler): Printer(disassembler) { }

String Chip8Printer::reg(const RegisterOperand &regop) const
{
    if(regop.tag == CHIP8_REG_I)
        return "i";

    if(regop.tag == CHIP8_REG_DT)
        return "dt";

    if(regop.tag == CHIP8_REG_ST)
        return "st";

    return ((regop.tag == CHIP8_REG_K) ? "k" : "v") + String::hex(regop.r);
}
