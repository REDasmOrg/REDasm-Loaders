#pragma once

#include <redasm/redasm.h>
#include <redasm/plugins/loader/loader.h>

using namespace REDasm;

class Chip8Loader: public Loader
{
    public:
        Chip8Loader();
        AssemblerRequest assembler() const override;
        bool test(const LoadRequest &request) const override;
        void load() override;
};
