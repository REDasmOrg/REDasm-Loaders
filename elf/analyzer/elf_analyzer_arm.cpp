#include "elf_analyzer_arm.h"
#include "../elf.h"
#include <cstring>

ElfAnalyzer_ARM::ElfAnalyzer_ARM(RDContext* ctx): ElfAnalyzer(ctx) { }

void ElfAnalyzer_ARM::analyze()
{
    ElfAnalyzer::analyze();

    this->walkPlt([this](rd_address address) {
        switch(m_loader->machine()) {
            case EM_ARM: this->checkPLT32(m_document, address); break;
            case EM_AARCH64: this->checkPLT64(m_document, address); break;
            default: rd_log("Unhandled machine '" + rd_tohex(m_loader->machine()) + "'"); break;
        }
    });
}

void ElfAnalyzer_ARM::findMain(rd_address address)
{

}

void ElfAnalyzer_ARM::checkPLT32(RDDocument* doc, rd_address funcaddress)
{
    rd_ptr<RDILFunction> il(RDILFunction_Create(m_context, funcaddress));

    if(!RDILFunction_Match(il.get(), "reg = reg + cnst;"
                                     "reg = reg + cnst;"
                                     "reg = [reg + cnst]")) return;

    //TODO: Improve matching algorithm
    /* if(!RDILFunction_Match(il.get(), "$ip = $pc + cnst;"
                                        "$ip = $ip + cnst;"
                                        "$pc = [$ip + cnst]")) return; */


    const RDILValue* values = nullptr;
    size_t n = RDILFunction_Extract(il.get(), &values);

    if(!n ||
       std::strcmp(values[0].reg, values[3].reg) ||
       std::strcmp(values[0].reg, values[4].reg) ||
       std::strcmp(values[0].reg, values[7].reg) ||
       std::strcmp(values[1].reg, values[6].reg) ||
       std::strcmp(values[0].reg, "ip")          ||
       std::strcmp(values[1].reg, "pc"))
        return;

    rd_address address = funcaddress + (sizeof(u32) * 2) +
                         values[2].u_value + values[5].u_value + values[8].u_value;

    rd_flag flags = RDDocument_GetFlags(doc, address);

    if(flags & AddressFlags_Imported)
    {
        const char* lbl = RDDocument_GetLabel(doc, address);
        if(lbl) RDDocument_UpdateLabel(doc, funcaddress, (lbl + std::string("@plt")).c_str());
    }
}

void ElfAnalyzer_ARM::checkPLT64(RDDocument* doc, rd_address funcaddress)
{

}
