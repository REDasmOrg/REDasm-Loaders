#include "chip8.h"
#include "chip8_printer.h"

#define SET_DECODE_TO(opmask, cb) m_opcodes[opmask] = [this](u16 opcode, Instruction* instruction) -> bool { return cb(opcode, instruction); };

Chip8Assembler::Chip8Assembler(): Assembler()
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

size_t Chip8Assembler::bits() const { return 16; }
Printer *Chip8Assembler::doCreatePrinter(Disassembler *disassembler) const { return new Chip8Printer(disassembler); }

bool Chip8Assembler::decodeInstruction(const BufferView& view, Instruction *instruction)
{
    u16be opcode = static_cast<u16be>(view);
    instruction->setId(opcode);
    instruction->setSize(sizeof(u16));

    auto it = m_opcodes.find(opcode & 0xF000);

    if((it == m_opcodes.end()) || !it->second(opcode, instruction))
        return false;

    return true;
}

void Chip8Assembler::onDecoded(Instruction *instruction)
{
    if(instruction->is("rts"))
        instruction->setType(InstructionType::Stop);
    else if(instruction->is("jmp"))
        instruction->setType(InstructionType::Jump);
    else if((instruction->is("ske") || instruction->is("skne") || instruction->is("skp") || instruction->is("sknp")))
        instruction->setType(InstructionType::ConditionalJump);
    else if(instruction->is("call"))
        instruction->setType(InstructionType::Call);
    else if(instruction->is("add"))
        instruction->setType(InstructionType::Add);
    else if(instruction->is("sub"))
        instruction->setType(InstructionType::Sub);
    else if(instruction->is("and"))
        instruction->setType(InstructionType::And);
    else if(instruction->is( "or"))
        instruction->setType(InstructionType::Or);
    else if(instruction->is("xor"))
        instruction->setType(InstructionType::Xor);
    else if(instruction->is("mov") || instruction->is("ldra"))
        instruction->setType(InstructionType::Load);
    else if(instruction->is("stra"))
        instruction->setType(InstructionType::Store);
    else if(instruction->is("sys"))
        instruction->setType(InstructionType::Privileged);
}

bool Chip8Assembler::decode0xxx(u16 opcode, Instruction *instruction) const
{
    if(opcode == 0x00E0)
        instruction->setMnemonic("cls");
    else if(opcode == 0x00EE)
        instruction->setMnemonic("rts");
    else if(opcode == 0x00FB) // SuperChip only
        instruction->setMnemonic("scright");
    else if(opcode == 0x00FC) // SuperChip only
        instruction->setMnemonic("scleft");
    else if(opcode == 0x00FE) // SuperChip only
        instruction->setMnemonic("low");
    else if(opcode == 0x00FF) // SuperChip only
        instruction->setMnemonic("high");
    else if((opcode & 0x00F0) == 0x00C0) // SuperChip only
    {
        instruction->setMnemonic("scdown");
        instruction->cnst(opcode & 0x000F);
    }
    else
    {
        instruction->setMnemonic("sys");
        instruction->cnst(opcode & 0x0FFF);
    }

    return true;
}

bool Chip8Assembler::decode1xxx(u16 opcode, Instruction *instruction) const
{
    instruction->setMnemonic("jmp");
    instruction->imm(opcode & 0x0FFF);
    instruction->targetIdx(0);
    return true;
}

bool Chip8Assembler::decode2xxx(u16 opcode, Instruction *instruction) const
{
    instruction->setMnemonic("call");
    instruction->imm(opcode & 0x0FFF);
    instruction->targetIdx(0);
    return true;
}

bool Chip8Assembler::decode3xxx(u16 opcode, Instruction *instruction) const
{
    instruction->setMnemonic("ske");
    instruction->reg((opcode & 0x0F00) >> 8);
    instruction->imm(opcode & 0x00FF);
    instruction->target(instruction->endAddress() + instruction->size());
    return true;
}

bool Chip8Assembler::decode4xxx(u16 opcode, Instruction *instruction) const
{
    instruction->setMnemonic("skne");
    instruction->reg((opcode & 0x0F00) >> 8);
    instruction->imm(opcode & 0x00FF);
    instruction->target(instruction->endAddress() + instruction->size());
    return true;
}

bool Chip8Assembler::decode5xxx(u16 opcode, Instruction *instruction) const
{
    if((opcode & 0x000F) != 0)
        return false;

    instruction->setMnemonic("ske");
    instruction->reg((opcode & 0x0F00) >> 8);
    instruction->reg((opcode & 0x00F0) >> 4);
    instruction->target(instruction->endAddress() + instruction->size());
    return true;
}

