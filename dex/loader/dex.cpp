#include "dex.h"
#include "dex_statemachine.h"
#include "dex_constants.h"
#include <redasm/types/leb128.h>
#include <redasm/ui.h>
#include <cctype>

#define IMPORT_SECTION_ADDRESS        0x10000000
#define IMPORT_SECTION_SIZE           0x1000000

const String DexLoader::m_invalidstring;
DexLoader::DexLoader(): Loader(), m_types(nullptr), m_strings(nullptr), m_methods(nullptr), m_fields(nullptr), m_protos(nullptr) { m_importbase = IMPORT_SECTION_ADDRESS; }
AssemblerRequest DexLoader::assembler() const { return "dalvik"; }

bool DexLoader::test(const LoadRequest &request) const
{
    const auto* header = request.pointer<DexHeader>();

    if(!DexLoader::validateSignature(header) || (!header->data_off || !header->data_size))
        return false;

    if((!header->type_ids_off || !header->type_ids_size) || (!header->string_ids_off || !header->string_ids_size))
        return false;

    if((!header->method_ids_off || !header->method_ids_size) || (!header->proto_ids_off || !header->proto_ids_size))
        return false;

    return true;
}

void DexLoader::load()
{
    m_header = pointer<DexHeader>();
    r_ctx->log("Loading DEX Version " + String(m_header->version, 3));

    m_types = pointer<DEXTypeIdItem>(m_header->type_ids_off);
    m_strings = pointer<DEXStringIdItem>(m_header->string_ids_off);
    m_methods = pointer<DEXMethodIdItem>(m_header->method_ids_off);
    m_protos = pointer<DEXProtoIdItem>(m_header->proto_ids_off);

    if(m_header->field_ids_off && m_header->field_ids_size)
        m_fields = pointer<DEXFieldIdItem>(m_header->field_ids_off);

    ldrdoc->segment("CODE", m_header->data_off, m_header->data_off, m_header->data_size, Segment::T_Code);
    ldrdoc->segment("IMPORT", 0, IMPORT_SECTION_ADDRESS, IMPORT_SECTION_SIZE, Segment::T_Bss);

    DEXClassIdItem* dexclasses = pointer<DEXClassIdItem>(m_header->class_defs_off);
    this->filterClasses(dexclasses);
}

bool DexLoader::getMethodOffset(u32 idx, offset_t &offset) const
{
    auto it = m_codeitems.find(idx);

    if(it == m_codeitems.end())
        return false;

    DEXCodeItem* dexcode = it->second;
    offset = fileoffset(&dexcode->insns);
    return true;
}

size_t DexLoader::getStringOffset(u32 idx, offset_t& offset) const
{
    if(!m_strings || (idx >= m_header->string_ids_size))
        return 0;

    u8* pstringdata = pointer<u8>(m_strings[idx].string_data_off);
    LEB128::unsignedOf<u32>(pstringdata, &pstringdata);
    offset = fileoffset(pstringdata);
    return std::strlen(reinterpret_cast<const char*>(pstringdata)) + 1;
}

const String &DexLoader::getString(u32 idx)
{
    if(!m_strings)
        return m_invalidstring;

    return cacheEntry(idx, m_cachedstrings, [=](String& s) {
        u8* pstringdata = pointer<u8>(m_strings[idx].string_data_off);
        u32 len = LEB128::unsignedOf<u32>(pstringdata, &pstringdata);
        s = String(reinterpret_cast<const char*>(pstringdata), len);
    });
}

const String &DexLoader::getType(u32 idx, bool full)
{
    return cacheEntry(idx, m_cachedtypes, [&](String& s) {
        if(idx >= m_header->type_ids_size) {
            s = "type_" + String::number(idx);
            return;
        }

        const DEXTypeIdItem& dextype = m_types[idx];
        s = this->getNormalizedString(dextype.descriptor_idx);

        if(full)
            return;

        // Strip full qualified name
        size_t idx = s.lastIndexOf(".");

        if(idx != String::npos)
            s = s.substring(idx + 1);
    });
}

const String &DexLoader::getMethodName(u32 idx)
{
    return cacheEntry(idx, m_cachedmethodnames, [&](String& s) {
        if(idx >= m_header->method_ids_size) {
            s = "method_" + String::number(idx);
            return;
        }

        const DEXMethodIdItem& dexmethod = m_methods[idx];
        s = this->getType(dexmethod.class_idx) + "." + this->getNormalizedString(dexmethod.name_idx);
    });
}

const String &DexLoader::getMethodProto(u32 idx)
{
    return cacheEntry(idx, m_cachedmethodproto, [&](String& s) {
        s = this->getMethodName(idx) + this->getParameters(idx) + ":" + this->getReturnType(idx);
    });
}

