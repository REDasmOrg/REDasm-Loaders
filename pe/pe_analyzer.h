#pragma once

#include "classifier.h"
#include <forward_list>
#include <rdapi/rdapi.h>

class PELoader;

class PEAnalyzer
{
    private:
        typedef std::pair<size_t, std::string> APIInfo;

    public:
        PEAnalyzer(PELoader* loader, RDDisassembler* disassembler);
        virtual void analyze();

    private:
        bool getImport(const std::string &library, const std::string &api, RDSymbol* symbol);
        size_t getAPIReferences(const std::string &library, const std::string &api, const rd_address** references);
        void findWndProc(rd_address address, size_t argidx);
        void findCRTWinMain();
        void findAllWndProc();
        void findAllExitAPI();

    protected:
        RDDisassembler* m_disassembler;
        PELoader* m_loader;

    private:
        std::forward_list<APIInfo> m_wndprocapi;
        std::forward_list<std::string> m_exitapi;
};
