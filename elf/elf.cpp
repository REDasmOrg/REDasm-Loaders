#include "elf.h"
#include "elf_analyzer.h"
#include <climits>
#include <memory>

#define E_VAL(f) e_valT((f), this->endianness())
#define ELF_STRING_TABLE this->m_shdr[E_VAL(this->m_ehdr->e_shstrndx)]
#define ELF_STRING(shdr, offset) reinterpret_cast<const char*>(RD_RelPointer(this->m_ehdr, E_VAL((shdr)->sh_offset) + offset))

ElfLoader* ElfLoader::parse(RDBuffer* buffer)
{
    Elf_EhdrBase* hdr = reinterpret_cast<Elf_EhdrBase*>(RDBuffer_Data(buffer));

    if((hdr->e_ident[EI_MAG0] != ELFMAG0) || (hdr->e_ident[EI_MAG1] != ELFMAG1)) return nullptr;
    if((hdr->e_ident[EI_MAG2] != ELFMAG2) || (hdr->e_ident[EI_MAG3] != ELFMAG3)) return nullptr;
    if(hdr->e_ident[EI_VERSION] != EV_CURRENT) return nullptr;

    switch(hdr->e_ident[EI_DATA])
    {
        case ELFDATA2MSB: break; // MSB -> BigEndian
        case ELFDATA2LSB: break; // LSB -> LittleEndian
        default: return nullptr;
    }

    switch(hdr->e_ident[EI_CLASS])
    {
        case ELFCLASS32: return new ElfLoaderT<32>(buffer);
        case ELFCLASS64: return new ElfLoaderT<64>(buffer);
        default: break;
    }

    return nullptr;
}

const char* ElfLoader::test(const RDLoaderPlugin*, const RDLoaderRequest* request)
{
    std::unique_ptr<ElfLoader> loader(ElfLoader::parse(request->buffer));
    return loader ? loader->assembler() : nullptr;
}

void ElfLoader::analyze(RDLoaderPlugin* plugin, RDDisassembler* disassembler)
{
    ElfAnalyzer a(plugin, disassembler);
    a.analyze();
}

bool ElfLoader::load(RDLoaderPlugin* plugin, RDLoader* loader)
{
    RDDocument* doc = RDLoader_GetDocument(loader);
    ElfLoader* l = ElfLoader::parse(RDLoader_GetBuffer(loader));

    l->loadSegments(doc);
    l->parseSegments(loader, doc);
    l->checkProgramHeader(doc);
    l->checkArray(loader, doc);
    l->checkEntryPoint(doc);

    plugin->p_data = l;
    return true;
}

template<size_t bits>
ElfLoaderT<bits>::ElfLoaderT(RDBuffer* buffer): m_buffer(buffer)
{
    this->m_ehdr = reinterpret_cast<EHDR*>(RDBuffer_Data(buffer));
    this->m_shdr = reinterpret_cast<SHDR*>(RD_RelPointer(this->m_ehdr, E_VAL(this->m_ehdr->e_shoff)));
    this->m_phdr = reinterpret_cast<PHDR*>(RD_RelPointer(this->m_ehdr, E_VAL(this->m_ehdr->e_phoff)));
}

template<size_t bits>
size_t ElfLoaderT<bits>::endianness() const { return (this->m_ehdr->e_ident[EI_DATA] == ELFDATA2MSB) ? Endianness_Big : Endianness_Little; }

template<size_t bits>
const char* ElfLoaderT<bits>::assembler() const
{
    switch(E_VAL(this->m_ehdr->e_machine))
    {
        case EM_AVR: return "avr8";
        case EM_386: return "x86_32";
        case EM_X86_64: return "x86_64";

        case EM_XTENSA:
            if(this->endianness() == Endianness_Big) return "xtensabe";
            else return "xtensale";

        case EM_ARM:
            if(this->m_ehdr->e_ident[EI_CLASS] == ELFCLASS64) return "arm64";
            return "armthumb";

        case EM_MIPS:
            if(E_VAL(this->m_ehdr->e_flags) & EF_MIPS_ABI_EABI64) {
                if(this->endianness() == Endianness_Big) return "mips64be";
                else return "mips64le";
            }

            if(this->endianness() == Endianness_Big) return "mips32be";
            else return "mips32le";

        default: break;
    }

    return nullptr;
}

