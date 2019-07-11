#include "pe.h"
#include <redasm/support/utils.h>
#include "vb/vb_analyzer.h"
#include "pe_analyzer.h"

PELoader::PELoader(): Loader() { }

AssemblerRequest PELoader::assembler() const
{
    if(m_peformat->classifier()->isDotNet())
        return "cil";

    switch(this->ntHeaders()->FileHeader.Machine)
    {
        case IMAGE_FILE_MACHINE_I386: return ASSEMBLER_REQUEST("x86", "x86_32");
        case IMAGE_FILE_MACHINE_AMD64: return ASSEMBLER_REQUEST("x86", "x86_64");

        case IMAGE_FILE_MACHINE_ARM:
            if(this->ntHeaders()->OptionalHeaderMagic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
                return "arm64";
            return "arm";

        default: break;
    }

    return nullptr;
}

size_t PELoader::bits() const
{
    const ImageNtHeaders* ntheaders = this->ntHeaders();

    if(ntheaders->OptionalHeaderMagic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        return 64;

    return 32;
}

const ImageDosHeader *PELoader::dosHeader() const { return this->pointer<ImageDosHeader>(); }

const ImageNtHeaders* PELoader::ntHeaders() const
{
    const ImageDosHeader* dosheader = this->dosHeader();
    return Utils::relpointer<const ImageNtHeaders>(dosheader, dosheader->e_lfanew);
}

Analyzer *PELoader::createAnalyzer(Disassembler *disassembler) const
{
    if(m_peformat->classifier()->isVisualBasic())
        return new VBAnalyzer(m_peformat->classifier(), disassembler);

    return new PEAnalyzer(m_peformat->classifier(), disassembler);
}

bool PELoader::test(const LoadRequest &request) const
{
    const ImageDosHeader* header = request.pointer<ImageDosHeader>();

    if((header->e_magic != IMAGE_DOS_SIGNATURE) || !request.view().inRange(header->e_lfanew))
        return false;

    const ImageNtHeaders* ntheaders = Utils::relpointer<const ImageNtHeaders>(header, header->e_lfanew);

    if(ntheaders->Signature != IMAGE_NT_SIGNATURE)
        return false;

    switch(ntheaders->OptionalHeaderMagic)
    {
        case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
        case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
            break;

        default:
            return false;
    }

    return true;
}

void PELoader::init(const LoadRequest &request)
{
    Loader::init(request);

    if(this->bits() == 64)
        m_peformat = std::make_unique< PEFormatT<64> >(this);
    else
        m_peformat = std::make_unique< PEFormatT<32> >(this);
}

void PELoader::load() { m_peformat->load(); }
