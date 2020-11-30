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
        const rd_address* references = nullptr;
        size_t c = this->getAPIReferences("user32.dll", it->second, &references);
        for(size_t i = 0; i < c; i++) this->findWndProc(references[i], it->first);
    }
}

bool WndProcAnalyzer::getImport(const std::string& library, const std::string& api, RDSymbol* symbol)
{
    RDDocument* doc = RDContext_GetDocument(m_context);

    if(RDDocument_GetSymbolByName(doc, IMPORT_THUNK(library, api), symbol)) return true;
    if(RDDocument_GetSymbolByName(doc, IMPORT_NAME(library, api).c_str(), symbol)) return true;

    return false;
}

size_t WndProcAnalyzer::getAPIReferences(const std::string& library, const std::string& api, const rd_address** references)
{
    RDSymbol symbol;
    if(!this->getImport(library, api, &symbol)) return 0;

    const RDNet* net = RDContext_GetNet(m_context);
    return RDNet_GetReferences(net, symbol.address, references);
}

void WndProcAnalyzer::findWndProc(rd_address refaddress, size_t argidx)
{
    auto loc = RDContext_GetFunctionStart(m_context, refaddress);
    if(!loc.valid) return;

    rd_ptr<RDILFunction> il(RDILFunction_Create(m_context, loc.address));
    if(!il) return;

    size_t c = RDILFunction_Size(il.get());
    std::deque<const RDILExpression*> args;

    RDDocument* doc = RDContext_GetDocument(m_context);

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

                        if(RDDocument_GetSegmentAddress(doc, v.address, &segment) && HAS_FLAG(&segment, SegmentFlags_Code))
                            RDContext_ScheduleFunction(m_context, v.address, (std::string("DlgProc_") + RD_ToHexAuto(v.address)).c_str());
                    }
                }
            }

            args.clear();
        }
    }
}
