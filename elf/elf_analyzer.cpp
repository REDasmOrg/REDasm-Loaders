#include "elf_analyzer.h"
#include "elf.h"
#include <cstring>

#define LIBC_START_MAIN        "__libc_start_main"
#define LIBC_START_MAIN_ARGC   7

ElfAnalyzer::ElfAnalyzer(RDLoaderPlugin* plugin, RDDisassembler* disassembler): m_disassembler(disassembler), m_plugin(plugin)
{
    m_loader = reinterpret_cast<ElfLoader*>(plugin->p_data);
}

void ElfAnalyzer::analyze()
{
    RDSymbol symlibcmain;

    if(this->getLibStartMain(&symlibcmain))
    {
        RDAssemblerPlugin* assembler = RDDisassembler_GetAssembler(m_disassembler);

        if(!std::string(assembler->id).find("x86")) this->findMain_x86(assembler, &symlibcmain);
        else rd_log("Unhandled architecture '" + std::string(assembler->name) + "'");
    }

    RDDocument* doc = RDDisassembler_GetDocument(m_disassembler);
    RDSymbol symbol;

    if(RDDocument_GetSymbolByName(doc, "main", &symbol))
        RDDocument_SetEntry(doc, symbol.address);
}

void ElfAnalyzer::findMain_x86(RDAssemblerPlugin* assembler, const RDSymbol *symlibcmain)
{
    const address_t* refs = nullptr;
    size_t c = RDDisassembler_GetReferences(m_disassembler, symlibcmain->address, &refs);

    if(!c) return;
    if(c > 1) rd_problem("'" + std::string(LIBC_START_MAIN) + "' contains " + std::to_string(c) + "references");

    const RDBlockContainer* blocks = RDDisassembler_GetBlocks(m_disassembler);

    RDBlock block;
    if(!RDBlockContainer_Find(blocks, refs[0], &block)) return;

    size_t index = RDBlockContainer_Index(blocks, &block);
    if(index == RD_NPOS) return;

    if(!std::strcmp(assembler->id, "x86_64")) this->findMainMode_x86_64(index);
    else if(!std::strcmp(assembler->id, "x86_32")) this->findMainMode_x86_32(index);
    this->disassembleLibStartMain();
}

void ElfAnalyzer::findMainMode_x86_32(size_t blockidx)
{
    const RDBlockContainer* blocks = RDDisassembler_GetBlocks(m_disassembler);
    RDDocument* doc = RDDisassembler_GetDocument(m_disassembler);
    RDBlock block;

    for(size_t i = 0; RDBlockContainer_Get(blocks, blockidx, &block) && (i < LIBC_START_MAIN_ARGC); blockidx--)
    {
        if(!IS_TYPE(&block, BlockType_Code)) break;

        InstructionLock instruction(doc, block.address);
        if(!instruction || !IS_TYPE(*instruction, InstructionType_Push)) continue;

        const RDOperand* op = &instruction->operands[0];

        if(IS_TYPE(op, OperandType_Immediate))
        {
            if(i == 0) m_libcmain["main"] = op->u_value;
            else if(i == 3) m_libcmain["init"] = op->u_value;
            else if(i == 4)
            {
                m_libcmain["fini"] = op->u_value;
                break;
            }
        }

        i++;
    }
}

void ElfAnalyzer::findMainMode_x86_64(size_t blockidx)
{
    const RDBlockContainer* blocks = RDDisassembler_GetBlocks(m_disassembler);
    RDDocument* doc = RDDisassembler_GetDocument(m_disassembler);
    RDBlock block;

    for(; blockidx && RDBlockContainer_Get(blocks, blockidx, &block); blockidx--)
    {
        if(!IS_TYPE(&block, BlockType_Code)) break;

        InstructionLock instruction(doc, block.address);
        if(!instruction || !RDInstruction_MnemonicIs(*instruction, "mov")) continue;

        const RDOperand* op1 = &instruction->operands[0];
        const RDOperand* op2 = &instruction->operands[1];

        if(!IS_TYPE(op1, OperandType_Register) || !IS_TYPE(op2, OperandType_Immediate)) continue;
        const char* regname = RDDisassembler_RegisterName(m_disassembler, *instruction, op1->reg);

        if(!std::strcmp(regname, "rdi")) m_libcmain["main"] = op2->u_value;
        else if(!std::strcmp(regname, "rcx")) m_libcmain["init"] = op2->u_value;
        else if(!std::strcmp(regname, "r8"))
        {
            m_libcmain["fini"] = op2->u_value;
            break;
        }
    }
}

void ElfAnalyzer::disassembleLibStartMain()
{
    RDDocument* doc = RDDisassembler_GetDocument(m_disassembler);

    for(auto& it : m_libcmain)
    {
        RDDocument_AddFunction(doc, it.second, it.first.c_str());
        RDDisassembler_Enqueue(m_disassembler, it.second);
    }

    m_libcmain.clear();
}

bool ElfAnalyzer::getLibStartMain(RDSymbol* symbol)
{
    RDDocument* doc = RDDisassembler_GetDocument(m_disassembler);
    if(RDDocument_GetSymbolByName(doc, RD_Trampoline(LIBC_START_MAIN), symbol)) return true;
    return RDDocument_GetSymbolByName(doc, LIBC_START_MAIN, symbol);
}
