#include "elf_abi.h"
#include "elf.h"
#include <climits>

#define ELF_ABI_VAL(f) e_valT((f), m_elf->endianness())

#define IF_PROCESS_RELOCATIONS(arch, cond) if((cond)) { \
        for(size_t sz = 0; sz < *size; sz += sizeof(RelType), rel++) { \
            this->processRelocation_##arch(rel, shdr); \
        }\
        return; \
    }

#define ELSEIF_PROCESS_RELOCATIONS(arch, cond) IF_PROCESS_RELOCATIONS(arch, cond)

template<size_t bits>
ElfABIT<bits>::ElfABIT(const ElfType* elf): m_elf(elf) { }

template<size_t bits>
std::optional<std::string> ElfABIT<bits>::plt(size_t idx) const
{
    auto it = m_plt.find(idx);
    if(it != m_plt.end()) return it->second;
    return std::nullopt;
}

template<size_t bits>
void ElfABIT<bits>::parse()
{
    auto reltype = m_elf->dynamic(DT_PLTREL);

    if(reltype)
    {
        if(*reltype == DT_RELA) this->processRelocations<typename ElfType::RELA>(DT_JMPREL, DT_PLTRELSZ);
        else this->processRelocations<typename ElfType::REL>(DT_JMPREL, DT_PLTRELSZ);
    }

    this->processRelocations<typename ElfType::RELA>(DT_RELA, DT_RELASZ);
    this->processRelocations<typename ElfType::REL>(DT_REL, DT_RELSZ);
}

template<size_t bits>
template<typename RelType>
void ElfABIT<bits>::processJmpSlot(const RelType* rel, const typename ElfType::SHDR* shdr)
{
    auto symidx = elf_r_sym<bits>(ELF_ABI_VAL(rel->r_info));
    auto name = m_elf->symbolName(shdr->sh_link, symidx);
    if(!name) return;

    const auto* s = m_elf->findSegment(rel->r_offset);
    if(!s) return;

    RDDocument_SetImported(m_elf->document(), rel->r_offset, bits / CHAR_BIT, name->c_str());
    m_plt[rel->r_offset - s->sh_addr] = *name;
}

template<size_t bits>
template<typename RelType>
void ElfABIT<bits>::processGlobDat(const RelType* rel, const typename ElfType::SHDR* shdr)
{
    auto symidx = elf_r_sym<bits>(ELF_ABI_VAL(rel->r_info));
    auto name = m_elf->symbolName(shdr->sh_link, symidx);
    if(!name) return;

    RDDocument_SetData(m_elf->document(), rel->r_offset, bits / CHAR_BIT, name->c_str());
}

template<size_t bits>
template<typename RelType>
void ElfABIT<bits>::processRelocation_ARM(const RelType* rel, const typename ElfType::SHDR* shdr)
{
    auto type = elf_r_type<bits>(ELF_ABI_VAL(rel->r_info));

    switch(type)
    {
        case R_ARM_GLOB_DAT: this->processGlobDat(rel, shdr); break;
        case R_ARM_JUMP_SLOT: this->processJmpSlot(rel, shdr); break;
        default: break;
    }
}

template<size_t bits>
template<typename RelType>
void ElfABIT<bits>::processRelocation_386(const RelType* rel, const typename ElfType::SHDR* shdr)
{
    auto type = elf_r_type<bits>(ELF_ABI_VAL(rel->r_info));

    switch(type)
    {
        case R_386_GLOB_DAT: this->processGlobDat(rel, shdr); break;
        case R_386_JMP_SLOT: this->processJmpSlot(rel, shdr); break;
        default: break;
    }
}

template<size_t bits>
template<typename RelType>
void ElfABIT<bits>::processRelocations(typename ElfType::SVAL dtag, typename ElfType::SVAL dtagsz)
{
    auto size = m_elf->dynamic(dtagsz);
    if(!size) return;

    const typename ElfType::SHDR* shdr = nullptr;
    const RelType* rel = m_elf->template elfdynptr<RelType>(dtag, &shdr);
    if(!rel) return;

    std::string a = m_elf->assembler();
    IF_PROCESS_RELOCATIONS(386, !a.find("x86"))
    ELSEIF_PROCESS_RELOCATIONS(ARM, (!a.find("arm")))
    /* else */ rd_log("Unhandled ELF ABI for '" + a + "'");
}

template class ElfABIT<32>;
template class ElfABIT<64>;
