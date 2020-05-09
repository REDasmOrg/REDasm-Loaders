#include "chip8.h"
#include <functional>
#include <optional>

#define SET_DECODE_TO(opmask, cb) m_opcodes[opmask] = std::bind(&Chip8Assembler::cb, this, std::placeholders::_1, std::placeholders::_2);

Chip8Assembler::Chip8Assembler()
{
    SET_DECODE_TO(0x0000, decode0xxx);
    SET_DECODE_TO(0x1000, decode1xxx);
    SET_DECODE_TO(0x2000, decode2xxx);
    SET_DECODE_TO(0x3000, decode3xxx);
    SET_DECODE_TO(0x4000, decode4xxx);
    SET_DECODE_TO(0x5000, decode5xxx);
    SET_DECODE_TO(0x6000, decode6xxx);
    SET_DECODE_TO(0x7000, decode7xxx);
    SET_DECODE_TO(0x8000, decode8xxx);
    SET_DECODE_TO(0x9000, decode9xxx);
    SET_DECODE_TO(0xA000, decodeAxxx);
    SET_DECODE_TO(0xB000, decodeBxxx);
    SET_DECODE_TO(0xC000, decodeCxxx);
    SET_DECODE_TO(0xD000, decodeDxxx);
    SET_DECODE_TO(0xE000, decodeExxx);
    SET_DECODE_TO(0xF000, decodeFxxx);
}

void Chip8Assembler::emulate(RDDisassembler* disassembler, const RDInstruction* instruction)
{
    std::optional<address_t> target;

    if(instruction->type == InstructionType_Jump)
    {
        if(RDInstruction_MnemonicIs(instruction, "ske"))  target = RDInstruction_EndAddress(instruction) + instruction->size;
        if(RDInstruction_MnemonicIs(instruction, "skne")) target = RDInstruction_EndAddress(instruction) + instruction->size;
        if(RDInstruction_MnemonicIs(instruction, "skp"))  target = RDInstruction_EndAddress(instruction) + instruction->size;
        if(RDInstruction_MnemonicIs(instruction, "sknp")) target = RDInstruction_EndAddress(instruction) + instruction->size;

        if(target) RDDisassembler_EnqueueAddress(disassembler, instruction, *target);
        else RDDisassembler_EnqueueAddress(disassembler, instruction, instruction->operands[0].u_value);

        if(!(instruction->flags & InstructionFlags_Conditional)) return;
    }
    else if(instruction->type == InstructionType_Call)
        RDDisassembler_EnqueueAddress(disassembler, instruction, instruction->operands[0].u_value);


    if(instruction->type != InstructionType_Stop)
        RDDisassembler_EnqueueNext(disassembler, instruction);
}

bool Chip8Assembler::decode(RDBufferView* view, RDInstruction *instruction)
{
    u16 opcode = RD_Swap16BE(*reinterpret_cast<u16*>(RDBufferView_Data(view)));
    instruction->id = opcode;
    instruction->size = sizeof(u16);

    auto it = m_opcodes.find(opcode & 0xF000);
    if((it == m_opcodes.end()) || !it->second(opcode, instruction)) return false;
    this->categorize(instruction);
    return true;
}

void Chip8Assembler::categorize(RDInstruction *instruction)
{
    if(RDInstruction_MnemonicIs(instruction, "rts"))
        instruction->type = InstructionType_Stop;
    else if(RDInstruction_MnemonicIs(instruction, "jmp"))
        instruction->type = InstructionType_Jump;
    else if((RDInstruction_MnemonicIs(instruction, "ske") || RDInstruction_MnemonicIs(instruction, "skne") || RDInstruction_MnemonicIs(instruction, "skp") || RDInstruction_MnemonicIs(instruction, "sknp")))
    {
        instruction->type = InstructionType_Jump;
        instruction->flags = InstructionFlags_Conditional;
    }
    else if(RDInstruction_MnemonicIs(instruction, "call"))
        instruction->type = InstructionType_Call;
    else if(RDInstruction_MnemonicIs(instruction, "add"))
        instruction->type = InstructionType_Add;
    else if(RDInstruction_MnemonicIs(instruction, "sub"))
        instruction->type = InstructionType_Sub;
    else if(RDInstruction_MnemonicIs(instruction, "and"))
        instruction->type = InstructionType_And;
    else if(RDInstruction_MnemonicIs(instruction, "or"))
        instruction->type = InstructionType_Or;
    else if(RDInstruction_MnemonicIs(instruction, "xor"))
        instruction->type = InstructionType_Xor;
    else if(RDInstruction_MnemonicIs(instruction, "mov") || RDInstruction_MnemonicIs(instruction, "ldra"))
        instruction->type = InstructionType_Load;
    else if(RDInstruction_MnemonicIs(instruction, "stra"))
        instruction->type = InstructionType_Store;
    else if(RDInstruction_MnemonicIs(instruction, "sys"))
        instruction->flags = InstructionFlags_Privileged;
}

