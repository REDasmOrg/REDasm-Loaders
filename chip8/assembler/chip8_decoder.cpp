#include "chip8.h"

#define SET_DECODE_TO(opmask, cb) m_opcodes[opmask] = &Chip8Assembler::cb;

std::array<CHIP8Decoder::OpCodeCallback, 0x10> CHIP8Decoder::m_opcodes = {
    &CHIP8Decoder::decode0xxx,  &CHIP8Decoder::decode1xxx,  &CHIP8Decoder::decode2xxx,
    &CHIP8Decoder::decode3xxx,  &CHIP8Decoder::decode4xxx,  &CHIP8Decoder::decode5xxx,
    &CHIP8Decoder::decode6xxx,  &CHIP8Decoder::decode7xxx,  &CHIP8Decoder::decode8xxx,
    &CHIP8Decoder::decode9xxx,  &CHIP8Decoder::decodeAxxx,  &CHIP8Decoder::decodeBxxx,
    &CHIP8Decoder::decodeCxxx,  &CHIP8Decoder::decodeDxxx,  &CHIP8Decoder::decodeExxx,
    &CHIP8Decoder::decodeFxxx,
};

bool CHIP8Decoder::decode(const RDBufferView* view, CHIP8Instruction* instruction)
{
    u16 opcode = RD_FromBigEndian16(*reinterpret_cast<const u16*>(view->data));
    u8 op = CHIP8Decoder::getB3(opcode);

    instruction->opcode = opcode;

    if(op < m_opcodes.size()) return m_opcodes[op](opcode, instruction);
    return false;
}

bool CHIP8Decoder::decode0xxx(u16 opcode, CHIP8Instruction* instruction)
{
    switch(opcode)
    {
        case 0x0000:
            instruction->op = CHIP8Opcode_Nop;
            instruction->mnemonic = "nop";
            break;

        case 0x00E0:
            instruction->op = CHIP8Opcode_Erase;
            instruction->mnemonic = "erase";
            break;

        case 0x00EE:
            instruction->op = CHIP8Opcode_Return;
            instruction->mnemonic = "return";
            break;

        default: return false;
    }

    return true;
}

bool CHIP8Decoder::decode1xxx(u16 opcode, CHIP8Instruction* instruction)
{
    instruction->op = CHIP8Opcode_Goto;
    instruction->mnemonic = "goto";

    instruction->operands[0].type = CHIP8Operand_Address;
    instruction->operands[0].addr = CHIP8Decoder::getMMM(opcode);
    return true;
}

bool CHIP8Decoder::decode2xxx(u16 opcode, CHIP8Instruction* instruction)
{
    instruction->op = CHIP8Opcode_Do;
    instruction->mnemonic = "call";

    instruction->operands[0].type = CHIP8Operand_Address;
    instruction->operands[0].addr = CHIP8Decoder::getMMM(opcode);
    return true;
}

bool CHIP8Decoder::decode3xxx(u16 opcode, CHIP8Instruction* instruction)
{
    instruction->op = CHIP8Opcode_Ske;
    instruction->mnemonic = "ske";

    instruction->operands[0].type = CHIP8Operand_Register;
    instruction->operands[0].reg = CHIP8Decoder::getB2(opcode);
    instruction->operands[1].type = CHIP8Operand_Constant;
    instruction->operands[1].cnst = CHIP8Decoder::getKK(opcode);
    return true;
}

bool CHIP8Decoder::decode4xxx(u16 opcode, CHIP8Instruction* instruction)
{
    instruction->op = CHIP8Opcode_Skne;
    instruction->mnemonic = "skne";

    instruction->operands[0].type = CHIP8Operand_Register;
    instruction->operands[0].reg = CHIP8Decoder::getB2(opcode);
    instruction->operands[1].type = CHIP8Operand_Register;
    instruction->operands[1].reg = CHIP8Decoder::getB1(opcode);
    return true;
}

bool CHIP8Decoder::decode5xxx(u16 opcode, CHIP8Instruction* instruction)
{
    instruction->op = CHIP8Opcode_Ske;
    instruction->mnemonic = "ske";

    instruction->operands[0].type = CHIP8Operand_Register;
    instruction->operands[0].reg = CHIP8Decoder::getB2(opcode);
    instruction->operands[1].type = CHIP8Operand_Register;
    instruction->operands[1].reg = CHIP8Decoder::getB1(opcode);
    return true;
}

