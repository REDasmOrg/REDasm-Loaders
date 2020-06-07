#pragma once

#include <unordered_map>
#include "esp_common.h"

class ESP8266: public ESPCommon
{
    public:
        bool load(RDLoader* loader, offset_t offset = 0) override;
        static void initImports();

    private:
        static std::unordered_map<address_t, const char*> m_imports;
};
