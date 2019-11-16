#pragma once

#include <redasm/plugins/loader/loader.h>
#include "elf_header.h"

#define ELF_STRING_TABLE this->m_shdr[this->m_ehdr->e_shstrndx]
#define ELF_STRING(shdr, offset) String(m_loader->pointer<const char>((shdr)->sh_offset + offset))

using namespace REDasm;

class ElfLoader;

namespace ElfTypes {
    template<size_t b, endianness_t e> struct elf_address_t { };
    template<> struct elf_address_t<32, Endianness::LittleEndian> { typedef u32le e_type; typedef u32 type; };
    template<> struct elf_address_t<32, Endianness::BigEndian> { typedef u32be e_type;  typedef u32 type;};
    template<> struct elf_address_t<64, Endianness::LittleEndian> { typedef u64le e_type; typedef u64 type; };
    template<> struct elf_address_t<64, Endianness::BigEndian> { typedef u64be e_type;  typedef u64 type; };
}

class ElfFormat
{
    public:
        virtual AssemblerRequest assembler() const = 0;
        virtual void load() = 0;
};

template<size_t b, endianness_t e> class ElfFormatT: public ElfFormat
{
    protected:

    public:
        typedef Elf_Ehdr<b, e> EHDR;
        typedef Elf_Shdr<b, e> SHDR;
        typedef Elf_Rel<b, e> REL;
        typedef Elf_Rela<b, e> RELA;
        typedef typename std::conditional<b == 64, Elf64_Phdr<e>, Elf32_Phdr<e> >::type PHDR;
        typedef typename std::conditional<b == 64, Elf64_Sym<e>, Elf32_Sym<e> >::type SYM;
        typedef typename ElfTypes::elf_address_t<b, e>::e_type E_ADDR;
        typedef typename ElfTypes::elf_address_t<b, e>::type ADDR;

    public:
        ElfFormatT(ElfLoader* loader);
        AssemblerRequest assembler() const override;
        void load() override;

    private:
        u64 relocationSymbol(const REL* rel) const;
        bool relocate(u64 symidx, u64* value) const;
        void loadSegments();
        void loadSymbols(const SHDR& shdr);
        void checkProgramHeader();
        void checkArray();
        void parseSegments();

    private:
        ElfLoader* m_loader;
        const EHDR* m_ehdr;
        const SHDR* m_shdr;
        const PHDR* m_phdr;
};
