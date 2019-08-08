#pragma once

#include <redasm/redasm.h>
#include "../esp_header.h"
#include "../esp_constants.h"

using namespace REDasm;

class ESPLoader;

class ESPCommon
{
    public:
        ESPCommon(ESPLoader* loader);
        void load(offset_t offset = 0);

    public:
        static bool test(const LoadRequest& request);

    protected:
        void load(const ESP8266RomHeader1* header, offset_location offset = REDasm::invalid_location<offset_t>());
        void load(const ESP8266RomHeader2* header);

    protected:
        ESPLoader* m_loader;
};
