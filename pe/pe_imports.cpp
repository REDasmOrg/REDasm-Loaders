#include "pe_imports.h"
#include <filesystem>

namespace fs = std::filesystem;

std::string PEImports::dbImportName(const std::string& dllname, int bits) const
{
    return (fs::path("loaders/pe/ordinals" + std::to_string(bits)) / this->dllStem(dllname)).string();
}

std::string PEImports::dllStem(const std::string& dllname) const
{
    std::string modulename = fs::path(dllname).stem().string();
    std::transform(modulename.begin(), modulename.end(), modulename.begin(), ::tolower);
    return modulename;
}
