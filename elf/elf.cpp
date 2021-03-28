#include "elf.h"
#include "elf_abi.h"
#include "analyzer/elf_analyzer_x86.h"
#include <climits>
#include <memory>

#define ELF_STRING_TABLE         this->m_shdr[ELF_LDR_VAL(this->m_ehdr->e_shstrndx)]
#define ELF_STRING(shdr, offset) reinterpret_cast<const char*>(RD_RelPointer(this->m_ehdr, ELF_LDR_VAL((shdr)->sh_offset) + offset))

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

const char* ElfLoader::test(const RDLoaderRequest* request)
{
    std::unique_ptr<ElfLoader> loader(ElfLoader::parse(request->buffer));
    if(!loader) return nullptr;
    loader->initialize();
    return loader->assembler();
}

bool ElfLoader::load(RDContext* ctx)
{
    ElfLoader* l = ElfLoader::parse(RDContext_GetBuffer(ctx));
    RDContext_SetUserData(ctx, ELFLOADER_USERDATA, reinterpret_cast<uintptr_t>(l));
    l->initialize();
    l->doLoad(ctx);
    return true;
}

template<size_t bits>
ElfLoaderT<bits>::ElfLoaderT(RDBuffer* buffer): ElfLoader(), m_buffer(buffer) { }

template<size_t bits>
void ElfLoaderT<bits>::initialize()
{
    this->m_ehdr = reinterpret_cast<EHDR*>(RDBuffer_Data(m_buffer));
    this->m_shdr = reinterpret_cast<SHDR*>(RD_RelPointer(this->m_ehdr, ELF_LDR_VAL(this->m_ehdr->e_shoff)));
    this->m_phdr = reinterpret_cast<PHDR*>(RD_RelPointer(this->m_ehdr, ELF_LDR_VAL(this->m_ehdr->e_phoff)));
}

template<size_t bits>
size_t ElfLoaderT<bits>::endianness() const { return (this->m_ehdr->e_ident[EI_DATA] == ELFDATA2MSB) ? Endianness_Big : Endianness_Little; }

