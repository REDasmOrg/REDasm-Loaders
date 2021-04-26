#include "dex.h"
#include <libdex/DexProto.h>
#include "../demangler.h"
#include <cctype>
#include <memory>
#include <list>

DexLoader::DexLoader(RDContext* ctx): m_context(ctx) { }
DexLoader::~DexLoader() { if(m_dexfile) dexFileFree(m_dexfile); }
const DexFile* DexLoader::dexFile() const { return m_dexfile; }

const char* DexLoader::test(const RDLoaderRequest* request)
{
    if(dexFileParse(RDBuffer_Data(request->buffer), RDBuffer_Size(request->buffer), 0))
        return "dalvik";

    return nullptr;
}

bool DexLoader::load()
{
    m_dexfile = dexFileParse(RDContext_GetBufferData(m_context), RDContext_GetBufferSize(m_context), 0);
    rd_log("Loading DEX Version " + std::string(reinterpret_cast<const char*>(m_dexfile->pHeader->magic + 4), 3));

    auto* doc = RDContext_GetDocument(m_context);
    RDDocument_SetSegment(doc, "CODE", m_dexfile->pHeader->dataOff, m_dexfile->pHeader->dataOff, m_dexfile->pHeader->dataSize, SegmentFlags_Code);
    return this->filterClasses();
}

const DexMethodId* DexLoader::addressToMethodId(rd_address address) const
{
    auto it = m_methodids.find(address);
    return (it != m_methodids.end()) ? it->second : nullptr;
}

std::optional<u16> DexLoader::addressToMethodIdx(rd_address address) const
{
    auto it = m_methodidx.find(address);
    return std::make_optional((it != m_methodidx.end()) ? it->second : RD_NVAL);
}

void DexLoader::loadMethod(const DexMethod &dexmethod, bool filter)
{
    if(!dexmethod.codeOff) return;

    auto* dexcode = reinterpret_cast<const DexCode*>(dexGetCode(m_dexfile, &dexmethod));
    auto* methodid = dexGetMethodId(m_dexfile, dexmethod.methodIdx);
    std::string methodname = dexStringById(m_dexfile, methodid->nameIdx);
    std::string classname = Demangler::getObjectName(dexStringByTypeIdx(m_dexfile, methodid->classIdx));
    std::string fullname = classname + "." + methodname;

    auto* doc = RDContext_GetDocument(m_context);
    auto loc = RD_FileOffset(m_context, &dexcode->insns);
    if(!loc.valid) return;

    if(filter) RDDocument_SetImported(doc, loc.address, sizeof(u16), fullname.c_str());
    else RDDocument_SetExportedFunction(doc, loc.address, fullname.c_str());

    // Cache method references
    m_methodidx[loc.address] = dexmethod.methodIdx;
    m_methodids[loc.address] = methodid;
}

void DexLoader::loadClass(const DexClassDef *classdef, bool filter)
{
    auto* encodeddata = dexGetClassData(m_dexfile, classdef);
    auto* classdata = dexReadAndVerifyClassData(&encodeddata, nullptr);
    if(!classdata) return;

    for(size_t i = 0; i < classdata->header.directMethodsSize; i++)
        this->loadMethod(classdata->directMethods[i], filter);

    for(size_t i = 0; i < classdata->header.virtualMethodsSize; i++)
        this->loadMethod(classdata->virtualMethods[i], filter);

    std::free(classdata);
}

bool DexLoader::filterClasses()
{
    std::vector<const DexClassDef*> classdefs;
    std::vector<RDUIOptions> options;
    std::list<std::string> classtypes;

    for(u32 i = 0; i < m_dexfile->pHeader->classDefsSize; i++)
    {
        auto* classdef = dexGetClassDef(m_dexfile, i);
        if(!classdef) continue;
        const char* pclassdescr = dexGetClassDescriptor(m_dexfile, classdef);
        if(!pclassdescr) continue;

        bool precheck = true;
        std::string packagename = Demangler::getPackageName(pclassdescr);

        if(!packagename.find("android.") || !packagename.find("com.google.")) // Apply prefiltering
            precheck = false;

        classtypes.push_back(Demangler::getObjectName(pclassdescr)); // Cache Strings
        options.push_back({ classtypes.back().c_str(), precheck });
        classdefs.push_back(classdef);
    }

    if(!RDUI_GetChecked("Class Loader", "Select one or more objects from the list below", options.data(), options.size()))
        return false;

    for(u32 i = 0; i < classdefs.size(); i++)
        this->loadClass(classdefs[i], !options[i].selected);

    return true;
}

std::string DexLoader::demangle(std::string s)
{
    if(s.front() == '[') return DexLoader::demangle(s.substr(1)) + "[]";
    if(s == "V") return "void";
    if(s == "Z") return "boolean";
    if(s == "B") return "byte";
    if(s == "S") return "short";
    if(s == "C") return "char";
    if(s == "I") return "int";
    if(s == "J") return "long";
    if(s == "F") return "float";
    if(s == "D") return "double";

    if(s.front() == 'L') s.erase(s.begin());
    if(s.back() == ';') s.pop_back();
    std::replace(s.begin(), s.end(), '/', '.');
    //std::replace(s.begin(), s.end(), '$', '.');
    return s;
}
