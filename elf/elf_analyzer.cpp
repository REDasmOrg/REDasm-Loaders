#include "elf_analyzer.h"
#include <redasm/plugins/assembler/assembler.h>
#include <redasm/support/utils.h>
#include <capstone/capstone.h>

#define LIBC_START_MAIN        "__libc_start_main"
#define LIBC_START_MAIN_ARGC   7

ElfAnalyzer::ElfAnalyzer(): Analyzer() { }

void ElfAnalyzer::analyze()
{
    Analyzer::analyze();
    Symbol* symbol = r_doc->symbol("main");

    if(!symbol)
    {
        Assembler* assembler = r_asm;
        Symbol* symlibcmain = this->getLibStartMain();

        if(symlibcmain)
        {
            if(assembler->id().startsWith("x86"))
                this->findMain_x86(symlibcmain);
            else
                r_ctx->log("Unhandled architecture " + assembler->description().quoted());

            symbol = r_doc->symbol("main");
        }
    }

    if(symbol)
        r_doc->setDocumentEntry(symbol->address);
    else
        r_ctx->problem("Cannot find 'main' symbol");
}

void ElfAnalyzer::findMain_x86(const Symbol *symlibcmain)
{
    SortedSet refs = r_disasm->getReferences(symlibcmain->address);

    if(refs.size() > 1)
        r_ctx->problem(String(LIBC_START_MAIN).quoted() + " contains " + String::number(refs.size()) + " reference(s)");

    size_t index = r_doc->findInstruction(refs.first().toU64());

    if(index == REDasm::npos)
        return;

    if(r_asm->id() == "x86_64")
        this->findMain_x86_64(index);
    else
        this->findMain_x86(index);

    this->disassembleLibStartMain();
}

void ElfAnalyzer::findMain_x86(size_t index)
{
    for(size_t i = 0; (index < r_doc->size()) && (i < LIBC_START_MAIN_ARGC); index--)
    {
        const ListingItem* item = r_doc->itemAt(index);

        if(!item)
            break;

        if(item->is(ListingItemType::InstructionItem))
        {
            CachedInstruction instruction = r_doc->instruction(item->address());

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

void ElfAnalyzer::findMain_x86_64(size_t index)
{
    for(; index < r_doc->size(); index--)
    {
        const ListingItem* item = r_doc->itemAt(index);

        if(item->is(ListingItemType::InstructionItem))
        {
            CachedInstruction instruction = r_doc->instruction(item->address());

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
        r_doc->lock(it.second, it.first, SymbolType::Function);
        r_disasm->disassemble(it.second);
    }

    m_libcmain.clear();
}

Symbol* ElfAnalyzer::getLibStartMain()
{
    Symbol* symlibcmain = r_doc->symbol(Utils::trampoline(LIBC_START_MAIN));

    if(!symlibcmain)
        symlibcmain = r_doc->symbol(LIBC_START_MAIN);

    return symlibcmain;
}
