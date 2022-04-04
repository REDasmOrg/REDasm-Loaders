#include "wndproc_analyzer.h"
#include "../pe_utils.h"
#include "../pe.h"

#define IMPORT_NAME(library, name)    PEUtils::importName(library, name)
#define IMPORT_THUNK(library, name)   RD_Thunk(IMPORT_NAME(library, name).c_str())
#define ADD_WNDPROC_API(argidx, name) m_wndprocapi.emplace_front(argidx, name)

WndProcAnalyzer::WndProcAnalyzer(RDContext* ctx, PELoader* peloader): m_context(ctx), m_peloader(peloader)
{
    ADD_WNDPROC_API(3, "DialogBoxA");
    ADD_WNDPROC_API(3, "DialogBoxW");
    ADD_WNDPROC_API(3, "DialogBoxParamA");
    ADD_WNDPROC_API(3, "DialogBoxParamW");
    ADD_WNDPROC_API(3, "DialogBoxIndirectA");
    ADD_WNDPROC_API(3, "DialogBoxIndirectW");
    ADD_WNDPROC_API(3, "DialogBoxIndirectParamA");
    ADD_WNDPROC_API(3, "DialogBoxIndirectParamW");
    ADD_WNDPROC_API(3, "CreateDialogA");
    ADD_WNDPROC_API(3, "CreateDialogW");
    ADD_WNDPROC_API(3, "CreateDialogParamA");
    ADD_WNDPROC_API(3, "CreateDialogParamW");
    ADD_WNDPROC_API(3, "CreateDialogIndirectParamA");
    ADD_WNDPROC_API(3, "CreateDialogIndirectParamW");
}

void WndProcAnalyzer::analyze()
{
    for(auto it = m_wndprocapi.begin(); it != m_wndprocapi.end(); it++)
    {
        const RDReference* refs = nullptr;
        size_t c = this->getAPIReferences("user32.dll", it->second, &refs);
        for(size_t i = 0; i < c; i++) this->findWndProc(refs[i].address, it->first);
    }
}

rd_address WndProcAnalyzer::getImport(const std::string& library, const std::string& api)
{
    RDDocument* doc = RDContext_GetDocument(m_context);
    rd_address address = RDDocument_GetAddress(doc, IMPORT_THUNK(library, api));
    if(address == RD_NVAL) address = RDDocument_GetAddress(doc, IMPORT_NAME(library, api).c_str());
    return address;
}

size_t WndProcAnalyzer::getAPIReferences(const std::string& library, const std::string& api, const RDReference** refs)
{
    rd_address address = this->getImport(library, api);
    if(address == RD_NVAL) return 0;

    const RDNet* net = RDContext_GetNet(m_context);
    return RDNet_GetReferences(net, address, refs);
}

void WndProcAnalyzer::findWndProc(rd_address refaddress, size_t argidx)
{
    rd_ptr<RDILExpression> e(RDILExpression_Create(m_context, refaddress));
    if(!e || RDILExpression_Type(e.get()) != RDIL_Call) return;

    const RDNet* net = RDContext_GetNet(m_context);
    std::vector<RDILValue> args;

    const auto* node = RDNet_FindNode(net, refaddress);
    if(!node) return;

    RDDocument* doc = RDContext_GetDocument(m_context);

    for(node = RDNet_GetPrevNode(net, node); node; node = RDNet_GetPrevNode(net, node))
    {
        e.reset(RDILExpression_Create(m_context, RDNetNode_GetAddress(node)));
        if(!e) return;
        if(RDILExpression_Type(e.get()) != RDIL_Push) continue;

        const RDILValue* values = nullptr;
        if(size_t n = RDILExpression_Extract(e.get(), &values); n >= 1) args.push_back(values[0]);
        else return;

        if(args.size() < argidx + 1) continue;
        else if(args.size() > argidx + 1) break;

        RDSegment segment;
        const RDILValue& wndproc = args.back();

        if(RDDocument_AddressToSegment(doc, wndproc.address, &segment) && HAS_FLAG(&segment, SegmentFlags_Code))
            RDDocument_CreateFunction(doc, wndproc.address, ("DlgProc_" + rd_tohexauto(m_context, wndproc.address)).c_str());

        break;
    }
}
