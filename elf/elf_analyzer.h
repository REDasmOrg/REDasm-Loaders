#pragma once

#include <redasm/redasm.h>
#include <redasm/plugins/loader/analyzer.h>
#include <redasm/disassembler/listing/listingdocumentiterator.h>

using namespace REDasm;

class ElfAnalyzer: public Analyzer
{
    public:
        ElfAnalyzer(Disassembler* disassembler);
        void analyze() override;

    private:
        void findMain_x86(const Symbol* symlibcmain);
        void findMain_x86_64(ListingDocumentIterator& it);
        void findMain_x86(ListingDocumentIterator& it);

   private:
        void disassembleLibStartMain();
        Symbol *getLibStartMain();

   protected:
        std::unordered_map<String, address_t> m_libcmain;
};
