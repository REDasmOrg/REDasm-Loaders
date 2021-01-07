#include "xbe.h"
#include <string>

#define XBE_XBOXKRNL_BASEADDRESS 0x80000000

const char* XbeLoader::test(const RDLoaderRequest* request)
{
    auto* header = reinterpret_cast<const XbeImageHeader*>(RDBuffer_Data(request->buffer));

    if((header->Magic != XBE_MAGIC_NUMBER) || !header->SectionHeader || !header->NumberOfSections)
        return nullptr;

    return "x86_32";
}

bool XbeLoader::load(RDContext* ctx, RDLoader* loader)
{
    const auto* header = reinterpret_cast<XbeImageHeader*>(RDLoader_GetData(loader));
    XbeLoader::loadSections(ctx, loader, header, XbeLoader::memoryoffset<XbeSectionHeader>(loader, header, header->SectionHeader));
    rd_address entrypoint = 0;

    if(!XbeLoader::decodeEP(ctx, header->EntryPoint, entrypoint))
    {
        RD_Log("Cannot decode Entry Point");
        return false;
    }

    //if(!XbeLoader::loadXBoxKrnl(header))
        //RD_Log("Cannot load XBoxKrnl Imports");

    RDDocument* doc = RDLoader_GetDocument(loader);
    RDDocument_SetEntry(doc, entrypoint);
    XbeLoader::displayXbeInfo(loader, header);
    return true;
}

void XbeLoader::displayXbeInfo(RDLoader* loader, const XbeImageHeader* header)
{
    auto* certificate = XbeLoader::memoryoffset<XbeCertificate>(loader, header, header->CertificateAddress);
    //auto title = std::wstring(&certificate->TitleName, XBE_TITLENAME_SIZE);

    //if(!title.empty())
        //rd_log("Game Title: " + title.quoted());

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

    if((decoded = RDDocument_GetSegmentAddress(doc, thunk, nullptr)))
    {
        thunk = encodedthunk ^ XBE_KERNEL_XOR_DEBUG;

        if(!RDDocument_GetSegmentAddress(doc, thunk, nullptr))
            return false;
    }

    return decoded;
}

void XbeLoader::loadSections(RDContext* ctx, RDLoader* loader, const XbeImageHeader* header, XbeSectionHeader *sectionhdr)
{
    auto* doc = RDContext_GetDocument(ctx);

    for(u32 i = 0; i < header->NumberOfSections; i++)
    {
        std::string sectname = XbeLoader::memoryoffset<const char>(loader, header, sectionhdr[i].SectionName);
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

// bool XbeLoader::loadXBoxKrnl(const XbeImageHeader* header)
// {
//     Ordinals ordinals;
//     ordinals.load(r_ctx->db(REDasm::FS::Path::join("xbe", "xboxkrnl.json")));
//
//     u32 kernelimagethunk = 0;
//
//     if(!this->decodeKernel(header->KernelImageThunk, kernelimagethunk))
//         return false;
//
//     offset_location thunkoffset = this->offset(kernelimagethunk);
//
//     if(!thunkoffset.valid)
//         return false;
//
//     u32* pthunk = this->pointer<u32>(thunkoffset);
//
//     while(*pthunk)
//     {
//         String ordinalname = ordinals.name(*pthunk ^ XBE_ORDINAL_FLAG, "XBoxKrnl!");
//         ldrdoc->imported(*pthunk, sizeof(u32), ordinalname);
//         pthunk++;
//     }
//
//     return true;
// }

void rdplugin_init(RDContext*, RDPluginModule* pm)
{
    RD_PLUGIN_ENTRY(RDEntryLoader, xbe, "XBox Executable");
    xbe.load = &XbeLoader::load;
    xbe.test = &XbeLoader::test;
    RDLoader_Register(pm, &xbe);
}
