#pragma once

#include "elf_analyzer.h"

class ElfAnalyzerX86: public ElfAnalyzer
{
    public:
        ElfAnalyzerX86(ElfLoader* loader, RDContext* ctx);
        void analyze() override;

    private:
        //void checkPLT_x86_64(RDDocument* doc, RDInstruction* instruction, RDAssembler* assembler, rd_address funcaddress);
        //void checkPLT_x86(RDDocument* doc, RDInstruction* instruction, RDAssembler* assembler, rd_address funcaddress);
};

