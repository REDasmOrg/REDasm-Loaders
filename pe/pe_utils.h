#pragma once

#include <redasm/redasm.h>
#include "pe_header.h"

using namespace REDasm;

class PEUtils
{
    public:
        PEUtils() = delete;
        static String sectionName(const char* psectionname);
        static String importName(String library, const String& name);
        static String importName(const String &library, s64 ordinal);
        static bool checkMsvcImport(const String& importdescriptor);
        static offset_location rvaToOffset(const ImageNtHeaders* ntheaders, u64 rva);
};