bool CHIP8Decoder::decode6xxx(u16 opcode, CHIP8Instruction* instruction)
{
    instruction->op = CHIP8Opcode_Copy;

    instruction->operands[0].type = CHIP8Operand_Register;
    instruction->operands[0].reg = CHIP8Decoder::getB2(opcode);
    instruction->operands[1].type = CHIP8Operand_Constant;
    instruction->operands[1].cnst = CHIP8Decoder::getKK(opcode);
    return true;
}

bool CHIP8Decoder::decode7xxx(u16 opcode, CHIP8Instruction* instruction)
{
    instruction->op = CHIP8Opcode_Add;

    instruction->operands[0].type = CHIP8Operand_Register;
    instruction->operands[0].reg = CHIP8Decoder::getB2(opcode);
    instruction->operands[1].type = CHIP8Operand_Constant;
    instruction->operands[1].cnst = CHIP8Decoder::getKK(opcode);
    return true;
}

bool CHIP8Decoder::decode8xxx(u16 opcode, CHIP8Instruction* instruction)
{
    u8 op = opcode & 0x000F;

    switch(op)
    {
        case 0x0: instruction->op = CHIP8Opcode_Copy; break;
        case 0x1: instruction->op = CHIP8Opcode_Or;   break;
        case 0x2: instruction->op = CHIP8Opcode_And;  break;
        case 0x3: instruction->op = CHIP8Opcode_Xor;  break;
        case 0x4: instruction->op = CHIP8Opcode_Add;  break;
        case 0x5: instruction->op = CHIP8Opcode_Sub;  break;
        default: return false;
    }

    instruction->operands[0].type = CHIP8Operand_Register;
    instruction->operands[0].reg = CHIP8Decoder::getB2(opcode);
    instruction->operands[1].type = CHIP8Operand_Register;
    instruction->operands[1].reg = CHIP8Decoder::getB1(opcode);
    return true;
}

bool CHIP8Decoder::decode9xxx(u16 opcode, CHIP8Instruction* instruction)
{
    instruction->op = CHIP8Opcode_Skne;
    instruction->mnemonic = "skne";

    instruction->operands[0].type = CHIP8Operand_Register;
    instruction->operands[0].reg = CHIP8Decoder::getB2(opcode);
    instruction->operands[1].type = CHIP8Operand_Register;
    instruction->operands[1].reg = CHIP8Decoder::getB1(opcode);
    return true;
}

bool CHIP8Decoder::decodeAxxx(u16 opcode, CHIP8Instruction* instruction)
{
    instruction->op = CHIP8Opcode_SetI;

    instruction->operands[0].type = CHIP8Operand_Register_I;
    instruction->operands[1].type = CHIP8Operand_Constant;
    instruction->operands[1].addr = CHIP8Decoder::getMMM(opcode);
    return true;
}

bool CHIP8Decoder::decodeBxxx(u16 opcode, CHIP8Instruction* instruction)
{
    instruction->op = CHIP8Opcode_GotoRel;
    instruction->mnemonic = "goto";

    instruction->operands[0].type = CHIP8Operand_Constant;
    instruction->operands[0].cnst = CHIP8Decoder::getMMM(opcode);
    instruction->operands[1].type = CHIP8Operand_Register;
    instruction->operands[1].reg = CHIP8Register_V0;
    return true;
}

bool CHIP8Decoder::decodeCxxx(u16 opcode, CHIP8Instruction* instruction)
{
    instruction->op = CHIP8Opcode_Rand;
    instruction->mnemonic = "rand";

    instruction->operands[0].type = CHIP8Operand_Register;
    instruction->operands[0].reg = CHIP8Decoder::getB2(opcode);
    instruction->operands[1].type = CHIP8Operand_Constant;
    instruction->operands[1].cnst = CHIP8Decoder::getKK(opcode);
    return true;
}

bool CHIP8Decoder::decodeDxxx(u16 opcode, CHIP8Instruction* instruction)
{
    instruction->op = CHIP8Opcode_Show;
    instruction->mnemonic = "show";

    instruction->operands[0].type = CHIP8Operand_Constant;
    instruction->operands[0].cnst = CHIP8Decoder::getB0(opcode);
    instruction->operands[1].type = CHIP8Operand_Register;
    instruction->operands[1].reg = CHIP8Decoder::getB2(opcode);
    instruction->operands[2].type = CHIP8Operand_Register;
    instruction->operands[2].reg = CHIP8Decoder::getB1(opcode);
    return true;
}

