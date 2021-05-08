#include "dalvik.h"
#include "../loader/dex.h"
#include "../common_types.h"
#include "../demangler.h"
#include <libdex/DexProto.h>
#include <climits>

std::unordered_map<u16, std::string> DalvikAssembler::m_methodcache, DalvikAssembler::m_fieldcache;
std::unordered_map<rd_address, std::string> DalvikAssembler::m_functioncache;

void DalvikAssembler::emulate(RDContext* ctx, RDEmulateResult* result)
{
    const auto* view = RDEmulateResult_GetView(result);
    const auto* pcode = reinterpret_cast<const dex_u2*>(view->data);

    DecodedInstruction di;
    if(!dexDecodeInstruction(pcode, &di)) return;

    rd_address address = RDEmulateResult_GetAddress(result);
    size_t size = dexGetWidthFromInstruction(pcode) * sizeof(dex_u2);
    RDEmulateResult_SetSize(result, size);

    if(address == 0x000723F6)
    {
        auto v = dexGetFormatFromOpcode(di.opcode);
        v = { };
    }

    switch(di.opcode)
    {
        case OP_FILL_ARRAY_DATA: {
            rd_address arraydata = address + (di.vB * sizeof(dex_u2));
            auto* parraydata = reinterpret_cast<DalvikFillArrayDataPayload*>(RD_FilePointer(ctx, arraydata));

            if(parraydata && (parraydata->ident == DALVIK_FILLARRAYDATA_IDENT)) {
                RDEmulateResult_AddTypeName(result, arraydata, DALVIK_FILLARRAYDATA_PATH);
                rd_ptr<RDType> t(RDType_CreateArray(parraydata->size, RDType_CreateInt(parraydata->element_width, false)));
                RDEmulateResult_AddType(result, arraydata + (sizeof(u32) * 2), t.get());
            }

            break;
        }

        case OP_SPARSE_SWITCH: {
            rd_address sparsedata = address + (di.vB * sizeof(dex_u2));
            auto* psparsedata = reinterpret_cast<DalvikSparseSwitchPayload*>(RD_FilePointer(ctx, sparsedata));

            rd_log("Sparse switch @ " + rd_tohex(address));

            if(psparsedata && (psparsedata->ident == DALVIK_SPARSESWITCH_IDENT)) {

            }
            break;
        }

        case OP_PACKED_SWITCH: {
            rd_address packeddata = address + (di.vB * sizeof(dex_u2));
            auto* ppackeddata = reinterpret_cast<DalvikPackedSwitchPayload*>(RD_FilePointer(ctx, packeddata));

            if(ppackeddata && (ppackeddata->ident == DALVIK_PACKEDSWITCH_IDENT)) {
                RDEmulateResult_AddTypeName(result, packeddata, DALVIK_PACKEDSWITCH_PATH);

                rd_ptr<RDType> t(RDType_CreateArray(ppackeddata->size, RDType_CreateInt(sizeof(u32), true)));
                RDType_SetName(t.get(), "targets");
                RDEmulateResult_AddType(result, packeddata + (sizeof(u32) * 2), t.get());

                s32* targets = &ppackeddata->targets[0];

                for(size_t i = 0; i < ppackeddata->size; i++, targets++)
                    RDEmulateResult_AddBranchTrue(result, address + (targets[i] * sizeof(u16)));
            }

            RDEmulateResult_AddBranchFalse(result, address + size);
            break;
        }

        case OP_RETURN_VOID:
        case OP_RETURN:
        case OP_RETURN_WIDE:
        case OP_RETURN_OBJECT:
            RDEmulateResult_AddReturn(result);
            break;

        case OP_IF_LEZ:
        case OP_IF_EQZ:
            RDEmulateResult_AddBranchTrue(result, address + (static_cast<dex_s2>(di.vB) * sizeof(dex_u2)));
            RDEmulateResult_AddBranchFalse(result, address + size);
            break;

        case OP_IF_LE:
        case OP_IF_LT:
            RDEmulateResult_AddBranchTrue(result, address + (static_cast<dex_s2>(di.vC) * sizeof(dex_u2)));
            RDEmulateResult_AddBranchFalse(result, address + size);
            break;

        case OP_GOTO:
            RDEmulateResult_AddBranch(result, address + (static_cast<dex_s2>(di.vA) * sizeof(dex_u2)));
            break;

        default: break;
    }

    if(di.indexType != kIndexNone)
    {
        DalvikIndex didx;
        DalvikAssembler::getIndex(di, didx);
        RDLocation loc;

        auto* l = reinterpret_cast<const DexLoader*>(RDContext_GetUserData(ctx, DEXLOADER_USERDATA));

        switch(di.indexType)
        {
            case kIndexStringRef:
                loc = RD_FileOffset(ctx, dexStringById(l->dexFile(), didx.index));
                if(loc.valid) RDEmulateResult_AddString(result, loc.address);
                break;

            default:
                break;
        }
    }
}

