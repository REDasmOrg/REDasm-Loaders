#include "elf_abi.h"
#include "elf.h"
#include <climits>

#define E_VAL(f) e_valT((f), m_elf->endianness())

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
    std::string a = m_elf->assembler();

    if(!a.find("x86")) this->parse_x86();
    else rd_log("Unhandled ELF ABI for '" + a + "'");
}

template<size_t bits>
void ElfABIT<bits>::processRelocations(const typename ElfType::REL* rel, typename ElfType::UVAL size, const typename ElfType::SHDR* shdr)
{
    for(size_t sz = 0; sz < size; sz += sizeof(typename ElfType::REL), rel++)
    {
        auto symidx = elf_r_sym<bits>(E_VAL(rel->r_info));
        auto type = elf_r_type<bits>(E_VAL(rel->r_info));

        switch(type)
        {
            case R_386_GLOB_DAT:
            {
                auto name = m_elf->symbolName(shdr->sh_link, symidx);
                if(!name) continue;
                RDDocument_AddImported(m_elf->document(), rel->r_offset, bits / CHAR_BIT, name->c_str());
                break;
            }

            case R_386_JMP_SLOT:
            {
                auto name = m_elf->symbolName(shdr->sh_link, symidx);
                if(!name) continue;
                const auto* shdr = m_elf->findSegment(rel->r_offset);
                if(!shdr) continue;

                RDDocument_AddData(m_elf->document(), rel->r_offset, bits / CHAR_BIT, name->c_str());
                m_plt[rel->r_offset - shdr->sh_addr] = *name;
                break;
            }

            default: break;
        }
    }
}

template<size_t bits>
void ElfABIT<bits>::processRelocations(typename ElfType::SVAL dtag, typename ElfType::SVAL dtagsz)
{
    auto sz = m_elf->dynamic(dtagsz);
    if(!sz) return;

    const typename ElfType::SHDR* shdr = nullptr;
    const auto* tag = m_elf->template elfdynptr<typename ElfType::REL>(dtag, &shdr);
    if(!tag) return;

    this->processRelocations(tag, *sz, shdr);
}

template<size_t bits>
void ElfABIT<bits>::parse_x86()
{
    this->processRelocations(DT_JMPREL, DT_PLTRELSZ);
    this->processRelocations(DT_REL, DT_RELSZ);
}

template class ElfABIT<32>;
template class ElfABIT<64>;
