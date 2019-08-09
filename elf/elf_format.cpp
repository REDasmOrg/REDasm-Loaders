#include "elf_format.h"
#include "elf.h"

template<size_t b, endianness_t e> ElfFormatT<b, e>::ElfFormatT(ElfLoader *loader): m_loader(loader)
{
    this->m_ehdr = m_loader->pointer<EHDR>();
    this->m_shdr = m_loader->pointer<SHDR>(this->m_ehdr->e_shoff);
    this->m_phdr = m_loader->pointer<PHDR>(this->m_ehdr->e_phoff);
}

template<size_t b, endianness_t e> AssemblerRequest ElfFormatT<b, e>::assembler() const
{
    switch(this->m_ehdr->e_machine)
    {
        case EM_AVR: return "avr8";
        case EM_386: return ASSEMBLER_REQUEST("x86", "x86_32");
        case EM_X86_64: return ASSEMBLER_REQUEST("x86", "x86_64");
        case EM_XTENSA: return ASSEMBLER_REQUEST("xtensa", (e == Endianness::BigEndian) ? "xtensabe" : "xtensale");

        case EM_ARM:
            if(this->m_ehdr->e_ident[EI_CLASS] == ELFCLASS64)
                return "arm64";
            return ASSEMBLER_REQUEST("arm", "armthumb");

        case EM_MIPS:
            if(this->m_ehdr->e_flags & EF_MIPS_ABI_EABI64)
                return ASSEMBLER_REQUEST("mips",  e == Endianness::BigEndian ? "mips64be" : "mips64le");
            return ASSEMBLER_REQUEST("mips", e == Endianness::BigEndian ? "mips32be" : "mips32le");

        default: break;
    }

    return nullptr;
}

template<size_t b, endianness_t e> void ElfFormatT<b, e>::load()
{
    this->loadSegments();
    this->parseSegments();
    this->checkProgramHeader();
    this->checkArray();

    if(m_loader->document()->segment(this->m_ehdr->e_entry))
        m_loader->document()->entry(this->m_ehdr->e_entry);
}

template<size_t b, endianness_t e> u64 ElfFormatT<b, e>::relocationSymbol(const REL* rel) const
{
    if(b == 64)
        return ELF64_R_SYM(rel->r_info);

    return ELF32_R_SYM(rel->r_info);
}

template<size_t b, endianness_t e> bool ElfFormatT<b, e>::relocate(u64 symidx, u64* value) const
{
    for(u64 i = 0; i < this->m_ehdr->e_shnum; i++)
    {
        const SHDR& shdr = this->m_shdr[i];

        if((shdr.sh_type != SHT_REL) && (shdr.sh_type != SHT_RELA))
            continue;

        offset_t offset = shdr.sh_offset, endoffset = offset + shdr.sh_size;

        while(offset < endoffset)
        {
            REL* rel = m_loader->pointer<REL>(offset);
            u64 sym = this->relocationSymbol(rel);

            if(sym == symidx)
            {
                *value = rel->r_offset;
                return true;
            }

            offset += (shdr.sh_type == SHT_REL) ? sizeof(REL) : sizeof(RELA);
        }
    }

    return false;
}

template<size_t b, endianness_t e> void ElfFormatT<b, e>::loadSegments()
{
    const SHDR& shstr = ELF_STRING_TABLE;

    for(u64 i = 0; i < this->m_ehdr->e_shnum; i++)
    {
        const SHDR& shdr = this->m_shdr[i];

        switch(shdr.sh_type)
        {
            case SHT_PROGBITS:
            case SHT_PREINIT_ARRAY:
            case SHT_INIT_ARRAY:
            case SHT_FINI_ARRAY:
                break;

            default:
                continue;
        }

        SegmentType type = SegmentType::Data;

        if(shdr.sh_type == SHT_PROGBITS)
        {
            if(!shdr.sh_addr)
                continue;

            if(shdr.sh_flags & SHF_EXECINSTR)
                type = SegmentType::Code;
        }

        if(shdr.sh_type == SHT_NOBITS)
            type = SegmentType::Bss;

        String name = ELF_STRING(&shstr, shdr.sh_name);
        m_loader->document()->segment(name, shdr.sh_offset, shdr.sh_addr, shdr.sh_size, type);
    }
}

