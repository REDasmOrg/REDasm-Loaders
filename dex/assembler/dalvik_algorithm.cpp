#include "dalvik_algorithm.h"
#include "../loader/dex.h"
#include "dalvik_metadata.h"
#include "dalvik.h"
#include "dalvik_payload.h"
#include <redasm/disassembler/disassembler.h>
#include <redasm/support/symbolize.h>
#include <redasm/redasm.h>

DalvikAlgorithm::DalvikAlgorithm(): Algorithm()
{
    m_dexloader = dynamic_cast<DexLoader*>(r_disasm->loader());

    REGISTER_STATE(DalvikAlgorithm::StringIndexState, &DalvikAlgorithm::stringIndexState);
    REGISTER_STATE(DalvikAlgorithm::MethodIndexState, &DalvikAlgorithm::methodIndexState);
    REGISTER_STATE(DalvikAlgorithm::PackedSwitchTableState, &DalvikAlgorithm::packedSwitchTableState);
    REGISTER_STATE(DalvikAlgorithm::SparseSwitchTableState, &DalvikAlgorithm::sparseSwitchTableState);
    REGISTER_STATE(DalvikAlgorithm::FillArrayDataState, &DalvikAlgorithm::fillArrayDataState);
    REGISTER_STATE(DalvikAlgorithm::DebugInfoState, &DalvikAlgorithm::debugInfoState);
}

void DalvikAlgorithm::validateTarget(const CachedInstruction &) const { /* Nop */ }

void DalvikAlgorithm::onDecodedOperand(const Operand* op, const CachedInstruction &instruction)
{
    if(op->tag == DalvikOperands::StringIndex)
        EXECUTE_STATE(DalvikAlgorithm::StringIndexState, op->tag, op->index, instruction);
    else if(op->tag == DalvikOperands::MethodIndex)
        EXECUTE_STATE(DalvikAlgorithm::MethodIndexState, op->tag, op->index, instruction);
    else if(op->tag == DalvikOperands::PackedSwitchTable)
        EXECUTE_STATE(DalvikAlgorithm::PackedSwitchTableState, op->tag, op->index, instruction);
    else if(op->tag == DalvikOperands::SparseSwitchTable)
        EXECUTE_STATE(DalvikAlgorithm::SparseSwitchTableState, op->tag, op->index, instruction);
    else if(op->tag == DalvikOperands::FillArrayData)
        EXECUTE_STATE(DalvikAlgorithm::FillArrayDataState, op->tag, op->index, instruction);
}

void DalvikAlgorithm::onDecoded(const CachedInstruction& instruction)
{
    Algorithm::onDecoded(instruction);
    auto it = m_methodbounds.find(instruction->endAddress());

    if((it == m_methodbounds.end()) && this->canContinue(instruction))
        this->enqueue(instruction->endAddress());
    else if(it != m_methodbounds.end())
        m_methodbounds.erase(it);
}

void DalvikAlgorithm::decodeState(const State *state)
{
    const Symbol* symbol = r_doc->symbol(state->address);

    if(symbol && symbol->isFunction())
    {
        m_methodbounds.insert(state->address + m_dexloader->getMethodSize(symbol->tag));
        FORWARD_STATE(DalvikAlgorithm::DebugInfoState, state);
    }

    Algorithm::decodeState(state);
}

void DalvikAlgorithm::stringIndexState(const State *state)
{
    if(!m_dexloader)
        return;

    const Operand* op = state->operand();
    offset_t offset = 0;
    size_t len = 0;

    if(!(len = m_dexloader->getStringOffset(op->u_value, offset)))
        return;

    r_doc->asciiString(offset, len);
    r_disasm->pushReference(offset, state->instruction->address);
}

void DalvikAlgorithm::methodIndexState(const State *state)
{
    if(!m_dexloader)
        return;

    this->checkImport(state);

    const Operand* op = state->operand();
    offset_t offset = 0;

    if(!m_dexloader->getMethodOffset(op->u_value, offset))
        return;

    r_disasm->pushReference(offset, state->instruction->address);
}

