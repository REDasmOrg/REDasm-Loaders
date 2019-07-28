#pragma once

#include <unordered_map>
#include <redasm/redasm.h>
#include <redasm/plugins/loader/analyzer.h>

using namespace REDasm;

class ElfAnalyzer: public Analyzer
{
    public:
        ElfAnalyzer();
        void analyze() override;

    private:
        void findMain_x86(const Symbol* symlibcmain);
        void findMain_x86_64(size_t index);
        void findMain_x86(size_t index);

   private:
        void disassembleLibStartMain();
        Symbol *getLibStartMain();

   protected:
        std::unordered_map<String, address_t> m_libcmain;
};
