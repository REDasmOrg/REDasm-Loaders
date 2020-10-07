#include "elf_analyzer_x86.h"
#include "../elf_abi.h"
#include "../elf.h"
#include <cstring>

ElfAnalyzerX86::ElfAnalyzerX86(ElfLoader* loader, RDContext* ctx): ElfAnalyzer(loader, ctx) { }

void ElfAnalyzerX86::analyze()
{
    // const u8* pltbase = m_loader->plt();
    // if(!pltbase) return;

    // RDAssembler* assembler = RDDisassembler_GetAssembler(m_disassembler);
    // RDDocument* doc = RDContext_GetDocument(m_disassembler);
    // size_t c = RDDocument_FunctionsCount(doc);
    // RDSegment segment{};

    // for(size_t i = 0; i < c; i++)
    // {
    //     auto loc = RDDocument_GetFunctionAt(doc, i);
    //     if(!loc.valid) continue;

    //     if(!RDSegment_ContainsAddress(std::addressof(segment), loc.address))
    //     {
    //         if(!RDDocument_GetSegmentAddress(doc, loc.address, &segment))
    //             continue;
    //     }

    //     if(std::strcmp(".plt", segment.name)) continue;

    //     InstructionLock instruction(doc, loc);
    //     if(!instruction) continue;

    //     switch(m_loader->machine())
    //     {
    //         case EM_386: this->checkPLT_x86(doc, *instruction, assembler, loc.address); break;
    //         case EM_X86_64: this->checkPLT_x86_64(doc, *instruction, assembler, loc.address); break;
    //         default: rd_log("Unhandled machine '" + rd_tohex(m_loader->machine()) + "'"); break;
    //     }
    // }
}

// void ElfAnalyzerX86::checkPLT_x86_64(RDDocument* doc, RDInstruction* instruction, RDAssembler* assembler, rd_address funcaddress)
// {
// }
//
// void ElfAnalyzerX86::checkPLT_x86(RDDocument* doc, RDInstruction* instruction, RDAssembler* assembler, rd_address funcaddress)
// {
    // // Handle PIE PLT case
    // if(!IS_TYPE(instruction, InstructionType_Jump)) return;

    // const RDOperand& op = instruction->operands[0];
    // if(!IS_TYPE(&op, OperandType_Displacement)) return;

    // std::string regname = RDAssembler_RegisterName(assembler, instruction, &op, op.base);
    // if(regname != "ebx") return;

    // auto name = m_loader->abi()->plt(op.displacement);
    // if(name) RDDocument_AddFunction(doc, funcaddress, RD_Trampoline(name->c_str()));
//}
