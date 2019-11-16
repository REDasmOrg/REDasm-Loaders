#include "pe_analyzer.h"
#include "pe_utils.h"
#include "pe_constants.h"
#include <redasm/support/utils.h>

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

const Symbol* PEAnalyzer::getImport(const String &library, const String &api)
{
    const Symbol* symbol = r_doc->symbol(IMPORT_TRAMPOLINE(library, api));
    if(!symbol) symbol = r_doc->symbol(IMPORT_NAME(library, api));

    return symbol;
}

SortedSet PEAnalyzer::getAPIReferences(const String &library, const String &api)
{
    const Symbol* symbol = this->getImport(library, api);
    if(!symbol) return SortedSet();

    return r_disasm->getReferences(symbol->address);
}

void PEAnalyzer::findAllWndProc()
{
    for(auto it = m_wndprocapi.begin(); it != m_wndprocapi.end(); it++)
    {
        SortedSet refs = this->getAPIReferences("user32.dll", it->second);

        refs.each([&](const Variant& v) {
            this->findWndProc(v.toU64(), it->first);
        });
    }
}

void PEAnalyzer::findWndProc(address_t address, size_t argidx)
{
    size_t index = r_doc->itemInstructionIndex(address);
    if(!index || (index == REDasm::npos)) return;

    size_t arg = 0;
    ListingItem item = r_doc->itemAt(--index); // Skip call

    while(item.isValid() && (arg < argidx))
    {
        CachedInstruction instruction = r_doc->instruction(item.address);
        if(!instruction) break;

        if(instruction->typeIs(InstructionType::Push))
        {
            arg++;

            if(arg == argidx)
            {
                const Operand* op = instruction->op(0);
                const Segment* segment = r_doc->segment(op->u_value);

                if(segment && segment->is(SegmentType::Code))
                {
                    r_doc->function(op->u_value, "DlgProc_" + String::hex(op->u_value));
                    r_disasm->disassemble(op->u_value);
                }
            }
        }

        if((arg == argidx) || !index || instruction->isStop())
            break;

        item = r_doc->itemAt(--index);
    }
}

void PEAnalyzer::findCRTWinMain()
{
    CachedInstruction instruction = r_doc->entryInstruction(); // Look for call
    if(!instruction || !instruction->isCall()) return;

    const Symbol* symbol = r_doc->symbol(PE_SECURITY_COOKIE_SYMBOL);
    if(!symbol) return;

    auto target = r_disasm->getTarget(instruction->address);
    if(!target.valid) return;

    bool found = false;
    SortedSet refs = r_disasm->getReferences(symbol->address);

    for(size_t i = 0; i < refs.size(); i++)
    {
        address_t ref = refs[i].toU64();
        ListingItem scfuncitem = r_doc->functionStart(ref);

        if(!scfuncitem.isValid() || ((target != scfuncitem.address)))
            continue;

        r_doc->data(scfuncitem.address, (m_classifier->bits() / CHAR_BIT), PE_SECURITY_INIT_COOKIE_SYMBOL);
        found = true;
        break;
    }

    if(!found || !r_doc->next(instruction) || !instruction->isJump())
        return;

    r_doc->function(target, PE_MAIN_CRT_STARTUP);
    r_doc->setEntry(target);
}
