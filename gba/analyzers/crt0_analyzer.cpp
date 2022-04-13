#include "crt0_analyzer.h"

Crt0Analyzer::Crt0Analyzer(RDContext* ctx): m_context(ctx) { }

void Crt0Analyzer::execute()
{
    auto* doc = RDContext_GetDocument(m_context);

    auto loc = RDDocument_GetEntry(doc);
    if(!loc.valid) return;

    rd_ptr<RDILFunction> il(RDILFunction_Create(m_context, loc.address));
    if(!il) return;

    bool found = RDILFunction_Match(il.get(), "reg = cnst;"    // 0
                                              "reg = reg;"     // 1
                                              "reg = [cnst];"  // 2
                                              "reg = cnst;"    // 3
                                              "reg = reg;"     // 4
                                              "reg = [cnst];"  // 5
                                              "reg = [cnst];"  // 6
                                              "reg = cnst;"    // 7
                                              "[reg] = reg;"   // 8
                                              "reg = [cnst];"  // 9
                                              "reg = reg;"     // 10
                                              "reg();"         // 11
                                              "goto cnst");    // 12

    if(!found)
    {
        rdcontext_addproblem(m_context, "CRT0 not found");
        return;
    }

    const RDILValue* values = nullptr;

    if(RDILExpression_Extract(RDILFunction_GetExpression(il.get(), 7), &values) == 2)
    {
        RDDocument_SetAddressAssembler(doc, values[1].address, "arm32le");
        RDDocument_SetFunction(doc, values[1].address, "intr_main");
        RDContext_Enqueue(m_context, values[1].address);
    }

    if(RDILExpression_Extract(RDILFunction_GetExpression(il.get(), 9), &values) == 2)
    {
        auto val = RDDocument_Dereference(doc, values[1].address);

        if(val.valid)
        {
            val.address &= ~1;
            RDDocument_SetAddressAssembler(doc, val.address, "thumble");
            RDDocument_SetFunction(doc, val.address, "AgbMain");
            RDContext_Enqueue(m_context, val.address);
        }
    }
}
