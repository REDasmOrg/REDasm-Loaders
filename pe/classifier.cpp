#include "classifier.h"
#include "borland/borland_types.h"
#include "borland/borland_version.h"

PEClassifier::PEClassifier(RDContext* ctx): m_context(ctx), m_classification(PEClassification::Unclassified), m_bits(0) { }
const std::unordered_set<std::string> &PEClassifier::signatures() const { return m_signatures; }

PEClassification PEClassifier::checkVisualBasic() const
{
    if((m_classification == PEClassification::VisualBasic_5) || (m_classification == PEClassification::VisualBasic_6))
        return m_classification;

    return PEClassification::Unclassified;
}

PEClassification PEClassifier::checkDotNet() const
{
    if((m_classification == PEClassification::DotNet_1) || (m_classification == PEClassification::DotNet))
        return m_classification;

    return PEClassification::Unclassified;
}

PEClassification PEClassifier::checkVisualStudio() const
{
    if((m_classification >= PEClassification::VisualStudio) && (m_classification <= PEClassification::VisualStudio_2017))
        return m_classification;

    return PEClassification::Unclassified;
}

PEClassification PEClassifier::checkBorland() const
{
    if((m_classification == PEClassification::BorlandDelphi) || (m_classification == PEClassification::BorlandCpp))
        return m_classification;

    return PEClassification::Unclassified;
}

PEClassification PEClassifier::checkDelphi() const
{
    if((m_classification >= PEClassification::BorlandDelphi) && (m_classification <= PEClassification::BorlandDelphi_XE2_6))
        return m_classification;

    return PEClassification::Unclassified;
}

bool PEClassifier::isDotNet() const { return this->checkDotNet() != PEClassification::Unclassified; }
bool PEClassifier::isVisualBasic() const { return this->checkVisualBasic() != PEClassification::Unclassified; }
bool PEClassifier::isVisualStudio() const { return this->checkVisualStudio() != PEClassification::Unclassified; }
bool PEClassifier::isBorland() const { return this->checkBorland() != PEClassification::Unclassified; }
bool PEClassifier::isDelphi() const { return this->checkDelphi() != PEClassification::Unclassified; }
size_t PEClassifier::bits() const { return m_bits; }
void PEClassifier::setBits(size_t bits) { m_bits = bits; }

void PEClassifier::classifyVisualStudio()
{
    if(this->isClassified()) return;
    m_classification = PEClassification::VisualStudio;
}

void PEClassifier::classifyDotNet(ImageCorHeader *corheader)
{
    if(!corheader || (corheader->cb < sizeof(ImageCorHeader)))
        return;

    if(corheader->MajorRuntimeVersion == 1) m_classification = PEClassification::DotNet_1;
    else m_classification = PEClassification::DotNet;
}

void PEClassifier::classifyImport(const std::string& library)
{
    if(!library.find("msvbvm50"))
        m_classification = PEClassification::VisualBasic_5;
    else if(!library.find("msvbvm60"))
        m_classification = PEClassification::VisualBasic_6;

    if((this->isVisualBasic() || this->isClassified()) && (m_classification != PEClassification::VisualStudio))
        return;

    if(!library.find("msvcp50"))
        m_classification = PEClassification::VisualStudio_5;
    else if(!library.find("msvcp60") || !library.find("msvcrt."))
        m_classification = PEClassification::VisualStudio_6;
    else if(!library.find("msvcp70") || !library.find("msvcr70"))
        m_classification = PEClassification::VisualStudio_2002;
    else if(!library.find("msvcp71") || !library.find("msvcr71"))
        m_classification = PEClassification::VisualStudio_2003;
    else if(!library.find("msvcp80") || !library.find("msvcr80"))
        m_classification = PEClassification::VisualStudio_2005;
    else if(!library.find("msvcp90") || !library.find("msvcr90"))
        m_classification = PEClassification::VisualStudio_2008;
    else if(!library.find("msvcp100") || library.find("msvcr100"))
        m_classification = PEClassification::VisualStudio_2010;
    else if(!library.find("msvcp110") || !library.find("msvcr110"))
        m_classification = PEClassification::VisualStudio_2012;
    else if(!library.find("msvcp120") || !library.find("msvcr120"))
        m_classification = PEClassification::VisualStudio_2013;
    else if(!library.find("msvcp140") || !library.find("vcruntime140"))
        m_classification = PEClassification::VisualStudio_2015;
}

