#include "n64.h"

// https://level42.ca/projects/ultra64/Documentation/man/pro-man/pro09/index9.3.html
// http://en64.shoutwiki.com/wiki/ROM#Cartridge_ROM_Header

#define N64_KUSEG_START_ADDR   0x00000000   // TLB mapped
#define N64_KUSEG_SIZE         0x7fffffff

#define N64_KSEG0_START_ADDR   0x80000000   // Direct mapped, cached
#define N64_KSEG0_SIZE         0x1FFFFFFF

#define N64_KSEG1_START_ADDR   0xa0000000   // Direct mapped, uncached
#define N64_KSEG1_SIZE         0x1FFFFFFF

#define N64_KSSEG_START_ADDR   0xc0000000   // TLB mapped
#define N64_KSSEG_SIZE         0x1FFFFFFF

#define N64_KSEG3_START_ADDR   0xe0000000   // TLB mapped
#define N64_KSEG3_SIZE         0x1FFFFFFF

#define N64_SEGMENT_AREA(name) N64_##name##_START_ADDR, N64_##name##_SIZE

#define N64_MAGIC_BS            0x37804012
#define N64_MAGIC_BE            0x80371240
#define N64_MAGIC_LE            0x40123780
#define N64_MAGIC_BE_B1         0x80
#define N64_MAGIC_BS_B1         0x37
#define N64_MAGIC_LE_B1         0x40

const char* N64Loader::test(const RDLoaderRequest* request)
{
    auto* header = reinterpret_cast<const N64RomHeader*>(RDBuffer_Data(request->buffer));
    u32 magic = rd_frombe32(header->magic);

    if((magic != N64_MAGIC_BS) && (magic != N64_MAGIC_BE) && (magic != N64_MAGIC_LE))
        return nullptr;

    //MemoryBuffer swappedbuffer;

    //if(magic != N64_MAGIC_BE)
    //{
    //    REDasm::swapEndianness<u16>(request.view().buffer(), &swappedbuffer, sizeof(N64RomHeader)); // Swap the header
    //    header = reinterpret_cast<const N64RomHeader*>(swappedbuffer.data());
    //}

    //if(!N64Loader::checkMediaType(header) || !N64Loader::checkCountryCode(header))
    //    return false;

    //if(!swappedbuffer.empty()) // Swap all
    //{
    //    REDasm::swapEndianness<u16>(request.view().buffer(), &swappedbuffer);
    //    header = reinterpret_cast<const N64RomHeader*>(swappedbuffer.data());

    //    BufferView swappedview = swappedbuffer.view();
    //    return N64Loader::checkChecksum(header, swappedview);
    //}

    //return N64Loader::checkChecksum(header, request.view())) "mips64be" : nullptr;
    return "mips64be";
}

bool N64Loader::load(RDContext* ctx)
{
    auto* header = reinterpret_cast<const N64RomHeader*>(RDContext_GetBufferData(ctx));
    RDDocument* doc = RDContext_GetDocument(ctx);

    //if(header->magic_0 != N64_MAGIC_BE_B1)
        //REDasm::swapEndianness<u16>(this->buffer());

    RDDocument_SetSegment(doc, "KSEG0", N64_ROM_HEADER_SIZE,
                          N64Loader::getEP(header), RDContext_GetBufferSize(ctx) - N64_ROM_HEADER_SIZE, SegmentFlags_CodeData);

    // TODO: map other segments
    RDDocument_SetEntry(doc, N64Loader::getEP(header));
    return true;
}

u32 N64Loader::getEP(const N64RomHeader* header)
{
    u32 pc = rd_frombe32(header->program_counter);
    u32 cicversion = N64Loader::getCICVersion(header);

    switch(cicversion)
    {
        case 6103: // CIC 6103 EP manipulation
            pc -= 0x100000;
            break;

        case 6106: // CIC 6106 EP manipulation
            pc -= 0x200000;
            break;

        default: break;
    }

    return pc;
}

