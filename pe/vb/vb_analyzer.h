#pragma once

#include "../pe_analyzer.h"
#include "vb_header.h"

class VBAnalyzer : public PEAnalyzer
{
    public:
        VBAnalyzer(const PEClassifier* classifier);
        void analyze() override;

    private:
        void disassembleTrampoline(address_t eventva, const String &name);
        void decompileObject(const VBPublicObjectDescriptor& pubobjdescr);
        bool decompile(address_t thunrtdata);

    private:
        VBHeader* m_vbheader{nullptr};
        VBProjectInfo* m_vbprojinfo{nullptr};
        VBObjectTable* m_vbobjtable{nullptr};
        VBObjectTreeInfo* m_vbobjtreeinfo{nullptr};
        VBPublicObjectDescriptor* m_vbpubobjdescr{nullptr};
};
