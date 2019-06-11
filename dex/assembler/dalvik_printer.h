#pragma once

#include <redasm/plugins/assembler/printer/printer.h>
#include "../loader/dex_header.h"

using namespace REDasm;

class DEXLoader;

class DalvikPrinter : public Printer
{
    public:
        DalvikPrinter(Disassembler* disassembler);
        void function(const Symbol* symbol, const FunctionCallback &plgfunc) override;
        std::string reg(const RegisterOperand &regop) const override;
        std::string imm(const Operand *op) const override;
};
