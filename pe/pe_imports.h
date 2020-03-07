#pragma once

#include <map>
#include <redasm/redasm.h>
#include <redasm/support/ordinals.h>
#include <redasm/support/filesystem.h>

using namespace REDasm;

class PEImports
{
    private:
        typedef std::map<String, Ordinals> ResolveMap;

    private:
        template<int b> static void loadImport(const String &dllname);
        template<int b> static void checkX64(String& modulename);

    public:
        PEImports() = delete;
        template<int b> static bool importName(const String& dllname, ordinal_t ordinal, String& name);

    private:
        static ResolveMap m_libraries;
};

template<int b> void PEImports::checkX64(String &modulename)
{
    if((b != 64) || modulename.startsWith("mfc"))
        return;

    modulename += "!x64";
}

template<int b> void PEImports::loadImport(const String& dllname)
{
    String modulename = FS::Path(dllname).stem();
    PEImports::checkX64<b>(modulename);

    if(m_libraries.find(dllname) != m_libraries.end())
        return;

    Ordinals ordinals;
    auto path = REDasm::FS::Path("pe").append("ordinals").append(modulename + ".json");
    ordinals.load(r_ctx->loaderdb(path));
    m_libraries[dllname] = ordinals;
}

template<int b> bool PEImports::importName(const String &dllname, ordinal_t ordinal, String &name)
{
    PEImports::loadImport<b>(dllname);
    auto it = m_libraries.find(dllname);

    if(it == m_libraries.end())
        return false;

    name = it->second.name(ordinal);
    return true;
}
