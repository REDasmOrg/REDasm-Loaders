#include "ulp_loader.h"
#include "ulp_header.h"

const char* ULPLoader::test(const RDLoaderRequest* request)
{
    const auto* ulpheader = reinterpret_cast<const ULPHeader*>(RDBuffer_Data(request->buffer));
    if(ulpheader->magic != ULP_MAGIC) return nullptr;

    size_t s = RDBuffer_Size(request->buffer);
    if(s != (ulpheader->textoffset + ulpheader->textsize + ulpheader->datasize)) return nullptr;

    return "esp32ulp";
}

bool ULPLoader::load(RDContext* ctx)
{
    const auto* ulpheader = reinterpret_cast<const ULPHeader*>(RDContext_GetBufferData(ctx));
    auto* doc = RDContext_GetDocument(ctx);

    if(ulpheader->datasize)
    {
        size_t dataoffset = ulpheader->textoffset + ulpheader->textsize;
        RDDocument_AddSegment(doc, "DATA", dataoffset, dataoffset, ulpheader->datasize, SegmentFlags_Data);
    }

    if(ulpheader->bsssize)
    {
        size_t bssoffset = ulpheader->textoffset + ulpheader->textsize + ulpheader->datasize;
        RDDocument_AddSegment(doc, "BSS", 0, bssoffset, ulpheader->bsssize, SegmentFlags_Bss);
    }

    if(ulpheader->textsize)
    {
        RDDocument_AddSegment(doc, "TEXT", ulpheader->textoffset, 0, ulpheader->textsize, SegmentFlags_CodeData);
        RDDocument_SetEntry(doc, 0);
    }

    return true;
}
