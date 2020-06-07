#include "esp_common.h"

#define IS_VALID_MAGIC(magic) ((magic == ESP_IMAGE1_MAGIC) || (magic == ESP_IMAGE2_MAGIC))

bool ESPCommon::load(RDLoader* loader, offset_t offset)
{
    u8* magic = RD_Pointer(loader, offset);

    switch(*magic)
    {
        case ESP_IMAGE1_MAGIC: return this->load(loader, reinterpret_cast<ESP8266RomHeader1*>(magic));
        case ESP_IMAGE2_MAGIC: return this->load(loader, reinterpret_cast<ESP8266RomHeader2*>(magic));
        default: rd_log("Unknown magic: " + rd_tohex(*magic)); break;
    }

    return false;
}

const char* ESPCommon::test(const RDLoaderPlugin*, const RDLoaderRequest* request)
{
    const auto* h = reinterpret_cast<const ESP8266RomHeader1*>(RDBuffer_Data(request->buffer));

    if(h->magic != ESP_IMAGE1_MAGIC) return nullptr;
    if(h->segments > 16) return nullptr;

    u8 freq = h->flashsizefreq & 0xF;

    switch(freq)
    {
        case FLASH_FREQ_40MHZ:
        case FLASH_FREQ_26MHZ:
        case FLASH_FREQ_20MHZ:
        case FLASH_FREQ_80MHZ:
            break;

        default: return nullptr;
    }

    u8 size = (h->flashsizefreq & 0xF0) > 4;

    switch(size)
    {
        case FLASH_SIZE_512K:
        case FLASH_SIZE_256K:
        case FLASH_SIZE_1M:
        case FLASH_SIZE_2M:
        case FLASH_SIZE_4M:
            break;

        default: return nullptr;
    }

    return "xtensale";
}

bool ESPCommon::load(RDLoader* loader, ESP8266RomHeader1 *header, offset_t offset)
{
    RDDocument* document = RDLoader_GetDocument(loader);
    auto* segment = reinterpret_cast<ESPSegment*>(RD_RelPointer(header, sizeof(ESP8266RomHeader1)));

    for(size_t i = 0 ; segment && (i < header->segments); i++)
    {
        std::string segname;
        flag_t segflags;

        if(segment->address == 0x40100000)
        {
            segname = ".user_rom";
            segflags = SegmentFlags_Code;
        }
        else if(segment->address == 0x3FFE8000)
        {
            segname = ".user_rom_data";
            segflags = SegmentFlags_Data;
        }
        else if(segment->address <= 0x3FFFFFFF)
        {
            segname = ".data_seg_" + std::to_string(i);
            segflags = SegmentFlags_Data;
        }
        else if(segment->address > 0x40100000)
        {
            segname = ".code_seg_" + std::to_string(i);
            segflags = SegmentFlags_Code;
        }
        else
        {
            segname = ".unknown_seg_" + std::to_string(i);
            segflags = SegmentFlags_Data;
        }

        RDDocument_AddSegment(document, segname.c_str(), RD_FileOffset(loader, segment).offset + sizeof(ESPSegment), segment->address, segment->size, segflags);
        if(offset != RD_NPOS) offset += segment->size;

        segment = reinterpret_cast<ESPSegment*>(RD_RelPointer(segment, sizeof(ESPSegment) + segment->size));
    }

    RDDocument_SetEntry(document, header->entrypoint);
    return true;
}

bool ESPCommon::load(RDLoader* loader, ESP8266RomHeader2 *header)
{
    auto* header1 = reinterpret_cast<ESP8266RomHeader1*>(RD_RelPointer(&header, sizeof(ESP8266RomHeader2) + header->size));
    if(!header1 || (header1->magic != ESP_IMAGE1_MAGIC)) return false;

    auto loc = RD_FileOffset(loader, header);
    if(!loc.valid) return false;

    return this->load(loader, header1, loc.offset + sizeof(ESP8266RomHeader2));
}