void PEClassifier::classifyDelphi(const ImageDosHeader* dosheader, const ImageNtHeaders* ntheaders, const PEResources &peresources)
{
    PEResources::ResourceItem ri = peresources.find(PEResources::RCDATA);

    if(!ri.second)
        return;

    ri = peresources.find("PACKAGEINFO", ri);

    if(!ri.second)
        return;

    u64 datasize = 0;
    PackageInfoHeader* packageinfo = peresources.data<PackageInfoHeader>(ri, dosheader, ntheaders, &datasize);

    if(!packageinfo)
    {
        rdcontext_addproblem(m_context, "Cannot parse 'PACKAGEINFO' header");
        return;
    }

    BorlandVersion borlandver(packageinfo, ri, datasize);

    if(borlandver.isDelphi())
    {
        m_borlandsignature = borlandver.getSignature();

        if(m_borlandsignature == "delphi3") m_classification = PEClassification::BorlandDelphi_3;
        else if(m_borlandsignature == "delphiXE2_6") m_classification = PEClassification::BorlandDelphi_XE2_6;
        else if(m_borlandsignature == "delphiXE") m_classification = PEClassification::BorlandDelphi_XE;
        else if(m_borlandsignature == "delphi9_10") m_classification = PEClassification::BorlandDelphi_9_10;
        else if(m_borlandsignature == "delphi6") m_classification = PEClassification::BorlandDelphi_6;
        else if(m_borlandsignature == "delphi6") m_classification = PEClassification::BorlandDelphi_7;
        else m_classification = PEClassification::BorlandDelphi;
    }
    else if(borlandver.isCpp())
        m_classification = PEClassification::BorlandCpp;
}

