#include "elf_analyzer_x86.h"
#include "../elf_abi.h"
#include "../elf.h"
#include <unordered_map>
#include <array>

ElfAnalyzerX86::ElfAnalyzerX86(RDContext* ctx): ElfAnalyzer(ctx) { }

void ElfAnalyzerX86::analyze()
{
    ElfAnalyzer::analyze();

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

void ElfAnalyzerX86::findMain(rd_address address)
{
    switch(RDContext_GetBits(m_context))
    {
        case 32: this->findMain32(address); break;
        case 64: this->findMain64(address); break;
        default: rd_log("Unsupported bits count, cannot find main"); break;
    }
}

void ElfAnalyzerX86::findMain32(rd_address address)
{
    static const std::array<const char*, 3> EP_NAMES = { "fini", "init", "main" };

    rd_ptr<RDILFunction> f(RDILFunction_Create(m_context, address));
    std::vector<rd_address> pushcexpr;

    for(size_t i = 0; i < RDILFunction_Size(f.get()); i++)
    {
        const auto* e = RDILFunction_GetExpression(f.get(), i);
        if(!RDILExpression_Match(e, "push(c)")) continue;

        auto* ve = RDILExpression_GetU(e);

        RDILValue v;
        if(!RDILExpression_GetValue(ve, &v)) continue;
        pushcexpr.push_back(v.address);
    }

    if(pushcexpr.size() != EP_NAMES.size())
    {
        rd_log("Cannot find main, got " + std::to_string(pushcexpr.size()) + " constant push");
        return;
    }

    auto* disassembler = RDContext_GetDisassembler(m_context);
    for(size_t i = 0; i < EP_NAMES.size(); i++) RDDisassembler_ScheduleFunction(disassembler, pushcexpr[i], EP_NAMES[i]);
}

void ElfAnalyzerX86::findMain64(rd_address address)
{
    static const std::unordered_map<std::string, const char*> EP_NAMES = {
        { "rdi", "main" },
        { "rcx", "init" },
        { "r8", "fini" },
    };

    rd_ptr<RDILFunction> f(RDILFunction_Create(m_context, address));
    std::unordered_map<std::string, rd_address> assignexpr;

    for(size_t i = 0; i < RDILFunction_Size(f.get()); i++)
    {
        const auto* e = RDILFunction_GetExpression(f.get(), i);
        if(!RDILExpression_Match(e, "r = c")) continue;

        const auto* rege = RDILExpression_GetLeft(e);
        const auto* cnste = RDILExpression_GetRight(e);

        RDILValue regv, cnstv;
        if(!RDILExpression_GetValue(rege, &regv)) continue;
        if(!RDILExpression_GetValue(cnste, &cnstv)) continue;
        assignexpr[regv.reg] = cnstv.address;
    }

    if(assignexpr.size() != EP_NAMES.size())
    {
        rd_log("Cannot find main, got " + std::to_string(assignexpr.size()) + " assignments");
        return;
    }

    auto* disassembler = RDContext_GetDisassembler(m_context);
    for(const auto& [regname, address] : assignexpr) RDDisassembler_ScheduleFunction(disassembler, address, EP_NAMES.at(regname));
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
