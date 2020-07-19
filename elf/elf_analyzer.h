#pragma once

#include <unordered_map>
#include <rdapi/rdapi.h>

class ElfLoader;

class ElfAnalyzer
{
    public:
        ElfAnalyzer(RDLoaderPlugin* plugin, RDDisassembler* disassembler);
        void analyze();

    private:
        void findMain_x86(RDAssemblerPlugin* assembler, RDDocument* doc, const RDSymbol* symlibcmain);
        void findMainMode_x86_64(RDDocument* doc, const RDBlockContainer* blocks, size_t blockidx);
        void findMainMode_x86_32(RDDocument* doc, const RDBlockContainer* blocks, size_t blockidx);

   private:
        void disassembleLibStartMain();
        bool getLibStartMain(RDSymbol* symbol);

   protected:
        std::unordered_map<std::string, rd_address> m_libcmain;
        RDDisassembler* m_disassembler;
        RDLoaderPlugin* m_plugin;
        ElfLoader* m_loader;
};
