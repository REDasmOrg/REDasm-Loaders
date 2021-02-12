#include "xbe.h"
#include <string>

#define XBE_XBOXKRNL_BASEADDRESS 0x80000000
#define XBOXKRNL_ORDINALS        "xboxkrnl_ordinals"

const char* XbeLoader::test(const RDLoaderRequest* request)
{
    auto* header = reinterpret_cast<const XbeImageHeader*>(RDBuffer_Data(request->buffer));

    if((header->Magic != XBE_MAGIC_NUMBER) || !header->SectionHeader || !header->NumberOfSections)
        return nullptr;

    return "x86_32";
}

bool XbeLoader::load(RDContext* ctx)
{
    const auto* header = reinterpret_cast<XbeImageHeader*>(RDContext_GetBufferData(ctx));
    XbeLoader::loadSections(ctx, header, XbeLoader::memoryoffset<XbeSectionHeader>(ctx, header, header->SectionHeader));
    rd_address entrypoint = 0;

    if(!XbeLoader::decodeEP(ctx, header->EntryPoint, entrypoint))
    {
        rd_log("Cannot decode Entry Point");
        return false;
    }

    if(!XbeLoader::loadXBoxKrnl(ctx, header)) rd_log("Cannot load XBoxKrnl Imports");

    RDDocument* doc = RDContext_GetDocument(ctx);
    RDDocument_SetEntry(doc, entrypoint);
    XbeLoader::displayXbeInfo(ctx, header);
    return true;
}

void XbeLoader::displayXbeInfo(RDContext* ctx, const XbeImageHeader* header)
{
    auto* certificate = XbeLoader::memoryoffset<XbeCertificate>(ctx, header, header->CertificateAddress);

    size_t len = XBE_TITLENAME_SIZE;
    const auto* title = RD_FromWString(reinterpret_cast<const char16_t*>(&certificate->TitleName), &len);
    if(title && len) rd_log("Game Title: '" + std::string(title) + "'");

    std::string s;

    if(certificate->GameRegion & XBE_GAME_REGION_RESTOFWORLD)
        s += "ALL";
    else
    {
        if(certificate->GameRegion & XBE_GAME_REGION_JAPAN)
            s += s.empty() ? "JAPAN" : ", JAPAN";

        if(certificate->GameRegion & XBE_GAME_REGION_NA)
            s += s.empty() ? "NORTH AMERICA" : ", NORTH AMERICA";
    }

    if(certificate->GameRegion & XBE_GAME_REGION_MANUFACTURING)
        s += s.empty() ? "DEBUG" : ", DEBUG";

    if(!s.empty())
        rd_log("Allowed Regions: " + s);
}

bool XbeLoader::decodeEP(RDContext* ctx, u32 encodedep, rd_address& ep)
{
    auto* doc = RDContext_GetDocument(ctx);
    ep = encodedep ^ XBE_ENTRYPOINT_XOR_RETAIL;

    bool decoded = false;

    if(!(decoded = RDDocument_GetSegmentAddress(doc, ep, nullptr)))
    {
        ep = encodedep ^ XBE_ENTRYPOINT_XOR_DEBUG;

        if((decoded = RDDocument_GetSegmentAddress(doc, ep, nullptr)))
            rd_log("Executable Type: DEBUG");
    }
    else
        rd_log("Executable Type: RETAIL");

    return decoded;
}

bool XbeLoader::decodeKernel(RDContext* ctx, u32 encodedthunk, u32 &thunk)
{
    auto* doc = RDContext_GetDocument(ctx);
    thunk = encodedthunk ^ XBE_KERNEL_XOR_RETAIL;

    bool decoded = false;

    if(!(decoded = RDDocument_GetSegmentAddress(doc, thunk, nullptr)))
    {
        thunk = encodedthunk ^ XBE_KERNEL_XOR_DEBUG;

        if(!(decoded = RDDocument_GetSegmentAddress(doc, thunk, nullptr)))
            return false;
    }

    return decoded;
}

void XbeLoader::loadSections(RDContext* ctx, const XbeImageHeader* header, XbeSectionHeader *sectionhdr)
{
    auto* doc = RDContext_GetDocument(ctx);

    for(u32 i = 0; i < header->NumberOfSections; i++)
    {
        std::string sectname = XbeLoader::memoryoffset<const char>(ctx, header, sectionhdr[i].SectionName);
        rd_flag secttype = SegmentFlags_None;

        if(sectionhdr[i].Flags.Executable)
        {
            if((sectname[0] == '.') && (sectname.find("data") != std::string::npos))
                secttype = SegmentFlags_Data;
            else
                secttype = SegmentFlags_Code;
        }
        else
            secttype = SegmentFlags_Data;

        if(!sectionhdr[i].RawSize)
            secttype = SegmentFlags_Bss;

        RDDocument_AddSegment(doc, sectname.c_str(), sectionhdr[i].RawAddress, sectionhdr[i].VirtualAddress, sectionhdr[i].RawSize, secttype);
    }

    RDDocument_AddSegment(doc, "XBOXKRNL", 0, XBE_XBOXKRNL_BASEADDRESS, 0x10000, SegmentFlags_Bss);
}

bool XbeLoader::loadXBoxKrnl(RDContext* ctx, const XbeImageHeader* header)
{
    auto* db = RDContext_GetDatabase(ctx);

    if(!RDDatabase_Add(db, XBOXKRNL_ORDINALS, "loaders/xbe/xboxkrnl"))
        return false;

    u32 kernelimagethunk = 0;

    if(!XbeLoader::decodeKernel(ctx, header->KernelImageThunk, kernelimagethunk))
        return false;

    auto loc = RD_Offset(ctx, kernelimagethunk);
    if(!loc.valid) return false;

    auto* doc = RDContext_GetDocument(ctx);
    u32* pthunk = reinterpret_cast<u32*>(RD_Pointer(ctx, loc.offset));

    while(*pthunk)
    {
        u32 ordinal = *pthunk ^ XBE_ORDINAL_FLAG;
        RDDatabaseValue value;

        if(RDDatabase_Query(db, (std::string(XBOXKRNL_ORDINALS) + "/" + std::to_string(ordinal)).c_str(), &value))
            RDDocument_AddImported(doc, *pthunk, sizeof(u32), value.s);
        else
            RDDocument_AddImported(doc, *pthunk, sizeof(u32), ("XBoxKrnl!Ordinal_" + rd_tohexbits(ordinal, 16, false)).c_str());

        pthunk++;
    }

    return true;
}

void rdplugin_init(RDContext*, RDPluginModule* pm)
{
    RD_PLUGIN_ENTRY(RDEntryLoader, xbe, "XBox Executable");
    xbe.load = &XbeLoader::load;
    xbe.test = &XbeLoader::test;
    RDLoader_Register(pm, &xbe);
}
