#pragma once

#include "elf_analyzer.h"

class ElfAnalyzer_ARM: public ElfAnalyzer
{
    public:
        ElfAnalyzer_ARM(RDContext* ctx);
        void analyze() override;

    protected:
        void findMain(rd_address address) override;

    private:
        void checkPLT32(RDDocument* doc, rd_address funcaddress);
        void checkPLT64(RDDocument* doc, rd_address funcaddress);
};

