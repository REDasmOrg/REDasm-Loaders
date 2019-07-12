#include "pe_analyzer.h"
#include "pe_utils.h"
#include "pe_constants.h"
#include <redasm/disassembler/listing/listingdocumentiterator.h>
//#include "../../support/rtti/msvc/rtti_msvc.h"

#define IMPORT_NAME(library, name) PEUtils::importName(library, name)
#define IMPORT_TRAMPOLINE(library, name) ("_" + IMPORT_NAME(library, name))
#define ADD_WNDPROC_API(argidx, name) m_wndprocapi.emplace_front(argidx, name)

PEAnalyzer::PEAnalyzer(const PEClassifier *classifier, Disassembler *disassembler): Analyzer(disassembler), m_classifier(classifier)
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
    Symbol* symbol = this->disassembler()->document()->symbol(IMPORT_TRAMPOLINE(library, api));

    if(!symbol)
        symbol = this->disassembler()->document()->symbol(IMPORT_NAME(library, api));

    return symbol;
}

ReferenceVector PEAnalyzer::getAPIReferences(const String &library, const String &api)
{
    Symbol* symbol = this->getImport(library, api);

    if(!symbol)
        return ReferenceVector();

    return this->disassembler()->getReferences(symbol->address);
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
    ListingDocumentIterator it(this->document(), address, ListingItemType::InstructionItem);

    if(!it.hasPrevious())
        return;

    size_t arg = 0;
    const ListingItem* item = it.prev(); // Skip call

    while(arg < argidx)
    {
        CachedInstruction instruction = this->document()->instruction(item->address());

        if(!instruction)
            break;

        if(instruction->is(InstructionType::Push))
        {
            arg++;

            if(arg == argidx)
            {
                const Operand* op = instruction->op(0);
                Segment* segment = this->document()->segment(op->u_value);

                if(segment && segment->is(SegmentType::Code))
                {
                    this->document()->lockFunction(op->u_value, "DlgProc_" + String::hex(op->u_value));
                    this->disassembler()->disassemble(op->u_value);
                }
            }
        }

        if((arg == argidx) || !it.hasPrevious() || instruction->is(InstructionType::Stop))
            break;


        item = it.prev();
    }
}

void PEAnalyzer::findCRTWinMain()
{
    CachedInstruction instruction = this->document()->entryInstruction(); // Look for call

    if(!instruction || !instruction->is(InstructionType::Call))
        return;

    Symbol* symbol = this->document()->symbol(PE_SECURITY_COOKIE_SYMBOL);

    if(!symbol)
        return;

    auto target = this->disassembler()->getTarget(instruction->address);

    if(!target.valid)
        return;

    bool found = false;
    ReferenceVector refs = this->disassembler()->getReferences(symbol->address);

    for(address_t ref : refs)
    {
        const ListingItem* scfuncitem = this->document()->functionStart(ref);

        if(!scfuncitem || ((target != scfuncitem->address())))
            continue;

        this->document()->lock(scfuncitem->address(), PE_SECURITY_INIT_COOKIE_SYMBOL);
        found = true;
        break;
    }

    if(!found || !this->document()->advance(instruction) || !instruction->is(InstructionType::Jump))
        return;

    this->document()->lock(target, PE_MAIN_CRT_STARTUP, SymbolType::Function);
    this->document()->setDocumentEntry(target);
}
