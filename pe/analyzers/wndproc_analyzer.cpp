#include "wndproc_analyzer.h"
#include "../pe_utils.h"
#include "../pe.h"
#include <deque>

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
    RDDocument* doc = RDContext_GetDocument(m_context);
    auto loc = RDDocument_GetFunctionStart(doc, refaddress);
    if(!loc.valid) return;

    rd_ptr<RDILFunction> il(RDILFunction_Create(m_context, loc.address));
    if(!il) return;

    size_t c = RDILFunction_Size(il.get());
    std::deque<const RDILExpression*> args;


    for(size_t i = 0; i < c; i++)
    {
        const RDILExpression* e = RDILFunction_GetExpression(il.get(), i);
        if(RDILExpression_Type(e) == RDIL_Push) args.push_front(RDILExpression_Extract(e, "u:*"));

        if(RDILExpression_Type(e) == RDIL_Call)
        {
            rd_address address;

            if(RDILFunction_GetAddress(il.get(), e, &address) && (address == refaddress) && (argidx < args.size()))
            {
                const RDILExpression* wndproc = args[argidx];

                if(RDILExpression_Type(wndproc) == RDIL_Cnst)
                {
                    RDILValue v;

                    if(RDILExpression_GetValue(wndproc, &v))
                    {
                        RDSegment segment;

                        if(RDDocument_AddressToSegment(doc, v.address, &segment) && HAS_FLAG(&segment, SegmentFlags_Code))
                            RDDocument_CreateFunction(doc, v.address, (std::string("DlgProc_") + RD_ToHexAuto(m_context, v.address)).c_str());
                    }
                }
            }

            args.clear();
        }
    }
}
