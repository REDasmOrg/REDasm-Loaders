#include "vb_analyzer.h"
#include "vb_components.h"
#include "../pe.h"
#include <redasm/support/symbolize.h>

#define HAS_OPTIONAL_INFO(objdescr, objinfo) (objdescr.lpObjectInfo + sizeof(VBObjectInfo) != objinfo->base.lpConstants)
#define VB_METHODNAME(pubobj, control, method) (pubobj + "_" + control + "_" + method)

VBAnalyzer::VBAnalyzer(const PEClassifier *classifier): PEAnalyzer(classifier) { }

void VBAnalyzer::analyze()
{
    CachedInstruction instruction = r_doc->entryInstruction();
    if(!instruction->typeIs(InstructionType::Push) || (instruction->operandscount != 1)) return;
    if(!REDasm::typeIs(instruction->op(0), OperandType::Immediate)) return;

    address_t thunrtdata = instruction->op(0)->u_value;

    if(!r_doc->segment(thunrtdata) || !r_doc->next(instruction) || !instruction->isCall())
        return;

    instruction->type = InstructionType::Stop;
    if(!this->decompile(thunrtdata)) return;
    PEAnalyzer::analyze();
}

void VBAnalyzer::disassembleTrampoline(address_t eventva, const String& name)
{
    if(!eventva) return;
    CachedInstruction instruction = r_disasm->decodeInstruction(eventva); // Disassemble trampoline

    if(instruction->mnemonic() == "sub")
    {
        this->disassembleTrampoline(instruction->endAddress(), name); // Jump follows...
        return;
    }

    r_ctx->statusAddress("Decoding " + name, eventva);

    if(instruction->isBranch())
    {
        const Operand* op = instruction->target();
        if(!op) return;

        r_disasm->disassemble(op->u_value);
        r_doc->function(op->u_value, name);
    }
}

void VBAnalyzer::decompileObject(const VBPublicObjectDescriptor &pubobjdescr)
{
    if(!pubobjdescr.lpObjectInfo) return;

    VBObjectInfoOptional* objinfo = r_ldr->addrpointer<VBObjectInfoOptional>(pubobjdescr.lpObjectInfo);

    // if lpConstants points to the address after it,
    // there's no optional object information
    if(!HAS_OPTIONAL_INFO(pubobjdescr, objinfo) || !objinfo->lpControls)
        return;

    String pubobjname = r_ldr->addrpointer<const char>(pubobjdescr.lpszObjectName);
    VBControlInfo* ctrlinfo = r_ldr->addrpointer<VBControlInfo>(objinfo->lpControls);

    for(size_t i = 0; i < objinfo->dwControlCount; i++)
    {
        const VBControlInfo& ctrl = ctrlinfo[i];
        const VBComponents::Component* component = VBComponents::get(r_ldr->addrpointer<GUID>(ctrl.lpGuid));
        if(!component) continue;

        VBEventInfo* eventinfo = r_ldr->addrpointer<VBEventInfo>(ctrl.lpEventInfo);
        String componentname = r_ldr->addrpointer<const char>(ctrl.lpszName);
        u32* events = &eventinfo->lpEvents[0];

        for(size_t j = 0; j < component->events.size(); j++)
            this->disassembleTrampoline(events[j], VB_METHODNAME(pubobjname, componentname, component->events[j]));
    }
}

bool VBAnalyzer::decompile(address_t thunrtdata)
{
    m_vbheader = r_ldr->addrpointer<VBHeader>(thunrtdata);

    if(std::strncmp(m_vbheader->szVbMagic, "VB5!", VB_SIGNATURE_SIZE))
        return false;

    m_vbprojinfo = r_ldr->addrpointer<VBProjectInfo>(m_vbheader->lpProjectData);
    m_vbobjtable = r_ldr->addrpointer<VBObjectTable>(m_vbprojinfo->lpObjectTable);
    m_vbobjtreeinfo = r_ldr->addrpointer<VBObjectTreeInfo>(m_vbobjtable->lpObjectTreeInfo);
    m_vbpubobjdescr = r_ldr->addrpointer<VBPublicObjectDescriptor>(m_vbobjtable->lpPubObjArray);

    REDASM_SYMBOLIZE(VBHeader, thunrtdata);
    REDASM_SYMBOLIZE(VBProjectInfo, m_vbheader->lpProjectData);
    REDASM_SYMBOLIZE(VBObjectTable, m_vbprojinfo->lpObjectTable);
    REDASM_SYMBOLIZE(VBObjectTreeInfo, m_vbobjtable->lpObjectTreeInfo);
    REDASM_SYMBOLIZE(VBPublicObjectDescriptor, m_vbobjtable->lpPubObjArray);

    for(size_t i = 0; i < m_vbobjtable->wTotalObjects; i++)
        this->decompileObject(m_vbpubobjdescr[i]);

    return true;
}
