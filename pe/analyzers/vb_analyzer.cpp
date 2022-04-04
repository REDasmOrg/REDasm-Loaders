#include "vb_analyzer.h"
#include "../vb/vb_components.h"
#include "../pe.h"
#include <cstring>

#define HAS_OPTIONAL_INFO(objdescr, objinfo) (objdescr.lpObjectInfo + sizeof(VBObjectInfo) != objinfo->base.lpConstants)
#define VB_METHODNAME(pubobj, control, method) (pubobj + "_" + control + "_" + method)

VBAnalyzer::VBAnalyzer(RDContext* ctx, PELoader* peloader): m_peloader(peloader), m_context(ctx) { }

void VBAnalyzer::analyze()
{
    RDDocument* doc = RDContext_GetDocument(m_context);
    auto entry = RDDocument_GetEntry(doc);
    if(!entry.valid) return;

    rd_ptr<RDILFunction> il(RDILFunction_Create(m_context, entry.address));
    if(!il) return;

    auto pushexpr = RDILFunction_GetExpression(il.get(), 0);
    auto callexpr = RDILFunction_GetExpression(il.get(), 1);
    if(!pushexpr || !callexpr) return;
    if(RDILExpression_Type(pushexpr) != RDIL_Push) return;
    if(RDILExpression_Type(callexpr) != RDIL_Call) return;

    const RDILValue* values = nullptr;
    if(size_t n = RDILExpression_Extract(pushexpr, &values); n != 1) return;

    if(!RDDocument_AddressToSegment(doc, values[0].address, nullptr)) return;
    if(!this->decompile(values[0].address)) return;
}

void VBAnalyzer::disassembleTrampoline(rd_address eventva, const std::string& name)
{
    if(!eventva) return;

    RDDocument* doc = RDContext_GetDocument(m_context);
    if(!RDDocument_CreateFunction(doc, eventva, RD_Thunk(name.c_str()))) return;

    rd_ptr<RDILFunction> il(RDILFunction_Create(m_context, eventva));
    if(!il || (RDILFunction_Size(il.get()) < 2)) return;

    auto* copyexpr = RDILFunction_GetExpression(il.get(), 0);
    auto* gotoexpr = RDILFunction_GetExpression(il.get(), 1);

    if(RDILExpression_Type(copyexpr) != RDIL_Copy) return;
    if(!RDILExpression_Match(gotoexpr, "goto cnst")) return;

    const RDILValue* values = nullptr;
    if(size_t n = RDILExpression_Extract(gotoexpr, &values); n != 1) return;

    if(!RDDocument_AddressToSegment(doc, values[0].address, nullptr)) return;

    rd_statusaddress("Decoding" + name, values[0].address);
    RDDocument_CreateFunction(doc, values[0].address, name.c_str());
}

void VBAnalyzer::decompileObject(const VBPublicObjectDescriptor &pubobjdescr)
{
    if(!pubobjdescr.lpObjectInfo) return;

    VBObjectInfoOptional* objinfo = reinterpret_cast<VBObjectInfoOptional*>(RD_AddrPointer(m_context, pubobjdescr.lpObjectInfo));

    // if lpConstants points to the address after it,
    // there's no optional object information
    if(!HAS_OPTIONAL_INFO(pubobjdescr, objinfo) || !objinfo->lpControls)
        return;

    std::string pubobjname = reinterpret_cast<const char*>(RD_AddrPointer(m_context, pubobjdescr.lpszObjectName));
    VBControlInfo* ctrlinfo = reinterpret_cast<VBControlInfo*>(RD_AddrPointer(m_context, objinfo->lpControls));

    for(size_t i = 0; i < objinfo->dwControlCount; i++)
    {
        const VBControlInfo& ctrl = ctrlinfo[i];
        const VBComponents::Component* component = VBComponents::get(m_context, reinterpret_cast<GUID*>(RD_AddrPointer(m_context, ctrl.lpGuid)));
        if(!component) continue;

        VBEventInfo* eventinfo = reinterpret_cast<VBEventInfo*>(RD_AddrPointer(m_context, ctrl.lpEventInfo));
        std::string componentname = reinterpret_cast<const char*>(RD_AddrPointer(m_context, ctrl.lpszName));
        u32* events = &eventinfo->lpEvents[0];

        for(size_t j = 0; j < component->events.size(); j++)
            this->disassembleTrampoline(events[j], VB_METHODNAME(pubobjname, componentname, component->events[j]));
    }
}

bool VBAnalyzer::decompile(rd_address thunrtdata)
{
    m_vbheader = reinterpret_cast<VBHeader*>(RD_AddrPointer(m_context, thunrtdata));
    if(!m_vbheader) return false;

    if(std::strncmp(m_vbheader->szVbMagic, "VB5!", VB_SIGNATURE_SIZE)) return false;

    m_vbprojinfo = reinterpret_cast<VBProjectInfo*>(RD_AddrPointer(m_context, m_vbheader->lpProjectData));
    m_vbobjtable = reinterpret_cast<VBObjectTable*>(RD_AddrPointer(m_context, m_vbprojinfo->lpObjectTable));
    m_vbobjtreeinfo = reinterpret_cast<VBObjectTreeInfo*>(RD_AddrPointer(m_context, m_vbobjtable->lpObjectTreeInfo));
    m_vbpubobjdescr = reinterpret_cast<VBPublicObjectDescriptor*>(RD_AddrPointer(m_context, m_vbobjtable->lpPubObjArray));

    //REDASM_SYMBOLIZE(VBHeader, thunrtdata);
    //REDASM_SYMBOLIZE(VBProjectInfo, m_vbheader->lpProjectData);
    //REDASM_SYMBOLIZE(VBObjectTable, m_vbprojinfo->lpObjectTable);
    //REDASM_SYMBOLIZE(VBObjectTreeInfo, m_vbobjtable->lpObjectTreeInfo);
    //REDASM_SYMBOLIZE(VBPublicObjectDescriptor, m_vbobjtable->lpPubObjArray);

    for(size_t i = 0; i < m_vbobjtable->wTotalObjects; i++)
        this->decompileObject(m_vbpubobjdescr[i]);

    return true;
}
