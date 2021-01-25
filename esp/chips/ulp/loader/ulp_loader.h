#pragma once

#include <rdapi/rdapi.h>

class ULPLoader
{
    public:
        ULPLoader() = delete;
        static const char* test(const RDLoaderRequest* request);
        static bool load(RDContext* ctx);
};
