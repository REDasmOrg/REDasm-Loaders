#include "pe.h"
#include "pe_constants.h"
#include "pe_header.h"
#include "pe_debug.h"
#include "dotnet/dotnet.h"
#include "borland/borland_version.h"
#include "vb/vb_analyzer.h"
#include <climits>

const ImageNtHeaders* PELoader::getNtHeaders(RDLoader* loader, const ImageDosHeader** dosheader)
{
    return PELoader::getNtHeaders(RDLoader_GetBuffer(loader), dosheader);
}

const ImageNtHeaders* PELoader::getNtHeaders(RDBuffer* buffer, const ImageDosHeader** dosheader)
{
    ImageDosHeader* header = reinterpret_cast<ImageDosHeader*>(RDBuffer_Data(buffer));

    if((header->e_magic != IMAGE_DOS_SIGNATURE) || (header->e_lfanew >= RDBuffer_Size(buffer)))
        return nullptr;

    const ImageNtHeaders* ntheaders = reinterpret_cast<const ImageNtHeaders*>(RD_RelPointer(header, header->e_lfanew));
    if(ntheaders->Signature != IMAGE_NT_SIGNATURE) return nullptr;

    if(dosheader) *dosheader = header;
    return ntheaders;
}

size_t PELoader::getBits(const ImageNtHeaders* ntheaders)
{
    switch(ntheaders->OptionalHeaderMagic)
    {
        case IMAGE_NT_OPTIONAL_HDR32_MAGIC: return 32;
        case IMAGE_NT_OPTIONAL_HDR64_MAGIC: return 64;
        default: break;
    }

    return 0;
}

template<size_t b>
PELoaderT<b>::PELoaderT(RDLoaderPlugin* plugin, RDLoader* loader): m_plugin(plugin), m_loader(loader)
{
    m_document = RDLoader_GetDocument(loader);
    m_classifier.setBits(b);

    m_validimportsections.insert(".text");
    m_validimportsections.insert(".idata");
    m_validimportsections.insert(".rdata");
}

template<size_t b> const DotNetReader *PELoaderT<b>::dotNetReader() const { return m_dotnetreader.get(); }
template<size_t b> rd_address PELoaderT<b>::rvaToVa(rd_address rva) const { return rva + m_imagebase; }
template<size_t b> rd_address PELoaderT<b>::vaToRva(rd_address va) const { return va - m_imagebase; }
template<size_t b> const PEClassifier *PELoaderT<b>::classifier() const { return &m_classifier; }

template<size_t b>
void PELoaderT<b>::parse()
{
    m_ntheaders = PELoader::getNtHeaders(m_loader, &m_dosheader);
    m_sectiontable = IMAGE_FIRST_SECTION(m_ntheaders);

    if(b == 32) m_optionalheader = reinterpret_cast<const ImageOptionalHeader*>(&m_ntheaders->OptionalHeader32);
    else m_optionalheader = reinterpret_cast<const ImageOptionalHeader*>(&m_ntheaders->OptionalHeader64);

    m_imagebase = m_optionalheader->ImageBase;
    m_sectionalignment = m_optionalheader->SectionAlignment;
    m_entrypoint = m_imagebase + m_optionalheader->AddressOfEntryPoint;
    m_datadirectory = reinterpret_cast<const ImageDataDirectory*>(&m_optionalheader->DataDirectory);

    this->loadSections();
    ImageCorHeader* corheader = this->checkDotNet();

    if(m_classifier.checkDotNet() == PEClassification::DotNet_1) rd_log(".NET 1.x is not supported");
    else if(!corheader) this->loadDefault();
    else this->loadDotNet(reinterpret_cast<ImageCor20Header*>(corheader));

    m_classifier.display();
}

template<size_t b> void PELoaderT<b>::checkResources()
{
    const ImageDataDirectory& resourcedatadir = m_datadirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE];
    if(!resourcedatadir.VirtualAddress) return;

    ImageResourceDirectory* resourcedir = this->rvaPointer<ImageResourceDirectory>(resourcedatadir.VirtualAddress);
    if(!resourcedir) return;

    m_classifier.classifyDelphi(m_dosheader, m_ntheaders, resourcedir);
}

