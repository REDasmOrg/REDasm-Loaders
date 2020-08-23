#pragma once

#include <type_traits>
#include "pe_header.h"
#include "pe_resources.h"
#include "pe_imports.h"
#include "pe_utils.h"
#include "classifier.h"
#include "dotnet/dotnet_header.h"
#include "dotnet/dotnet_reader.h"

class PELoader
{
    public:
        virtual ~PELoader() = default;
        virtual const PEClassifier* classifier() const = 0;
        virtual void parse() = 0;

    public:
        static void free(RDPluginHeader* plugin);
        static const char* test(const RDLoaderPlugin*, const RDLoaderRequest* request);
        static bool load(RDLoaderPlugin* plugin, RDLoader* loader);
        static const char* assembler(const ImageNtHeaders* ntheaders);

    public:
        static const ImageNtHeaders* getNtHeaders(RDLoader* loader, const ImageDosHeader** dosheader = nullptr);
        static const ImageNtHeaders* getNtHeaders(RDBuffer* buffer, const ImageDosHeader** dosheader = nullptr);
        static size_t getBits(const ImageNtHeaders* ntheaders);
};

template<size_t b> class PELoaderT: public PELoader
{
    private:
        typedef typename std::conditional<b == 64, u64, u32>::type pe_integer_t;
        typedef typename std::conditional<b == 64, ImageOptionalHeader64, ImageOptionalHeader32>::type ImageOptionalHeader;
        typedef typename std::conditional<b == 64, ImageThunkData64, ImageThunkData32>::type ImageThunkData;
        typedef typename std::conditional<b == 64, ImageTlsDirectory64, ImageTlsDirectory32>::type ImageTlsDirectory;
        typedef typename std::conditional<b == 64, ImageLoadConfigDirectory64, ImageLoadConfigDirectory32>::type ImageLoadConfigDirectory;

    public:
        PELoaderT(RDLoaderPlugin* plugin, RDLoader* loader);
        const PEClassifier* classifier() const override;
        void parse() override;

    public:
        const DotNetReader *dotNetReader() const;
        rd_address rvaToVa(rd_address rva) const;
        rd_address vaToRva(rd_address rva) const;

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
        template<typename T> T* rvaPointer(pe_integer_t rva) const {
            RDLocation offset = PEUtils::rvaToOffset(m_ntheaders, rva);
            return offset.valid ? reinterpret_cast<T*>(RD_Pointer(m_loader, offset.value)) : nullptr;
        }

    private:
        RDLoaderPlugin* m_plugin;
        RDLoader* m_loader;
        RDDocument* m_document;
        PEClassifier m_classifier;
        PEImports m_imports;
        std::unique_ptr<DotNetReader> m_dotnetreader;
        const ImageDosHeader* m_dosheader{nullptr};
        const ImageNtHeaders* m_ntheaders{nullptr};
        const ImageOptionalHeader* m_optionalheader{nullptr};
        const ImageSectionHeader* m_sectiontable{nullptr};
        const ImageDataDirectory* m_datadirectory{nullptr};
        pe_integer_t m_imagebase{0}, m_sectionalignment{0}, m_entrypoint{0};
        std::unordered_set<std::string> m_validimportsections;
};
