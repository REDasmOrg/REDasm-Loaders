#include "pe_utils.h"
#include "pe_constants.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

std::string PEUtils::sectionName(const char *psectionname)
{
    const char *pend = psectionname;
    size_t i = 0;

    for( ; i < IMAGE_SIZEOF_SHORT_NAME; i++, pend++)
    {
        if(!*pend)
            break;
    }

    return std::string(psectionname, i);
}

std::string PEUtils::importName(std::string library, const std::string &name)
{
    std::transform(library.begin(), library.end(), library.begin(), ::tolower);
    if(library.find(".dll") == std::string::npos) library += ".dll";
    return library + "_" + name;
}

std::string PEUtils::importName(const std::string& library, s64 ordinal)
{
    std::string ordinalstring = rd_tostringbase(ordinal, 16, 4, '0');
    std::transform(ordinalstring.begin(), ordinalstring.end(), ordinalstring.begin(), ::toupper);
    return PEUtils::importName(library, "Ordinal__" + ordinalstring);
}

bool PEUtils::checkMsvcImport(const std::string &importdescriptor)
{
    if(importdescriptor.find("vcruntime") != std::string::npos) return true;
    if(importdescriptor.find("mfc") != std::string::npos) return true;
    if(importdescriptor.find("api-ms-win-crt-") != std::string::npos) return true;
    return false;
}

RDLocation PEUtils::rvaToOffset(const ImageNtHeaders *ntheaders, u64 rva)
{
    const ImageSectionHeader* sectiontable = IMAGE_FIRST_SECTION(ntheaders);

    for(size_t i = 0; i < ntheaders->FileHeader.NumberOfSections; i++)
    {
        const ImageSectionHeader& section = sectiontable[i];

        if((rva >= section.VirtualAddress) && (rva < (section.VirtualAddress + section.Misc.VirtualSize)))
        {
            if(!section.SizeOfRawData) // Check if section not BSS
                break;

            rd_offset offset = section.PointerToRawData + (rva - section.VirtualAddress);
            return { {offset}, offset < (section.PointerToRawData + section.SizeOfRawData) };
        }
    }

    return {{0}, false};
}
