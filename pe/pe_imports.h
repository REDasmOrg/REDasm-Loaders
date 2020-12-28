#pragma once

#include <rdapi/rdapi.h>

typedef u16 ordinal_t;

class PEImports
{
    public:
        PEImports() = default;
        template<int b> bool importName(RDContext* ctx, const std::string& dllname, ordinal_t ordinal, std::string& name);
        std::string dbImportName(const std::string& dllname, int bits) const;
        std::string dllStem(const std::string& dllname) const;
};

template<int b> bool PEImports::importName(RDContext* ctx, const std::string &dllname, ordinal_t ordinal, std::string &name)
{
    std::string importname = this->dbImportName(dllname, b), ordinalspath = "ordinals/" + this->dllStem(dllname);

    auto* db = RDContext_GetDatabase(ctx);
    if(!RDDatabase_Add(db, ordinalspath.c_str(), importname.c_str())) return false;

    RDDatabaseValue value;
    if(!RDDatabase_Query(db, (ordinalspath + "/" + std::to_string(ordinal)).c_str(), &value)) return false;
    name = RD_Demangle(value.s);
    return true;
}
