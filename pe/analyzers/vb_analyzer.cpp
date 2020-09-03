#include "vb_analyzer.h"
#include "../vb/vb_components.h"
#include "../pe.h"
#include <cstring>

#define HAS_OPTIONAL_INFO(objdescr, objinfo) (objdescr.lpObjectInfo + sizeof(VBObjectInfo) != objinfo->base.lpConstants)
#define VB_METHODNAME(pubobj, control, method) (pubobj + "_" + control + "_" + method)

VBAnalyzer::VBAnalyzer(RDDisassembler* disassembler, PELoader* peloader): m_peloader(peloader), m_disassembler(disassembler) { }

void VBAnalyzer::analyze()
{
    RDDocument* doc = RDDisassembler_GetDocument(m_disassembler);
    auto entry = RDDocument_GetEntryPoint(doc);
    if(!entry.valid) return;

    rd_ptr<RDILFunction> il(RDILFunction_Generate(m_disassembler, entry.address));
    if(!il) return;

    auto pushexpr = RDILFunction_GetExpression(il.get(), 0);
    auto callexpr = RDILFunction_GetExpression(il.get(), 1);
    if(!pushexpr || !callexpr) return;
    if(RDILExpression_Type(pushexpr) != RDIL_Push) return;
    if(RDILExpression_Type(callexpr) != RDIL_Call) return;

    auto* argexpr = RDILExpression_GetU(pushexpr);
    if(RDILExpression_Type(argexpr) != RDIL_Cnst) return;

    RDILValue v;
    if(!RDILExpression_GetValue(argexpr, &v)) return;
    if(!RDDocument_GetSegmentAddress(doc, v.address, nullptr)) return;
    if(!this->decompile(v.address)) return;
}

void VBAnalyzer::disassembleTrampoline(rd_address eventva, const std::string& name)
{
    if(!eventva) return;
    RD_DisassembleAt(m_disassembler, eventva);

    rd_ptr<RDILFunction> il(RDILFunction_Generate(m_disassembler, eventva));
    if(!il) return;

    auto* copyexpr = RDILFunction_GetFirstExpression(il.get());
    auto* gotoexpr = RDILFunction_GetLastExpression(il.get());

    if(RDILExpression_Type(copyexpr) != RDIL_Copy) return;
    if(!RDILExpression_Match(gotoexpr, "goto c")) return;

    auto* eventexpr = RDILExpression_GetU(gotoexpr);
    RDILValue val;
    if(!RDILExpression_GetValue(eventexpr, &val)) return;

    RDDocument* doc = RDDisassembler_GetDocument(m_disassembler);
    if(!RDDocument_GetSegmentAddress(doc, val.address, nullptr)) return;

    rd_statusaddress("Decoding" + name, val.address);
    RDDisassembler_Enqueue(m_disassembler, val.address);
    RDDocument_AddFunction(doc, val.address, name.c_str());
    RDDocument_AddFunction(doc, eventva, RD_Thunk(name.c_str()));
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