bool CHIP8Decoder::decodeExxx(u16 opcode, CHIP8Instruction* instruction)
{
    u16 op = opcode & 0x00FF;

    switch(op)
    {
        case 0x9E:
            instruction->op = CHIP8Opcode_Ske_Key;
            instruction->mnemonic = "ske";
            break;

        case 0xA1:
            instruction->op = CHIP8Opcode_Skne_Key;
            instruction->mnemonic = "skne";
            break;

        default: return false;
    }

    instruction->operands[0].type = CHIP8Operand_Register;
    instruction->operands[0].reg = CHIP8Decoder::getB2(opcode);
    instruction->operands[1].type = CHIP8Operand_Register_KEY;
    return true;
}

bool CHIP8Decoder::decodeFxxx(u16 opcode, CHIP8Instruction* instruction)
{
    if(!opcode)
    {
        instruction->op = CHIP8Opcode_Stop;
        instruction->mnemonic = "stop";
        return true;
    }

    u16 x = CHIP8Decoder::getB2(opcode);
    u16 op = opcode & 0x00FF;

    switch(op)
    {
        case 0x07:
            instruction->op = CHIP8Opcode_GetTime;
            instruction->operands[0].type = CHIP8Operand_Register;
            instruction->operands[0].reg = x;
            instruction->operands[1].type = CHIP8Operand_Register_TIME;
            break;

        case 0x0A:
            instruction->op = CHIP8Opcode_GetKey;
            instruction->operands[0].type = CHIP8Operand_Register;
            instruction->operands[0].reg = x;
            instruction->operands[1].type = CHIP8Operand_Register_KEY;
            break;

        case 0x15:
            instruction->op = CHIP8Opcode_SetTime;
            instruction->operands[0].type = CHIP8Operand_Register_TIME;
            instruction->operands[1].type = CHIP8Operand_Register;
            instruction->operands[1].reg = x;
            break;

        case 0x17:
            instruction->op = CHIP8Opcode_SetPitch;
            instruction->operands[0].type = CHIP8Operand_Register_PITCH;
            instruction->operands[1].type = CHIP8Operand_Register;
            instruction->operands[1].reg = x;
            break;

        case 0x18:
            instruction->op = CHIP8Opcode_SetTone;
            instruction->operands[0].type = CHIP8Operand_Register_TONE;
            instruction->operands[1].type = CHIP8Operand_Register;
            instruction->operands[1].reg = x;
            break;

        case 0x1E:
            instruction->op = CHIP8Opcode_Add;
            instruction->operands[0].type = CHIP8Operand_Register_I;
            instruction->operands[1].type = CHIP8Operand_Register;
            instruction->operands[1].reg = x;
            break;

        case 0x29:
            instruction->op = CHIP8Opcode_Copy2;
            instruction->operands[0].type = CHIP8Operand_Register_MI;
            instruction->operands[1].type = CHIP8Operand_Register_DEQ;
            instruction->operands[2].type = CHIP8Operand_Register;
            instruction->operands[2].reg = x;
            break;

        case 0x33:
            instruction->op = CHIP8Opcode_Copy2;
            instruction->operands[0].type = CHIP8Operand_Register_MI;
            instruction->operands[1].type = CHIP8Operand_Register_DSP;
            instruction->operands[2].type = CHIP8Operand_Register;
            instruction->operands[2].reg = x;
            break;

        case 0x55:
            instruction->op = CHIP8Opcode_Copy;
            instruction->operands[0].type = CHIP8Operand_Register_MI;
            instruction->operands[1].type = CHIP8Operand_Register_Range;
            instruction->operands[1].reg1 = CHIP8Register_V0;
            instruction->operands[1].reg2 = x;
            break;

        case 0x65:
            instruction->op = CHIP8Opcode_Copy;
            instruction->operands[0].type = CHIP8Operand_Register_Range;
            instruction->operands[0].reg1 = CHIP8Register_V0;
            instruction->operands[0].reg2 = x;
            instruction->operands[1].type = CHIP8Operand_Register_MI;
            break;

        case 0x70:
            instruction->op = CHIP8Opcode_Copy;
            instruction->operands[0].reg = CHIP8Operand_Register_RS485;
            instruction->operands[1].type = CHIP8Operand_Register;
            instruction->operands[1].reg = x;
            break;

        case 0x71:
            instruction->op = CHIP8Opcode_Copy;
            instruction->operands[0].type = CHIP8Operand_Register;
            instruction->operands[1].type = CHIP8Operand_Register_RS485;
            instruction->operands[1].reg = x;
            break;

        case 0x72:
            instruction->op = CHIP8Opcode_Copy;
            instruction->operands[0].type = CHIP8Operand_Register_BAUD;
            instruction->operands[1].type = CHIP8Operand_Register;
            instruction->operands[1].reg = x;
            break;

        default: return false;
    }

    return true;
}
