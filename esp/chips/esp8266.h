#pragma once

#include "esp_common.h"

class ESP8266: public ESPCommon
{
    public:
        ESP8266(ESPLoader* loader);

    public:
        static bool test(const LoadRequest& request);
};
