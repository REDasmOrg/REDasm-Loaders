#include "vb_analyzer.h"
#include "vb_components.h"
#include "../pe.h"
#include <cstring>

#define HAS_OPTIONAL_INFO(objdescr, objinfo) (objdescr.lpObjectInfo + sizeof(VBObjectInfo) != objinfo->base.lpConstants)
#define VB_METHODNAME(pubobj, control, method) (pubobj + "_" + control + "_" + method)

VBAnalyzer::VBAnalyzer(PELoader* loader, RDDisassembler* disassembler): PEAnalyzer(loader, disassembler) { }

void VBAnalyzer::analyze()
{
    RDDocument* doc = RDDisassembler_GetDocument(m_disassembler);
    RDLocation loc = RDDocument_EntryPoint(doc);
    if(!loc.valid) return;

    InstructionLock instruction(doc, loc.address);
    if(!instruction || !IS_TYPE(*instruction, InstructionType_Push) || (instruction->operandscount != 1)) return;
    if(!IS_TYPE(&instruction->operands[0], OperandType_Immediate)) return;

    rd_address thunrtdata = instruction->operands[0].address;
    if(!RDDocument_GetSegmentAddress(doc, thunrtdata, nullptr)) return;

    instruction.lock(RDInstruction_NextAddress(*instruction));
    if(!instruction || !IS_TYPE(*instruction, InstructionType_Call)) return;

    instruction->flags = InstructionFlags_Stop;
    if(!this->decompile(thunrtdata)) return;
    PEAnalyzer::analyze();
}

void VBAnalyzer::disassembleTrampoline(rd_address eventva, const std::string& name)
{
    if(!eventva) return;

    InstructionLock instruction(m_disassembler, eventva); // Disassemble trampoline
    if(!instruction) return;

    if(RDInstruction_MnemonicIs(*instruction, "sub"))
    {
        this->disassembleTrampoline(RDInstruction_NextAddress(*instruction), name); // Jump follows...
        return;
    }

    if(!IS_TYPE(*instruction, InstructionType_Jump)) return;
    if(!IS_TYPE(&instruction->operands[0], OperandType_Immediate)) return;

    rd_statusaddress("Decoding" + name, eventva);
    RDDisassembler_EnqueueAddress(m_disassembler, *instruction, instruction->operands[0].address);
    RDDocument_AddFunction(RDDisassembler_GetDocument(m_disassembler), instruction->operands[0].address, name.c_str());
}

void VBAnalyzer::decompileObject(RDLoader* loader, const VBPublicObjectDescriptor &pubobjdescr)
{
    if(!pubobjdescr.lpObjectInfo) return;

    VBObjectInfoOptional* objinfo = reinterpret_cast<VBObjectInfoOptional*>(RD_AddrPointer(loader, pubobjdescr.lpObjectInfo));

    // if lpConstants points to the address after it,
    // there's no optional object information
    if(!HAS_OPTIONAL_INFO(pubobjdescr, objinfo) || !objinfo->lpControls)
        return;

    std::string pubobjname = reinterpret_cast<const char*>(RD_AddrPointer(loader, pubobjdescr.lpszObjectName));
    VBControlInfo* ctrlinfo = reinterpret_cast<VBControlInfo*>(RD_AddrPointer(loader, objinfo->lpControls));

    for(size_t i = 0; i < objinfo->dwControlCount; i++)
    {
        const VBControlInfo& ctrl = ctrlinfo[i];
        const VBComponents::Component* component = VBComponents::get(reinterpret_cast<GUID*>(RD_AddrPointer(loader, ctrl.lpGuid)));
        if(!component) continue;

        VBEventInfo* eventinfo = reinterpret_cast<VBEventInfo*>(RD_AddrPointer(loader, ctrl.lpEventInfo));
        std::string componentname = reinterpret_cast<const char*>(RD_AddrPointer(loader, ctrl.lpszName));
        u32* events = &eventinfo->lpEvents[0];

        for(size_t j = 0; j < component->events.size(); j++)
            this->disassembleTrampoline(events[j], VB_METHODNAME(pubobjname, componentname, component->events[j]));
    }
}

bool VBAnalyzer::decompile(rd_address thunrtdata)
{
    RDLoader* loader = RDDisassembler_GetLoader(m_disassembler);

    m_vbheader = reinterpret_cast<VBHeader*>(RD_AddrPointer(loader, thunrtdata));
    if(!m_vbheader) return false;

    if(std::strncmp(m_vbheader->szVbMagic, "VB5!", VB_SIGNATURE_SIZE)) return false;

    m_vbprojinfo = reinterpret_cast<VBProjectInfo*>(RD_AddrPointer(loader, m_vbheader->lpProjectData));
    m_vbobjtable = reinterpret_cast<VBObjectTable*>(RD_AddrPointer(loader, m_vbprojinfo->lpObjectTable));
    m_vbobjtreeinfo = reinterpret_cast<VBObjectTreeInfo*>(RD_AddrPointer(loader, m_vbobjtable->lpObjectTreeInfo));
    m_vbpubobjdescr = reinterpret_cast<VBPublicObjectDescriptor*>(RD_AddrPointer(loader, m_vbobjtable->lpPubObjArray));

    //REDASM_SYMBOLIZE(VBHeader, thunrtdata);
    //REDASM_SYMBOLIZE(VBProjectInfo, m_vbheader->lpProjectData);
    //REDASM_SYMBOLIZE(VBObjectTable, m_vbprojinfo->lpObjectTable);
    //REDASM_SYMBOLIZE(VBObjectTreeInfo, m_vbobjtable->lpObjectTreeInfo);
    //REDASM_SYMBOLIZE(VBPublicObjectDescriptor, m_vbobjtable->lpPubObjArray);

    for(size_t i = 0; i < m_vbobjtable->wTotalObjects; i++)
        this->decompileObject(loader, m_vbpubobjdescr[i]);

    return true;
}
