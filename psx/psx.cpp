#include "psxexe.h"
#include "psxbios.h"

void redasm_entry()
{
    RD_PLUGIN_CREATE(RDLoaderPlugin, psxexe, "PS-X Executable");
    psxexe.test = &PsxExeLoader::test;
    psxexe.load = &PsxExeLoader::load;
    RDLoader_Register(&psxexe);

    RD_PLUGIN_CREATE(RDLoaderPlugin, psxbios, "Sony Playstation PSX BIOS");
    psxbios.test = &PsxBiosLoader::test;
    psxbios.load = &PsxBiosLoader::load;
    RDLoader_Register(&psxbios);
}
