#include "elf_analyzer.h"
#include "../elf_common.h"

ElfAnalyzer::ElfAnalyzer(RDContext* ctx): m_context(ctx), m_document(RDContext_GetDocument(ctx))
{
    m_loader = reinterpret_cast<ElfLoader*>(RDContext_GetUserData(ctx, ELFLOADER_USERDATA));
}

void ElfAnalyzer::analyze()
{
    RDDocument* doc = RDContext_GetDocument(m_context);
    RDLocation loc = RDDocument_GetEntry(doc);
    if(!loc.valid) return;
    this->findMain(loc.address);

    rd_address address = RDDocument_GetAddress(doc, "main");
    if(address != RD_NVAL) RDDocument_SetEntry(doc, address);
}