bool Chip8Assembler::decode0xxx(u16 opcode, RDInstruction *instruction) const
{
    if(opcode == 0x00E0)
        RDInstruction_SetMnemonic(instruction, "cls");
    else if(opcode == 0x00EE)
        RDInstruction_SetMnemonic(instruction, "rts");
    else if(opcode == 0x00FB) // SuperChip only
        RDInstruction_SetMnemonic(instruction, "scright");
    else if(opcode == 0x00FC) // SuperChip only
        RDInstruction_SetMnemonic(instruction, "scleft");
    else if(opcode == 0x00FE) // SuperChip only
        RDInstruction_SetMnemonic(instruction, "low");
    else if(opcode == 0x00FF) // SuperChip only
        RDInstruction_SetMnemonic(instruction, "high");
    else if((opcode & 0x00F0) == 0x00C0) // SuperChip only
    {
        RDInstruction_SetMnemonic(instruction, "scdown");
        RDInstruction_PushOperand(instruction, OperandType_Constant)->u_value = opcode & 0x000F;
    }
    else
    {
        RDInstruction_SetMnemonic(instruction, "sys");
        RDInstruction_PushOperand(instruction, OperandType_Constant)->u_value = opcode & 0x0FFF;
    }

    return true;
}

bool Chip8Assembler::decode1xxx(u16 opcode, RDInstruction *instruction) const
{
    RDInstruction_SetMnemonic(instruction, "jmp");
    RDInstruction_PushOperand(instruction, OperandType_Immediate)->u_value = opcode & 0x0FFF;
    return true;
}

bool Chip8Assembler::decode2xxx(u16 opcode, RDInstruction *instruction) const
{
    RDInstruction_SetMnemonic(instruction, "call");
    RDInstruction_PushOperand(instruction, OperandType_Immediate)->u_value = opcode & 0x0FFF;
    return true;
}

bool Chip8Assembler::decode3xxx(u16 opcode, RDInstruction *instruction) const
{
    RDInstruction_SetMnemonic(instruction, "ske");
    RDInstruction_PushOperand(instruction, OperandType_Register)->reg = (opcode & 0x0F00) >> 8;
    RDInstruction_PushOperand(instruction, OperandType_Immediate)->u_value = opcode & 0x00FF;
    return true;
}

bool Chip8Assembler::decode4xxx(u16 opcode, RDInstruction *instruction) const
{
    RDInstruction_SetMnemonic(instruction, "skne");
    RDInstruction_PushOperand(instruction, OperandType_Register)->reg = (opcode & 0x0F00) >> 8;
    RDInstruction_PushOperand(instruction, OperandType_Immediate)->u_value = opcode & 0x00FF;
    return true;
}

bool Chip8Assembler::decode5xxx(u16 opcode, RDInstruction *instruction) const
{
    if((opcode & 0x000F) != 0) return false;

    RDInstruction_SetMnemonic(instruction, "ske");
    RDInstruction_PushOperand(instruction, OperandType_Register)->reg = (opcode & 0x0F00) >> 8;
    RDInstruction_PushOperand(instruction, OperandType_Register)->reg = (opcode & 0x00F0) >> 4;
    return true;
}

bool Chip8Assembler::decode6xxx(u16 opcode, RDInstruction* instruction) const
{
    RDInstruction_SetMnemonic(instruction, "mov");
    RDInstruction_PushOperand(instruction, OperandType_Register)->reg = (opcode & 0x0F00) >> 8;
    RDInstruction_PushOperand(instruction, OperandType_Constant)->u_value = opcode & 0x00FF;
    return true;
}

bool Chip8Assembler::decode7xxx(u16 opcode, RDInstruction* instruction) const
{
    RDInstruction_SetMnemonic(instruction, "add");
    RDInstruction_PushOperand(instruction, OperandType_Register)->reg = (opcode & 0x0F00) >> 8;
    RDInstruction_PushOperand(instruction, OperandType_Constant)->u_value = opcode & 0x00FF;
    return true;
}

bool Chip8Assembler::decode8xxx(u16 opcode, RDInstruction* instruction) const
{
    u8 op = opcode & 0x000F;

    switch(op)
    {
        case 0x0: RDInstruction_SetMnemonic(instruction, "mov"); break;
        case 0x1: RDInstruction_SetMnemonic(instruction, "or");  break;
        case 0x2: RDInstruction_SetMnemonic(instruction, "and"); break;
        case 0x3: RDInstruction_SetMnemonic(instruction, "xor"); break;
        case 0x4: RDInstruction_SetMnemonic(instruction, "add"); break;
        case 0x5: RDInstruction_SetMnemonic(instruction, "sub"); break;
        case 0x6: RDInstruction_SetMnemonic(instruction, "shr"); break;
        case 0x7: RDInstruction_SetMnemonic(instruction, "sub"); break;
        case 0xE: RDInstruction_SetMnemonic(instruction, "shl"); break;
        default: return false;
    }

    RDInstruction_PushOperand(instruction, OperandType_Register)->reg = (opcode & 0x0F00) >> 8;

    if((op != 0x6) && (op != 0xE)) // Skip 2nd operand if op == shift_instructions
        RDInstruction_PushOperand(instruction, OperandType_Register)->reg = (opcode & 0x00F0) >> 4;

    return true;
}

