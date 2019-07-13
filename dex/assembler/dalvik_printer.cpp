#include "dalvik_printer.h"
#include "dalvik_metadata.h"
#include <redasm/disassembler/disassembler.h>
#include <redasm/support/utils.h>
#include <redasm/redasm.h>
#include "../loader/dex_constants.h"
#include "../loader/dex.h"
#include "dalvik.h"

#define HAS_ACCESS_FLAGS(dexmethod, ac) static_cast<DexAccessFlags>(dexmethod.access_flags) & ac

DalvikPrinter::DalvikPrinter(): Printer() { }

void DalvikPrinter::function(const Symbol *symbol, const Printer::FunctionCallback& headerfunc)
{
    auto* dexloader = dynamic_cast<DexLoader*>(r_ldr);

    if(!dexloader)
    {
        Printer::function(symbol, headerfunc);
        return;
    }

    DexEncodedMethod dexmethod;
    String access;

    if(dexloader->getMethodInfo(symbol->tag, dexmethod))
    {
        if(HAS_ACCESS_FLAGS(dexmethod, DexAccessFlags::Public))
            access += access.empty() ? "public" : " public";

        if(HAS_ACCESS_FLAGS(dexmethod, DexAccessFlags::Protected))
            access += access.empty() ? "protected" : " protected";

        if(HAS_ACCESS_FLAGS(dexmethod, DexAccessFlags::Private))
            access += access.empty() ? "private" : " private";

        if(HAS_ACCESS_FLAGS(dexmethod, DexAccessFlags::Static))
            access += access.empty() ? "static" : " static";

        if(!access.empty())
            access += " ";
    }

    headerfunc(access + dexloader->getReturnType(symbol->tag) + " ",
               symbol->name, dexloader->getParameters(symbol->tag));
}

String DalvikPrinter::reg(const RegisterOperand &regop) const
{
    String s = DalvikAssembler::registerName(regop.r);

    if(regop.tag & DalvikOperands::ParameterFirst)
        s = "{" + s;

    if(regop.tag & DalvikOperands::ParameterLast)
        s += "}";

    return s;
}

String DalvikPrinter::imm(const Operand *op) const
{
    DexLoader* dexloader = nullptr;

    if(op->tag && (dexloader = dynamic_cast<DexLoader*>(r_ldr)))
    {
        switch(op->tag)
        {
            case DalvikOperands::StringIndex:
                return dexloader->getString(op->u_value).quoted();

            case DalvikOperands::TypeIndex:
                return dexloader->getType(op->u_value);

            case DalvikOperands::MethodIndex:
                return dexloader->getMethodProto(op->u_value);

            case DalvikOperands::FieldIndex:
                return dexloader->getField(op->u_value);

            default:
                break;
        }
    }

    return Printer::imm(op);
}