void PEClassifier::classify(const ImageNtHeaders *ntheaders)
{
    if(!this->isVisualBasic() || !this->isClassified())
    {
        if(ntheaders->OptionalHeaderMagic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
            this->checkLinkerVersion(ntheaders->OptionalHeader64.MajorLinkerVersion, ntheaders->OptionalHeader64.MinorLinkerVersion);
        else
            this->checkLinkerVersion(ntheaders->OptionalHeader32.MajorLinkerVersion, ntheaders->OptionalHeader32.MinorLinkerVersion);
    }

    if(this->isBorland()) this->addSignature(m_borlandsignature);
    else if(m_classification == PEClassification::VisualStudio_6) this->addSignature("msvc6");
    else if(m_classification == PEClassification::VisualStudio_2003) this->addSignature("msvc2003");
    else if(m_classification == PEClassification::VisualStudio_2005) this->addSignature("msvc2005");
    else if(m_classification == PEClassification::VisualStudio_2008) this->addSignature("msvc2008");
    else if(m_classification == PEClassification::VisualStudio_2017) this->addSignature("msvc2017");
}

void PEClassifier::display()
{
    switch(m_classification)
    {
        case PEClassification::VisualBasic_5: rd_log("PE Classification: Visual Basic 5"); break;
        case PEClassification::VisualBasic_6: rd_log("PE Classification: Visual Basic 6"); break;
        case PEClassification::VisualStudio: rd_log("PE Classification: Visual Studio"); break;
        case PEClassification::VisualStudio_4: rd_log("PE Classification: Visual Studio 4"); break;
        case PEClassification::VisualStudio_5: rd_log("PE Classification: Visual Studio 5"); break;
        case PEClassification::VisualStudio_6: rd_log("PE Classification: Visual Studio 6"); break;
        case PEClassification::VisualStudio_2002: rd_log("PE Classification: Visual Studio 2002"); break;
        case PEClassification::VisualStudio_2003: rd_log("PE Classification: Visual Studio 2003"); break;
        case PEClassification::VisualStudio_2005: rd_log("PE Classification: Visual Studio 2005"); break;
        case PEClassification::VisualStudio_2008: rd_log("PE Classification: Visual Studio 2008"); break;
        case PEClassification::VisualStudio_2010: rd_log("PE Classification: Visual Studio 2010"); break;
        case PEClassification::VisualStudio_2012: rd_log("PE Classification: Visual Studio 2012"); break;
        case PEClassification::VisualStudio_2013: rd_log("PE Classification: Visual Studio 2013"); break;
        case PEClassification::VisualStudio_2015: rd_log("PE Classification: Visual Studio 2015"); break;
        case PEClassification::VisualStudio_2017: rd_log("PE Classification: Visual Studio 2017"); break;
        case PEClassification::DotNet_1: rd_log("PE Classification: .NET 1.x"); break;
        case PEClassification::DotNet: rd_log("PE Classification: .NET >= 2.x"); break;
        case PEClassification::BorlandDelphi: rd_log("PE Classification: Borland Delphi"); break;
        case PEClassification::BorlandDelphi_3: rd_log("PE Classification: Borland Delphi 3"); break;
        case PEClassification::BorlandDelphi_6: rd_log("PE Classification: Borland Delphi 6"); break;
        case PEClassification::BorlandDelphi_7: rd_log("PE Classification: Borland Delphi 7"); break;
        case PEClassification::BorlandDelphi_9_10: rd_log("PE Classification: Borland Delphi 9/10"); break;
        case PEClassification::BorlandDelphi_XE: rd_log("PE Classification: Borland Delphi XE"); break;
        case PEClassification::BorlandDelphi_XE2_6: rd_log("PE Classification: Borland Delphi XE 2.6"); break;
        case PEClassification::BorlandCpp: rd_log("PE Classification: Borland C++"); break;
        default: rd_log("PE Classification: Unclassified"); break;
    }

    this->applyABI();
}

void PEClassifier::applyABI()
{
    switch(m_classification)
    {
        case PEClassification::VisualBasic_5:
        case PEClassification::VisualBasic_6:
        case PEClassification::VisualStudio:
        case PEClassification::VisualStudio_4:
        case PEClassification::VisualStudio_5:
        case PEClassification::VisualStudio_6:
        case PEClassification::VisualStudio_2002:
        case PEClassification::VisualStudio_2003:
        case PEClassification::VisualStudio_2005:
        case PEClassification::VisualStudio_2008:
        case PEClassification::VisualStudio_2010:
        case PEClassification::VisualStudio_2012:
        case PEClassification::VisualStudio_2013:
        case PEClassification::VisualStudio_2015:
        case PEClassification::VisualStudio_2017:
            RDContext_SetABI(m_context, CompilerABI_MSVC);
            RDContext_SetCC(m_context, CompilerCC_Cdecl);
            break;

        case PEClassification::DotNet_1:
        case PEClassification::DotNet:
            RDContext_SetABI(m_context, CompilerABI_DotNET);
            break;

        case PEClassification::BorlandDelphi:
        case PEClassification::BorlandDelphi_3:
        case PEClassification::BorlandDelphi_6:
        case PEClassification::BorlandDelphi_7:
        case PEClassification::BorlandDelphi_9_10:
        case PEClassification::BorlandDelphi_XE:
        case PEClassification::BorlandDelphi_XE2_6:
        case PEClassification::BorlandCpp:
            RDContext_SetABI(m_context, CompilerABI_Borland);
            break;

        default: break;
    }
}

bool PEClassifier::isClassified() const { return m_classification != PEClassification::Unclassified;  }

void PEClassifier::checkLinkerVersion(u8 major, u8 minor)
{
    if(major == 4) m_classification = PEClassification::VisualStudio_4;
    else if(major == 5) m_classification = PEClassification::VisualStudio_5;
    else if(major == 6) m_classification = PEClassification::VisualStudio_6;
    else if(major == 7)
    {
        if(minor < 10) m_classification = PEClassification::VisualStudio_2002;
        else m_classification = PEClassification::VisualStudio_2003;
    }
    else if(major == 8) m_classification = PEClassification::VisualStudio_2005;
    else if(major == 9) m_classification = PEClassification::VisualStudio_2008;
    else if(major == 10) m_classification = PEClassification::VisualStudio_2010;
    else if(major == 11) m_classification = PEClassification::VisualStudio_2012;
    else if(major == 12) m_classification = PEClassification::VisualStudio_2013;
    else if(major == 14)
    {
        if(!minor) m_classification = PEClassification::VisualStudio_2015;
        else m_classification = PEClassification::VisualStudio_2017;
    }
}

void PEClassifier::addSignature(const std::string &s)
{
    if(s.empty()) return;
    m_signatures.insert(s);
}
