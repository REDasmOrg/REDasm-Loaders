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
        void analyze();

    //private:
        //const Symbol* getImport(const std::string &library, const std::string &api);
        //SortedSet getAPIReferences(const std::string &library, const std::string &api);
        //void findWndProc(address_t address, size_t argidx);
        void findCRTWinMain();
        //void findAllWndProc();

    private:
        std::forward_list<APIInfo> m_wndprocapi;
        RDDisassembler* m_disassembler;
        PELoader* m_loader;
};
