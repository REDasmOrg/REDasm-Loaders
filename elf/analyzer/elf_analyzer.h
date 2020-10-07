#pragma once

#include <unordered_map>
#include <rdapi/rdapi.h>

class ElfLoader;

class ElfAnalyzer
{
    public:
        ElfAnalyzer(ElfLoader* loader, RDContext* ctx);
        virtual void analyze();

    private:
        void findMain_x86(const std::string& id, RDDocument* doc, const RDSymbol* symlibcmain);
        void findMainMode_x86_64(RDDocument* doc, const RDBlockContainer* blocks, size_t blockidx);
        void findMainMode_x86_32(RDDocument* doc, const RDBlockContainer* blocks, size_t blockidx);

    private:
        void disassembleLibStartMain();
        bool getLibStartMain(RDSymbol* symbol);

    private:
        std::unordered_map<std::string, rd_address> m_libcmain;

    protected:
        RDContext* m_context;
        ElfLoader* m_loader;
};
