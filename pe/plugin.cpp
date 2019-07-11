#include "pe.h"
#include <redasm/context.h>

REDASM_LOADER("PE", "Dax", "MIT", 1)

REDASM_LOAD
{
    pe.plugin = new PELoader();
    return true;
}

REDASM_UNLOAD { pe.plugin->release(); }
