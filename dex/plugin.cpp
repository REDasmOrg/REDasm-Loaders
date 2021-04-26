#include "assembler/dalvik.h"
#include "loader/dex.h"
#include "../common_types.h"

static void createTypes(RDContext* ctx) {
    auto* db = RDContext_GetDatabase(ctx);

    rd_ptr<RDType> s(RDType_CreateStructure(DALVIK_FILLARRAYDATA_NAME));
    RDStructure_Append(s.get(), RDType_CreateInt(2, false), "ident");
    RDStructure_Append(s.get(), RDType_CreateInt(2, false), "element_width");
    RDStructure_Append(s.get(), RDType_CreateInt(4, false), "size");
    RDDatabase_WriteType(db, DALVIK_FILLARRAYDATA_PATH, s.get());

    s.reset(RDType_CreateStructure(DALVIK_PACKEDSWITCH_NAME));
    RDStructure_Append(s.get(), RDType_CreateInt(2, false), "ident");
    RDStructure_Append(s.get(), RDType_CreateInt(2, false), "size");
    RDStructure_Append(s.get(), RDType_CreateInt(4, false), "first_key");
    RDDatabase_WriteType(db, DALVIK_PACKEDSWITCH_PATH, s.get());

    s.reset(RDType_CreateStructure(DALVIK_SPARSESWITCH_NAME));
    RDStructure_Append(s.get(), RDType_CreateInt(2, false), "ident");
    RDStructure_Append(s.get(), RDType_CreateInt(2, false), "size");
    RDDatabase_WriteType(db, DALVIK_SPARSESWITCH_PATH, s.get());
}

void rdplugin_init(RDContext*, RDPluginModule* pm)
{
    RD_PLUGIN_ENTRY(RDEntryAssembler, dalvik, "Dalvik");
    dalvik.bits = 32;
    dalvik.emulate = &DalvikAssembler::emulate;
    dalvik.renderinstruction = &DalvikAssembler::renderInstruction;
    dalvik.renderfunction = &DalvikAssembler::renderFunction;
    RDAssembler_Register(pm, &dalvik);

    RD_PLUGIN_ENTRY(RDEntryLoader, dex, "Dalvik Executable");
    dex.test = &DexLoader::test;

    dex.load = [](RDContext* ctx) {
        createTypes(ctx);
        RDContext_DisableAnalyzer(ctx, "analyzerunexplored_builtin");

        auto* dexloader = new DexLoader(ctx);
        RDContext_SetUserData(ctx, DEXLOADER_USERDATA, reinterpret_cast<uintptr_t>(dexloader));
        return dexloader->load();
    };

    RDLoader_Register(pm, &dex);
}

void rdplugin_free(RDContext* ctx)
{
    auto* dexloader = reinterpret_cast<DexLoader*>(RDContext_GetUserData(ctx, DEXLOADER_USERDATA));
    if(dexloader) delete dexloader;
}
