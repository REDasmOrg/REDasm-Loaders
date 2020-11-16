#include "elf_analyzer.h"
#include "elf.h"
#include "../elf_common.h"

ElfAnalyzer::ElfAnalyzer(RDContext* ctx): m_context(ctx) { m_loader = reinterpret_cast<ElfLoader*>(RDContext_GetUserData(ctx, ELFLOADER_USERDATA)); }

void ElfAnalyzer::analyze()
{
    RDLocation loc = RDContext_GetEntryPoint(m_context);
    if(!loc.valid) return;
    this->findMain(loc.address);

    RDDocument* doc = RDContext_GetDocument(m_context);

    RDSymbol symbol;
    if(RDDocument_GetSymbolByName(doc, "main", &symbol))
        RDDocument_SetEntry(doc, symbol.address);
}
