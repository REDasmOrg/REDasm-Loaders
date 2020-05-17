#include "pe_analyzer.h"
#include "pe_constants.h"
#include "pe_utils.h"
#include "pe.h"
#include <climits>

#define IMPORT_NAME(library, name)       PEUtils::importName(library, name)
#define IMPORT_TRAMPOLINE(library, name) RD_Trampoline(IMPORT_NAME(library, name).c_str())
#define ADD_WNDPROC_API(argidx, name)    m_wndprocapi.emplace_front(argidx, name)

PEAnalyzer::PEAnalyzer(PELoader* loader, RDDisassembler* disassembler): m_disassembler(disassembler), m_loader(loader)
{
    ADD_WNDPROC_API(4, "DialogBoxA");
    ADD_WNDPROC_API(4, "DialogBoxW");
    ADD_WNDPROC_API(4, "DialogBoxParamA");
    ADD_WNDPROC_API(4, "DialogBoxParamW");
    ADD_WNDPROC_API(4, "CreateDialogParamA");
    ADD_WNDPROC_API(4, "CreateDialogParamW");
    ADD_WNDPROC_API(4, "CreateDialogIndirectParamA");
    ADD_WNDPROC_API(4, "CreateDialogIndirectParamW");

    m_exitapi.push_front("ExitProcess");
    m_exitapi.push_front("TerminateProcess");
}

void PEAnalyzer::analyze()
{
    const PEClassifier* classifier = m_loader->classifier();

    if(!classifier->isClassified() || classifier->isVisualStudio())
        this->findCRTWinMain();

    this->findAllExitAPI();
    this->findAllWndProc();
}

/*
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
*/

bool PEAnalyzer::getImport(const std::string &library, const std::string &api, RDSymbol* symbol)
{
    RDDocument* doc = RDDisassembler_GetDocument(m_disassembler);

    if(RDDocument_GetSymbolByName(doc, IMPORT_TRAMPOLINE(library, api), symbol)) return true;
    if(RDDocument_GetSymbolByName(doc, IMPORT_NAME(library, api).c_str(), symbol)) return true;

    return false;
}

size_t PEAnalyzer::getAPIReferences(const std::string &library, const std::string &api, const address_t** references)
{
    RDSymbol symbol;

    if(!this->getImport(library, api, &symbol)) return 0;
    return RDDisassembler_GetReferences(m_disassembler, symbol.address, references);
}

void PEAnalyzer::findAllWndProc()
{
    for(auto it = m_wndprocapi.begin(); it != m_wndprocapi.end(); it++)
    {
        const address_t* references = nullptr;
        size_t c = this->getAPIReferences("user32.dll", it->second, &references);

        for(size_t i = 0; i < c; i++) this->findWndProc(references[i], it->first);
    }
}

void PEAnalyzer::findAllExitAPI()
{
    RDDocument* doc = RDDisassembler_GetDocument(m_disassembler);

    for(auto it = m_exitapi.begin(); it != m_exitapi.end(); it++)
    {
        const address_t* references = nullptr;
        size_t c = this->getAPIReferences("kernel32.dll", *it, &references);

        for(size_t i = 0; i < c; i++)
        {
            InstructionLock instruction(doc, references[i]);
            if(instruction) instruction->flags |= InstructionFlags_Stop;
        }
    }
}

void PEAnalyzer::findWndProc(address_t address, size_t argidx)
{
    const RDBlockContainer* c = RDDisassembler_GetBlocks(m_disassembler);

    RDBlock block;
    if(!RDBlockContainer_Find(c, address, &block)) return;

    size_t index = RDBlockContainer_Index(c, &block);
    if(!index || (index == RD_NPOS)) return;

    RDDocument* doc = RDDisassembler_GetDocument(m_disassembler);
    size_t arg = 0;

    while(RDBlockContainer_Get(c, --index, &block) && IS_TYPE(&block, BlockType_Code))
    {
        InstructionLock instruction(doc, block.address);
        if(!instruction) break;

        if(instruction->type == InstructionType_Push)
        {
            arg++;

            if(arg == argidx)
            {
                const RDOperand& op = instruction->operands[0];

                RDSegment segment;
                if(!RDDocument_GetSegmentAddress(doc, op.u_value, &segment) || !HAS_FLAG(&segment, SegmentFlags_Code)) continue;

                RDDocument_AddFunction(doc, op.address, (std::string("DlgProc_") + RD_ToHex(op.address)).c_str());
                RDDisassembler_Enqueue(m_disassembler, op.u_value);
            }
        }

        if((arg == argidx) || !index || HAS_FLAG(instruction, InstructionFlags_Stop)) break;
    }
}

void PEAnalyzer::findCRTWinMain()
{
    RDDocument* doc = RDDisassembler_GetDocument(m_disassembler);

    InstructionLock instruction(doc, RDDocument_EntryPoint(doc));
    if(!instruction || (instruction->type != InstructionType_Call)) return;

    RDSymbol symbol;
    if(!RDDocument_GetSymbolByName(doc, PE_SECURITY_COOKIE_SYMBOL, &symbol)) return;

    RDLocation target = RDDisassembler_GetTarget(m_disassembler, instruction->address);
    if(!target.valid) return;

    bool found = false;
    const address_t* references = nullptr;
    size_t c = RDDisassembler_GetReferences(m_disassembler, symbol.address, &references);

    for(size_t i = 0; i < c; i++)
    {
        address_t ref = references[i];
        RDLocation loc = RDDocument_FunctionStart(doc, ref);
        if(!loc.valid || ((target.address != loc.address))) continue;

        RDDocument_AddData(doc, loc.address, m_loader->classifier()->bits() / CHAR_BIT, PE_SECURITY_INIT_COOKIE_SYMBOL);
        found = true;
        break;
    }

    if(!found || !IS_TYPE(*instruction, InstructionType_Jump)) return;
    RDDocument_AddFunction(doc, target.address, PE_MAIN_CRT_STARTUP);
    RDDocument_SetEntry(doc, target.address);
}
