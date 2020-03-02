#include "binary.h"
#include <limits>

BinaryLoader::BinaryLoader(): Loader() {  }
int BinaryLoader::weight() const { return std::numeric_limits<int>::max(); }
AssemblerRequest BinaryLoader::assembler() const { return m_assembler.c_str(); }
flag_t BinaryLoader::flags() const { return LoaderFlags::Binary; }
bool BinaryLoader::test(const LoadRequest &request) const { return true; }
void BinaryLoader::load() { /* NOP */ }

void BinaryLoader::build(const String &assembler, offset_t offset, address_t baseaddress, address_t entrypoint)
{
    m_assembler = assembler;
    size_t vsize = this->buffer()->size();

    if(entrypoint >= vsize)
        vsize = entrypoint << 1;

    ldrdoc->segment("BINARY", offset, baseaddress, this->buffer()->size(), vsize, Segment::T_Code | Segment::T_Data);
    ldrdoc->entry(baseaddress + entrypoint);
}

REDASM_LOADER("Binary", "Dax", "MIT", 1)
REDASM_LOAD { binary.plugin = new BinaryLoader(); return true; }
REDASM_UNLOAD { binary.plugin->release(); }
