#pragma once

#include <functional>
#include <unordered_map>
#include <rdapi/rdapi.h>

class ElfLoader;

class ElfAnalyzer
{
    protected:
        using PltCallback = std::function<void(rd_address)>;

    public:
        ElfAnalyzer(RDContext* ctx);
        virtual ~ElfAnalyzer() = default;
        virtual void analyze();

    protected:
        virtual void findMain(rd_address address) = 0;
        void walkPlt(PltCallback&& cb);

    protected:
        RDContext* m_context;
        RDDocument* m_document;
        ElfLoader* m_loader;
};