u32 N64Loader::calculateChecksum(const N64RomHeader* header, const RDBufferView* view, u32 *crc) // Adapted from n64crc (http://n64dev.org/n64crc.html)
{
    u32 bootcode, i, seed, t1, t2, t3, t4, t5, t6, r;
    u32 d;

    if(!N64Loader::getBootcodeAndSeed(header, &bootcode, &seed))
        return 1;

    t1 = t2 = t3 = t4 = t5 = t6 = seed;
    i = N64_ROM_CHECKSUM_START;

    while (i < (N64_ROM_CHECKSUM_START + N64_ROM_CHECKSUM_LENGTH))
    {
        d = rd_frombe32(*reinterpret_cast<const u32*>(view->data + i));

        if((t6 + d) < t6)
            t4++;

        t6 += d;
        t3 ^= d;
        r = RD_Rol32(d, (d & 0x1F));
        t5 += r;
        if (t2 > d) t2 ^= r;
        else t2 ^= t6 ^ d;

        if(bootcode == 6105)
            t1 += rd_frombe32(*reinterpret_cast<const u32*>(view->data + (N64_ROM_HEADER_SIZE + 0x0710 + (i & 0xFF)))) ^ d;
        else
            t1 += t5 ^ d;

        i += 4;
    }

    if(bootcode == 6103)
    {
        crc[0] = (t6 ^ t4) + t3;
        crc[1] = (t5 ^ t2) + t1;
    }
    else if(bootcode == 6106)
    {
        crc[0] = (t6 * t4) + t3;
        crc[1] = (t5 * t2) + t1;
    }
    else
    {
        crc[0] = t6 ^ t4 ^ t3;
        crc[1] = t5 ^ t2 ^ t1;
    }

    return 0;
}

bool N64Loader::checkChecksum(const N64RomHeader *header, const RDBufferView* view)
{
    u32 crc[2] = { 0 };

    if(!N64Loader::calculateChecksum(header, view, crc))
    {
        if((crc[0] == header->crc1) && (crc[1] == header->crc2))
            return true;
    }

    return false;
}

bool N64Loader::getBootcodeAndSeed(const N64RomHeader *header, u32 *bootcode, u32 *seed)
{
    switch((*bootcode = N64Loader::getCICVersion(header)))
    {
        case 6101:
        case 6102:
        case 7102: *seed = N64_ROM_CHECKSUM_CIC_6102; break;

        case 6103: *seed = N64_ROM_CHECKSUM_CIC_6103; break;
        case 6105: *seed = N64_ROM_CHECKSUM_CIC_6105; break;
        case 6106: *seed = N64_ROM_CHECKSUM_CIC_6106; break;
        default: *seed = 0; return false;
    }

    return true;
}

u32 N64Loader::getCICVersion(const N64RomHeader* header)
{
    u32 bootcodecrc = RD_Crc32(reinterpret_cast<const u8*>(header->boot_code), N64_BOOT_CODE_SIZE);
    u32 cic = 0;

    switch(bootcodecrc)
    {
        case N64_BOOT_CODE_CIC_6101_CRC: cic = 6101; break;
        case N64_BOOT_CODE_CIC_7102_CRC: cic = 7102; break;
        case N64_BOOT_CODE_CIC_6102_CRC: cic = 6102; break;
        case N64_BOOT_CODE_CIC_6103_CRC: cic = 6103; break;
        case N64_BOOT_CODE_CIC_6105_CRC: cic = 6105; break;
        case N64_BOOT_CODE_CIC_6106_CRC: cic = 6106; break;
        default: break;
    }

    return cic;
}

bool N64Loader::checkMediaType(const N64RomHeader *header)
{
    switch(header->media_format[3])
    {
        case 'N': // Cart
        case 'D': // 64DD disk
        case 'C': // Cartridge part of expandable game
        case 'E': // 64DD expansion for cart
        case 'Z': // Aleck64 cart
            return true;

        default:
            break;
    }

    return false;
}

bool N64Loader::checkCountryCode(const N64RomHeader *header)
{
    /*
     * 0x37 '7' "Beta"
     * 0x41 'A' "Asian (NTSC)"
     * 0x42 'B' "Brazilian"
     * 0x43 'C' "Chinese"
     * 0x44 'D' "German"
     * 0x45 'E' "North America"
     * 0x46 'F' "French"
     * 0x47 'G': Gateway 64 (NTSC)
     * 0x48 'H' "Dutch"
     * 0x49 'I' "Italian"
     * 0x4A 'J' "Japanese"
     * 0x4B 'K' "Korean"
     * 0x4C 'L': Gateway 64 (PAL)
     * 0x4E 'N' "Canadian"
     * 0x50 'P' "European (basic spec.)"
     * 0x53 'S' "Spanish"
     * 0x55 'U' "Australian"
     * 0x57 'W' "Scandinavian"
     * 0x58 'X' "European"
     * 0x59 'Y' "European"
     */

    if(header->country_code == 0x37) return true;
    if((header->country_code >= 0x41) && (header->country_code <= 0x4C)) return true;
    if((header->country_code == 0x4E) || (header->country_code == 0x50)) return true;
    if((header->country_code == 0x53) || (header->country_code == 0x55)) return true;
    if((header->country_code >= 0x57) && (header->country_code <= 0x59)) return true;
    return false;
}

void rdplugin_init(RDContext*, RDPluginModule* pm)
{
    RD_PLUGIN_ENTRY(RDEntryLoader, n64, "Nintendo 64 ROM");
    n64.test = &N64Loader::test;
    n64.load = &N64Loader::load;
    RDLoader_Register(pm, &n64);
}
