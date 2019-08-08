#include "esp.h"
#include "chips/esp8266.h"
#include <cassert>
#include <utility>
#include <deque>

#define IS_VALID_MAGIC(magic) ((magic == ESP_IMAGE1_MAGIC) || (magic == ESP_IMAGE2_MAGIC))

ESPLoader::ESPLoader(): Loader() { }
AssemblerRequest ESPLoader::assembler() const { return { "xtensa", "xtensale"}; }
bool ESPLoader::test(const LoadRequest &request) const { return ESP8266::test(request); }

void ESPLoader::init(const LoadRequest &request)
{
    if(ESP8266::test(request))
        m_esp = std::make_unique<ESP8266>(this);
    else
        r_ctx->log("Unknown ESP Chip");

    Loader::init(request);
}

void ESPLoader::load()
{
    if(!m_esp)
        return;

    std::deque<ImageItem> images;

    const u8* magic = this->pointer<u8>();

    if(IS_VALID_MAGIC(*magic))
        images.push_back({ 0, "BootLoader" });

    magic = this->pointer<u8>(ESP_IMAGE0_OFFSET);

    if(magic && IS_VALID_MAGIC(*magic))
        images.push_back({ ESP_IMAGE0_OFFSET, "Image 0" });

    offset_t image1offset = ESP_IMAGE1_OFFSET(this->buffer()->size());
    magic = this->pointer<u8>(image1offset);

    if(magic && IS_VALID_MAGIC(*magic))
        images.push_back({ image1offset, "Image 1" });

    List names;
    std::for_each(images.begin(), images.end(), [&names](const ImageItem& item) { names.append(item.second); });

    int idx = r_ui->select("Image selector", "Select an image", names);

    if(idx != -1)
        m_esp->load(images[idx].first);
}
