#pragma once

// Documentation: www.caustik.com/cxbx/download/xbe.htm

#include <rdapi/rdapi.h>
#include "xbe_header.h"

class XbeLoader
{
    public:
        XbeLoader();
        static const char* test(const RDLoaderRequest* request);
        static bool load(RDContext* ctx);

    private:
        static void displayXbeInfo(RDContext* ctx, const XbeImageHeader *header);
        static bool decodeEP(RDContext* ctx, u32 encodedep, rd_address &ep);
        static bool decodeKernel(RDContext* ctx, u32 encodedthunk, u32 &thunk);
        static void loadSections(RDContext* ctx, const XbeImageHeader *header, XbeSectionHeader* sectionhdr);
        static bool loadXBoxKrnl(RDContext* ctx, const XbeImageHeader* header);

    private:
        template<typename T> static T* memoryoffset(RDContext* ctx, const XbeImageHeader* header, u32 memaddress);
};

template<typename T>
T* XbeLoader::memoryoffset(RDContext* ctx, const XbeImageHeader* header, u32 memaddress) {
    return reinterpret_cast<T*>(RD_Pointer(ctx, memaddress - header->BaseAddress));
}
