#include "demangler.h"
#include <string>
#include <regex>

std::string Demangler::getPackageName(const std::string& s)
{
    std::regex rgx("L(.+)\\/.+;");
    std::smatch m;

    if(!std::regex_match(s, m, rgx)) return s;

    std::string sm = m[1];
    std::replace(sm.begin(), sm.end(), '/', '.');
    return sm;
}

std::string Demangler::getObjectName(const std::string& s)
{
    std::regex rgx("L(.+\\/)+(.+);");
    std::smatch m;

    return std::regex_match(s, m, rgx) ? m[2] : s;
}

std::string Demangler::getFullName(const std::string& s)
{
    std::regex rgx("L(.+);");
    std::smatch m;

    if(!std::regex_match(s, m, rgx)) return s;

    std::string sm = m[1];
    std::replace(sm.begin(), sm.end(), '/', '.');
    return sm;
}

std::string Demangler::getSignature(const std::string& s, bool wrap)
{
    std::regex rgx("(L.+?;)");
    std::sregex_iterator it(s.begin(), s.end(), rgx);
    std::string res;

    for( ; it != std::sregex_iterator(); it++)
    {
        if(!res.empty()) res += ", ";
        res += Demangler::getObjectName((*it)[1]);
    }

    if(wrap) res = "(" + res + ")";
    return res;
}

std::string Demangler::getReturn(const std::string& s)
{
    std::regex rgx("\\(.*\\)(.+)");
    std::smatch m;

    return std::regex_match(s, m, rgx) ? Demangler::getType(m[1]) : s;
}

std::string Demangler::getType(const std::string& s)
{
    if(s == "V") return "void";
    if(s == "Z") return "boolean";
    if(s == "B") return "byte";
    if(s == "S") return "short";
    if(s == "C") return "char";
    if(s == "I") return "int";
    if(s == "J") return "long";
    if(s == "F") return "float";
    if(s == "D") return "double";
    return s;
}
