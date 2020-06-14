#include "chips/esp_common.h"
#include "chips/esp8266.h"
#include <rdapi/rdapi.h>
#include <vector>
#include <deque>

// Based on: https://github.com/espressif/esptool/blob/master/esptool.py
// https://web.archive.org/web/20170528084713/http://esp8266-re.foogod.com/wiki/SPI_Flash_Format

#define IS_VALID_MAGIC(magic) ((magic == ESP_IMAGE1_MAGIC) || (magic == ESP_IMAGE2_MAGIC))

typedef std::pair<rd_offset, const char*> ImageItem;

template<typename ESPModel> bool load(RDLoaderPlugin*, RDLoader* loader)
{
    std::deque<ImageItem> images;

    const u8* magic = RDLoader_GetData(loader);
    if(IS_VALID_MAGIC(*magic)) images.push_back({ 0, "BootLoader" });

    magic = RD_Pointer(loader, ESP_IMAGE0_OFFSET);
    if(magic && IS_VALID_MAGIC(*magic)) images.push_back({ ESP_IMAGE0_OFFSET, "Image 0" });

    magic = RD_Pointer(loader, ESP_IMAGE1_OFFSET);
    if(magic && IS_VALID_MAGIC(*magic)) images.push_back({ ESP_IMAGE1_OFFSET, "Image 1" });

    std::vector<RDUIOptions> options;
    std::for_each(images.begin(), images.end(), [&options](const ImageItem& item) { options.push_back({ item.second, false }); });

    int idx = RDUI_GetItem("Image selector", "Select an image", options.data(), options.size());
    if(idx == -1) return false;

    ESPModel espmodel;
    return espmodel.load(loader, images[idx].first);
}

void redasm_entry()
{
    ESP8266::initImports();

    RD_PLUGIN_CREATE(RDLoaderPlugin, esp8266, "ESP8266 ROM");
    esp8266.test = &ESP8266::test;
    esp8266.load = &load<ESP8266>;

    RDLoader_Register(&esp8266);
}