void DalvikAssembler::renderInstruction(RDContext* ctx, const RDRendererParams* rp)
{
    DecodedInstruction di;
    if(!dexDecodeInstruction(reinterpret_cast<const dex_u2*>(rp->view.data), &di)) return;

    RDRenderer_MnemonicWord(rp->renderer, dexGetOpcodeName(di.opcode), DalvikAssembler::getTheme(di));

    switch(dexGetFormatFromOpcode(di.opcode))
    {
        case kFmt10t:
        case kFmt20t:
            RDRenderer_Reference(rp->renderer, rp->address + (static_cast<dex_s4>(di.vA) * sizeof(dex_u2)));
            break;

        case kFmt12x:
            RDRenderer_Register(rp->renderer, DalvikAssembler::reg(di.vA).c_str());
            RDRenderer_Text(rp->renderer, ", ");
            RDRenderer_Register(rp->renderer, DalvikAssembler::reg(di.vB).c_str());
            break;

        case kFmt11n:
            RDRenderer_Register(rp->renderer, DalvikAssembler::reg(di.vB).c_str());
            RDRenderer_Text(rp->renderer, ", ");
            RDRenderer_Constant(rp->renderer, RD_ToHex(static_cast<dex_u1>(di.vB)));
            break;

        case kFmt11x:
            RDRenderer_Register(rp->renderer, DalvikAssembler::reg(di.vA).c_str());
            break;

        case kFmt21c:
        case kFmt31c:
            RDRenderer_Register(rp->renderer, DalvikAssembler::reg(di.vA).c_str());
            RDRenderer_Text(rp->renderer, ", ");
            DalvikAssembler::renderIndex(ctx, di, rp->renderer);
            break;

        case kFmt21h: {
            if (di.opcode == OP_CONST_HIGH16) {
                RDRenderer_Register(rp->renderer, DalvikAssembler::reg(di.vA).c_str());
                RDRenderer_Text(rp->renderer, ", ");
                RDRenderer_Constant(rp->renderer, RD_ToHex(static_cast<dex_s4>(di.vB) << 16));
            }
            else {
                RDRenderer_Register(rp->renderer, DalvikAssembler::reg(di.vA).c_str());
                RDRenderer_Text(rp->renderer, ", ");
                RDRenderer_Constant(rp->renderer, RD_ToHex(static_cast<dex_s8>(di.vB) << 48));
            }
            break;
        }

        case kFmt21s:
            RDRenderer_Register(rp->renderer, DalvikAssembler::reg(di.vA).c_str());
            RDRenderer_Text(rp->renderer, ", ");
            RDRenderer_Constant(rp->renderer, RD_ToHex(static_cast<dex_s4>(di.vB)));
            break;

        case kFmt21t:
            RDRenderer_Register(rp->renderer, DalvikAssembler::reg(di.vA).c_str());
            RDRenderer_Text(rp->renderer, ", ");
            RDRenderer_Reference(rp->renderer, rp->address + (static_cast<dex_s4>(di.vB) * sizeof(dex_u2)));
            break;

        case kFmt22b:
            RDRenderer_Register(rp->renderer, DalvikAssembler::reg(di.vA).c_str());
            RDRenderer_Text(rp->renderer, ", ");
            RDRenderer_Register(rp->renderer, DalvikAssembler::reg(static_cast<dex_s4>(di.vB)).c_str());
            RDRenderer_Text(rp->renderer, ", ");
            RDRenderer_Constant(rp->renderer, RD_ToHex(static_cast<dex_u1>(di.vC)));
            break;

        case kFmt22c:
        case kFmt22cs:
            RDRenderer_Register(rp->renderer, DalvikAssembler::reg(di.vA).c_str());
            RDRenderer_Text(rp->renderer, ", ");
            RDRenderer_Register(rp->renderer, DalvikAssembler::reg(di.vB).c_str());
            RDRenderer_Text(rp->renderer, ", ");
            DalvikAssembler::renderIndex(ctx, di, rp->renderer);
            break;

        case kFmt22t:
            RDRenderer_Register(rp->renderer, DalvikAssembler::reg(di.vA).c_str());
            RDRenderer_Text(rp->renderer, ", ");
            RDRenderer_Register(rp->renderer, DalvikAssembler::reg(di.vB).c_str());
            RDRenderer_Text(rp->renderer, ", ");
            RDRenderer_Reference(rp->renderer, rp->address + (static_cast<dex_s4>(di.vC) * sizeof(dex_u2)));
            break;

        case kFmt23x:
            RDRenderer_Register(rp->renderer, DalvikAssembler::reg(di.vA).c_str());
            RDRenderer_Text(rp->renderer, ", ");
            RDRenderer_Register(rp->renderer, DalvikAssembler::reg(di.vB).c_str());
            RDRenderer_Text(rp->renderer, ", ");
            RDRenderer_Register(rp->renderer, DalvikAssembler::reg(di.vC).c_str());
            break;

        case kFmt31i:
            RDRenderer_Register(rp->renderer, DalvikAssembler::reg(di.vA).c_str());
            RDRenderer_Text(rp->renderer, ", ");
            RDRenderer_Constant(rp->renderer, RD_ToHex(di.vB));
            break;

        case kFmt31t:
            RDRenderer_Register(rp->renderer, DalvikAssembler::reg(di.vA).c_str());
            RDRenderer_Text(rp->renderer, ", ");
            RDRenderer_Reference(rp->renderer, rp->address + (di.vB * sizeof(dex_u2)));
            break;

        case kFmt32x:
            RDRenderer_Register(rp->renderer, DalvikAssembler::reg(di.vA).c_str());
            RDRenderer_Text(rp->renderer, ", ");
            RDRenderer_Register(rp->renderer, DalvikAssembler::reg(di.vB).c_str());
            break;

        case kFmt35c:
        case kFmt35ms:
        case kFmt35mi: {
            RDRenderer_Text(rp->renderer, "{");

            for(size_t i = 0; i < di.vA; i++) {
                if(i) RDRenderer_Text(rp->renderer, ", ");
                RDRenderer_Register(rp->renderer, DalvikAssembler::reg(di.arg[i]).c_str());
            }

            RDRenderer_Text(rp->renderer, "}");
            RDRenderer_Text(rp->renderer, ", ");
            DalvikAssembler::renderIndex(ctx, di, rp->renderer);
        }

        case kFmt10x: break;
        default: RDRenderer_Unknown(rp->renderer); break;
    }
}

