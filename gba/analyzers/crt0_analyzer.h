#pragma once

#include <rdapi/rdapi.h>

class Crt0Analyzer
{
    public:
        Crt0Analyzer(RDContext* ctx);
        void execute();

    private:
        RDContext* m_context;
};
