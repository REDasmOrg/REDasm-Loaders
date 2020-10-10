#pragma once

#include <unordered_map>
#include <rdapi/rdapi.h>

class ElfLoader;

class ElfAnalyzer
{
    public:
        ElfAnalyzer(RDContext* ctx);
        virtual ~ElfAnalyzer() = default;
        virtual void analyze();

    protected:
        virtual void findMain(rd_address address) = 0;

    protected:
        RDContext* m_context;
        ElfLoader* m_loader;
};
