#include "dex_statemachine.h"
#include <redasm/types/ext_types.h>
#include <redasm/support/utils.h>

#define DBG_FIRST_SPECIAL 0x0A // The smallest special opcode
#define DBG_LINE_BASE     -4   // The smallest line number increment
#define DBG_LINE_RANGE    15   // The number of line increments represented

#define BIND_STATE(opcode)   m_statesmap[opcode] = [this](u8** data) { this->execute##opcode(data); }
#define VALIDATE_LINE()      if(m_line == 0) r_ctx->problem("line register == 0")

DEXStateMachine::DEXStateMachine(address_t address, DexDebugInfo &debuginfo): m_debuginfo(debuginfo), m_address(address), m_line(debuginfo.line_start), m_atend(false)
{
    BIND_STATE(0x00);
    BIND_STATE(0x01);
    BIND_STATE(0x02);
    BIND_STATE(0x03);
    BIND_STATE(0x04);
    BIND_STATE(0x05);
    BIND_STATE(0x06);
    BIND_STATE(0x07);
    BIND_STATE(0x08);
    BIND_STATE(0x09);
}

void DEXStateMachine::execute(u8 *data)
{
    while(!m_atend)
    {
        u8 opcode = *data;
        data++;

        if(opcode >= DBG_FIRST_SPECIAL)
        {
            this->executeSpecial(opcode);
            continue;
        }

        auto it = m_statesmap.find(opcode);

        if(it == m_statesmap.end())
        {
            r_ctx->log("Unknown opcode '" + Utils::hex(opcode) + "'");
            return;
        }

        it->second(&data);
    }
}

void DEXStateMachine::execute0x00(u8 **data) // DBG_END_SEQUENCE
{
    m_atend = true;
}

void DEXStateMachine::execute0x01(u8 **data) // DBG_ADVANCE_PC
{
    m_address += LEB128::unsignedOf<u32>(*data, data);
}

void DEXStateMachine::execute0x02(u8 **data) // DBG_ADVANCE_LINE
{
    m_line += LEB128::signedOf<s32>(*data, data);
    VALIDATE_LINE();
}

void DEXStateMachine::execute0x03(u8 **data) // DBG_START_LOCAL
{
    u32 r = LEB128::unsignedOf<u32>(*data, data);
    s32 n = LEB128::unsigned1COf<s32>(*data, data), t = LEB128::unsigned1COf<s32>(*data, data);
    this->setDebugData(DEXDebugData::local(r, n, t));
}

void DEXStateMachine::execute0x04(u8 **data) // DBG_START_LOCAL_EXTENDED
{
    u32 r = LEB128::unsignedOf<u32>(*data, data);
    s32 n = LEB128::unsigned1COf<s32>(*data, data), t = LEB128::unsigned1COf<s32>(*data, data), s = LEB128::unsigned1COf<s32>(*data, data);
    this->setDebugData(DEXDebugData::localext(r, n, t, s));
}

void DEXStateMachine::execute0x05(u8 **data) // DBG_END_LOCAL
{
    this->setDebugData(DEXDebugData::endLocal(LEB128::unsignedOf<u32>(*data, data)));
}

void DEXStateMachine::execute0x06(u8 **data) // DBG_RESTART_LOCAL
{
    this->setDebugData(DEXDebugData::restartLocal(LEB128::unsignedOf<u32>(*data, data)));
}

void DEXStateMachine::execute0x07(u8 **data) // DBG_SET_PROLOGUE_END
{
    this->setDebugData(DEXDebugData::prologueEnd());
}

void DEXStateMachine::execute0x08(u8 **data) // DBG_SET_EPILOGUE_BEGIN
{
    this->setDebugData(DEXDebugData::epilogueBegin());
}

void DEXStateMachine::execute0x09(u8 **data) // DBG_SET_FILE
{
    this->setDebugData(DEXDebugData::file(LEB128::unsignedOf<u32>(*data, data)));
}

void DEXStateMachine::executeSpecial(u8 opcode) // Special opcodes
{
    u16 adjopcode = opcode - DBG_FIRST_SPECIAL;
    m_line += DBG_LINE_BASE + (adjopcode % DBG_LINE_RANGE);
    m_address += (adjopcode / DBG_LINE_RANGE) * sizeof(u16);

    VALIDATE_LINE();
    this->setDebugData(DEXDebugData::line(m_line));
}

void DEXStateMachine::setDebugData(const DEXDebugData &debugdata)
{
    auto it = m_debuginfo.debug_data.find(m_address);

    if(it == m_debuginfo.debug_data.end())
    {
        std::list<DEXDebugData> dbgdatalist;
        dbgdatalist.push_back(debugdata);
        m_debuginfo.debug_data[m_address] = dbgdatalist;
    }
    else
        it->second.push_back(debugdata);
}
