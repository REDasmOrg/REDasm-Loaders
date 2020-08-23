#pragma once

#include <rdapi/rdapi.h>
#include <type_traits>
#include <optional>
#include "elf_header.h"

class ElfABI;

class ElfLoader
{
    public:
        virtual ~ElfLoader() = default;
        virtual size_t endianness() const = 0;
        virtual const char* assembler() const = 0;
        virtual RDLoader* loader() const = 0;
        virtual RDDocument* document() const = 0;
        virtual const ElfABI* abi() const = 0;
        virtual const u8* plt() const = 0;
        virtual u16 machine() const = 0;

    protected:
        virtual void doLoad(RDLoader* loader) = 0;

    public:
        static ElfLoader* parse(RDBuffer* buffer);
        static const char* test(const RDLoaderPlugin*, const RDLoaderRequest* request);
        static void analyze(RDLoaderPlugin* plugin, RDDisassembler* disassembler);
        static bool load(RDLoaderPlugin* plugin, RDLoader* loader);
};

template<size_t bits> class ElfLoaderT: public ElfLoader
{
    public:
        typedef Elf_Ehdr<bits> EHDR;
        typedef Elf_Shdr<bits> SHDR;
        typedef Elf_Rel<bits> REL;
        typedef Elf_Rela<bits> RELA;
        typedef Elf_Dyn<bits> DYN;
        typedef typename std::conditional<bits == 64, Elf64_Phdr, Elf32_Phdr>::type PHDR;
        typedef typename std::conditional<bits == 64, Elf64_Sym, Elf32_Sym>::type SYM;
        typedef typename elf_unsigned_t<bits>::type SVAL;
        typedef typename elf_unsigned_t<bits>::type UVAL;
        typedef UVAL ADDR;

    public:
        ElfLoaderT(RDBuffer* buffer);
        size_t endianness() const override;
        const char* assembler() const override;
        RDLoader* loader() const override;
        RDDocument* document() const override;
        const ElfABI* abi() const override;
        const u8* plt() const override;
        u16 machine() const override;
        std::optional<UVAL> dynamic(SVAL d) const;
        std::optional<std::string> symbolName(UVAL sectidx, UVAL symidx) const;
        const SHDR* findSegment(rd_address address) const;

    protected:
        void doLoad(RDLoader* loader) override;

    private:
        void readProgramHeader(RDDocument* doc);
        void readDynamic(const PHDR* phdr, RDDocument* doc);
        void loadSegments(const PHDR* phdr, RDDocument* doc);
        void readArray(RDDocument* doc, UVAL address, UVAL size, SVAL type);
        void readVersions(UVAL address, UVAL count);

    private:
        bool findSegments(const PHDR* phdr, std::vector<const SHDR*>& segments) const;
        const SHDR* findSegment(const PHDR* phdr) const;
        void loadSymbols(const SHDR& shdr, RDLoader* loader, RDDocument* doc);

    public:
        template<typename T> const T* elfdynptr(SVAL d, const SHDR** resshdr) const {
            auto it = m_dynamic.find(d);
            return (it != m_dynamic.end()) ? this->elfptr<T>(it->second, resshdr) : nullptr;
        }

        template<typename T> const T* elfptr(rd_address address, const SHDR** resshdr) const {
            const auto* shdr = this->findSegment(address);
            if(!shdr || (shdr->sh_type == SHT_NOBITS)) return nullptr;
            if(resshdr) *resshdr = shdr;
            return this->elfptr<T>(shdr, address - shdr->sh_addr);
        }

        template<typename T> const T* elfptr(const SHDR* shdr, rd_offset offset) const {
            return reinterpret_cast<const T*>(reinterpret_cast<u8*>(RDLoader_GetData(m_loader)) + (shdr->sh_offset + offset));
        }

    private:
        std::unordered_map<UVAL, std::string> m_versions;
        std::unordered_map<SVAL, UVAL> m_dynamic;
        std::unique_ptr<ElfABI> m_abi;
        RDLoader* m_loader{nullptr};
        RDBuffer* m_buffer;
        EHDR* m_ehdr;
        SHDR* m_shdr;
        PHDR* m_phdr;
};
