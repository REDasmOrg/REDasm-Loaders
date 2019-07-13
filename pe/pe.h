#pragma once

#include <memory>
#include <redasm/plugins/loader/loader.h>
#include "pe_format.h"
#include "pe_utils.h"

class PELoader: public Loader
{
    public:
        PELoader();
        AssemblerRequest assembler() const override;
        bool test(const LoadRequest &request) const override;
        void init(const LoadRequest &request) override;
        void load() override;
        size_t bits() const;
        const ImageDosHeader *dosHeader() const;
        const ImageNtHeaders *ntHeaders() const;

    public:
        template<typename T> T* rvaPointer(u64 rva) const {
            offset_location offset = PEUtils::rvaToOffset(this->ntHeaders(), rva);
            return offset.valid ? this->pointer<T>(offset) : nullptr;
        }

    protected:
        Analyzer* createAnalyzer() const override;

    private:
        std::unique_ptr<PEFormat> m_peformat;
};
