#pragma once

// https://web.archive.org/web/20170528084713/http://esp8266-re.foogod.com/wiki/SPI_Flash_Format

#include <memory>
#include <redasm/plugins/loader/loader.h>
#include <redasm/redasm.h>
#include "chips/esp_common.h"

using namespace REDasm;

class ESPLoader: public Loader
{
    private:
        typedef std::pair<offset_t, String> ImageItem;

    public:
        ESPLoader();
        AssemblerRequest assembler() const override;
        bool test(const LoadRequest &request) const override;
        void init(const LoadRequest &request) override;
        void load() override;

    private:
        std::unique_ptr<ESPCommon> m_esp;
};
