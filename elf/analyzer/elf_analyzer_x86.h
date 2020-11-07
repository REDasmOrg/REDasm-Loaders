#pragma once

#include "elf_analyzer.h"

class ElfAnalyzerX86: public ElfAnalyzer
{
    public:
        ElfAnalyzerX86(RDContext* ctx);
        void analyze() override;

    protected:
        void findMain(rd_address address) override;
        void parsePlt();

    private:
        void findMain32(rd_address address);
        void findMain64(rd_address address);
        void checkPLT32(RDDocument* doc, rd_address funcaddress);
        void checkPLT64(RDDocument* doc, rd_address funcaddress);

    private:
        RDDocument* m_document{nullptr};
        RDSegment m_segment{ };
};

