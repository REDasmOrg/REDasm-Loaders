#pragma once

/*
 * References:
 * - http://www.multigesture.net/wp-content/uploads/mirror/goldroad/chip8_instruction_set.shtml
 * - https://en.wikipedia.org/wiki/CHIP-8
 * - https://massung.github.io/CHIP-8
 */

#include <unordered_map>
#include <redasm/redasm.h>
#include <redasm/plugins/assembler/assembler.h>
#include "chip8_registers.h"

using namespace REDasm;

class Chip8Assembler : public Assembler
{
    private:
        typedef std::function<bool(u16, Instruction*)> OpCodeCallback;

    public:
        Chip8Assembler();
        size_t bits() const override;

    protected:
        Printer* doCreatePrinter() const override;
        bool decodeInstruction(const BufferView &view, Instruction* instruction) override;
        void onDecoded(Instruction* instruction) override;

    private:
        bool decode0xxx(u16 opcode, Instruction* instruction) const;
        bool decode1xxx(u16 opcode, Instruction* instruction) const;
        bool decode2xxx(u16 opcode, Instruction* instruction) const;
        bool decode3xxx(u16 opcode, Instruction* instruction) const;
        bool decode4xxx(u16 opcode, Instruction* instruction) const;
        bool decode5xxx(u16 opcode, Instruction* instruction) const;
        bool decode6xxx(u16 opcode, Instruction* instruction) const;
        bool decode7xxx(u16 opcode, Instruction* instruction) const;
        bool decode8xxx(u16 opcode, Instruction* instruction) const;
        bool decode9xxx(u16 opcode, Instruction* instruction) const;
        bool decodeAxxx(u16 opcode, Instruction* instruction) const;
        bool decodeBxxx(u16 opcode, Instruction* instruction) const;
        bool decodeCxxx(u16 opcode, Instruction* instruction) const;
        bool decodeDxxx(u16 opcode, Instruction* instruction) const;
        bool decodeExxx(u16 opcode, Instruction* instruction) const;
        bool decodeFxxx(u16 opcode, Instruction* instruction) const;

    private:
        std::unordered_map<u16, OpCodeCallback> m_opcodes;
};