template<size_t b>
void PELoaderT<b>::checkDebugInfo()
{
    const ImageDataDirectory& debuginfodir = m_datadirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
    if(!debuginfodir.VirtualAddress) return;

    ImageDebugDirectory* debugdir = this->rvaPointer<ImageDebugDirectory>(debuginfodir.VirtualAddress);
    if(!debugdir) return;

    u64 dbgoffset = 0;

    if(debugdir->AddressOfRawData)
    {
        RDLocation offset = PEUtils::rvaToOffset(m_ntheaders, m_imagebase - debugdir->AddressOfRawData);
        if(offset.valid) dbgoffset = offset.value;
    }

    if(!dbgoffset && debugdir->PointerToRawData)
        dbgoffset = debugdir->PointerToRawData;

    if(debugdir->Type == IMAGE_DEBUG_TYPE_CODEVIEW)
    {
        rd_log("Debug info type: CodeView");
        m_classifier.classifyVisualStudio();

        CVHeader* cvhdr = reinterpret_cast<CVHeader*>(RD_Pointer(m_loader, dbgoffset));
        if(!cvhdr) return;

        if(cvhdr->Signature == PE_PDB_NB10_SIGNATURE)
        {
            CvInfoPDB20* pdb20 = reinterpret_cast<CvInfoPDB20*>(cvhdr);
            rd_log("PDB 2.0 @ '" + std::string(pdb20->PdbFileName) + "'");
        }
        else if(cvhdr->Signature == PE_PDB_RSDS_SIGNATURE)
        {
            CvInfoPDB70* pdb70 = reinterpret_cast<CvInfoPDB70*>(cvhdr);
            rd_log("PDB 7.0 @ '" + std::string(pdb70->PdbFileName) + "'");
        }
        else
            rd_log("Unknown Signature: '" + std::string(reinterpret_cast<const char*>(&cvhdr->Signature), sizeof(u32)) + "'");
    }
    else if(debugdir->Type == IMAGE_DEBUG_TYPE_UNKNOWN) rd_log("Debug info type: UNKNOWN");
    else if(debugdir->Type == IMAGE_DEBUG_TYPE_COFF) rd_log("Debug info type: COFF");
    else if(debugdir->Type == IMAGE_DEBUG_TYPE_FPO) rd_log("Debug info type: FPO");
    else if(debugdir->Type == IMAGE_DEBUG_TYPE_MISC) rd_log("Debug info type: Misc");
    else if(debugdir->Type == IMAGE_DEBUG_TYPE_EXCEPTION) rd_log("Debug info type: Exception");
    else if(debugdir->Type == IMAGE_DEBUG_TYPE_FIXUP) rd_log("Debug info type: FixUp");
    else if(debugdir->Type == IMAGE_DEBUG_TYPE_OMAP_TO_SRC) rd_log("Debug info type: OMAP to Src");
    else if(debugdir->Type == IMAGE_DEBUG_TYPE_OMAP_FROM_SRC) rd_log("Debug info type: OMAP from Src");
    else if(debugdir->Type == IMAGE_DEBUG_TYPE_BORLAND) rd_log("Debug info type: Borland");
    else if(debugdir->Type == IMAGE_DEBUG_TYPE_RESERVED10) rd_log("Debug info type: Reserved10");
    else if(debugdir->Type == IMAGE_DEBUG_TYPE_CLSID) rd_log("Debug info type: CLSID");
    else if(debugdir->Type == IMAGE_DEBUG_TYPE_VC_FEATURE) rd_log("Debug info type: VC Feature");
    else if(debugdir->Type == IMAGE_DEBUG_TYPE_POGO) rd_log("Debug info type: POGO");
    else if(debugdir->Type == IMAGE_DEBUG_TYPE_ILTCG) rd_log("Debug info type: ILTCG");
    else if(debugdir->Type == IMAGE_DEBUG_TYPE_REPRO) rd_log("Debug info type: REPRO");
    else rd_log("Unknown Debug info type (value " + rd_tohexbits(debugdir->Type, 32, true) + ")");
}

template<size_t b> ImageCorHeader* PELoaderT<b>::checkDotNet()
{
    const ImageDataDirectory& dotnetdir = m_datadirectory[IMAGE_DIRECTORY_ENTRY_DOTNET];
    if(!dotnetdir.VirtualAddress) return nullptr;

    ImageCorHeader* corheader = this->rvaPointer<ImageCorHeader>(dotnetdir.VirtualAddress);
    m_classifier.classifyDotNet(corheader);
    return corheader;
}

