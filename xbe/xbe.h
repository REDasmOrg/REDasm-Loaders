#pragma once

// Documentation: www.caustik.com/cxbx/download/xbe.htm

#include <redasm/plugins/loader/loader.h>
#include "xbe_header.h"

using namespace REDasm;

class XbeLoader : public Loader
{
    public:
        XbeLoader();
        AssemblerRequest assembler() const override;
        bool test(const LoadRequest &request) const override;
        void load() override;

    private:
        void displayXbeInfo(const XbeImageHeader *header);
        bool decodeEP(u32 encodedep, address_t &ep);
        bool decodeKernel(u32 encodedthunk, u32 &thunk);
        void loadSections(const XbeImageHeader *header, XbeSectionHeader* sectionhdr);
        bool loadXBoxKrnl(const XbeImageHeader* header);

    private:
        template<typename T> T* memoryoffset(const XbeImageHeader* header, u32 memaddress) const;
};

template<typename T> T* XbeLoader::memoryoffset(const XbeImageHeader* header, u32 memaddress) const { return this->pointer<T>(memaddress - header->BaseAddress); }
