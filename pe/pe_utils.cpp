#include "pe_utils.h"
#include "pe_constants.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

String PEUtils::sectionName(const char *psectionname)
{
    const char *pend = psectionname;
    size_t i = 0;

    for( ; i < IMAGE_SIZEOF_SHORT_NAME; i++, pend++)
    {
        if(!*pend)
            break;
    }

    return String(psectionname, i);
}

String PEUtils::importName(String library, const String &name)
{
    library = library.toLower();

    if(!library.endsWith(".dll"))
        library += ".dll";

    return library + "_" + name;
}

String PEUtils::importName(const String& library, s64 ordinal) { return PEUtils::importName(library, "Ordinal__" + String::number(ordinal, 16, 4, '0').toUpper()); }

bool PEUtils::checkMsvcImport(const String &importdescriptor)
{
    if(importdescriptor.contains("vcruntime"))
        return true;
    if(importdescriptor.contains("mfc"))
        return true;
    if(importdescriptor.contains("api-ms-win-crt-"))
        return true;

    return false;
}

offset_location PEUtils::rvaToOffset(const ImageNtHeaders *ntheaders, u64 rva)
{
    const ImageSectionHeader* sectiontable = IMAGE_FIRST_SECTION(ntheaders);

    for(size_t i = 0; i < ntheaders->FileHeader.NumberOfSections; i++)
    {
        const ImageSectionHeader& section = sectiontable[i];

        if((rva >= section.VirtualAddress) && (rva < (section.VirtualAddress + section.Misc.VirtualSize)))
        {
            if(!section.SizeOfRawData) // Check if section not BSS
                break;

            offset_t offset = section.PointerToRawData + (rva - section.VirtualAddress);
            return REDasm::make_location(offset, offset < (section.PointerToRawData + section.SizeOfRawData));
        }
    }

    return REDasm::invalid_location<offset_t>();
}
