#include "elf_analyzer.h"
#include <redasm/plugins/assembler/assembler.h>
#include <redasm/support/utils.h>
#include <capstone/capstone.h>

#define LIBC_START_MAIN        "__libc_start_main"
#define LIBC_START_MAIN_ARGC   7

ElfAnalyzer::ElfAnalyzer(Disassembler *disassembler): Analyzer(disassembler) { }

void ElfAnalyzer::analyze()
{
    Analyzer::analyze();
    Symbol* symbol = this->document()->symbol("main");

    if(!symbol)
    {
        Assembler* assembler = this->disassembler()->assembler();
        Symbol* symlibcmain = this->getLibStartMain();

        if(symlibcmain)
        {
            if(assembler->id().startsWith("x86"))
                this->findMain_x86(symlibcmain);
            else
                r_ctx->log("Unhandled architecture " + assembler->description().quoted());

            symbol = this->document()->symbol("main");
        }
    }

    if(symbol)
        this->document()->setDocumentEntry(symbol->address);
    else
        r_ctx->problem("Cannot find 'main' symbol");
}

void ElfAnalyzer::findMain_x86(const Symbol *symlibcmain)
{
    ReferenceVector refs = this->disassembler()->getReferences(symlibcmain->address);

    if(refs.size() > 1)
        r_ctx->log(String(LIBC_START_MAIN).quoted() + " contains " + String::number(refs.size()) + " reference(s)");


    ListingDocumentIterator it(this->document(), refs.front(), ListingItemType::InstructionItem);

    if(!it.hasNext())
        return;

    if(this->disassembler()->assembler()->id() == "x86_64")
        this->findMain_x86_64(it);
    else
        this->findMain_x86(it);

    this->disassembleLibStartMain();
}

void ElfAnalyzer::findMain_x86(ListingDocumentIterator &it)
{
    const ListingItem* item = it.current();

    for(int i = 0; it.hasPrevious() && (i < LIBC_START_MAIN_ARGC); item = it.prev())
    {
        if(item->is(ListingItemType::InstructionItem))
        {
            CachedInstruction instruction = this->document()->instruction(item->address());

            if(instruction->is(InstructionType::Push))
            {
                const Operand* op = instruction->op(0);

                if(op->isNumeric())
                {
                    if(i == 0)
                        m_libcmain["main"] = op->u_value;
                    else if(i == 3)
                        m_libcmain["init"] = op->u_value;
                    else if(i == 4)
                    {
                        m_libcmain["fini"] = op->u_value;
                        break;
                    }
                }

                i++;
            }
        }
    }
}

void ElfAnalyzer::findMain_x86_64(ListingDocumentIterator& it)
{
    while(it.hasPrevious())
    {
        const ListingItem* item = it.prev();

        if(item->is(ListingItemType::InstructionItem))
        {
            CachedInstruction instruction = this->document()->instruction(item->address());

            if(instruction->is(InstructionType::Load))
            {
                const Operand* op1 = instruction->op(0);
                const Operand* op2 = instruction->op(1);

                if(!op1->is(OperandType::Register) || !op2->isNumeric())
                    continue;

                if(op1->reg.r == X86_REG_RDI)
                    m_libcmain["main"] = op2->u_value;
                else if(op1->reg.r == X86_REG_RCX)
                    m_libcmain["init"] = op2->u_value;
                else if(op1->reg.r == X86_REG_R8)
                {
                    m_libcmain["fini"] = op2->u_value;
                    break;
                }
            }
        }
    }
}

void ElfAnalyzer::disassembleLibStartMain()
{
    for(auto& it : m_libcmain)
    {
        this->document()->lock(it.second, it.first, SymbolType::Function);
        this->disassembler()->disassemble(it.second);
    }

    m_libcmain.clear();
}

Symbol* ElfAnalyzer::getLibStartMain()
{
    Symbol* symlibcmain = this->document()->symbol(Utils::trampoline(LIBC_START_MAIN));

    if(!symlibcmain)
        symlibcmain = this->document()->symbol(LIBC_START_MAIN);

    return symlibcmain;
}
