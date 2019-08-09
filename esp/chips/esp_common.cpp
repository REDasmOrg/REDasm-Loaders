#include "esp_common.h"
#include "../esp.h"
#include <redasm/support/utils.h>

#define IS_VALID_MAGIC(magic) ((magic == ESP_IMAGE1_MAGIC) || (magic == ESP_IMAGE2_MAGIC))

ESPCommon::ESPCommon(ESPLoader *loader): m_loader(loader) { }

void ESPCommon::load(offset_t offset)
{
    const u8* magic = m_loader->pointer<u8>(offset);

    switch(*magic)
    {
        case ESP_IMAGE1_MAGIC: this->load(reinterpret_cast<const ESP8266RomHeader1*>(magic)); break;
        case ESP_IMAGE2_MAGIC: this->load(reinterpret_cast<const ESP8266RomHeader2*>(magic)); break;
        default:               r_ctx->log("Unknown magic: " + String::hex(*magic)); break;
    }
}

bool ESPCommon::test(const LoadRequest &request)
{
    const ESP8266RomHeader1* h = request.pointer<ESP8266RomHeader1>();

    if(h->magic != ESP_IMAGE1_MAGIC)
        return false;
    if(h->segments > 16)
        return false;

    u8 freq = h->flashsizefreq & 0xF;

    switch(freq)
    {
        case FLASH_FREQ_40MHZ:
        case FLASH_FREQ_26MHZ:
        case FLASH_FREQ_20MHZ:
        case FLASH_FREQ_80MHZ:
            break;

        default: return false;
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

        default: return false;
    }

    return true;
}

void ESPCommon::load(const ESP8266RomHeader1 *header, offset_location offset)
{
    const ESPSegment* segment = Utils::relpointer<ESPSegment>(header, sizeof(ESP8266RomHeader1));

    for(size_t i = 0 ; segment && (i < header->segments); i++)
    {
        String segname;
        SegmentType segtype;

        if(segment->address == 0x40100000)
        {
            segname = ".user_rom";
            segtype = SegmentType::Code;
        }
        else if(segment->address == 0x3FFE8000)
        {
            segname = ".user_rom_data";
            segtype = SegmentType::Data;
        }
        else if(segment->address <= 0x3FFFFFFF)
        {
            segname = ".data_seg_" + String::number(i);
            segtype = SegmentType::Data;
        }
        else if(segment->address > 0x40100000)
        {
            segname = ".code_seg_" + String::number(i);
            segtype = SegmentType::Code;
        }
        else
        {
            segname = ".unknown_seg_" + String::number(i);
            segtype = SegmentType::Data;
        }

        if(offset.valid)
        {
            m_loader->document()->segment(segname, m_loader->fileoffset(segment) + sizeof(ESPSegment), segment->address, segment->size, segtype);
            offset.value += segment->size;
        }
        else
            m_loader->document()->segment(segname, m_loader->fileoffset(segment) + sizeof(ESPSegment), segment->address, segment->size, segtype);

        segment = Utils::relpointer<ESPSegment>(segment, sizeof(ESPSegment) + segment->size);
    }

    m_loader->document()->entry(header->entrypoint);
}

void ESPCommon::load(const ESP8266RomHeader2 *header)
{
    const ESP8266RomHeader1* header1 = Utils::relpointer<ESP8266RomHeader1>(header, sizeof(ESP8266RomHeader2) + header->size);

    if(!header1 || (header1->magic != ESP_IMAGE1_MAGIC))
        return;

    this->load(header1, REDasm::make_location(m_loader->fileoffset(header) + sizeof(ESP8266RomHeader2)));
}
