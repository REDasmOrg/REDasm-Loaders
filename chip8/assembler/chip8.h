#pragma once

#include "chip8_decoder.h"

class CHIP8: public CHIP8Decoder
{
    public:
        CHIP8() = delete;
        static void renderInstruction(RDContext*, const RDRenderItemParams* rip);
        static void emulate(RDContext* ctx, RDEmulateResult* result);

    private:
        static void renderOperand(const RDRenderItemParams* rip, const CHIP8Operand* op);

    private:
        static void renderCopy(CHIP8Instruction* instruction, const RDRenderItemParams* rip);
        static void renderCopy2(CHIP8Instruction* instruction, const RDRenderItemParams* rip);
        static void renderMath(CHIP8Instruction* instruction, const RDRenderItemParams* rip);
};