template<size_t bits>
u64 ElfLoaderT<bits>::relocationSymbol(const REL* rel) const
{
    if constexpr(bits == 64) return ELF64_R_SYM(E_VAL(rel->r_info));
    else return ELF32_R_SYM(E_VAL(rel->r_info));
}

template<size_t bits>
bool ElfLoaderT<bits>::relocate(RDLoader* loader, u64 symidx, u64* value) const
{
    for(u64 i = 0; i < E_VAL(this->m_ehdr->e_shnum); i++)
    {
        const SHDR& shdr = this->m_shdr[i];

        if((E_VAL(shdr.sh_type) != SHT_REL) && (E_VAL(shdr.sh_type) != SHT_RELA))
            continue;

        offset_t offset = E_VAL(shdr.sh_offset), endoffset = offset + E_VAL(shdr.sh_size);

        while(offset < endoffset)
        {
            REL* rel = reinterpret_cast<REL*>(RD_Pointer(loader, offset));
            u64 sym = this->relocationSymbol(rel);

            if(sym == symidx)
            {
                *value = E_VAL(rel->r_offset);
                return true;
            }

            offset += (E_VAL(shdr.sh_type) == SHT_REL) ? sizeof(REL) : sizeof(RELA);
        }
    }

    return false;
}

template<size_t bits>
void ElfLoaderT<bits>::loadSegments(RDDocument* doc)
{
    const SHDR& shstr = ELF_STRING_TABLE;

    for(u64 i = 0; i < E_VAL(this->m_ehdr->e_shnum); i++)
    {
        const SHDR& shdr = this->m_shdr[i];
        if(!shdr.sh_size) continue;

        switch(E_VAL(shdr.sh_type))
        {
            case SHT_PROGBITS:
            case SHT_PREINIT_ARRAY:
            case SHT_INIT_ARRAY:
            case SHT_FINI_ARRAY:
                break;

            default: continue;
        }

        flag_t flags = SegmentFlags_Data;

        if(E_VAL(shdr.sh_type) == SHT_PROGBITS)
        {
            if(!shdr.sh_addr) continue;
            if(E_VAL(shdr.sh_flags) & SHF_EXECINSTR) flags = SegmentFlags_Code;
        }

        if(E_VAL(shdr.sh_type) == SHT_NOBITS) flags = SegmentFlags_Bss;

        const char* name = ELF_STRING(&shstr, E_VAL(shdr.sh_name));
        RDDocument_AddSegment(doc, name, E_VAL(shdr.sh_offset), E_VAL(shdr.sh_addr), E_VAL(shdr.sh_size), flags);
    }
}

template<size_t bits>
void ElfLoaderT<bits>::checkProgramHeader(RDDocument* doc)
{
    if(this->m_ehdr->e_shnum) return;

    for(u64 i = 0; i < E_VAL(this->m_ehdr->e_phnum); i++)
    {
        const PHDR& phdr = this->m_phdr[i];

        if((E_VAL(phdr.p_type) != PT_LOAD) || !phdr.p_memsz)
            continue;

        RDDocument_AddSegment(doc, "LOAD", E_VAL(phdr.p_offset), E_VAL(phdr.p_vaddr), E_VAL(phdr.p_memsz), SegmentFlags_Code);
    }
}

template<size_t bits>
void ElfLoaderT<bits>::checkArray(RDLoader* loader, RDDocument* doc)
{
    for(u64 i = 0; i < E_VAL(this->m_ehdr->e_shnum); i++)
    {
        const SHDR& shdr = this->m_shdr[i];
        std::string prefix;

        if(E_VAL(shdr.sh_type) == SHT_INIT_ARRAY) prefix = "init";
        else if(E_VAL(shdr.sh_type) == SHT_FINI_ARRAY) prefix = "fini";
        else if(E_VAL(shdr.sh_type) == SHT_FINI_ARRAY) prefix = "preinit";
        else continue;

        ADDR* arr = reinterpret_cast<ADDR*>(RD_Pointer(loader, E_VAL(shdr.sh_offset)));
        if(!arr) continue;

        for(ADDR j = 0; j < E_VAL(shdr.sh_size); j += bits, arr++)
        {
            ADDR val = E_VAL(*arr);
            if(!val || (val == static_cast<ADDR>(-1))) continue;

            RDLocation loc = RD_AddressOf(loader, arr);
            if(!loc.valid) continue;

            RDDocument_AddPointer(doc, loc.address, SymbolType_Data, RDSymbol_NameHint(loc.address, prefix.c_str(), SymbolType_Data, SymbolFlags_Pointer));
            RDDocument_AddFunction(doc, val, RDSymbol_NameHint(loc.address, prefix.c_str(), SymbolType_Function, SymbolFlags_None));
        }
    }
}

