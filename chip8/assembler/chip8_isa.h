#pragma once

#include <rdapi/rdapi.h>
#include <string>
#include <array>

enum CHIP8Registers {
    CHIP8Register_V0 = 0,
    CHIP8Register_V1,
    CHIP8Register_V2,
    CHIP8Register_V3,
    CHIP8Register_V4,
    CHIP8Register_V5,
    CHIP8Register_V6,
    CHIP8Register_V7,
    CHIP8Register_V8,
    CHIP8Register_V9,
    CHIP8Register_VA,
    CHIP8Register_VB,
    CHIP8Register_VC,
    CHIP8Register_VD,
    CHIP8Register_VE,
    CHIP8Register_VF,

};

enum CHIP8Opcodes {
    CHIP8Op_Unknown = 0,
    CHIP8Opcode_Nop, CHIP8Opcode_Erase, CHIP8Opcode_Return,
    CHIP8Opcode_Goto, CHIP8Opcode_GotoRel, CHIP8Opcode_Do,
    CHIP8Opcode_Ske, CHIP8Opcode_Skne, CHIP8Opcode_Ske_Key, CHIP8Opcode_Skne_Key,
    CHIP8Opcode_Copy, CHIP8Opcode_Copy2,
    CHIP8Opcode_And, CHIP8Opcode_Add, CHIP8Opcode_Sub, CHIP8Opcode_Or, CHIP8Opcode_Xor,
    CHIP8Opcode_Stop, CHIP8Opcode_Rand,
    CHIP8Opcode_SetI, CHIP8Opcode_SetTime, CHIP8Opcode_SetPitch, CHIP8Opcode_SetTone,
    CHIP8Opcode_GetTime, CHIP8Opcode_GetKey,
    CHIP8Opcode_Show,
};

enum CHIP8OperandType {
    CHIP8Operand_Null,
    CHIP8Operand_Register,
    CHIP8Operand_Register_I,
    CHIP8Operand_Register_MI,
    CHIP8Operand_Register_TIME,
    CHIP8Operand_Register_PITCH,
    CHIP8Operand_Register_TONE,
    CHIP8Operand_Register_KEY,
    CHIP8Operand_Register_DSP,
    CHIP8Operand_Register_DEQ,
    CHIP8Operand_Register_RS485,
    CHIP8Operand_Register_BAUD,
    CHIP8Operand_Register_Range,
    CHIP8Operand_Constant,
    CHIP8Operand_Address,
};

struct CHIP8Operand {
    rd_type type{CHIP8Operand_Null};

    union {
        u16 cnst{0};
        u16 addr;
        u16 reg;
        u16 reg1;
    };

    u16 reg2;
};

struct CHIP8Instruction {
    rd_type op{CHIP8Op_Unknown};
    u16 opcode;
    std::string mnemonic;
    std::array<CHIP8Operand, 3> operands{ };
};
