#pragma once

// https://github.com/JesusFreke/smali/wiki/TypesMethodsAndFields
// https://source.android.com/devices/tech/dalvik/dex-format
// https://android.googlesource.com/platform/dalvik

#include <libdex/DexClass.h>
#include <libdex/DexFile.h>
#include <rdapi/rdapi.h>
#include <unordered_map>
#include <optional>
#include <string>
#include "dex_constants.h"

class DexLoader
{
    public:
        DexLoader(RDContext* ctx);
        ~DexLoader();
        const DexFile* dexFile() const;
        bool load();

    public:
        const DexMethodId* addressToMethodId(rd_address address) const;
        std::optional<u16> addressToMethodIdx(rd_address address) const;

    public:
        static const char* test(const RDLoaderRequest* request);
        static std::string demangle(std::string s);

    private:
        bool filterClasses();
        void loadMethod(const DexMethod& dexmethod, bool filter);
        void loadClass(const DexClassDef* classdef, bool filter);

    private:
        std::unordered_map<rd_address, const DexMethodId*> m_methodids;
        std::unordered_map<rd_address, u16> m_methodidx;
        DexFile* m_dexfile{nullptr};
        RDContext* m_context;
};
