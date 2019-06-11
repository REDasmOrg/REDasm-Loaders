#pragma once

// https://source.android.com/devices/tech/dalvik/dex-format

#include <redasm/plugins/loader/loader.h>
#include "dex_header.h"

using namespace REDasm;

class DexLoader : public Loader
{
    public:
        DexLoader();
        AssemblerRequest assembler() const override;
        bool test(const LoadRequest &request) const override;
        void load() override;

    public:
        bool getMethodOffset(u32 idx, offset_t &offset) const;
        bool getStringOffset(u32 idx, offset_t &offset) const;
        const std::string& getType(u32 idx, bool full = false);
        const std::string& getString(u32 idx);
        const std::string& getMethodName(u32 idx);
        const std::string& getMethodProto(u32 idx);
        const std::string& getField(u32 idx);
        const std::string& getReturnType(u32 methodidx);
        const std::string& getParameters(u32 methodidx);
        bool getMethodInfo(u32 methodidx, DexEncodedMethod& dexmethod);
        bool getDebugInfo(u32 methodidx, DexDebugInfo& debuginfo);
        u32 getMethodSize(u32 methodidx) const;
        address_t nextImport(address_t *res = nullptr);

    private:
        void filterClasses(const DEXClassIdItem* dexclasses);
        bool getClassData(const DEXClassIdItem& dexclass, DEXClassData& dexclassdata);
        void loadMethod(const DexEncodedMethod& dexmethod, u16 &idx, bool filter);
        void loadClass(const DEXClassIdItem& dexclass, bool filter);
        const std::string &getNormalizedString(u32 idx);
        const std::string &getTypeList(u32 typelistoff);

    private:
        static const std::string& cacheEntry(u32 idx, std::unordered_map<u32, std::string> &cache, const std::function<void(std::string&)>& cb);
        static std::string normalized(const std::string& type);

    public:
        static bool validateSignature(const DexHeader *header);

    private:
        u64 m_importbase;
        const DexHeader* m_header;
        std::unordered_map<u32, DEXCodeItem*> m_codeitems;
        std::unordered_map<u32, DexEncodedMethod> m_encmethods;
        DEXTypeIdItem* m_types;
        DEXStringIdItem* m_strings;
        DEXMethodIdItem* m_methods;
        DEXFieldIdItem* m_fields;
        DEXProtoIdItem* m_protos;

    private: // Caching
        static const std::string m_invalidstring;
        std::unordered_map<u32, std::string> m_cachedstrings;
        std::unordered_map<u32, std::string> m_cachednstrings;
        std::unordered_map<u32, std::string> m_cachedfields;
        std::unordered_map<u32, std::string> m_cachedtypes;
        std::unordered_map<u32, std::string> m_cachedtypelist;
        std::unordered_map<u32, std::string> m_cachedparameters;
        std::unordered_map<u32, std::string> m_cachedmethodnames;
        std::unordered_map<u32, std::string> m_cachedmethodproto;
};
