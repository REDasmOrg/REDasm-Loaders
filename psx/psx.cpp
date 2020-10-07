#include "psxexe.h"
#include "psxbios.h"

void rdplugin_init(RDContext*, RDPluginModule* pm)
{
    RD_PLUGIN_ENTRY(RDEntryLoader, psxexe, "PS-X Executable");
    psxexe.test = &PsxExeLoader::test;
    psxexe.load = &PsxExeLoader::load;
    RDLoader_Register(pm, &psxexe);

    RD_PLUGIN_ENTRY(RDEntryLoader, psxbios, "Sony Playstation PSX BIOS");
    psxbios.test = &PsxBiosLoader::test;
    psxbios.load = &PsxBiosLoader::load;
    RDLoader_Register(pm, &psxbios);
}
