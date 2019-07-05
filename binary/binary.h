#pragma once

#include <redasm/redasm.h>
#include <redasm/plugins/loader/loader.h>

using namespace REDasm;

class BinaryLoader: public Loader
{
    public:
        BinaryLoader();
        int weight() const override;
        AssemblerRequest assembler() const override;
        LoaderFlags flags() const override;
        bool test(const LoadRequest &request) const override;
        void load() override;
        void build(const String &assembler, offset_t offset, address_t baseaddress, address_t entrypoint) override;

    private:
        String m_assembler;
};