template<size_t b>
void PELoaderT<b>::loadDotNet(ImageCor20Header* corheader)
{
    if(!corheader->MetaData.VirtualAddress)
    {
        rd_log("Invalid .NET MetaData");
        return;
    }

    ImageCor20MetaData* cormetadata = this->rvaPointer<ImageCor20MetaData>(corheader->MetaData.VirtualAddress);
    if(!cormetadata) return;

    m_dotnetreader = std::make_unique<DotNetReader>(cormetadata);
    if(!m_dotnetreader->isValid()) return;

    m_dotnetreader->iterateTypes([&](u32 rva, const std::string& name) {
        RDDocument_AddFunction(m_document, m_imagebase + rva, name.c_str());
    });
}

template<size_t b> void PELoaderT<b>::loadDefault()
{
    this->loadExports();

    if(!this->loadImports())
        rd_log("WARNING: This file seems to be PACKED");

    this->loadTLS();
    this->loadConfig();
    this->loadExceptions();
    this->loadSymbolTable();
    this->checkDebugInfo();
    this->checkResources();

    RDDocument_SetEntry(m_document, m_entrypoint);
    m_classifier.classify(m_ntheaders);

    //for(const auto& sig : m_classifier.signatures())
        //m_peloader->signature(sig);
}

template<size_t b>
void PELoaderT<b>::loadSections()
{
    for(size_t i = 0; i < m_ntheaders->FileHeader.NumberOfSections; i++)
    {
        const ImageSectionHeader& section = m_sectiontable[i];
        rd_type type = SegmentFlags_None;

        if((section.Characteristics & IMAGE_SCN_CNT_CODE) || (section.Characteristics & IMAGE_SCN_MEM_EXECUTE))
            type |= SegmentFlags_Code;

        if((section.Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA) || (section.Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA))
            type |= SegmentFlags_Data;

        u64 vsize = section.Misc.VirtualSize;
        if(!section.SizeOfRawData) type |= SegmentFlags_Bss;

        u64 diff = vsize % m_sectionalignment;
        if(diff) vsize += m_sectionalignment - diff;

        std::string name = PEUtils::sectionName(reinterpret_cast<const char*>(section.Name));

        if(name.empty()) // Rename unnamed sections
            name = "sect" + std::string(RD_ToString(i));

        rd_address va = m_imagebase + section.VirtualAddress;

        if(RD_InRangeSize(m_entrypoint, va, vsize)) // Entry point always points to code segment
            type |= SegmentFlags_Code;

        RDDocument_AddSegmentSize(m_document, name.c_str(), section.PointerToRawData, va, section.SizeOfRawData, vsize, type);
    }
}

template<size_t b>
void PELoaderT<b>::loadExports()
{
    const ImageDataDirectory& exportdir = m_datadirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if(!exportdir.VirtualAddress) return;

    ImageExportDirectory* exporttable = this->rvaPointer<ImageExportDirectory>(exportdir.VirtualAddress);
    if(!exporttable) return;

    u32* functions = this->rvaPointer<u32>(exporttable->AddressOfFunctions);
    u32* names = this->rvaPointer<u32>(exporttable->AddressOfNames);
    u16* nameords = this->rvaPointer<u16>(exporttable->AddressOfNameOrdinals);

    if(!functions || !names || !nameords)
    {
        rd_log("Corrupted export table");
        return;
    }

    for(size_t i = 0; i < exporttable->NumberOfFunctions; i++)
    {
        if(!functions[i]) continue;

        bool namedfunction = false;
        u64 funcep = m_imagebase + functions[i];

        RDSegment segment;
        if(!RDDocument_GetSegmentAddress(m_document, funcep, &segment)) continue;

        bool isfunction = HAS_FLAG(&segment, SegmentFlags_Code);

        for(pe_integer_t j = 0; j < exporttable->NumberOfNames; j++)
        {
            if(nameords[j] != i) continue;
            namedfunction = true;

            if(isfunction) RDDocument_AddExportedFunction(m_document, funcep, this->rvaPointer<const char>(names[j]));
            else RDDocument_AddExported(m_document, funcep, this->rvaPointer<const char>(names[j]));
            break;
        }

        if(namedfunction) continue;

        std::stringstream ss;
        ss << "Ordinal__" << RD_ToHexBits(exporttable->Base + i, 16, false);

        if(isfunction) RDDocument_AddExportedFunction(m_document, funcep, ss.str().c_str());
        else RDDocument_AddExported(m_document, funcep, ss.str().c_str());
    }
}

