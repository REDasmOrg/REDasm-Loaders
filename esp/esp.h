#pragma once

#include <memory>
#include <redasm/plugins/loader/loader.h>
#include <redasm/redasm.h>
#include "chips/esp_common.h"

using namespace REDasm;

class ESPLoader: public Loader
{
    public:
        ESPLoader();
        AssemblerRequest assembler() const override;
        bool test(const LoadRequest &request) const override;
        void init(const LoadRequest &request) override;
        void load() override;

    private:
        std::unique_ptr<ESPCommon> m_esp;
};
