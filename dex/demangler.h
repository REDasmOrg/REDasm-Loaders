#pragma once

#include <string>

class Demangler
{
    public:
        Demangler() = delete;
        static std::string getPackageName(const std::string& s);
        static std::string getObjectName(const std::string& s);
        static std::string getFullName(const std::string& s);
        static std::string getSignature(const std::string& s, bool wrap = true);
        static std::string getReturn(const std::string& s);

    private:
        static std::string getType(const std::string& s);
};
