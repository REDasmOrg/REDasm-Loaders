#include "pe_analyzer.h"
#include "pe_utils.h"
#include "pe_constants.h"

#define IMPORT_NAME(library, name) PEUtils::importName(library, name)
#define IMPORT_TRAMPOLINE(library, name) ("_" + IMPORT_NAME(library, name))
#define ADD_WNDPROC_API(argidx, name) m_wndprocapi.emplace_front(argidx, name)

PEAnalyzer::PEAnalyzer(const PEClassifier *classifier): Analyzer(), m_classifier(classifier)
{
    ADD_WNDPROC_API(4, "DialogBoxA");
    ADD_WNDPROC_API(4, "DialogBoxW");
    ADD_WNDPROC_API(4, "DialogBoxParamA");
    ADD_WNDPROC_API(4, "DialogBoxParamW");
    ADD_WNDPROC_API(4, "CreateDialogParamA");
    ADD_WNDPROC_API(4, "CreateDialogParamW");
    ADD_WNDPROC_API(4, "CreateDialogIndirectParamA");
    ADD_WNDPROC_API(4, "CreateDialogIndirectParamW");
}

void PEAnalyzer::analyze()
{
    Analyzer::analyze();

    if(!m_classifier->isClassified() || m_classifier->isVisualStudio())
        this->findCRTWinMain();

    if(m_classifier->isVisualStudio())
    {
        this->findAllWndProc();
        r_ctx->log("Searching MSVC RTTI...");
        r_pm->execute("rttimsvc", { m_classifier->bits() });
        return;
    }

    this->findAllWndProc();
}

Symbol* PEAnalyzer::getImport(const String &library, const String &api)
{
    Symbol* symbol = r_disasm->document()->symbol(IMPORT_TRAMPOLINE(library, api));

    if(!symbol)
        symbol = r_disasm->document()->symbol(IMPORT_NAME(library, api));

    return symbol;
}

ReferenceVector PEAnalyzer::getAPIReferences(const String &library, const String &api)
{
    Symbol* symbol = this->getImport(library, api);

    if(!symbol)
        return ReferenceVector();

    return r_disasm->getReferences(symbol->address);
}

void PEAnalyzer::findAllWndProc()
{
    for(auto it = m_wndprocapi.begin(); it != m_wndprocapi.end(); it++)
    {
        ReferenceVector refs = this->getAPIReferences("user32.dll", it->second);

        for(address_t ref : refs)
            this->findWndProc(ref, it->first);
    }
}

void PEAnalyzer::findWndProc(address_t address, size_t argidx)
{
    size_t index = r_doc->findInstruction(address);

    if(!index || (index == REDasm::npos))
        return;

    size_t arg = 0;
    const ListingItem* item = r_doc->itemAt(--index); // Skip call

    while(item && (arg < argidx))
    {
        CachedInstruction instruction = r_doc->instruction(item->address());

        if(!instruction)
            break;

        if(instruction->is(InstructionType::Push))
        {
            arg++;

            if(arg == argidx)
            {
                const Operand* op = instruction->op(0);
                Segment* segment = r_doc->segment(op->u_value);

                if(segment && segment->is(SegmentType::Code))
                {
                    r_doc->lockFunction(op->u_value, "DlgProc_" + String::hex(op->u_value));
                    r_disasm->disassemble(op->u_value);
                }
            }
        }

        if((arg == argidx) || !index || instruction->is(InstructionType::Stop))
            break;


        item = r_doc->itemAt(--index);
    }
}

void PEAnalyzer::findCRTWinMain()
{
    CachedInstruction instruction = r_doc->entryInstruction(); // Look for call

    if(!instruction || !instruction->is(InstructionType::Call))
        return;

    Symbol* symbol = r_doc->symbol(PE_SECURITY_COOKIE_SYMBOL);

    if(!symbol)
        return;

    auto target = r_disasm->getTarget(instruction->address);

    if(!target.valid)
        return;

    bool found = false;
    ReferenceVector refs = r_disasm->getReferences(symbol->address);

    for(address_t ref : refs)
    {
        const ListingItem* scfuncitem = r_doc->functionStart(ref);

        if(!scfuncitem || ((target != scfuncitem->address())))
            continue;

        r_doc->lock(scfuncitem->address(), PE_SECURITY_INIT_COOKIE_SYMBOL);
        found = true;
        break;
    }

    if(!found || !r_doc->advance(instruction) || !instruction->is(InstructionType::Jump))
        return;

    r_doc->lock(target, PE_MAIN_CRT_STARTUP, SymbolType::Function);
    r_doc->setDocumentEntry(target);
}