template<size_t bits>
const char* ElfLoaderT<bits>::assembler() const
{
    switch(ELF_LDR_VAL(this->m_ehdr->e_machine))
    {
        case EM_AVR: return "avr8";
        case EM_386: return "x86_32";
        case EM_X86_64: return "x86_64";

        case EM_XTENSA:
            if(this->endianness() == Endianness_Big) return "xtensabe";
            else return "xtensale";

        case EM_AARCH64:
            if(this->endianness() == Endianness_Big) return "arm64be";
            return "arm64le";

        case EM_ARM:
            if(this->endianness() == Endianness_Big) return "armbe";
            else return "armle";

        case EM_MIPS:
            if(ELF_LDR_VAL(this->m_ehdr->e_flags) & EF_MIPS_ABI_EABI64) {
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
RDContext* ElfLoaderT<bits>::context() const { return m_context; }

template<size_t bits>
RDDocument* ElfLoaderT<bits>::document() const { return RDContext_GetDocument(m_context); }

template<size_t bits>
const ElfABI* ElfLoaderT<bits>::abi() const { return m_abi.get(); }

template<size_t bits>
const u8* ElfLoaderT<bits>::plt() const
{
    auto it = m_dynamic.find(DT_JMPREL);
    if(it == m_dynamic.end()) return nullptr;
    return RDContext_GetBufferData(m_context) + it->second;
}

template<size_t bits>
u16 ElfLoaderT<bits>::machine() const { return ELF_LDR_VAL(this->m_ehdr->e_machine); }

template<size_t bits>
const typename ElfLoaderT<bits>::SHDR* ElfLoaderT<bits>::findSegment(rd_address address) const
{
    for(size_t i = 0; i < ELF_LDR_VAL(this->m_ehdr->e_shnum); i++)
    {
        const SHDR& shdr = this->m_shdr[i];
        if(address < ELF_LDR_VAL(shdr.sh_addr)) continue;
        if(address >= (ELF_LDR_VAL(shdr.sh_addr) + ELF_LDR_VAL(shdr.sh_size))) continue;
        return &shdr;
    }

    return nullptr;
}

template<size_t bits>
std::optional<typename ElfLoaderT<bits>::UVAL> ElfLoaderT<bits>::dynamic(ElfLoaderT::SVAL d) const
{
    auto it = m_dynamic.find(d);
    return (it != m_dynamic.end()) ? std::make_optional(it->second) : std::nullopt;
}

template<size_t bits>
std::optional<std::string> ElfLoaderT<bits>::symbolName(ElfLoaderT::UVAL sectidx, ElfLoaderT::UVAL symidx) const
{
    if(sectidx >= this->m_ehdr->e_shnum) return std::nullopt;

    const SHDR& symboltable = this->m_shdr[sectidx];
    auto entries = symboltable.sh_size / symboltable.sh_entsize;
    if(symidx >= entries) return std::nullopt;

    if(symboltable.sh_link >= this->m_ehdr->e_shnum) return std::nullopt;
    const SHDR& stringtable = this->m_shdr[symboltable.sh_link];

    const auto* sym = this->elfptr<const SYM>(&symboltable, 0);
    if(!sym) return std::nullopt;

    std::string s = ELF_STRING(&stringtable, sym[symidx].st_name);

    auto it = m_versions.find(symidx);
    if(it != m_versions.end()) s += "@" + it->second;
    return s;
}

template<size_t bits>
void ElfLoaderT<bits>::doLoad(RDContext* ctx)
{
    m_context = ctx;

    RDDocument* doc = RDContext_GetDocument(ctx);
    this->readSectionHeader(doc);
    this->readProgramHeader(doc);

    m_abi.reset(new ElfABIT<bits>(this));
    m_abi->parse();

    if(RDDocument_AddressToSegment(doc, ELF_LDR_VAL(this->m_ehdr->e_entry), nullptr))
        RDDocument_SetEntry(doc, ELF_LDR_VAL(this->m_ehdr->e_entry));
}

template<size_t bits>
void ElfLoaderT<bits>::readSectionHeader(RDDocument* doc)
{
    for(u64 i = 0; i < ELF_LDR_VAL(this->m_ehdr->e_shnum); i++)
    {
        const SHDR& shdr = this->m_shdr[i];
        this->checkMappedSegment(&shdr, doc);

        switch(ELF_LDR_VAL(shdr.sh_type))
        {
            case SHT_SYMTAB: {
                auto val = ELF_LDR_VAL(shdr.sh_offset);
                if(shdr.sh_entsize) rd_log("Reading symbol table @ " + rd_tohex(val));
                this->readSymbols(doc, &shdr, val, shdr.sh_entsize);
                break;
            }

            default: break;
        }
    }
}

template<size_t bits>
void ElfLoaderT<bits>::readProgramHeader(RDDocument* doc)
{
    for(u64 i = 0; i < ELF_LDR_VAL(this->m_ehdr->e_phnum); i++)
    {
        const PHDR& phdr = this->m_phdr[i];
        if(!phdr.p_memsz) continue;

        switch(ELF_LDR_VAL(phdr.p_type))
        {
            case PT_DYNAMIC:
                if(phdr.p_offset) this->readDynamic(&phdr, doc);
                break;

            default: break;
        }
    }
}

template<size_t bits>
void ElfLoaderT<bits>::readDynamic(const ElfLoaderT::PHDR* phdr, RDDocument* doc)
{
    if(!m_dynamic.empty())
    {
        rd_log("Skipping duplicate DYNAMIC entry @ " + rd_tohex(ELF_LDR_VAL(phdr->p_offset)));
        return;
    }

    const auto* dyn = reinterpret_cast<const DYN*>(RD_FilePointer(m_context, ELF_LDR_VAL(phdr->p_offset)));
    if(!dyn) return;

    while(ELF_LDR_VAL(dyn->d_tag) != DT_NULL)
    {
        m_dynamic[ELF_LDR_VAL(dyn->d_tag)] = ELF_LDR_VAL(dyn->d_val);
        dyn++;
    }

    for(const auto& [tag, value] : m_dynamic)
    {
        if(!value) continue;

        switch(tag)
        {
            case DT_PREINIT_ARRAY:
            {
                auto it = m_dynamic.find(DT_PREINIT_ARRAYSZ);
                if(it != m_dynamic.end()) this->readArray(doc, value, it->second, tag);
                break;
            }

            case DT_INIT_ARRAY:
            {
                auto it = m_dynamic.find(DT_INIT_ARRAYSZ);
                if(it != m_dynamic.end()) this->readArray(doc, value, it->second, tag);
                break;
            }

            case DT_FINI_ARRAY:
            {
                auto it = m_dynamic.find(DT_FINI_ARRAYSZ);
                if(it != m_dynamic.end()) this->readArray(doc, value, it->second, tag);
                break;
            }

            case DT_VERNEED:
            {
                auto it = m_dynamic.find(DT_VERNEEDNUM);
                if(it != m_dynamic.end()) this->readVersions(value, it->second);
                break;
            }

            case DT_SYMTAB:
            {
                auto it = m_dynamic.find(DT_SYMENT);

                if(it != m_dynamic.end()) {
                    auto* shdr = this->findSegment(value);
                    if(shdr) {
                        rd_log("Reading dynamic symbol table @ " + rd_tohex(value));
                        this->readSymbols(doc, shdr, ELF_LDR_VAL(shdr->sh_offset) + (value - ELF_LDR_VAL(shdr->sh_addr)), it->second);
                    }
                }

                break;
            }

            case DT_PLTGOT:
                RDDocument_SetData(doc, value, bits / CHAR_BIT, "_GLOBAL_OFFSET_TABLE_");
                break;

            case DT_INIT:
                RDDocument_SetFunction(doc, value, RD_MakeLabel(value, "sub_init"));
                break;

            case DT_FINI:
                RDDocument_SetFunction(doc, value, RD_MakeLabel(value, "sub_fini"));
                break;

            default:
                break;
        }
    }
}

template<size_t bits>
void ElfLoaderT<bits>::checkMappedSegment(const SHDR* shdr, RDDocument* doc)
{
    if(!shdr->sh_addr || !shdr->sh_size || !(shdr->sh_flags & SHF_ALLOC)) return;

    const SHDR& shstr = ELF_STRING_TABLE;
    rd_flag flags = SegmentFlags_None;

    switch(ELF_LDR_VAL(shdr->sh_type))
    {
        case SHT_PROGBITS:
            if(ELF_LDR_VAL(shdr->sh_flags) & SHF_EXECINSTR) flags = SegmentFlags_Code;
            else flags = SegmentFlags_Data;
            break;

        case SHT_INIT_ARRAY:
        case SHT_FINI_ARRAY:
            flags = SegmentFlags_Data;
            break;

        case SHT_NOBITS:
            flags = SegmentFlags_Bss;
            break;

        default: break;
    }

    if(flags == SegmentFlags_None) return;
    const char* name = ELF_STRING(&shstr, ELF_LDR_VAL(shdr->sh_name));
    RDDocument_SetSegment(doc, name, ELF_LDR_VAL(shdr->sh_offset), ELF_LDR_VAL(shdr->sh_addr), ELF_LDR_VAL(shdr->sh_size), flags);
}

template<size_t bits>
void ElfLoaderT<bits>::readArray(RDDocument* doc, UVAL address, UVAL size, SVAL type)
{
    std::string prefix;

    switch(type)
    {
        case DT_PREINIT_ARRAY: prefix = "preinit"; break;
        case DT_INIT_ARRAY: prefix = "init"; break;
        case DT_FINI_ARRAY: prefix = "fini"; break;
        default: return;
    }

    ADDR* arr = reinterpret_cast<ADDR*>(RD_AddrPointer(m_context, ELF_LDR_VAL(address)));
    if(!arr) return;

    for(ADDR sz = 0; sz < ELF_LDR_VAL(size); sz += (bits / CHAR_BIT), arr++)
    {
        ADDR val = ELF_LDR_VAL(*arr);
        if(!val || (val == ADDR(-1))) continue;

        RDLocation loc = RD_AddressOf(m_context, arr);
        if(!loc.valid) continue;

        RDDocument_SetPointer(doc, loc.address, rd_makelabel(loc.address, "ptr_" + prefix));
        RDDocument_SetFunction(doc, val, rd_makelabel(loc.address, "sub_" + prefix));
    }
}

template<size_t bits>
void ElfLoaderT<bits>::readVersions(ElfLoaderT::UVAL address, ElfLoaderT::UVAL count)
{
    const SHDR* shdr = this->findSegment(address);
    if(!shdr || (ELF_LDR_VAL(shdr->sh_link) >= ELF_LDR_VAL(this->m_ehdr->e_shnum))) return;

    auto* verneed = reinterpret_cast<Elf_Verneed*>(RDContext_GetBufferData(m_context) + ((address - ELF_LDR_VAL(shdr->sh_addr)) + ELF_LDR_VAL(shdr->sh_offset)));
    const SHDR* segstrings = &m_shdr[ELF_LDR_VAL(shdr->sh_link)];

    for(UVAL i = 0; i < count; i++)
    {
        auto* veraux = reinterpret_cast<Elf_Vernaux*>(RD_RelPointer(verneed, ELF_LDR_VAL(verneed->vn_aux)));

        for(UVAL j = 0; j < verneed->vn_cnt; j++)
        {
            if(!(ELF_LDR_VAL(veraux->vna_other) & (1 << 15)))
                m_versions[ELF_LDR_VAL(veraux->vna_other)] = ELF_STRING(segstrings, ELF_LDR_VAL(veraux->vna_name));

            veraux = reinterpret_cast<Elf_Vernaux*>(RD_RelPointer(veraux, ELF_LDR_VAL(veraux->vna_next)));
        }

        verneed = reinterpret_cast<Elf_Verneed*>(RD_RelPointer(verneed, ELF_LDR_VAL(verneed->vn_next)));
    }
}

template<size_t bits>
void ElfLoaderT<bits>::readSymbols(RDDocument* doc, const ElfLoaderT::SHDR* shdr, ElfLoaderT::UVAL offset, UVAL entrysize)
{
    if(!entrysize || !shdr || (ELF_LDR_VAL(shdr->sh_link) >= ELF_LDR_VAL(this->m_ehdr->e_shnum))) return;

    rd_offset baseoffset = offset - ELF_LDR_VAL(shdr->sh_offset);
    ElfLoaderT::UVAL count = (ELF_LDR_VAL(shdr->sh_size) - baseoffset) / entrysize;
    if(!count) return;

    auto* sym = elfptr<ElfLoaderT::SYM>(shdr, baseoffset);
    if(!sym) return;

    const SHDR* shstr = &m_shdr[ELF_LDR_VAL(shdr->sh_link)];

    for(ElfLoaderT::UVAL i = 0; i < count; i++, sym = reinterpret_cast<ElfLoaderT::SYM*>(RD_RelPointer(const_cast<ElfLoaderT::SYM*>(sym), entrysize)))
    {
        if(!sym->st_name || !sym->st_value) continue;

        const char* symname = ELF_STRING(shstr, ELF_LDR_VAL(sym->st_name));
        if(!symname) continue;

        auto symvalue = ELF_LDR_VAL(sym->st_value);
        auto symtype = ELF_ST_TYPE(ELF_LDR_VAL(sym->st_info));
        auto bind = ELF_ST_BIND(ELF_LDR_VAL(sym->st_info));

        switch(symtype)
        {
            case STT_FUNC:
            {
                if(bind == STB_GLOBAL) RDDocument_SetExportedFunction(doc, symvalue, symname);
                else RDDocument_SetFunction(doc, symvalue, symname);
                break;
            }

            case STT_OBJECT:
            {
                auto sz = ELF_LDR_VAL(sym->st_size);
                if(!sz) sz = bits / CHAR_BIT;

                if(bind == STB_GLOBAL) RDDocument_SetExported(doc, symvalue, sz, symname);
                else RDDocument_SetData(doc, symvalue, sz, symname);
                break;
            }

            default: break;
        }
    }
}

template<size_t bits>
bool ElfLoaderT<bits>::findSegments(const ElfLoaderT<bits>::PHDR* phdr, std::vector<const SHDR*>& segments) const
{
    for(size_t i = 0; i < ELF_LDR_VAL(this->m_ehdr->e_shnum); i++)
    {
        const SHDR& shdr = this->m_shdr[i];
        if(ELF_LDR_VAL(shdr.sh_addr) < ELF_LDR_VAL(phdr->p_vaddr)) continue;
        if((ELF_LDR_VAL(shdr.sh_addr) + ELF_LDR_VAL(shdr.sh_size)) > ELF_LDR_VAL(phdr->p_vaddr + phdr->p_memsz)) continue;
        segments.push_back(&shdr);
    }

    return !segments.empty();
}

template<size_t bits>
const typename ElfLoaderT<bits>::SHDR* ElfLoaderT<bits>::findSegment(const ElfLoaderT::PHDR* phdr) const
{
    std::vector<const SHDR*> segments;
    if(!this->findSegments(phdr, segments)) return nullptr;
    return segments.front();
}

template class ElfLoaderT<32>;
template class ElfLoaderT<64>;

void rdplugin_init(RDContext*, RDPluginModule* pm)
{
    RD_PLUGIN_ENTRY(RDEntryLoader, elf, "ELF Executable");
    elf.test = &ElfLoader::test;
    elf.load = &ElfLoader::load;
    RDLoader_Register(pm, &elf);

    RD_PLUGIN_ENTRY(RDEntryAnalyzer, elfabi_x86, "x86 ABI Analyzer");
    elfabi_x86.description = "Analyze x86 specific information";
    elfabi_x86.flags = AnalyzerFlags_Selected | AnalyzerFlags_RunOnce;
    elfabi_x86.order = 1000;

    elfabi_x86.isenabled = [](const RDContext* ctx) -> bool {
        auto* elfloader = reinterpret_cast<ElfLoader*>(RDContext_GetUserData(ctx, ELFLOADER_USERDATA));
        if(!elfloader) return false;
        if((elfloader->machine() == EM_386) || (elfloader->machine() == EM_X86_64)) return true;
        return false;
    };

    elfabi_x86.execute = [](RDContext* ctx) {
        ElfAnalyzerX86 a(ctx);
        a.analyze();
    };

    RDAnalyzer_Register(pm, &elfabi_x86);
}

void rdplugin_free(RDContext* ctx)
{
    auto* elfloader = reinterpret_cast<ElfLoader*>(RDContext_GetUserData(ctx, ELFLOADER_USERDATA));
    if(elfloader) delete elfloader;
}
