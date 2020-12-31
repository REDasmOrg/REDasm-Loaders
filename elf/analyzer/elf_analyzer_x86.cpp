#include "elf_analyzer_x86.h"
#include "../elf_abi.h"
#include "../elf.h"
#include <unordered_map>
#include <cstring>
#include <string>
#include <array>

ElfAnalyzerX86::ElfAnalyzerX86(RDContext* ctx): ElfAnalyzer(ctx) { }

void ElfAnalyzerX86::analyze()
{
    ElfAnalyzer::analyze();
    this->parsePlt();
}

void ElfAnalyzerX86::findMain(rd_address address)
{
    switch(m_loader->machine())
    {
        case EM_386: this->findMain32(address); break;
        case EM_X86_64: this->findMain64(address); break;
        default: rd_log("Unsupported machine: " + rd_tohex(m_loader->machine()) + ", cannot find main"); break;
    }
}

void ElfAnalyzerX86::parsePlt()
{
    const u8* pltbase = m_loader->plt();
    if(!pltbase) return;

    m_document = RDContext_GetDocument(m_context);

    RDDocument_EachFunction(m_document, [](rd_address address, void* userdata) {
        auto* thethis = reinterpret_cast<ElfAnalyzerX86*>(userdata);

        if(!RDSegment_ContainsAddress(&thethis->m_segment, address))
        {
            if(!RDDocument_GetSegmentAddress(thethis->m_document, address, &thethis->m_segment))
                return true;
        }

        if(std::strcmp(".plt", thethis->m_segment.name)) return true;

        switch(thethis->m_loader->machine())
        {
            case EM_386: thethis->checkPLT32(thethis->m_document, address); break;
            case EM_X86_64: thethis->checkPLT64(thethis->m_document, address); break;
            default: rd_log("Unhandled machine '" + rd_tohex(thethis->m_loader->machine()) + "'"); break;
        }

        return true;
    }, this);
}

void ElfAnalyzerX86::findMain32(rd_address address)
{
    static const std::array<const char*, 3> EP_NAMES = { "fini", "init", "main" };

    rd_ptr<RDILFunction> il(RDILFunction_Create(m_context, address));
    if(!il) return;
    std::vector<rd_address> pushcexpr;

    for(size_t i = 0; i < RDILFunction_Size(il.get()); i++)
    {
        const auto* e = RDILFunction_GetExpression(il.get(), i);
        if(!RDILExpression_Match(e, "push(cnst)")) continue;

        const auto* ve = RDILExpression_Extract(e, "u:cnst");

        RDILValue v;
        if(!RDILExpression_GetValue(ve, &v)) continue;
        pushcexpr.push_back(v.address);
    }

    if(pushcexpr.size() != EP_NAMES.size())
    {
        rd_log("Cannot find main, got " + std::to_string(pushcexpr.size()) + " constant push");
        return;
    }

    for(size_t i = 0; i < EP_NAMES.size(); i++) RDContext_DisassembleFunction(m_context, pushcexpr[i], EP_NAMES[i]);
}

void ElfAnalyzerX86::findMain64(rd_address address)
{
    static const std::unordered_map<std::string, const char*> EP_NAMES = {
        { "rdi", "main" },
        { "rcx", "init" },
        { "r8", "fini" },
    };

    rd_ptr<RDILFunction> il(RDILFunction_Create(m_context, address));
    if(!il) return;
    std::unordered_map<std::string, rd_address> assignexpr;

    for(size_t i = 0; i < RDILFunction_Size(il.get()); i++)
    {
        const auto* e = RDILFunction_GetExpression(il.get(), i);
        if(!RDILExpression_Match(e, "reg = cnst")) continue;

        const auto* regel = RDILExpression_Extract(e, "left:reg");
        const auto* cnstel = RDILExpression_Extract(e, "right:cnst");

        RDILValue regv, cnstv;
        if(!RDILExpression_GetValue(regel, &regv)) continue;
        if(!RDILExpression_GetValue(cnstel, &cnstv)) continue;
        assignexpr[regv.reg] = cnstv.address;
    }

    if(assignexpr.size() != EP_NAMES.size())
    {
        rd_log("Cannot find main, got " + std::to_string(assignexpr.size()) + " assignments");
        return;
    }

    for(const auto& [regname, address] : assignexpr) RDContext_DisassembleFunction(m_context, address, EP_NAMES.at(regname));
}

void ElfAnalyzerX86::checkPLT32(RDDocument* doc, rd_address funcaddress)
{
    rd_ptr<RDILFunction> il(RDILFunction_Create(m_context, funcaddress));
    const auto* fil = RDILFunction_GetFirstExpression(il.get());
    if(!RDILExpression_Match(fil, "goto [reg + cnst]")) return;

    const auto* regel = RDILExpression_Extract(fil, "u:mem/u:add/left:reg");
    const auto* idxel = RDILExpression_Extract(fil, "u:mem/u:add/right:cnst");

    RDILValue regval, idxval;
    if(!RDILExpression_GetValue(regel, &regval) || std::strcmp(regval.reg, "ebx")) return;
    if(!RDILExpression_GetValue(idxel, &idxval)) return;

    auto name = m_loader->abi()->plt(idxval.u_value);
    if(name) RDDocument_AddFunction(doc, funcaddress, RD_Thunk(name->c_str()));
}

void ElfAnalyzerX86::checkPLT64(RDDocument* doc, rd_address funcaddress)
{

}
