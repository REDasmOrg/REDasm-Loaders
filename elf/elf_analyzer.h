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
        void findMainMode_x86_64(size_t index);
        void findMainMode_x86_32(size_t index);

   private:
        void disassembleLibStartMain();
        const Symbol* getLibStartMain();

   protected:
        std::unordered_map<String, address_t> m_libcmain;
};
