#include "esp_common.h"
#include "../esp.h"
#include <redasm/support/utils.h>

ESPCommon::ESPCommon(ESPLoader *loader): m_loader(loader) { }

bool ESPCommon::test(const LoadRequest &request)
{
    const ESPFirmwareHeader* h = request.pointer<ESPFirmwareHeader>();

    if(h->magic != ESP_IMAGE_MAGIC)
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

void ESPCommon::load()
{
    const ESPFirmwareHeader* header = m_loader->pointer<ESPFirmwareHeader>();
    const ESPFirmwareSegment* segment = Utils::relpointer<ESPFirmwareSegment>(header, sizeof(ESPFirmwareHeader));

    for(size_t i = 0 ; segment && (i < header->segments); i++)
    {
        m_loader->document()->segment("seg" + String::number(i),
                                      m_loader->fileoffset(segment) + sizeof(ESPFirmwareSegment),
                                      segment->address, segment->size, SegmentType::Code | SegmentType::Data);

        segment = Utils::relpointer<ESPFirmwareSegment>(segment, sizeof(ESPFirmwareSegment) + segment->size);
    }

    m_loader->document()->entry(header->entrypoint);
}
