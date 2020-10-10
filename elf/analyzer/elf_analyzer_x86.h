#pragma once

#include "elf_analyzer.h"

class ElfAnalyzerX86: public ElfAnalyzer
{
    public:
        ElfAnalyzerX86(RDContext* ctx);
        void analyze() override;

    protected:
        void findMain(rd_address address) override;

    private:
        void findMain32(rd_address address);
        void findMain64(rd_address address);

    private:
        //void checkPLT_x86_64(RDDocument* doc, RDInstruction* instruction, RDAssembler* assembler, rd_address funcaddress);
        //void checkPLT_x86(RDDocument* doc, RDInstruction* instruction, RDAssembler* assembler, rd_address funcaddress);
};

