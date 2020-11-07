#pragma once

#include "chip8_decoder.h"

class CHIP8: public CHIP8Decoder
{
    public:
        CHIP8() = delete;
        static void renderInstruction(RDContext*, const RDRendererParams* rp);
        static void emulate(RDContext* ctx, RDEmulateResult* result);

    private:
        static void renderOperand(const RDRendererParams* rp, const CHIP8Operand* op);

    private:
        static void renderCopy(CHIP8Instruction* instruction, const RDRendererParams* rp);
        static void renderCopy2(CHIP8Instruction* instruction, const RDRendererParams* rp);
        static void renderMath(CHIP8Instruction* instruction, const RDRendererParams* rp);
};
