#pragma once

#include <redasm/plugins/loader/analyzer.h>
#include "pe_classifier.h"
#include <forward_list>

class PEAnalyzer: public Analyzer
{
    private:
        typedef std::pair<size_t, String> APIInfo;

    public:
        PEAnalyzer(const PEClassifier* classifier, Disassembler* disassembler);
        void analyze() override;

    private:
        Symbol *getImport(const String &library, const String &api);
        ReferenceVector getAPIReferences(const String &library, const String &api);
        void findWndProc(address_t address, size_t argidx);
        void findCRTWinMain();
        void findAllWndProc();

    private:
        const PEClassifier* m_classifier;
        std::forward_list<APIInfo> m_wndprocapi;
};
