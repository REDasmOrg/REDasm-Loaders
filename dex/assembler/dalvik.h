#pragma once

// http://pallergabor.uw.hu/androidblog/dalvik_opcodes.html

#include <rdapi/rdapi.h>
#include <unordered_map>
#include <optional>
#include <libdex/InstrUtils.h>

struct DalvikIndex {
    dex_u4 index{0};
    dex_u4 secindex{0};
    dex_u4 width{0};
};

class DalvikAssembler
{
    public:
        DalvikAssembler() = delete;
        static void emulate(RDContext* ctx, RDEmulateResult* result);
        static void renderInstruction(RDContext* ctx, const RDRendererParams* rp);
        static void renderFunction(RDContext* ctx, const RDRendererParams* rp);

    private:
        static std::string reg(dex_u4 id);
        static void getIndex(const DecodedInstruction& di, DalvikIndex& res);
        static void renderIndex(RDContext* ctx, const DecodedInstruction& di, RDRenderer* r);
        static rd_type getTheme(const DecodedInstruction& di);
        static bool isInvoke(const DecodedInstruction& di);
        static std::optional<std::string> getMethodInfo(RDContext* ctx, dex_u4 methodidx);
        static std::optional<std::string> getFieldInfo(RDContext* ctx, dex_u4 fieldidx);
        static std::optional<std::string> getFunctionName(RDContext* ctx, const DexMethodId* methodid);

    private:
        static std::unordered_map<u16, std::string> m_methodcache, m_fieldcache;
        static std::unordered_map<rd_address, std::string> m_functioncache;
};
