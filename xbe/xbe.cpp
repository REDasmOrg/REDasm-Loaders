#include "xbe.h"
#include <redasm/support/utils.h>
#include <redasm/support/ordinals.h>

#define XBE_XBOXKRNL_BASEADDRESS 0x80000000

XbeLoader::XbeLoader(): Loader() { }
AssemblerRequest XbeLoader::assembler() const { return ASSEMBLER_REQUEST("x86", "x86_32"); }

bool XbeLoader::test(const LoadRequest &request) const
{
    auto* header = request.pointer<XbeImageHeader>();

    if((header->Magic != XBE_MAGIC_NUMBER) || !header->SectionHeader || !header->NumberOfSections)
        return false;

    return true;
}

void XbeLoader::load()
{
    const auto* header = this->pointer<XbeImageHeader>();
    this->loadSections(header, this->memoryoffset<XbeSectionHeader>(header, header->SectionHeader));
    address_t entrypoint = 0;

    if(!this->decodeEP(header->EntryPoint, entrypoint))
    {
        r_ctx->log("Cannot decode Entry Point");
        return;
    }

    if(!this->loadXBoxKrnl(header))
    {
        r_ctx->log("Cannot load XBoxKrnl Imports");
        return;
    }

    ldrdoc->entry(entrypoint);
    this->displayXbeInfo(header);
}

void XbeLoader::displayXbeInfo(const XbeImageHeader* header)
{
    auto* certificate = this->memoryoffset<XbeCertificate>(header, header->CertificateAddress);
    String title = String::wide(certificate->TitleName, XBE_TITLENAME_SIZE);

    if(!title.empty())
        r_ctx->log("Game Title: " + title.quoted());

    String s;

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
        r_ctx->log("Allowed Regions: " + s);
}

bool XbeLoader::decodeEP(u32 encodedep, address_t& ep)
{
    ep = encodedep ^ XBE_ENTRYPOINT_XOR_RETAIL;
    const Segment* segment = ldrdoc->segment(ep);

    if(!segment)
    {
        ep = encodedep ^ XBE_ENTRYPOINT_XOR_DEBUG;
        segment = ldrdoc->segment(ep);

        if(segment)
            r_ctx->log("Executable Type: DEBUG");
    }
    else
        r_ctx->log("Executable Type: RETAIL");

    return segment != nullptr;
}

bool XbeLoader::decodeKernel(u32 encodedthunk, u32 &thunk)
{
    thunk = encodedthunk ^ XBE_KERNEL_XOR_RETAIL;
    const Segment* segment = ldrdoc->segment(thunk);

    if(!segment)
    {
        thunk = encodedthunk ^ XBE_KERNEL_XOR_DEBUG;
        segment = ldrdoc->segment(thunk);
    }

    return segment != nullptr;
}

void XbeLoader::loadSections(const XbeImageHeader* header, XbeSectionHeader *sectionhdr)
{
    for(u32 i = 0; i < header->NumberOfSections; i++)
    {
        String sectname = this->memoryoffset<const char>(header, sectionhdr[i].SectionName);
        type_t secttype = SegmentType::None;

        if(sectionhdr[i].Flags.Executable)
        {
            if((sectname[0] == '.') && sectname.contains("data"))
                secttype = SegmentType::Data;
            else
                secttype = SegmentType::Code;
        }
        else
            secttype = SegmentType::Data;

        if(!sectionhdr[i].RawSize)
            secttype = SegmentType::Bss;

        ldrdoc->segment(sectname, sectionhdr[i].RawAddress, sectionhdr[i].VirtualAddress, sectionhdr[i].RawSize, secttype);
    }

    ldrdoc->segment("XBOXKRNL", 0, XBE_XBOXKRNL_BASEADDRESS, 0x10000, SegmentType::Bss);
}

bool XbeLoader::loadXBoxKrnl(const XbeImageHeader* header)
{
    Ordinals ordinals;
    ordinals.load(r_ctx->db("xbe", "xboxkrnl.json"));

    u32 kernelimagethunk = 0;

    if(!this->decodeKernel(header->KernelImageThunk, kernelimagethunk))
        return false;

    offset_location thunkoffset = this->offset(kernelimagethunk);

    if(!thunkoffset.valid)
        return false;

    u32* pthunk = this->pointer<u32>(thunkoffset);

    while(*pthunk)
    {
        String ordinalname = ordinals.name(*pthunk ^ XBE_ORDINAL_FLAG, "XBoxKrnl!");
        ldrdoc->imported(*pthunk, sizeof(u32), ordinalname);
        pthunk++;
    }

    return true;
}

REDASM_LOADER("XBox Executable", "Dax", "MIT", 1)
REDASM_LOAD { xbe.plugin = new XbeLoader(); return true; }
REDASM_UNLOAD { xbe.plugin->release(); }
