#pragma once

/*
 * References:
 * - http://www.multigesture.net/wp-content/uploads/mirror/goldroad/chip8_instruction_set.shtml
 * - https://en.wikipedia.org/wiki/CHIP-8
 * - https://massung.github.io/CHIP-8
 */

#include <unordered_map>
#include <functional>
#include <rdapi/rdapi.h>
#include "chip8_registers.h"

class Chip8Assembler
{
    private:
        typedef std::function<bool(u16, RDInstruction*)> OpCodeCallback;

    public:
        Chip8Assembler();
        void emulate(RDDisassembler* disassembler, const RDInstruction* instruction);
        bool decode(RDBufferView* view, RDInstruction* instruction);

    private:
        void categorize(RDInstruction* instruction);

    private:
        bool decode0xxx(u16 opcode, RDInstruction* instruction) const;
        bool decode1xxx(u16 opcode, RDInstruction* instruction) const;
        bool decode2xxx(u16 opcode, RDInstruction* instruction) const;
        bool decode3xxx(u16 opcode, RDInstruction* instruction) const;
        bool decode4xxx(u16 opcode, RDInstruction* instruction) const;
        bool decode5xxx(u16 opcode, RDInstruction* instruction) const;
        bool decode6xxx(u16 opcode, RDInstruction* instruction) const;
        bool decode7xxx(u16 opcode, RDInstruction* instruction) const;
        bool decode8xxx(u16 opcode, RDInstruction* instruction) const;
        bool decode9xxx(u16 opcode, RDInstruction* instruction) const;
        bool decodeAxxx(u16 opcode, RDInstruction* instruction) const;
        bool decodeBxxx(u16 opcode, RDInstruction* instruction) const;
        bool decodeCxxx(u16 opcode, RDInstruction* instruction) const;
        bool decodeDxxx(u16 opcode, RDInstruction* instruction) const;
        bool decodeExxx(u16 opcode, RDInstruction* instruction) const;
        bool decodeFxxx(u16 opcode, RDInstruction* instruction) const;

    private:
        std::unordered_map<u16, OpCodeCallback> m_opcodes;
};