template<size_t bits>
void ElfLoaderT<bits>::checkEntryPoint(RDDocument* doc)
{
    if(RDDocument_GetSegmentAddress(doc, E_VAL(this->m_ehdr->e_entry), nullptr))
        RDDocument_SetEntry(doc, E_VAL(this->m_ehdr->e_entry));
}

template<size_t bits>
void ElfLoaderT<bits>::loadSymbols(const SHDR& shdr, RDLoader* loader, RDDocument* doc)
{
    offset_t offset = E_VAL(shdr.sh_offset), endoffset = offset + E_VAL(shdr.sh_size);
    const SHDR& shstr = shdr.sh_link ? this->m_shdr[E_VAL(shdr.sh_link)] : ELF_STRING_TABLE;

    for(u64 idx = 0; offset < endoffset; idx++)
    {
        bool isrelocated = false;
        SYM* sym = reinterpret_cast<SYM*>(RD_Pointer(loader, offset));
        u8 info = ELF_ST_TYPE(E_VAL(sym->st_info));
        u64 symvalue = E_VAL(sym->st_value);

        if(!sym->st_name)
        {
            offset += sizeof(SYM);
            continue;
        }

        if(!symvalue) isrelocated = this->relocate(loader, idx, &symvalue);
        const char* symname = ELF_STRING(&shstr, E_VAL(sym->st_name));

        if(!isrelocated)
        {
            bool isexport = false;
            u8 bind = ELF_ST_BIND(E_VAL(sym->st_info));
            u8 visibility = ELF_ST_VISIBILITY(E_VAL(sym->st_other));

            if(visibility == STV_DEFAULT) isexport = (bind == STB_GLOBAL) || (bind == STB_WEAK);
            else if(bind == STB_GLOBAL) isexport = true;

            if(isexport)
            {
                if(info == STT_FUNC) RDDocument_AddExportedFunction(doc, symvalue, symname);
                else RDDocument_AddExported(doc, symvalue, symname);
            }
            else if(info == STT_FUNC)
                RDDocument_AddFunction(doc, symvalue, symname);
            else if(info == STT_OBJECT)
            {
                RDSegment segment;

                if(RDDocument_GetSegmentAddress(doc, symvalue, &segment) && !HAS_FLAG(&segment, SegmentFlags_Code))
                    RDDocument_AddData(doc, symvalue, E_VAL(sym->st_size), symname);
            }
        }
        else
            RDDocument_AddImported(doc, symvalue, bits / CHAR_BIT, symname);

        offset += sizeof(SYM);
    }
}

template<size_t bits>
void ElfLoaderT<bits>::parseSegments(RDLoader* loader, RDDocument* doc)
{
    for(u64 i = 0; i < E_VAL(this->m_ehdr->e_shnum); i++)
    {
        const SHDR& shdr = this->m_shdr[i];

        if(shdr.sh_offset && ((E_VAL(shdr.sh_type) == SHT_SYMTAB) || (E_VAL(shdr.sh_type) == SHT_DYNSYM)))
        {
            const SHDR& shstr = ELF_STRING_TABLE;
            rd_log("Section '" + std::string(ELF_STRING(&shstr, E_VAL(shdr.sh_name))) + "' contains a symbol table @ offset " + rd_tohex(E_VAL(shdr.sh_offset)));
            this->loadSymbols(shdr, loader, doc);
        }
    }
}

template class ElfLoaderT<32>;
template class ElfLoaderT<64>;

static void free(RDPluginHeader* plugin)
{
    if(!plugin->p_data) return;

    delete reinterpret_cast<ElfLoader*>(plugin->p_data);
    plugin->p_data = nullptr;
}

void redasm_entry()
{
    RD_PLUGIN_CREATE(RDLoaderPlugin, elf, "ELF Executable");
    elf.free = &free;
    elf.test = &ElfLoader::test;
    elf.load = &ElfLoader::load;
    elf.analyze = &ElfLoader::analyze;

    RDLoader_Register(&elf);
}
