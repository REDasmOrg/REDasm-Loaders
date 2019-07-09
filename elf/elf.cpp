#include "elf.h"
#include "elf_header.h"
#include "elf_analyzer.h"

ElfLoader::ElfLoader(): Loader() { }

size_t ElfLoader::bits() const
{
    const u8* ident = this->pointer<u8>();
    return (ident[EI_CLASS] == ELFCLASS64) ? 64 : 32;
}

endianness_t ElfLoader::endianness() const
{
    const u8* ident = this->pointer<u8>();
    return (ident[EI_DATA] == ELFDATA2MSB) ? Endianness::BigEndian : Endianness::LittleEndian;
}

AssemblerRequest ElfLoader::assembler() const { return m_elfformat->assembler(); }

bool ElfLoader::test(const LoadRequest &request) const
{
    const u8* ident = request.pointer<u8>();

    if((ident[EI_MAG0] != ELFMAG0) || (ident[EI_MAG1] != ELFMAG1))
        return false;

    if((ident[EI_MAG2] != ELFMAG2) || (ident[EI_MAG3] != ELFMAG3))
        return false;

    if(ident[EI_VERSION] != EV_CURRENT)
        return false;

    switch(ident[EI_DATA])
    {
        case ELFDATA2MSB: break; // MSB -> BigEndian
        case ELFDATA2LSB: break; // LSB -> LittleEndian
        default: return false;
    }

    switch(ident[EI_CLASS])
    {
        case ELFCLASS32: break; // 32 Bit ELF
        case ELFCLASS64: break; // 64 Bit ELF
        default: return false;
    }

    return true;
}

void ElfLoader::init(const LoadRequest &request)
{
    Loader::init(request);

    if(this->bits() == 32)
    {
        if(this->endianness() == Endianness::LittleEndian)
            m_elfformat = std::make_unique< ElfFormatT<32, Endianness::LittleEndian> >(this);
        else
            m_elfformat = std::make_unique< ElfFormatT<32, Endianness::BigEndian> >(this);
    }
    else if(this->bits() == 64)
    {
        if(this->endianness() == Endianness::LittleEndian)
            m_elfformat = std::make_unique< ElfFormatT<64, Endianness::LittleEndian> >(this);
        else
            m_elfformat = std::make_unique< ElfFormatT<64, Endianness::BigEndian> >(this);
    }
    else
        assert(false);
}

void ElfLoader::load() { m_elfformat->load(); }
Analyzer *ElfLoader::createAnalyzer(Disassembler *disassembler) const { return new ElfAnalyzer(disassembler); }
