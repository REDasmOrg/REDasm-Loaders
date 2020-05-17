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
        void findMain_x86(RDAssemblerPlugin* assembler, const RDSymbol* symlibcmain);
        void findMainMode_x86_64(size_t blockidx);
        void findMainMode_x86_32(size_t blockidx);

   private:
        void disassembleLibStartMain();
        bool getLibStartMain(RDSymbol* symbol);

   protected:
        std::unordered_map<std::string, address_t> m_libcmain;
        RDDisassembler* m_disassembler;
        RDLoaderPlugin* m_plugin;
        ElfLoader* m_loader;
};
