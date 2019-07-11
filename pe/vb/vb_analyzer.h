#pragma once

#include "../pe_analyzer.h"
#include "vb_header.h"

class VBAnalyzer : public PEAnalyzer
{
    public:
        VBAnalyzer(const PEClassifier* classifier, Disassembler* disassembler);
        void analyze() override;

    private:
        void disassembleTrampoline(address_t eventva, const String &name);
        void decompileObject(const VBPublicObjectDescriptor& pubobjdescr);
        bool decompile(address_t thunrtdata);

    private:
        const Loader* m_loader;
        VBHeader* m_vbheader;
        VBProjectInfo* m_vbprojinfo;
        VBObjectTable* m_vbobjtable;
        VBObjectTreeInfo* m_vbobjtreeinfo;
        VBPublicObjectDescriptor* m_vbpubobjdescr;
};