const String &DexLoader::getField(u32 idx)
{
    return cacheEntry(idx, m_cachedfields, [&](String& s) {
        if(!m_fields || (idx >= m_header->field_ids_size)) {
            s = "field_" + String::number(idx);
            return;
        }

        const DEXFieldIdItem& dexfield = m_fields[idx];
        s = this->getType(dexfield.class_idx) + "." + this->getNormalizedString(dexfield.name_idx) + ":" + this->getType(dexfield.type_idx);
    });
}

const String &DexLoader::getReturnType(u32 methodidx)
{
    if(methodidx >= m_header->method_ids_size)
        return m_invalidstring;

    const DEXMethodIdItem& dexmethod = m_methods[methodidx];
    const DEXProtoIdItem& dexproto = m_protos[dexmethod.proto_idx];

    return this->getNormalizedString(m_types[dexproto.return_type_idx].descriptor_idx);
}

const String &DexLoader::getParameters(u32 methodidx)
{
    if(methodidx >= m_header->method_ids_size)
        return m_invalidstring;

    return this->cacheEntry(methodidx, m_cachedparameters, [&](String& s) {
        const DEXMethodIdItem& dexmethod = m_methods[methodidx];
        const DEXProtoIdItem& dexproto = m_protos[dexmethod.proto_idx];

        if(!dexproto.parameters_off)
            s = "()";
        else
            s = "(" + this->getTypeList(dexproto.parameters_off) + ")";
    });
}

bool DexLoader::getMethodInfo(u32 methodidx, DexEncodedMethod &dexmethod)
{
    auto it = m_encmethods.find(methodidx);

    if(it == m_encmethods.end())
        return false;

    dexmethod = it->second;
    return true;
}

bool DexLoader::getDebugInfo(u32 methodidx, DexDebugInfo &debuginfo)
{
    auto it = m_codeitems.find(methodidx);

    if(it == m_codeitems.end())
        return false;

    DEXCodeItem* dexcode = it->second;

    if(!dexcode->debug_info_off)
        return false;

    u8* pdebuginfo = pointer<u8>(dexcode->debug_info_off);
    debuginfo.line_start = LEB128::unsignedOf<u32>(pdebuginfo, &pdebuginfo);
    debuginfo.parameters_size = LEB128::unsignedOf<u32>(pdebuginfo, &pdebuginfo);

    for(u32 i = 0; i < debuginfo.parameters_size; i++)
    {
        s32 idx = LEB128::unsigned1COf<s32>(pdebuginfo, &pdebuginfo);

        if(idx == DEX_NO_INDEX)
            debuginfo.parameter_names.push_back(String());
        else
            debuginfo.parameter_names.push_back(this->getNormalizedString(idx));
    }

    DEXStateMachine dexstatemachine(fileoffset(&dexcode->insns), debuginfo);
    dexstatemachine.execute(pdebuginfo);
    return true;
}

u32 DexLoader::getMethodSize(u32 methodidx) const { return m_codeitems.at(methodidx)->insn_size * sizeof(u16); }

address_t DexLoader::nextImport(address_t *res)
{
    address_t importbase = m_importbase;
    m_importbase += sizeof(u16);

    if(res)
        *res = importbase;

    return importbase;
}

bool DexLoader::getClassData(const DEXClassIdItem &dexclass, DEXClassData &dexclassdata)
{
    if(!dexclass.class_data_off)
        return false;

    DEXEncodedField dexfield;
    DexEncodedMethod dexmethod;
    u8* pclassdata = pointer<u8>(dexclass.class_data_off);

    dexclassdata.static_fields_size = LEB128::unsignedOf<u32>(pclassdata, &pclassdata);
    dexclassdata.instance_fields_size = LEB128::unsignedOf<u32>(pclassdata, &pclassdata);
    dexclassdata.direct_methods_size = LEB128::unsignedOf<u32>(pclassdata, &pclassdata);
    dexclassdata.virtual_methods_size = LEB128::unsignedOf<u32>(pclassdata, &pclassdata);

    for(u32 i = 0; i < dexclassdata.static_fields_size; i++)
    {
        dexfield.field_idx_diff = LEB128::unsignedOf<u32>(pclassdata, &pclassdata);
        dexfield.access_flags = LEB128::unsignedOf<u32>(pclassdata, &pclassdata);
        dexclassdata.static_fields.push_back(dexfield);
    }

    for(u32 i = 0; i < dexclassdata.instance_fields_size; i++)
    {
        dexfield.field_idx_diff = LEB128::unsignedOf<u32>(pclassdata, &pclassdata);
        dexfield.access_flags = LEB128::unsignedOf<u32>(pclassdata, &pclassdata);
        dexclassdata.instance_fields.push_back(dexfield);
    }

    for(u32 i = 0; i < dexclassdata.direct_methods_size; i++)
    {
        dexmethod.method_idx_diff = LEB128::unsignedOf<u32>(pclassdata, &pclassdata);
        dexmethod.access_flags = LEB128::unsignedOf<u32>(pclassdata, &pclassdata);
        dexmethod.code_off = LEB128::unsignedOf<u32>(pclassdata, &pclassdata);
        dexclassdata.direct_methods.push_back(dexmethod);
    }

    for(u32 i = 0; i < dexclassdata.virtual_methods_size; i++)
    {
        dexmethod.method_idx_diff = LEB128::unsignedOf<u32>(pclassdata, &pclassdata);
        dexmethod.access_flags = LEB128::unsignedOf<u32>(pclassdata, &pclassdata);
        dexmethod.code_off = LEB128::unsignedOf<u32>(pclassdata, &pclassdata);
        dexclassdata.virtual_methods.push_back(dexmethod);
    }

    return true;
}