void DalvikAssembler::renderFunction(RDContext* ctx, const RDRendererParams* rp)
{
    auto it = m_functioncache.find(rp->address);
    std::string fn;

    if(it == m_functioncache.end())
    {
        auto* l = reinterpret_cast<const DexLoader*>(RDContext_GetUserData(ctx, DEXLOADER_USERDATA));
        auto methodid = l->addressToMethodId(rp->address);
        if(!methodid) return;

        auto ofn = DalvikAssembler::getFunctionName(ctx, methodid);
        if(!ofn) return;

        m_functioncache[rp->address] = *ofn;
        fn = *ofn;
    }
    else
        fn = it->second;

    RDRenderer_Themed(rp->renderer, fn.c_str(), Theme_Function);
}

std::string DalvikAssembler::reg(dex_u4 id) { return "v" + std::to_string(id); }

void DalvikAssembler::getIndex(const DecodedInstruction& di, DalvikIndex& res)
{
    switch(dexGetFormatFromOpcode(di.opcode))
    {
        case kFmt20bc:
        case kFmt21c:
        case kFmt35c:
        case kFmt35ms:
        case kFmt3rc:
        case kFmt3rms:
        case kFmt35mi:
        case kFmt3rmi: res.index = di.vB; res.width = 4; break;

        case kFmt31c: res.index = di.vB; res.width = 8; break;

        case kFmt22c:
        case kFmt22cs: res.index = di.vC; res.width = 4; break;

        case kFmt45cc:
        case kFmt4rcc:
            res.index = di.vB;        // method index
            res.secindex = di.arg[4]; // proto index
            res.width = 4;
            break;

        default: res.index = 0; res.width = 4; break;
    }
}