bool Chip8Assembler::decode6xxx(u16 opcode, Instruction* instruction) const
{
    instruction->setMnemonic("mov");
    instruction->reg((opcode & 0x0F00) >> 8);
    instruction->cnst(opcode & 0x00FF);
    return true;
}

bool Chip8Assembler::decode7xxx(u16 opcode, Instruction* instruction) const
{
    instruction->setMnemonic("add");
    instruction->reg((opcode & 0x0F00) >> 8);
    instruction->cnst(opcode & 0x00FF);
    return true;
}

bool Chip8Assembler::decode8xxx(u16 opcode, Instruction* instruction) const
{
    u8 op = opcode & 0x000F;

    if(op == 0x0)
        instruction->setMnemonic("mov");
    else if(op == 0x1)
        instruction->setMnemonic("or");
    else if(op == 0x2)
        instruction->setMnemonic("and");
    else if(op == 0x3)
        instruction->setMnemonic("xor");
    else if(op == 0x4)
        instruction->setMnemonic("add");
    else if(op == 0x5)
        instruction->setMnemonic("sub");
    else if(op == 0x6)
        instruction->setMnemonic("shr");
    else if(op == 0x7)
        instruction->setMnemonic("sub");
    else if(op == 0xE)
        instruction->setMnemonic("shl");
    else
        return false;

    instruction->reg((opcode & 0x0F00) >> 8);

    if((op != 0x6) && (op != 0xE)) // Skip 2nd operand if op == shift_instructions
        instruction->reg((opcode & 0x00F0) >> 4);

    return true;
}

bool Chip8Assembler::decode9xxx(u16 opcode, Instruction* instruction) const
{
    if((opcode & 0x000F) != 0)
        return false;

    instruction->setMnemonic("skne");
    instruction->reg((opcode & 0x0F00) >> 8);
    instruction->reg((opcode & 0x00F0) >> 4);
    instruction->target(instruction->endAddress() + instruction->size());
    return true;
}

bool Chip8Assembler::decodeAxxx(u16 opcode, Instruction* instruction) const
{
    instruction->setMnemonic("mov");
    instruction->reg(CHIP8_REG_I_ID, CHIP8_REG_I);
    instruction->cnst(opcode & 0x0FFF);
    return true;
}

bool Chip8Assembler::decodeBxxx(u16 opcode, Instruction* instruction) const
{
    instruction->setMnemonic("jmp");
    instruction->disp(CHIP8_REG_V0_ID, opcode & 0x0FFF);
    return true;
}

bool Chip8Assembler::decodeCxxx(u16 opcode, Instruction* instruction) const
{
    instruction->setMnemonic("rand");
    instruction->reg((opcode & 0x0F00) >> 8);
    instruction->cnst(opcode & 0x00FF);
    return true;
}

bool Chip8Assembler::decodeDxxx(u16 opcode, Instruction* instruction) const
{
    instruction->setMnemonic("draw");
    instruction->reg((opcode & 0x0F00) >> 8);
    instruction->reg((opcode & 0x00F0) >> 4);
    instruction->cnst(opcode & 0x000F);
    return true;
}

bool Chip8Assembler::decodeExxx(u16 opcode, Instruction* instruction) const
{
    u16 op = opcode & 0xFF;

    if(op == 0x9E)
        instruction->setMnemonic("skp");
    else if(op == 0xA1)
        instruction->setMnemonic("sknp");

    instruction->reg((opcode & 0x0F00) >> 8, CHIP8_REG_K);
    instruction->target(instruction->endAddress() + instruction->size());
    return true;
}

bool Chip8Assembler::decodeFxxx(u16 opcode, Instruction* instruction) const
{
    u16 op = opcode & 0x00FF;

    if(op == 0x07)
        instruction->setMnemonic("gdelay");
    else if(op == 0x0A)
        instruction->setMnemonic("wkey");
    else if(op == 0x15)
        instruction->setMnemonic("sdelay");
    else if(op == 0x18)
        instruction->setMnemonic("ssound");
    else if(op == 0x1E)
    {
        instruction->setMnemonic("add");
        instruction->reg(CHIP8_REG_I_ID, CHIP8_REG_I);
    }
    else if(op == 0x29)
        instruction->setMnemonic("font");
    else if(op == 0x30) // SuperChip only
        instruction->setMnemonic("xfont");
    else if(op == 0x33)
        instruction->setMnemonic("bcd");
    else if(op == 0x55)
        instruction->setMnemonic("stra");
    else if(op == 0x65)
        instruction->setMnemonic("ldra");
    else
        return false;

    instruction->reg((opcode & 0x0F00) >> 8);
    return true;
}
