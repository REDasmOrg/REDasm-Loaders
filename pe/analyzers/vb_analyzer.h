#pragma once

#include "../vb/vb_header.h"

class PELoader;

class VBAnalyzer
{
    public:
        VBAnalyzer(RDContext* ctx, PELoader* peloader);
        void analyze();

    private:
        void disassembleTrampoline(rd_address eventva, const std::string &name);
        void decompileObject(RDLoader* loader, const VBPublicObjectDescriptor& pubobjdescr);
        bool decompile(rd_address thunrtdata);

    private:
        PELoader* m_peloader;
        RDContext* m_context;
        VBHeader* m_vbheader{nullptr};
        VBProjectInfo* m_vbprojinfo{nullptr};
        VBObjectTable* m_vbobjtable{nullptr};
        VBObjectTreeInfo* m_vbobjtreeinfo{nullptr};
        VBPublicObjectDescriptor* m_vbpubobjdescr{nullptr};
};
