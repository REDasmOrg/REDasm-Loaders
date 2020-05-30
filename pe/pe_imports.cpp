#include "pe_imports.h"

PEImports::PEImports()
{
    m_ordinalsdb.reset(RDDatabase_Open("loaders/pe/ordinals"));
}
