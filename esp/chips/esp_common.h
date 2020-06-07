#pragma once

#include <rdapi/rdapi.h>
#include "../esp_header.h"
#include "../esp_constants.h"

enum ESPImageType {
    ESPImage_Unknown,
    ESPImage_8266,
};

class ESPCommon
{
    public:
        ESPCommon() = default;
        virtual bool load(RDLoader* loader, offset_t offset = RD_NPOS);

    public:
        static const char* test(const RDLoaderPlugin*, const RDLoaderRequest* request);

    protected:
        bool load(RDLoader* loader, ESP8266RomHeader1* header, offset_t offset = 0);
        bool load(RDLoader* loader, ESP8266RomHeader2* header);
};
