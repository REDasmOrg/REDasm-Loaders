#pragma once

#include <filesystem>
#include <rdapi/rdapi.h>

namespace fs = std::filesystem;
typedef u16 ordinal_t;

class PEImports
{
    public:
        PEImports() = default;
        template<int b> bool importName(const std::string& dllname, ordinal_t ordinal, std::string& name);

    private:
        rd_ptr<RDDatabase> m_ordinalsdb;
};

template<int b> bool PEImports::importName(const std::string &dllname, ordinal_t ordinal, std::string &name)
{
    if(!m_ordinalsdb) m_ordinalsdb.reset(RDDatabase_Open(("loaders/pe/ordinals" + std::to_string(b)).c_str()));
    if(!m_ordinalsdb) return false;

    std::string modulename = (fs::path(dllname).stem() / std::to_string(ordinal)).string();
    std::transform(modulename.begin(), modulename.end(), modulename.begin(), ::tolower);

    RDDatabaseValue value;
    if(!RDDatabase_Query(m_ordinalsdb.get(), modulename.c_str(), &value)) return false;
    name = RD_Demangle(value.s);
    return true;
}
