#pragma once

#include <rdapi/rdapi.h>
#include <type_traits>
#include "elf_header.h"

class ElfLoader
{
    public:
        virtual ~ElfLoader() = default;
        virtual size_t endianness() const = 0;
        virtual const char* assembler() const = 0;

    protected:
        virtual void loadSegments(RDDocument* doc) = 0;
        virtual void parseSegments(RDLoader* loader, RDDocument* doc) = 0;
        virtual void checkProgramHeader(RDDocument* doc) = 0;
        virtual void checkArray(RDLoader* loader, RDDocument* doc) = 0;
        virtual void checkEntryPoint(RDDocument* doc) = 0;

    public:
        static ElfLoader* parse(RDBuffer* buffer);
        static const char* test(const RDLoaderPlugin*, const RDLoaderRequest* request);
        static void analyze(RDLoaderPlugin* plugin, RDDisassembler* disassembler);
        static bool load(RDLoaderPlugin* plugin, RDLoader* loader);
};

template<size_t bits> class ElfLoaderT: public ElfLoader
{
    protected:

    public:
        typedef Elf_Ehdr<bits> EHDR;
        typedef Elf_Shdr<bits> SHDR;
        typedef Elf_Rel<bits> REL;
        typedef Elf_Rela<bits> RELA;
        typedef typename std::conditional<bits == 64, Elf64_Phdr, Elf32_Phdr>::type PHDR;
        typedef typename std::conditional<bits == 64, Elf64_Sym, Elf32_Sym>::type SYM;
        typedef typename elf_unsigned_t<bits>::type ADDR;

    public:
        ElfLoaderT(RDBuffer* buffer);
        size_t endianness() const override;
        const char* assembler() const override;

    protected:
        void loadSegments(RDDocument* doc) override;
        void parseSegments(RDLoader* loader, RDDocument* doc) override;
        void checkProgramHeader(RDDocument* doc) override;
        void checkArray(RDLoader* loader, RDDocument* doc) override;
        void checkEntryPoint(RDDocument* doc) override;

    private:
        void loadSymbols(const SHDR& shdr, RDLoader* loader, RDDocument* doc);
        u64 relocationSymbol(const REL* rel) const;
        bool relocate(RDLoader* loader, u64 symidx, u64* value) const;

    private:
        RDBuffer* m_buffer;
        EHDR* m_ehdr;
        SHDR* m_shdr;
        PHDR* m_phdr;
};