void DalvikAlgorithm::packedSwitchTableState(const State *state)
{
    const Operand* op = state->operand();
    const DalvikPackedSwitchPayload* packedswitchpayload = r_ldr->addrpointer<const DalvikPackedSwitchPayload>(op->u_value);

    if(!packedswitchpayload || (packedswitchpayload->ident != DALVIK_PACKED_SWITCH_IDENT))
        return;

    REDasm::symbolize<DalvikPackedSwitchPayload>(op->u_value, "packed_switch");

    CachedInstruction instruction = state->instruction;
    r_doc->autoComment(instruction->address, String::number(packedswitchpayload->size) + " case(s)");
    const u32* targets = packedswitchpayload->targets;
    PackagedCaseMap cases;

    for(u16 i = 0; i < packedswitchpayload->size; i++, targets++)
    {
        s32 caseidx = packedswitchpayload->first_key + i;
        address_t target = instruction->address + (*targets * sizeof(u16));
        this->enqueue(target);

        r_doc->pointer(r_ldr->addressof(targets), "packed_switch_" + String::hex(op->u_value) + "_case_" + String::number(caseidx));
        r_doc->label(target);
        r_disasm->pushReference(target, instruction->address);
        r_disasm->pushTarget(target, instruction->address);
        this->enqueue(target);

        auto it = cases.find(target);
        if(it != cases.end()) it->second.push_back(caseidx);
        else cases[target] = { caseidx };
    }

    this->emitCaseInfo(op->u_value, cases);
}

void DalvikAlgorithm::sparseSwitchTableState(const State *state)
{
    const Operand* op = state->operand();
    const DalvikSparseSwitchPayload* sparseswitchpayload = r_ldr->addrpointer<const DalvikSparseSwitchPayload>(op->u_value);

    if(!sparseswitchpayload || (sparseswitchpayload->ident != DALVIK_SPARSE_SWITCH_IDENT))
        return;

    REDasm::symbolize<DalvikSparseSwitchPayload>(op->u_value, "sparse_switch");

    CachedInstruction instruction = state->instruction;
    r_doc->autoComment(instruction->address, String::number(sparseswitchpayload->size) + " case(s)");
    const u32* keys = sparseswitchpayload->keys;
    const u32* targets = Utils::relpointer<const u32>(keys, sizeof(u32) * sparseswitchpayload->size);

    SparseCaseMap cases;

    for(u32 i = 0; i < sparseswitchpayload->size; i++)
    {
        address_t address = r_ldr->addressof(&keys[i]);
        r_doc->data(address, sizeof(u32), SymbolTable::name("sparse_switch.key", address));
    }

    for(u32 i = 0; i < sparseswitchpayload->size; i++)
    {
        address_t address = r_ldr->addressof(&targets[i]);
        address_t target = instruction->address + (targets[i] * sizeof(u16));
        r_doc->pointer(address, SymbolTable::name("sparse_switch.target", address));
        r_doc->label(target);
        r_disasm->pushReference(target, instruction->address);
        r_disasm->pushTarget(target, instruction->address);
        cases[keys[i]] = target;
        this->enqueue(target);
    }

    this->emitCaseInfo(op->u_value, instruction, cases);
}

void DalvikAlgorithm::fillArrayDataState(const State *state)
{
    const Operand* op = state->operand();
    const DalvikFillArrayDataPayload* fillarraydatapayload = r_ldr->addrpointer<const DalvikFillArrayDataPayload>(op->u_value);

    if(!fillarraydatapayload || (fillarraydatapayload->ident != DALVIK_FILL_ARRAY_DATA_IDENT))
        return;

    REDasm::symbolize<DalvikFillArrayDataPayload>(op->u_value, "array_payload");
}

void DalvikAlgorithm::debugInfoState(const State *state)
{
    const Symbol* symbol = r_doc->symbol(state->address);
    if(!symbol || !symbol->isFunction()) return;

    DexEncodedMethod dexmethod;
    if(!m_dexloader->getMethodInfo(symbol->tag, dexmethod)) return;

    DexDebugInfo dexdebuginfo;
    if(!m_dexloader->getDebugInfo(symbol->tag, dexdebuginfo)) return;

    this->emitArguments(state, dexmethod, dexdebuginfo);
    this->emitDebugData(dexdebuginfo);
}

