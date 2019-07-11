#include "vb_analyzer.h"
#include "vb_components.h"
#include "../pe.h"
#include <redasm/support/symbolize.h>

#define HAS_OPTIONAL_INFO(objdescr, objinfo) (objdescr.lpObjectInfo + sizeof(VBObjectInfo) != objinfo->base.lpConstants)
#define VB_METHODNAME(pubobj, control, method) (pubobj + "_" + control + "_" + method)

VBAnalyzer::VBAnalyzer(const PEClassifier *classifier, Disassembler *disassembler): PEAnalyzer(classifier, disassembler)
{
    m_loader = nullptr;
    m_vbheader = nullptr;
    m_vbprojinfo = nullptr;
    m_vbobjtable = nullptr;
    m_vbobjtreeinfo = nullptr;
    m_vbpubobjdescr = nullptr;
}

void VBAnalyzer::analyze()
{
    CachedInstruction instruction = this->document()->entryInstruction();

    if(!instruction->is(InstructionType::Push) || (instruction->operandsCount() != 1))
        return;

    if(!instruction->op(0)->is(OperandType::Immediate))
        return;

    address_t thunrtdata = instruction->op(0)->u_value;

    if(!this->document()->segment(thunrtdata) || !this->document()->advance(instruction) || !instruction->is(InstructionType::Call))
        return;

    instruction->type |= InstructionType::Stop;

    if(!this->decompile(thunrtdata))
        return;

    PEAnalyzer::analyze();
}

void VBAnalyzer::disassembleTrampoline(address_t eventva, const String& name)
{
    if(!eventva)
        return;

    CachedInstruction instruction = this->disassembler()->disassembleInstruction(eventva); // Disassemble trampoline

    if(instruction->mnemonic == "sub")
    {
        this->disassembleTrampoline(instruction->endAddress(), name); // Jump follows...
        return;
    }

    r_ctx->statusAddress("Decoding " + name, eventva);

    if(instruction->is(InstructionType::Branch))
    {
        const Operand* op = instruction->target();

        if(!op)
            return;

        this->disassembler()->disassemble(op->u_value);
        this->document()->lock(op->u_value, name, SymbolType::Function);
    }
}

void VBAnalyzer::decompileObject(const VBPublicObjectDescriptor &pubobjdescr)
{
    if(!pubobjdescr.lpObjectInfo)
        return;

    VBObjectInfoOptional* objinfo = m_loader->addrpointer<VBObjectInfoOptional>(pubobjdescr.lpObjectInfo);

    // if lpConstants points to the address after it,
    // there's no optional object information
    if(!HAS_OPTIONAL_INFO(pubobjdescr, objinfo) || !objinfo->lpControls)
        return;

    String pubobjname = m_loader->addrpointer<const char>(pubobjdescr.lpszObjectName);
    VBControlInfo* ctrlinfo = m_loader->addrpointer<VBControlInfo>(objinfo->lpControls);

    for(size_t i = 0; i < objinfo->dwControlCount; i++)
    {
        const VBControlInfo& ctrl = ctrlinfo[i];
        const VBComponents::Component* component = VBComponents::get(m_loader->addrpointer<GUID>(ctrl.lpGuid));

        if(!component)
            continue;

        VBEventInfo* eventinfo = m_loader->addrpointer<VBEventInfo>(ctrl.lpEventInfo);
        String componentname = m_loader->addrpointer<const char>(ctrl.lpszName);
        u32* events = &eventinfo->lpEvents[0];

        for(size_t j = 0; j < component->events.size(); j++)
            this->disassembleTrampoline(events[j], VB_METHODNAME(pubobjname, componentname, component->events[j]));
    }
}

bool VBAnalyzer::decompile(address_t thunrtdata)
{
    m_loader = this->disassembler()->loader();
    m_vbheader = m_loader->addrpointer<VBHeader>(thunrtdata);

    if(std::strncmp(m_vbheader->szVbMagic, "VB5!", VB_SIGNATURE_SIZE))
        return false;

    m_vbprojinfo = m_loader->addrpointer<VBProjectInfo>(m_vbheader->lpProjectData);
    m_vbobjtable = m_loader->addrpointer<VBObjectTable>(m_vbprojinfo->lpObjectTable);
    m_vbobjtreeinfo = m_loader->addrpointer<VBObjectTreeInfo>(m_vbobjtable->lpObjectTreeInfo);
    m_vbpubobjdescr = m_loader->addrpointer<VBPublicObjectDescriptor>(m_vbobjtable->lpPubObjArray);

    REDASM_SYMBOLIZE(VBHeader, this->disassembler(), thunrtdata);
    REDASM_SYMBOLIZE(VBProjectInfo, this->disassembler(), m_vbheader->lpProjectData);
    REDASM_SYMBOLIZE(VBObjectTable, this->disassembler(), m_vbprojinfo->lpObjectTable);
    REDASM_SYMBOLIZE(VBObjectTreeInfo, this->disassembler(), m_vbobjtable->lpObjectTreeInfo);
    REDASM_SYMBOLIZE(VBPublicObjectDescriptor, this->disassembler(), m_vbobjtable->lpPubObjArray);

    for(size_t i = 0; i < m_vbobjtable->wTotalObjects; i++)
        this->decompileObject(m_vbpubobjdescr[i]);

    return true;
}
