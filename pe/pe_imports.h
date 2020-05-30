#pragma once

#include <filesystem>
#include <algorithm>
#include <cctype>
#include <rdapi/rdapi.h>

typedef u16 ordinal_t;

class PEImports
{
    public:
        PEImports();
        template<int b> bool importName(const std::string& dllname, ordinal_t ordinal, std::string& name);

    private:
        template<int b> static void checkX64(std::string& modulename);

    private:
        rd_ptr<RDDatabase> m_ordinalsdb;
};

template<int b> bool PEImports::importName(const std::string &dllname, ordinal_t ordinal, std::string &name)
{
    std::string modulename = std::filesystem::path(dllname).stem();
    std::transform(modulename.begin(), modulename.end(), modulename.begin(), ::tolower);
    PEImports::checkX64<b>(modulename);

    if(!m_ordinalsdb || !RDDatabase_Select(m_ordinalsdb.get(), modulename.c_str())) return false;

    RDDatabaseItem item;
    if(!RDDatabase_Find(m_ordinalsdb.get(), std::to_string(ordinal).c_str(), &item)) return false;
    name = item.s_value;
    return true;
}

template<int b> void PEImports::checkX64(std::string &modulename)
{
    if((b != 64) || !modulename.find("mfc")) return;
    modulename += "!x64";
}
