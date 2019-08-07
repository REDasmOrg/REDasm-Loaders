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
        virtual void load();

    public:
        static bool test(const LoadRequest& request);

    protected:
        ESPLoader* m_loader;
};
