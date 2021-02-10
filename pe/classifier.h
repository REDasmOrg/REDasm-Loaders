#pragma once

// https://en.wikipedia.org/wiki/Microsoft_Foundation_Class_Library
// https://github.com/horsicq/Detect-It-Easy/blob/master/db/PE/Microsoft%20Visual%20Studio.4.sg
// http://matthew-brett.github.io/pydagogue/python_msvc.html

#include <unordered_set>
#include <string>
#include "pe_header.h"
#include "pe_resources.h"
#include "dotnet/dotnet_header.h"

enum class PEClassification {
    Unclassified = 0,
    MinGW,
    VisualBasic_5, VisualBasic_6,
    VisualStudio, VisualStudio_4, VisualStudio_5, VisualStudio_6,
    VisualStudio_2002, VisualStudio_2003, VisualStudio_2005, VisualStudio_2008,
    VisualStudio_2010, VisualStudio_2012, VisualStudio_2013, VisualStudio_2015, VisualStudio_2017,
    DotNet_1, DotNet,
    BorlandDelphi, BorlandDelphi_3, BorlandDelphi_6, BorlandDelphi_7, BorlandDelphi_9_10,
    BorlandDelphi_XE, BorlandDelphi_XE2_6,
    BorlandCpp,
};

class PEClassifier
{
    public:
        PEClassifier(RDContext* ctx);
        const std::unordered_set<std::string> &signatures() const;
        bool isClassified() const;
        PEClassification checkDotNet() const;
        PEClassification checkVisualBasic() const;
        PEClassification checkVisualStudio() const;
        PEClassification checkBorland() const;
        PEClassification checkDelphi() const;
        bool isDotNet() const;
        bool isVisualBasic() const;
        bool isVisualStudio() const;
        bool isBorland() const;
        bool isDelphi() const;
        size_t bits() const;
        void setBits(size_t bits);
        void classifyVisualStudio();
        void classifyDotNet(ImageCorHeader* corheader);
        void classifyImport(const std::string &library);
        void classifyDelphi(const ImageDosHeader *dosheader, const ImageNtHeaders *ntheaders, const PEResources& peresources);
        void classify(const ImageNtHeaders* ntheaders);
        void display();

    private:
        void applyABI();
        void checkLinkerVersion(u8 major, u8 minor);
        void addSignature(const std::string &s);

    private:
        RDContext* m_context;
        PEClassification m_classification;
        size_t m_bits;
        std::string m_borlandsignature;
        std::unordered_set<std::string> m_signatures;
};
