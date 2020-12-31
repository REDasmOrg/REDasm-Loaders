#include "vb_analyzer.h"
#include "../vb/vb_components.h"
#include "../pe.h"
#include <cstring>

#define HAS_OPTIONAL_INFO(objdescr, objinfo) (objdescr.lpObjectInfo + sizeof(VBObjectInfo) != objinfo->base.lpConstants)
#define VB_METHODNAME(pubobj, control, method) (pubobj + "_" + control + "_" + method)

VBAnalyzer::VBAnalyzer(RDContext* ctx, PELoader* peloader): m_peloader(peloader), m_context(ctx) { }

void VBAnalyzer::analyze()
{
    auto entry = RDContext_GetEntryPoint(m_context);
    if(!entry.valid) return;

    rd_ptr<RDILFunction> il(RDILFunction_Create(m_context, entry.address));
    if(!il) return;

    auto pushexpr = RDILFunction_GetExpression(il.get(), 0);
    auto callexpr = RDILFunction_GetExpression(il.get(), 1);
    if(!pushexpr || !callexpr) return;
    if(RDILExpression_Type(pushexpr) != RDIL_Push) return;
    if(RDILExpression_Type(callexpr) != RDIL_Call) return;

    auto* argexpr = RDILExpression_Extract(pushexpr, "u:cnst");
    if(!argexpr) return;

    RDILValue v;
    if(!RDILExpression_GetValue(argexpr, &v)) return;

    RDDocument* doc = RDContext_GetDocument(m_context);
    if(!RDDocument_GetSegmentAddress(doc, v.address, nullptr)) return;
    if(!this->decompile(v.address)) return;
}

void VBAnalyzer::disassembleTrampoline(rd_address eventva, const std::string& name)
{
    if(!eventva) return;

    if(!RDContext_CreateFunction(m_context, eventva, RD_Thunk(name.c_str()))) return;

    rd_ptr<RDILFunction> il(RDILFunction_Create(m_context, eventva));
    if(!il) return;

    auto* copyexpr = RDILFunction_GetFirstExpression(il.get());
    auto* gotoexpr = RDILFunction_GetLastExpression(il.get());

    if(RDILExpression_Type(copyexpr) != RDIL_Copy) return;
    if(!RDILExpression_Match(gotoexpr, "goto cnst")) return;

    auto* eventexpr = RDILExpression_Extract(gotoexpr, "u:cnst");
    if(!eventexpr) return;

    RDILValue val;
    if(!RDILExpression_GetValue(eventexpr, &val)) return;

    RDDocument* doc = RDContext_GetDocument(m_context);
    if(!RDDocument_GetSegmentAddress(doc, val.address, nullptr)) return;

    rd_statusaddress("Decoding" + name, val.address);
    RDContext_DisassembleFunction(m_context, val.address, name.c_str());
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
        const VBComponents::Component* component = VBComponents::get(m_context, reinterpret_cast<GUID*>(RD_AddrPointer(loader, ctrl.lpGuid)));
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
    RDLoader* loader = RDContext_GetLoader(m_context);

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
