#pragma once

#include <redasm/plugins/loader/loader.h>
#include "elf_format.h"

using namespace REDasm;

class ElfLoader: public Loader
{
    public:
        ElfLoader();
        size_t bits() const;
        endianness_t endianness() const;
        AssemblerRequest assembler() const override;
        bool test(const LoadRequest &request) const override;
        void init(const LoadRequest &request) override;
        void load() override;

    protected:
        Analyzer* createAnalyzer(Disassembler *disassembler) const override;

    private:
        std::unique_ptr<ElfFormat> m_elfformat;
};
