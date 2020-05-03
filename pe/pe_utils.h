#pragma once

#include "pe_header.h"

class PEUtils
{
    public:
        PEUtils() = delete;
        static std::string sectionName(const char* psectionname);
        static std::string importName(std::string library, const std::string& name);
        static std::string importName(const std::string &library, s64 ordinal);
        static bool checkMsvcImport(const std::string& importdescriptor);
        static RDLocation rvaToOffset(const ImageNtHeaders* ntheaders, u64 rva);
};
