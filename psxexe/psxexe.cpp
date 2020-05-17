#include "psxexe.h"
#include <cstring>

RDAssemblerPlugin* PsxExeLoader::test(const RDLoaderPlugin*, const RDLoaderRequest* request)
{
    const auto* header = reinterpret_cast<const PsxExeHeader*>(RDBuffer_Data(request->buffer));
    if(std::strncmp(header->id, PSXEXE_SIGNATURE, PSXEXE_SIGNATURE_SIZE)) return nullptr;
    return RDAssembler_Find("mips32_le");
}

void PsxExeLoader::load(RDLoaderPlugin*, RDLoader* loader)
{
    // this->signature("psyq");
    RDDocument* doc = RDLoader_GetDocument(loader);

    const auto* header = reinterpret_cast<const PsxExeHeader*>(RDLoader_GetData(loader));

    if(header->t_addr > PSX_USER_RAM_START)
        RDDocument_AddSegment(doc, "RAM0", 0, PSX_USER_RAM_START, (header->t_addr - PSX_USER_RAM_START), SegmentFlags_Bss);

    RDDocument_AddSegment(doc, "TEXT", PSXEXE_TEXT_OFFSET, header->t_addr, header->t_size, SegmentFlags_CodeData);

    if((header->t_addr + header->t_size) < PSX_USER_RAM_END)
        RDDocument_AddSegment(doc, "RAM1", 0, header->t_addr + header->t_size, PSX_USER_RAM_END - (header->t_addr + header->t_size), SegmentFlags_Bss);

    RDDocument_SetEntry(doc, header->pc0);
}

RD_PLUGIN(RDLoaderPlugin, psxexe, "PS-X Executable");

void redasm_entry()
{
    psxexe.test = &PsxExeLoader::test;
    psxexe.load = &PsxExeLoader::load;
    RDLoader_Register(&psxexe);
}
