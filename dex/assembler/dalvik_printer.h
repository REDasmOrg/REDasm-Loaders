#pragma once

#include <redasm/plugins/assembler/printer/printer.h>
#include "../loader/dex_header.h"

using namespace REDasm;

class DEXLoader;

class DalvikPrinter : public Printer
{
    public:
        DalvikPrinter();
        void function(const Symbol* symbol, const FunctionCallback &plgfunc) override;
        String reg(const RegisterOperand &regop) const override;
        String imm(const Operand *op) const override;
};