template<size_t b>
bool PELoaderT<b>::loadImports()
{
    const ImageDataDirectory& importdir = m_datadirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if(!importdir.VirtualAddress) return false;

    ImageImportDescriptor* importtable = this->rvaPointer<ImageImportDescriptor>(importdir.VirtualAddress);
    if(!importtable) return false;

    for(size_t i = 0; i < importtable[i].FirstThunk; i++)
        this->readDescriptor(importtable[i], b == 64 ? IMAGE_ORDINAL_FLAG64 : IMAGE_ORDINAL_FLAG32);

    RDSegment segment;
    return RDDocument_GetSegmentAddress(m_document, m_imagebase + importdir.VirtualAddress, &segment) && (m_validimportsections.find(segment.name) != m_validimportsections.end());
}

template<size_t b>
void PELoaderT<b>::loadExceptions()
{
    const ImageDataDirectory& exceptiondir = m_datadirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
    if(!exceptiondir.VirtualAddress || !exceptiondir.Size) return;

    ImageRuntimeFunctionEntry* runtimeentry = this->rvaPointer<ImageRuntimeFunctionEntry>(exceptiondir.VirtualAddress);
    if(!runtimeentry) return;

    u64 c = 0, csize = 0;

    for(pe_integer_t i = 0; csize < exceptiondir.Size; i++, csize += sizeof(ImageRuntimeFunctionEntry))
    {
        rd_address va = m_imagebase + runtimeentry[i].BeginAddress;
        if(!RDDocument_GetSegmentAddress(m_document, va, nullptr) || (runtimeentry[i].UnwindInfoAddress & 1)) continue;

        UnwindInfo* unwindinfo = this->rvaPointer<UnwindInfo>(runtimeentry[i].UnwindInfoAddress & ~1u);
        if(!unwindinfo || (unwindinfo->Flags & UNW_FLAG_CHAININFO)) continue;

        RDDocument_AddFunction(m_document, va, nullptr);
        c++;
    }

    if(c) rd_log("Found " + std::to_string(c) + " function(s) in Exception Directory");
}

template<size_t b>
void PELoaderT<b>::loadConfig()
{
    const ImageDataDirectory& configdir = m_datadirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG];
    if(!configdir.VirtualAddress) return;

    ImageLoadConfigDirectory* loadconfigdir = this->rvaPointer<ImageLoadConfigDirectory>(configdir.VirtualAddress);
    if(!loadconfigdir || !loadconfigdir->SecurityCookie) return;

    RDDocument_AddData(m_document, loadconfigdir->SecurityCookie, b, PE_SECURITY_COOKIE_SYMBOL);
}

template<size_t b>
void PELoaderT<b>::loadTLS()
{
    const ImageDataDirectory& tlsdir = m_datadirectory[IMAGE_DIRECTORY_ENTRY_TLS];
    if(!tlsdir.VirtualAddress) return;

    ImageTlsDirectory* imagetlsdir = this->rvaPointer<ImageTlsDirectory>(tlsdir.VirtualAddress);
    if(imagetlsdir) this->readTLSCallbacks(imagetlsdir);
}

template<size_t b> void PELoaderT<b>::loadSymbolTable()
{
    if(!m_ntheaders->FileHeader.PointerToSymbolTable || !m_ntheaders->FileHeader.NumberOfSymbols)
        return;

    rd_log("Loading symbol table @ " + rd_tohex(m_ntheaders->FileHeader.PointerToSymbolTable));

    RDArguments a;
    RDArguments_Init(&a);
    RDArguments_PushPointer(&a, RDLoader_GetDocument(m_loader));
    RDArguments_PushPointer(&a, RD_Pointer(m_loader, m_ntheaders->FileHeader.PointerToSymbolTable));
    RDArguments_PushUInt(&a, m_ntheaders->FileHeader.NumberOfSymbols);
    RDCommand_Execute("COFF", &a);
}

template<size_t b>
void PELoaderT<b>::readTLSCallbacks(const ImageTlsDirectory *tlsdirectory)
{
    if(!tlsdirectory->AddressOfCallBacks) return;

    pe_integer_t* callbacks = reinterpret_cast<pe_integer_t*>(RD_AddrPointer(m_loader, tlsdirectory->AddressOfCallBacks));

    for(pe_integer_t i = 0; *callbacks; i++, callbacks++)
        RDDocument_AddFunction(m_document, *callbacks, rd_str("TlsCallback_" + std::to_string(i)));
}

