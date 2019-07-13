#pragma once

#include <redasm/plugins/loader/analyzer.h>

using namespace REDasm;

class N64Analyzer : public Analyzer
{
    public:
        N64Analyzer();
        void analyze() override;
};
