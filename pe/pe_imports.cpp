#include "pe_imports.h"

//PEImports::ResolveMap PEImports::m_libraries;

PEImports::PEImports()
{
    m_ordinalsdb.reset(RDDatabase_Open("loaders/pe/ordinals"));
}
