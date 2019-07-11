#pragma once

#include <type_traits>
#include "pe_header.h"
#include "pe_resources.h"
#include "pe_imports.h"
#include "pe_utils.h"
#include "pe_utils.h"
#include "pe_classifier.h"
#include "dotnet/dotnet_header.h"
#include "dotnet/dotnet_reader.h"

using namespace REDasm;

class PELoader;

class PEFormat
{
    public:
        virtual const PEClassifier* classifier() const = 0;
        virtual void load() = 0;
};

template<size_t b> class PEFormatT: public PEFormat
{
    private:
        typedef typename std::conditional<b == 64, u64, u32>::type pe_integer_t;
        typedef typename std::conditional<b == 64, ImageOptionalHeader64, ImageOptionalHeader32>::type ImageOptionalHeader;
        typedef typename std::conditional<b == 64, ImageThunkData64, ImageThunkData32>::type ImageThunkData;
        typedef typename std::conditional<b == 64, ImageTlsDirectory64, ImageTlsDirectory32>::type ImageTlsDirectory;
        typedef typename std::conditional<b == 64, ImageLoadConfigDirectory64, ImageLoadConfigDirectory32>::type ImageLoadConfigDirectory;

    public:
        PEFormatT(PELoader* loader);
        const PEClassifier* classifier() const override;
        void load() override;

    public:
        const DotNetReader *dotNetReader() const;
        address_t rvaToVa(address_t rva) const;
        address_t vaToRva(address_t rva) const;

    private:
        ImageCorHeader *checkDotNet();
        void readDescriptor(const ImageImportDescriptor& importdescriptor, pe_integer_t ordinalflag);
        void readTLSCallbacks(const ImageTlsDirectory* tlsdirectory);
        void checkResources();
        void checkDebugInfo();
        void loadDotNet(ImageCor20Header* corheader);
        void loadSymbolTable();
        void loadDefault();
        void loadSections();
        void loadExports();
        bool loadImports();
        void loadExceptions();
        void loadConfig();
        void loadTLS();

    private:
        PELoader* m_peloader;
        PEClassifier m_classifier;
        std::unique_ptr<DotNetReader> m_dotnetreader;
        const ImageOptionalHeader* m_optionalheader;
        const ImageSectionHeader* m_sectiontable;
        const ImageDataDirectory* m_datadirectory;
        pe_integer_t m_imagebase, m_sectionalignment, m_entrypoint;
        std::unordered_set<String> m_validimportsections;
};
