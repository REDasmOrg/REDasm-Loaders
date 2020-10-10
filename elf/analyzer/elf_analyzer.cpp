#include "elf_analyzer.h"
#include "elf.h"
#include "../elf_common.h"

ElfAnalyzer::ElfAnalyzer(RDContext* ctx): m_context(ctx) { m_loader = reinterpret_cast<ElfLoader*>(RDContext_GetUserData(ctx, ELFLOADER_USERDATA)); }

void ElfAnalyzer::analyze()
{
    RDDocument* doc = RDContext_GetDocument(m_context);
    RDLocation loc = RDDocument_GetEntryPoint(doc);
    if(!loc.valid) return;
    this->findMain(loc.address);

    RDSymbol symbol;
    if(RDDocument_GetSymbolByName(doc, "main", &symbol))
        RDDocument_SetEntry(doc, symbol.address);
}
