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

    this->walkPlt([this](rd_address address) {
        switch(m_loader->machine()) {
            case EM_386: this->checkPLT32(m_document, address); break;
            case EM_X86_64: this->checkPLT64(m_document, address); break;
            default: rd_log("Unhandled machine '" + rd_tohex(m_loader->machine()) + "'"); break;
        }
    });
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

        const RDILValue* values = nullptr;

        size_t n = RDILExpression_Extract(e, &values);
        if(!n || values[0].type != RDIL_Cnst) continue;
        pushcexpr.push_back(values[0].address);
    }

    if(pushcexpr.size() != EP_NAMES.size())
    {
        rd_log("Cannot find main, got " + std::to_string(pushcexpr.size()) + " constant push");
        return;
    }

    for(size_t i = 0; i < EP_NAMES.size(); i++) RDDocument_CreateFunction(m_document, pushcexpr[i], EP_NAMES[i]);
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

        const RDILValue* values = nullptr;
        size_t n = RDILExpression_Extract(e, &values);
        if(n == 2) assignexpr[values[0].reg] = values[1].address;
    }

    if(assignexpr.size() != EP_NAMES.size())
    {
        rd_log("Cannot find main, got " + std::to_string(assignexpr.size()) + " assignments");
        return;
    }

    for(const auto& [regname, address] : assignexpr) RDDocument_CreateFunction(m_document, address, EP_NAMES.at(regname));
}

void ElfAnalyzerX86::checkPLT32(RDDocument* doc, rd_address funcaddress)
{
    rd_ptr<RDILFunction> il(RDILFunction_Create(m_context, funcaddress));
    const auto* fil = RDILFunction_GetFirstExpression(il.get());
    if(!RDILExpression_Match(fil, "goto [reg + cnst]")) return;

    const RDILValue* values = nullptr;
    size_t n = RDILExpression_Extract(fil, &values);
    if(n != 2 || std::strcmp(values[0].reg, "ebx")) return;

    auto name = m_loader->abi()->plt(values[1].u_value);
    if(name) RDDocument_SetFunction(doc, funcaddress, (*name + "@plt").c_str());
}

void ElfAnalyzerX86::checkPLT64(RDDocument* doc, rd_address funcaddress)
{

}