template<size_t b>
void PELoaderT<b>::readDescriptor(const ImageImportDescriptor& importdescriptor, pe_integer_t ordinalflag)
{
    // Check if OFT exists
    ImageThunkData* thunk = this->rvaPointer<ImageThunkData>(importdescriptor.OriginalFirstThunk ? importdescriptor.OriginalFirstThunk : importdescriptor.FirstThunk);
    if(!thunk) return;

    std::string descriptorname = this->rvaPointer<const char>(importdescriptor.Name);
    std::transform(descriptorname.begin(), descriptorname.end(), descriptorname.begin(), ::tolower);
    m_classifier.classifyImport(descriptorname);

    for(size_t i = 0; thunk[i]; i++)
    {
        std::string importname;
        rd_address address = m_imagebase + (importdescriptor.FirstThunk + (i * sizeof(ImageThunkData))); // Instructions refers to FT

        if(!(thunk[i] & ordinalflag))
        {
            ImageImportByName* importbyname = this->rvaPointer<ImageImportByName>(thunk[i]);
            if(!importbyname) continue;

            importname = PEUtils::importName(descriptorname, reinterpret_cast<const char*>(&importbyname->Name));
        }
        else
        {
            u16 ordinal = static_cast<u16>(ordinalflag ^ thunk[i]);

            if(!m_imports.importName<b>(descriptorname, ordinal, importname)) importname = PEUtils::importName(descriptorname, ordinal);
            else importname = PEUtils::importName(descriptorname, importname);
        }

        RDDocument_AddImported(m_document, address, b / CHAR_BIT, importname.c_str());
    }
}

const char* PELoader::assembler(const ImageNtHeaders* ntheaders)
{
    // if(m_classifier.isDotNet())
    // {
    //     m_plugin->header.puserdata = RD_FindAssembler("cil");
    //     return;
    // }

    switch(ntheaders->FileHeader.Machine)
    {
        case IMAGE_FILE_MACHINE_I386: return "x86_32";
        case IMAGE_FILE_MACHINE_AMD64: return "x86_64";

        case IMAGE_FILE_MACHINE_ARM:
            if(ntheaders->OptionalHeaderMagic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) return "arm64le";
            return "armle";

        default: break;
    }

    return nullptr;
}

void PELoader::free(RDPluginHeader* plugin)
{
    if(!plugin->p_data) return;

    delete reinterpret_cast<PELoader*>(plugin->p_data);
    plugin->p_data = nullptr;
}

const char* PELoader::test(const RDLoaderPlugin*, const RDLoaderRequest* request)
{
    const ImageNtHeaders* ntheaders = PELoader::getNtHeaders(request->buffer, nullptr);
    if(!ntheaders) return nullptr;

    size_t bits = PELoader::getBits(ntheaders);
    if(!bits) return nullptr;

    return PELoader::assembler(ntheaders);
}

bool PELoader::load(RDLoaderPlugin* plugin, RDLoader* loader)
{
    const ImageNtHeaders* ntheaders = PELoader::getNtHeaders(RDLoader_GetBuffer(loader), nullptr);
    PELoader* peloader = nullptr;

    if(PELoader::getBits(ntheaders) == 32) peloader = new PELoaderT<32>(plugin, loader);
    else peloader = new PELoaderT<64>(plugin, loader);

    plugin->p_data = peloader;
    peloader->parse();
    return true;
}

void PELoader::analyze(RDLoaderPlugin* plugin, RDDisassembler* disassembler)
{
    PELoader* loader = reinterpret_cast<PELoader*>(plugin->p_data);

    if(loader->classifier()->isVisualBasic())
    {
        VBAnalyzer a(loader, disassembler);
        a.analyze();
    }
    else
    {
        PEAnalyzer a(loader, disassembler);
        a.analyze();
    }
}


void redasm_entry()
{
    RD_PLUGIN_CREATE(RDLoaderPlugin, pe, "Portable Executable");
    pe.free = &PELoader::free;
    pe.test = &PELoader::test;
    pe.load = &PELoader::load;
    pe.analyze = &PELoader::analyze;

    RDLoader_Register(&pe);
}
