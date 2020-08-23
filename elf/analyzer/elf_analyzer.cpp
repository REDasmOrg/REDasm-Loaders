#include "elf_analyzer.h"
#include "elf.h"

#define LIBC_START_MAIN        "__libc_start_main"
#define LIBC_START_MAIN_ARGC   7

ElfAnalyzer::ElfAnalyzer(ElfLoader* loader, RDDisassembler* disassembler): m_disassembler(disassembler), m_loader(loader) { }

void ElfAnalyzer::analyze()
{
    // RDSymbol symlibcmain;
    // RDDocument* doc = RDDisassembler_GetDocument(m_disassembler);

    // if(this->getLibStartMain(&symlibcmain))
    // {
    //     std::string id = RDDisassembler_GetAssemblerId(m_disassembler);
    //     if(!id.find("x86")) this->findMain_x86(id, doc, &symlibcmain);
    // }

    // RDSymbol symbol;

    // if(RDDocument_GetSymbolByName(doc, "main", &symbol))
    //     RDDocument_SetEntry(doc, symbol.address);
}

void ElfAnalyzer::findMain_x86(const std::string& id, RDDocument* doc, const RDSymbol *symlibcmain)
{
    // const rd_address* refs = nullptr;
    // size_t c = RDDisassembler_GetReferences(m_disassembler, symlibcmain->address, &refs);

    // if(!c) return;
    // if(c > 1) rd_problem("'" + std::string(LIBC_START_MAIN) + "' contains " + std::to_string(c) + "references");

    // const RDBlockContainer* blocks = RDDocument_GetBlocks(doc, symlibcmain->address);

    // RDBlock block;
    // if(!RDBlockContainer_Find(blocks, refs[0], &block)) return;

    // size_t index = RDBlockContainer_Index(blocks, &block);
    // if(index == RD_NPOS) return;

    // if(id == "x86_64") this->findMainMode_x86_64(doc, blocks, index);
    // else if(id == "x86_32") this->findMainMode_x86_32(doc, blocks, index);
    // this->disassembleLibStartMain();
}

void ElfAnalyzer::findMainMode_x86_32(RDDocument* doc, const RDBlockContainer* blocks, size_t blockidx)
{
    // RDBlock block;

    // for(size_t i = 0; RDBlockContainer_Get(blocks, blockidx, &block) && (i < LIBC_START_MAIN_ARGC); blockidx--)
    // {
    //     if(!IS_TYPE(&block, BlockType_Code)) break;

    //     InstructionLock instruction(doc, block.address);
    //     if(!instruction || !IS_TYPE(*instruction, InstructionType_Push)) continue;

    //     const RDOperand* op = &instruction->operands[0];

    //     if(IS_TYPE(op, OperandType_Immediate))
    //     {
    //         if(i == 0) m_libcmain["main"] = op->u_value;
    //         else if(i == 3) m_libcmain["init"] = op->u_value;
    //         else if(i == 4)
    //         {
    //             m_libcmain["fini"] = op->u_value;
    //             break;
    //         }
    //     }

    //     i++;
    // }
}

void ElfAnalyzer::findMainMode_x86_64(RDDocument* doc, const RDBlockContainer* blocks, size_t blockidx)
{
    // RDBlock block;

    // for(; blockidx && RDBlockContainer_Get(blocks, blockidx, &block); blockidx--)
    // {
    //     if(!IS_TYPE(&block, BlockType_Code)) break;

    //     InstructionLock instruction(doc, block.address);
    //     if(!instruction || !RDInstruction_MnemonicIs(*instruction, "mov")) continue;

    //     const RDOperand* op1 = &instruction->operands[0];
    //     const RDOperand* op2 = &instruction->operands[1];

    //     if(!IS_TYPE(op1, OperandType_Register) || !IS_TYPE(op2, OperandType_Immediate)) continue;
    //     std::string regname = RDDisassembler_RegisterName(m_disassembler, *instruction, op1, op1->reg);

    //     if(regname == "rdi") m_libcmain["main"] = op2->u_value;
    //     else if(regname == "rcx") m_libcmain["init"] = op2->u_value;
    //     else if(regname == "r8")
    //     {
    //         m_libcmain["fini"] = op2->u_value;
    //         break;
    //     }
    // }
}

void ElfAnalyzer::disassembleLibStartMain()
{
    // RDDocument* doc = RDDisassembler_GetDocument(m_disassembler);

    // for(auto& it : m_libcmain)
    // {
    //     RDDocument_AddFunction(doc, it.second, it.first.c_str());
    //     RDDisassembler_Enqueue(m_disassembler, it.second);
    // }

    // m_libcmain.clear();
}

bool ElfAnalyzer::getLibStartMain(RDSymbol* symbol)
{
    RDDocument* doc = RDDisassembler_GetDocument(m_disassembler);
    if(RDDocument_GetSymbolByName(doc, RD_Thunk(LIBC_START_MAIN), symbol)) return true;
    return RDDocument_GetSymbolByName(doc, LIBC_START_MAIN, symbol);
}
