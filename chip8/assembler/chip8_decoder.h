#pragma once

/*
 * References:
 * - https://storage.googleapis.com/wzukusers/user-34724694/documents/5c83d6a5aec8eZ0cT194/CHIP-8%20Classic%20Manual%20Rev%201.3.pdf
 * - http://www.multigesture.net/wp-content/uploads/mirror/goldroad/chip8_instruction_set.shtml
 * - https://en.wikipedia.org/wiki/CHIP-8
 * - https://massung.github.io/CHIP-8
 */

#include <array>
#include <functional>
#include <rdapi/rdapi.h>
#include "chip8_isa.h"

class CHIP8Decoder
{
    private:
        typedef std::function<bool(u16, CHIP8Instruction*)> OpCodeCallback;

    public:
        CHIP8Decoder() = delete;

    protected:
        static bool decode(const RDBufferView* view, CHIP8Instruction* instruction);

    private:
        static inline u16 getB0(u16 opcode)  { return opcode & 0xF;        } // 000X
        static inline u16 getB1(u16 opcode)  { return (opcode >> 4) & 0xF; } // 00X0
        static inline u16 getB2(u16 opcode)  { return (opcode >> 8) & 0xF; } // 0X00
        static inline u16 getB3(u16 opcode)  { return opcode >> 12;        } // X000
        static inline u16 getKK(u16 opcode)  { return opcode & 0x00FF;     } // 00KK
        static inline u16 getMMM(u16 opcode) { return opcode & 0x0FFF;     } // 0MMM

    private:
        static bool decode0xxx(u16 opcode, CHIP8Instruction* instruction);
        static bool decode1xxx(u16 opcode, CHIP8Instruction* instruction);
        static bool decode2xxx(u16 opcode, CHIP8Instruction* instruction);
        static bool decode3xxx(u16 opcode, CHIP8Instruction* instruction);
        static bool decode4xxx(u16 opcode, CHIP8Instruction* instruction);
        static bool decode5xxx(u16 opcode, CHIP8Instruction* instruction);
        static bool decode6xxx(u16 opcode, CHIP8Instruction* instruction);
        static bool decode7xxx(u16 opcode, CHIP8Instruction* instruction);
        static bool decode8xxx(u16 opcode, CHIP8Instruction* instruction);
        static bool decode9xxx(u16 opcode, CHIP8Instruction* instruction);
        static bool decodeAxxx(u16 opcode, CHIP8Instruction* instruction);
        static bool decodeBxxx(u16 opcode, CHIP8Instruction* instruction);
        static bool decodeCxxx(u16 opcode, CHIP8Instruction* instruction);
        static bool decodeDxxx(u16 opcode, CHIP8Instruction* instruction);
        static bool decodeExxx(u16 opcode, CHIP8Instruction* instruction);
        static bool decodeFxxx(u16 opcode, CHIP8Instruction* instruction);

    private:
        static std::array<OpCodeCallback, 0x10> m_opcodes;
};
