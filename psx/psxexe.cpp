#include "psxexe.h"
#include <cstring>

const char* PsxExeLoader::test(const RDLoaderRequest* request)
{
    const auto* header = reinterpret_cast<const PsxExeHeader*>(RDBuffer_Data(request->buffer));
    if(std::strncmp(header->id, PSXEXE_SIGNATURE, PSXEXE_SIGNATURE_SIZE)) return nullptr;
    return "mips32le";
}

bool PsxExeLoader::load(RDContext* ctx)
{
    // this->signature("psyq");
    RDDocument* doc = RDContext_GetDocument(ctx);

    const auto* header = reinterpret_cast<const PsxExeHeader*>(RDContext_GetBufferData(ctx));

    if(header->t_addr > PSX_USER_RAM_START)
        RDDocument_SetSegment(doc, "RAM0", 0, PSX_USER_RAM_START, (header->t_addr - PSX_USER_RAM_START), SegmentFlags_Bss);

    RDDocument_SetSegment(doc, "TEXT", PSXEXE_TEXT_OFFSET, header->t_addr, header->t_size, SegmentFlags_CodeData);

    if((header->t_addr + header->t_size) < PSX_USER_RAM_END)
        RDDocument_SetSegment(doc, "RAM1", 0, header->t_addr + header->t_size, PSX_USER_RAM_END - (header->t_addr + header->t_size), SegmentFlags_Bss);

    RDDocument_SetEntry(doc, header->pc0);
    return true;
}