void DalvikAlgorithm::emitCaseInfo(address_t address, const DalvikAlgorithm::PackagedCaseMap &casemap)
{
    for(const auto& item : casemap)
    {
        String casestring;

        std::for_each(item.second.begin(), item.second.end(), [&casestring](s32 caseidx) {
            if(!casestring.empty())
                casestring += ", ";

            casestring += "#" + String::number(caseidx);
        });

        r_doc->meta(item.first, "@ " + String::hex(address) + " (Case(s) " + casestring + ")", "packaged_switch_table");
    }
}

void DalvikAlgorithm::emitCaseInfo(address_t address, const CachedInstruction& instruction, const DalvikAlgorithm::SparseCaseMap &casemap)
{
    for(const auto& item : casemap)
        r_doc->meta(item.second, "@ " + String::hex(address) + " (Case Key " + String::hex(item.first, 0, true) + ")", "sparse_switch_table");

    r_doc->meta(instruction->endAddress(), "@ " + String::hex(address) + " (Default)", "sparse_switch_table");
}

void DalvikAlgorithm::emitArguments(const State* state, const DexEncodedMethod& dexmethod, const DexDebugInfo& dexdebuginfo)
{
    u32 delta = (static_cast<DexAccessFlags>(dexmethod.access_flags) & DexAccessFlags::Static) ? 0 : 1;

    for(size_t i = 0; i < dexdebuginfo.parameter_names.size(); i++)
    {
        const String& param = dexdebuginfo.parameter_names[i];
        r_doc->meta(state->address, String::number(i + delta) + ": " + param, "arg");
    }
}

void DalvikAlgorithm::emitDebugData(const DexDebugInfo &dexdebuginfo)
{
    if(dexdebuginfo.line_start == DEX_NO_INDEX_U)
        return;

    for(const auto& item: dexdebuginfo.debug_data)
    {
        for(const auto& dbgdata : item.second)
        {
            if((dbgdata.data_type == DEXDebugDataTypes::StartLocal) || ((dbgdata.data_type == DEXDebugDataTypes::StartLocalExtended)))
            {
                if(dbgdata.name_idx == DEX_NO_INDEX)
                    continue;

                const String& name = m_dexloader->getString(dbgdata.name_idx);
                String type;

                if(dbgdata.type_idx != DEX_NO_INDEX)
                    type = ": " + m_dexloader->getType(dbgdata.type_idx);

                r_doc->meta(item.first, DalvikAssembler::registerName(dbgdata.register_num) + " = " + name + type, "localstart");
            }
            else if(dbgdata.data_type == DEXDebugDataTypes::RestartLocal)
                r_doc->meta(item.first, DalvikAssembler::registerName(dbgdata.register_num), "localrestart");
            else if(dbgdata.data_type == DEXDebugDataTypes::EndLocal)
                r_doc->meta(item.first,  DalvikAssembler::registerName(dbgdata.register_num), "localend");
            else if(dbgdata.data_type == DEXDebugDataTypes::PrologueEnd)
                r_doc->meta(item.first, String(), "prologue_end");
            else if(dbgdata.data_type == DEXDebugDataTypes::Line)
                r_doc->meta(item.first, String::number(dbgdata.line_no), "line");
        }
    }
}

void DalvikAlgorithm::checkImport(const State* state)
{
    const Operand* op = state->operand();
    const String& methodname = m_dexloader->getMethodName(op->u_value);

    auto it = m_imports.find(methodname);
    if(it != m_imports.end()) return;

    m_imports.insert(methodname);
    address_t importaddress = 0;

    if(!methodname.startsWith("java.")) r_doc->imported(m_dexloader->nextImport(&importaddress), sizeof(u16), methodname);
    else return;

    r_disasm->pushReference(importaddress, state->instruction->address);
}

bool DalvikAlgorithm::canContinue(const CachedInstruction& instruction) const
{
    if(instruction->isStop()) return false;
    if(instruction->isJump() && !instruction->isConditional()) return false;
    return true;
}
