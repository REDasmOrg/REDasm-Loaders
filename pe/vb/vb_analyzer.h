#pragma once

#include "../pe_analyzer.h"
#include "vb_header.h"

class VBAnalyzer : public PEAnalyzer
{
    public:
        VBAnalyzer(PELoader* loader, RDDisassembler* disassembler);
        void analyze() override;

    private:
        void disassembleTrampoline(rd_address eventva, const std::string &name);
        void decompileObject(RDLoader* loader, const VBPublicObjectDescriptor& pubobjdescr);
        bool decompile(rd_address thunrtdata);

    private:
        VBHeader* m_vbheader{nullptr};
        VBProjectInfo* m_vbprojinfo{nullptr};
        VBObjectTable* m_vbobjtable{nullptr};
        VBObjectTreeInfo* m_vbobjtreeinfo{nullptr};
        VBPublicObjectDescriptor* m_vbpubobjdescr{nullptr};
};
