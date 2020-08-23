#pragma once

#include <forward_list>
#include <rdapi/rdapi.h>

class PELoader;

class WndProcAnalyzer
{
    private:
        typedef std::pair<size_t, std::string> APIInfo;

    public:
        WndProcAnalyzer(RDDisassembler* disassembler, PELoader* peloader);
        void analyze();

    private:
        bool getImport(const std::string &library, const std::string &api, RDSymbol* symbol);
        size_t getAPIReferences(const std::string &library, const std::string &api, const rd_address** references);
        void findWndProc(rd_address refaddress, size_t argidx);

    private:
        RDDisassembler* m_disassembler;
        PELoader* m_peloader;

    private:
        std::forward_list<APIInfo> m_wndprocapi;
};