bool Chip8Assembler::decode9xxx(u16 opcode, RDInstruction* instruction) const
{
    if((opcode & 0x000F) != 0)
        return false;

    RDInstruction_SetMnemonic(instruction, "skne");
    RDInstruction_PushOperand(instruction, OperandType_Register)->reg = (opcode & 0x0F00) >> 8;
    RDInstruction_PushOperand(instruction, OperandType_Register)->reg = (opcode & 0x00F0) >> 4;
    return true;
}

bool Chip8Assembler::decodeAxxx(u16 opcode, RDInstruction* instruction) const
{
    RDInstruction_SetMnemonic(instruction, "mov");

    RDOperand* operand = RDInstruction_PushOperand(instruction, OperandType_Register);
    operand->reg = CHIP8_REG_I_ID;
    operand->u_data = CHIP8_REG_I;

    RDInstruction_PushOperand(instruction, OperandType_Constant)->u_value = opcode & 0x0FFF;
    return true;
}

bool Chip8Assembler::decodeBxxx(u16 opcode, RDInstruction* instruction) const
{
    RDInstruction_SetMnemonic(instruction, "jmp");

    RDOperand* operand = RDInstruction_PushOperand(instruction, OperandType_Displacement);
    operand->base = CHIP8_REG_V0_ID;
    operand->displacement = opcode & 0x0FFF;
    return true;
}

bool Chip8Assembler::decodeCxxx(u16 opcode, RDInstruction* instruction) const
{
    RDInstruction_SetMnemonic(instruction, "rand");
    RDInstruction_PushOperand(instruction, OperandType_Register)->reg = (opcode & 0x0F00) >> 8;
    RDInstruction_PushOperand(instruction, OperandType_Constant)->u_value = opcode & 0x00FF;
    return true;
}

bool Chip8Assembler::decodeDxxx(u16 opcode, RDInstruction* instruction) const
{
    RDInstruction_SetMnemonic(instruction, "draw");
    RDInstruction_PushOperand(instruction, OperandType_Register)->reg = (opcode & 0x0F00) >> 8;
    RDInstruction_PushOperand(instruction, OperandType_Register)->reg = (opcode & 0x00F0) >> 4;
    RDInstruction_PushOperand(instruction, OperandType_Constant)->u_value = opcode & 0x000F;
    return true;
}

bool Chip8Assembler::decodeExxx(u16 opcode, RDInstruction* instruction) const
{
    u16 op = opcode & 0xFF;

    if(op == 0x9E) RDInstruction_SetMnemonic(instruction, "skp");
    else if(op == 0xA1) RDInstruction_SetMnemonic(instruction, "sknp");

    RDOperand* operand = RDInstruction_PushOperand(instruction, OperandType_Register);
    operand->reg = (opcode & 0x0F00) >> 8;
    operand->u_data = CHIP8_REG_K;

    return true;
}

bool Chip8Assembler::decodeFxxx(u16 opcode, RDInstruction* instruction) const
{
    RDOperand* operand = nullptr;
    u16 op = opcode & 0x00FF;

    switch(op)
    {
        case 0x07: RDInstruction_SetMnemonic(instruction, "gdelay"); break;
        case 0x0A: RDInstruction_SetMnemonic(instruction, "wkey");   break;
        case 0x15: RDInstruction_SetMnemonic(instruction, "sdelay"); break;
        case 0x18: RDInstruction_SetMnemonic(instruction, "ssound"); break;

        case 0x1E:
            RDInstruction_SetMnemonic(instruction, "add");
            operand = RDInstruction_PushOperand(instruction, OperandType_Register);
            operand->reg = CHIP8_REG_I_ID;
            operand->u_data = CHIP8_REG_I;
            break;

        case 0x29: RDInstruction_SetMnemonic(instruction, "font");  break;
        case 0x30: RDInstruction_SetMnemonic(instruction, "xfont"); break; // SuperChip only

        case 0x33: RDInstruction_SetMnemonic(instruction, "bcd");   break;
        case 0x55: RDInstruction_SetMnemonic(instruction, "stra");  break;
        case 0x65: RDInstruction_SetMnemonic(instruction, "ldra");  break;

        default: return false;
    }

    RDInstruction_PushOperand(instruction, OperandType_Register)->reg = (opcode & 0x0F00) >> 8;
    return true;
}