template<size_t b, endianness_t e> void ElfFormatT<b, e>::checkProgramHeader()
{
    if(this->m_ehdr->e_shnum)
        return;

    for(u64 i = 0; i < this->m_ehdr->e_phnum; i++)
    {
        const PHDR& phdr = this->m_phdr[i];

        if((phdr.p_type != PT_LOAD) || !phdr.p_memsz)
            continue;

        m_loader->document()->segment("LOAD", phdr.p_offset, phdr.p_vaddr, phdr.p_memsz, SegmentType::Code);
    }
}

template<size_t b, endianness_t e> void ElfFormatT<b, e>::checkArray()
{
    for(u64 i = 0; i < this->m_ehdr->e_shnum; i++)
    {
        const SHDR& shdr = this->m_shdr[i];
        String prefix;

        if(shdr.sh_type == SHT_INIT_ARRAY)
            prefix = "init";
        else if(shdr.sh_type == SHT_FINI_ARRAY)
            prefix = "fini";
        else if(shdr.sh_type == SHT_FINI_ARRAY)
            prefix = "preinit";
        else
            continue;

        ADDR* arr = m_loader->pointer<ADDR>(shdr.sh_offset);

        if(!arr)
            continue;

        for(ADDR j = 0; j < shdr.sh_size; j += b, arr++)
        {
            E_ADDR val = *arr;

            if(!val || (val == static_cast<ADDR>(-1)))
                continue;

            address_t address = m_loader->addressof(arr);
            m_loader->document()->symbol(address, SymbolTable::name(address, prefix, SymbolType::Pointer), SymbolType::Pointer | SymbolType::Data);
            m_loader->document()->function(val, SymbolTable::name(val, prefix, SymbolType::Function));
        }
    }
}

template<size_t b, endianness_t e> void ElfFormatT<b, e>::loadSymbols(const SHDR& shdr)
{
    offset_t offset = shdr.sh_offset, endoffset = offset + shdr.sh_size;
    const SHDR& shstr = shdr.sh_link ? this->m_shdr[shdr.sh_link] : ELF_STRING_TABLE;

    for(u64 idx = 0; offset < endoffset; idx++)
    {
        bool isrelocated = false;
        SYM* sym = m_loader->pointer<SYM>(offset);
        u8 info = ELF_ST_TYPE(sym->st_info);
        u64 symvalue = sym->st_value;

        if(!sym->st_name)
        {
            offset += sizeof(SYM);
            continue;
        }

        if(!symvalue)
            isrelocated = this->relocate(idx, &symvalue);

        String symname = ELF_STRING(&shstr, sym->st_name);

        if(!isrelocated)
        {
            bool isexport = false;
            u8 bind = ELF_ST_BIND(sym->st_info);
            u8 visibility = ELF_ST_VISIBILITY(sym->st_other);

            if(visibility == STV_DEFAULT)
                isexport = (bind == STB_GLOBAL) || (bind == STB_WEAK);
            else if(bind == STB_GLOBAL)
                isexport = true;

            if(isexport)
                m_loader->document()->lock(symvalue, symname, (info == STT_FUNC) ? SymbolType::ExportFunction : SymbolType::ExportData);
            else if(info == STT_FUNC)
                m_loader->document()->lock(symvalue, symname);
            else if(info == STT_OBJECT)
            {
                const Segment* segment = m_loader->document()->segment(symvalue);

                if(segment && !segment->is(SegmentType::Code))
                    m_loader->document()->lock(symvalue, symname, SymbolType::Data);
            }
        }
        else
            m_loader->document()->lock(symvalue, symname, SymbolType::Import);

        offset += sizeof(SYM);
    }
}

template<size_t b, endianness_t e> void ElfFormatT<b, e>::parseSegments()
{
    for(u64 i = 0; i < this->m_ehdr->e_shnum; i++)
    {
        const SHDR& shdr = this->m_shdr[i];

        if(shdr.sh_offset && ((shdr.sh_type == SHT_SYMTAB) || (shdr.sh_type == SHT_DYNSYM)))
        {
            const SHDR& shstr = ELF_STRING_TABLE;

            r_ctx->log("Section" + ELF_STRING(&shstr, shdr.sh_name).quoted() +
                       " contains a symbol table @ offset " + String::hex(shdr.sh_offset));

            this->loadSymbols(shdr);
        }
    }
}

template class ElfFormatT<32, Endianness::LittleEndian>;
template class ElfFormatT<64, Endianness::LittleEndian>;
template class ElfFormatT<32, Endianness::BigEndian>;
template class ElfFormatT<64, Endianness::BigEndian>;