void DalvikAssembler::renderIndex(RDContext* ctx, const DecodedInstruction& di, RDRenderer* r)
{
    DalvikIndex didx;
    DalvikAssembler::getIndex(di, didx);

    auto* l = reinterpret_cast<const DexLoader*>(RDContext_GetUserData(ctx, DEXLOADER_USERDATA));
    RDLocation loc;

    switch(di.indexType)
    {
        case kIndexMethodRef: {
            auto mi = DalvikAssembler::getMethodInfo(ctx, didx.index);
            if(mi) RDRenderer_Themed(r, mi->c_str(), Theme_Function);
            else RDRenderer_Unknown(r);
            break;
        }

        case kIndexFieldRef: {
            auto fi = DalvikAssembler::getFieldInfo(ctx, didx.index);
            if(fi) RDRenderer_Themed(r, fi->c_str(), Theme_Data);
            else RDRenderer_Unknown(r);
            break;
        }

        case kIndexStringRef: {
            if (didx.index < l->dexFile()->pHeader->stringIdsSize) {
                loc = RD_FileOffset(ctx, dexStringById(l->dexFile(), didx.index));
                if(loc.valid) RDRenderer_Reference(r, loc.address);
                else RDRenderer_Unknown(r);
            }
            else RDRenderer_Unknown(r);
            break;
        }

        case kIndexTypeRef: {
            if(didx.index < l->dexFile()->pHeader->typeIdsSize)
                RDRenderer_Themed(r, DexLoader::demangle(dexStringByTypeIdx(l->dexFile(), didx.index)).c_str(), Theme_Type);
            else
                RDRenderer_Unknown(r);
            break;
        }

        default:
            RDRenderer_Unknown(r);
            break;
    }
}

rd_type DalvikAssembler::getTheme(const DecodedInstruction& di)
{
    switch(di.opcode)
    {
        case OP_RETURN:
        case OP_RETURN_OBJECT:
        case OP_RETURN_VOID:
        case OP_RETURN_WIDE:
            return Theme_Ret;

        case OP_INVOKE_DIRECT:
        case OP_INVOKE_DIRECT_RANGE:
        case OP_INVOKE_INTERFACE:
        case OP_INVOKE_INTERFACE_RANGE:
        case OP_INVOKE_STATIC:
        case OP_INVOKE_STATIC_RANGE:
        case OP_INVOKE_SUPER:
        case OP_INVOKE_SUPER_RANGE:
        case OP_INVOKE_VIRTUAL:
        case OP_INVOKE_VIRTUAL_RANGE:
            return Theme_Call;

        case OP_IF_EQ:
        case OP_IF_EQZ:
        case OP_IF_GE:
        case OP_IF_GEZ:
        case OP_IF_GT:
        case OP_IF_GTZ:
        case OP_IF_LE:
        case OP_IF_LEZ:
        case OP_IF_LT:
        case OP_IF_LTZ:
        case OP_IF_NE:
        case OP_IF_NEZ:
        case OP_SPARSE_SWITCH:
            return Theme_JumpCond;

        case OP_GOTO:
        case OP_GOTO_16:
        case OP_GOTO_32:
        case OP_PACKED_SWITCH:
            return Theme_Jump;

        case OP_NOP: return Theme_Nop;
        default: break;
    }

    return Theme_Default;
}

