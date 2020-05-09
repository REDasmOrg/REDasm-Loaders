#pragma once

#include "../pe_analyzer.h"
#include "vb_header.h"

class VBAnalyzer : public PEAnalyzer
{
    public:
        VBAnalyzer(PELoader* loader, RDDisassembler* disassembler);
        void analyze() override;

    private:
        void disassembleTrampoline(address_t eventva, const std::string &name);
        void decompileObject(RDLoader* loader, const VBPublicObjectDescriptor& pubobjdescr);
        bool decompile(address_t thunrtdata);

    private:
        VBHeader* m_vbheader{nullptr};
        VBProjectInfo* m_vbprojinfo{nullptr};
        VBObjectTable* m_vbobjtable{nullptr};
        VBObjectTreeInfo* m_vbobjtreeinfo{nullptr};
        VBPublicObjectDescriptor* m_vbpubobjdescr{nullptr};
};
