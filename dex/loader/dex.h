#pragma once

// https://source.android.com/devices/tech/dalvik/dex-format

#include <unordered_map>
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
        const String& getType(u32 idx, bool full = false);
        const String& getString(u32 idx);
        const String& getMethodName(u32 idx);
        const String& getMethodProto(u32 idx);
        const String& getField(u32 idx);
        const String& getReturnType(u32 methodidx);
        const String& getParameters(u32 methodidx);
        bool getMethodInfo(u32 methodidx, DexEncodedMethod& dexmethod);
        bool getDebugInfo(u32 methodidx, DexDebugInfo& debuginfo);
        u32 getMethodSize(u32 methodidx) const;
        address_t nextImport(address_t *res = nullptr);

    private:
        void filterClasses(const DEXClassIdItem* dexclasses);
        bool getClassData(const DEXClassIdItem& dexclass, DEXClassData& dexclassdata);
        void loadMethod(const DexEncodedMethod& dexmethod, u16 &idx, bool filter);
        void loadClass(const DEXClassIdItem& dexclass, bool filter);
        const String &getNormalizedString(u32 idx);
        const String &getTypeList(u32 typelistoff);

    private:
        static const String& cacheEntry(u32 idx, std::unordered_map<u32, String> &cache, const std::function<void(String&)>& cb);
        static String normalized(const String &type);

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
        static const String m_invalidstring;
        std::unordered_map<u32, String> m_cachedstrings;
        std::unordered_map<u32, String> m_cachednstrings;
        std::unordered_map<u32, String> m_cachedfields;
        std::unordered_map<u32, String> m_cachedtypes;
        std::unordered_map<u32, String> m_cachedtypelist;
        std::unordered_map<u32, String> m_cachedparameters;
        std::unordered_map<u32, String> m_cachedmethodnames;
        std::unordered_map<u32, String> m_cachedmethodproto;
};