bool DalvikAssembler::isInvoke(const DecodedInstruction& di)
{
    switch(di.opcode)
    {
        case OP_INVOKE_DIRECT:
        case OP_INVOKE_DIRECT_RANGE:
        case OP_INVOKE_INTERFACE:
        case OP_INVOKE_INTERFACE_RANGE:
        case OP_INVOKE_STATIC:
        case OP_INVOKE_STATIC_RANGE:
        case OP_INVOKE_SUPER:
        case OP_INVOKE_SUPER_RANGE:
        case OP_INVOKE_VIRTUAL:
        case OP_INVOKE_VIRTUAL_RANGE:
            return true;

        default: break;
    }

    return false;
}

std::optional<std::string> DalvikAssembler::getMethodInfo(RDContext* ctx, dex_u4 methodidx)
{
    auto* l = reinterpret_cast<const DexLoader*>(RDContext_GetUserData(ctx, DEXLOADER_USERDATA));
    if(methodidx >= l->dexFile()->pHeader->methodIdsSize) return std::nullopt;

    auto it = m_methodcache.find(methodidx);
    if(it != m_methodcache.end()) return it->second;

    const auto* methodid = dexGetMethodId(l->dexFile(), methodidx);
    if(!methodid) return std::nullopt;

    const auto* name = dexStringById(l->dexFile(), methodid->nameIdx);
    if(!name) return std::nullopt;

    const auto* classdescriptor = dexStringByTypeIdx(l->dexFile(), methodid->classIdx);
    if(!classdescriptor) return std::nullopt;

    auto* signature = dexCopyDescriptorFromMethodId(l->dexFile(), methodid);
    m_methodcache[methodidx] = Demangler::getObjectName(classdescriptor) + "." + name + Demangler::getSignature(signature);
    std::free(signature);
    return m_methodcache[methodidx];
}

std::optional<std::string> DalvikAssembler::getFieldInfo(RDContext* ctx, dex_u4 fieldidx)
{
    auto* l = reinterpret_cast<const DexLoader*>(RDContext_GetUserData(ctx, DEXLOADER_USERDATA));
    if(fieldidx >= l->dexFile()->pHeader->fieldIdsSize) return std::nullopt;

    auto it = m_fieldcache.find(fieldidx);
    if(it != m_fieldcache.end()) return it->second;

    const auto* fieldid = dexGetFieldId(l->dexFile(), fieldidx);
    if(!fieldid) return std::nullopt;

    const auto* name = dexStringById(l->dexFile(), fieldid->nameIdx);
    if(!name) return std::nullopt;

    const auto* signature = dexStringByTypeIdx(l->dexFile(), fieldid->typeIdx);
    if(!signature) return std::nullopt;

    const auto* classdescriptor = dexStringByTypeIdx(l->dexFile(), fieldid->classIdx);
    if(!classdescriptor) return std::nullopt;

    m_fieldcache[fieldidx] = Demangler::getObjectName(classdescriptor) + "." + name + Demangler::getSignature(signature);
    return m_fieldcache[fieldidx];
}

std::optional<std::string> DalvikAssembler::getFunctionName(RDContext* ctx, const DexMethodId* methodid)
{
    auto* l = reinterpret_cast<const DexLoader*>(RDContext_GetUserData(ctx, DEXLOADER_USERDATA));

    const auto* classdescriptor = dexStringByTypeIdx(l->dexFile(), methodid->classIdx);
    if(!classdescriptor) return std::nullopt;

    const auto* name = dexStringById(l->dexFile(), methodid->nameIdx);
    if(!name) return std::nullopt;

    auto* descriptor = dexCopyDescriptorFromMethodId(l->dexFile(), methodid);
    std::string m = Demangler::getReturn(descriptor) + " " + Demangler::getFullName(classdescriptor) + "." + name + Demangler::getSignature(descriptor);
    std::free(descriptor);
    return m;
}