void DexLoader::loadMethod(const DexEncodedMethod &dexmethod, u16& idx, bool filter)
{
    if(!dexmethod.code_off) return;

    if(!idx) idx = dexmethod.method_idx_diff;
    else idx += dexmethod.method_idx_diff;

    DEXCodeItem* dexcode = pointer<DEXCodeItem>(dexmethod.code_off);

    m_encmethods[idx] = dexmethod;
    m_codeitems[idx] = dexcode;

    const String& methodname = this->getMethodName(idx);

    if(filter)
        ldrdoc->imported(fileoffset(&dexcode->insns), sizeof(u16), methodname, idx);
    else
        ldrdoc->exportedFunction(fileoffset(&dexcode->insns), methodname, idx);
}

void DexLoader::loadClass(const DEXClassIdItem &dexclass, bool filter)
{
    DEXClassData dexclassdata;

    if(!this->getClassData(dexclass, dexclassdata))
        return;

    u16 idx = 0;

    std::for_each(dexclassdata.direct_methods.begin(), dexclassdata.direct_methods.end(), [&](const DexEncodedMethod& dexmethod) {
        this->loadMethod(dexmethod, idx, filter);
    });

    idx = 0;

    std::for_each(dexclassdata.virtual_methods.begin(), dexclassdata.virtual_methods.end(), [&](const DexEncodedMethod& dexmethod) {
        this->loadMethod(dexmethod, idx, filter);
    });
}

void DexLoader::filterClasses(const DEXClassIdItem *dexclasses)
{
    UI::CheckList items;

    for(u32 i = 0; i < m_header->class_defs_size; i++)
    {
        const String& classtype = this->getType(dexclasses[i].class_idx, true);
        bool precheck = true;

        if(!classtype.startsWith("android.") || !classtype.startsWith("com.google.")) // Apply prefiltering
            precheck = false;

        items.push_back({ classtype , precheck });
    }

    r_ui->checkList("Class Loader", "Select one or more classes from the list below", items);

    for(u32 i = 0; i < m_header->class_defs_size; i++)
        this->loadClass(dexclasses[i], !items[i].second);
}

const String &DexLoader::getNormalizedString(u32 idx)
{
    return cacheEntry(idx, m_cachednstrings, [&](String& s) {
        s = this->normalized(this->getString(idx));
    });
}

const String &DexLoader::getTypeList(u32 typelistoff)
{
    return cacheEntry(typelistoff, m_cachedtypelist, [=](String& s) {
        u32 size = *pointer<u32>(typelistoff);
        DEXTypeItem* dextypeitem = pointer<DEXTypeItem>(typelistoff + sizeof(u32));

        for(u32 i = 0; i < size; i++) {
            if(i)
                s += ", ";

            s += this->getType(dextypeitem[i].type_idx);
        }
    });
}

const String &DexLoader::cacheEntry(u32 idx, std::unordered_map<u32, String> &cache, const std::function<void(String &)> &cb)
{
    auto it = cache.find(idx);

    if(it != cache.end())
        return it->second;

    String s;
    cb(s);

    auto iit = cache.emplace(idx, std::move(s));
    return iit.first->second;
}

bool DexLoader::validateSignature(const DexHeader *header)
{
    if(strncmp(header->dex, DEX_FILE_MAGIC, 3))
        return false;

    if(header->newline != '\n')
        return false;

    for(u32 i = 0; i < 3; i++)
    {
        if(!std::isdigit(header->version[i]))
            return false;
    }

    if(header->zero != '\0')
        return false;

    return true;
}

String DexLoader::normalized(const String &type)
{
    if(type[0] == '[')
        return DexLoader::normalized(type.left(1)) + "[]";

    if(type == "V")
        return "void";
    if(type == "Z")
        return "boolean";
    if(type == "B")
        return "byte";
    if(type == "S")
        return "short";
    if(type == "C")
        return "char";
    if(type == "I")
        return "int";
    if(type == "J")
        return "long";
    if(type == "F")
        return "float";
    if(type == "D")
        return "double";

    String s = type;

    if(s.first() == 'L')
       s.removeFirst();

    if(s.last() == ';')
       s.removeLast();

    s.replace('/', '.');
    return s;
}
