#pragma once

#include <unordered_map>
#include "esp_common.h"

class ESP8266: public ESPCommon
{
    public:
        ESP8266(ESPLoader* loader);
        void load(offset_t offset = 0) override;

    private:
        static void initImports();

    public:
        static bool test(const LoadRequest& request);

    private:
        static std::unordered_map<address_t, String> m_imports;
};
