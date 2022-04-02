#include "elf_analyzer.h"
#include "../elf_common.h"
#include "../elf.h"
#include <cstring>

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

void ElfAnalyzer::walkPlt(PltCallback&& cb)
{
    const u8* pltbase = m_loader->plt();
    if(!pltbase) return;

    RDSegment segment{ };

    const rd_address* addresses = nullptr;
    size_t c = RDDocument_GetFunctions(m_document, &addresses);

    for(size_t i = 0; i < c; i++)
    {
        if(!RDSegment_ContainsAddress(&segment, addresses[i]))
        {
            if(!RDDocument_AddressToSegment(m_document, addresses[i], &segment))
                continue;
        }

        if(std::strcmp(".plt", segment.name)) continue;
        cb(addresses[i]);
    }
}
