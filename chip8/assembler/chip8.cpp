#include "chip8.h"

void CHIP8::renderInstruction(RDContext*, const RDRendererParams* rp)
{
    //RDSurfaceRenderer_HexDump(rp->renderer, &rp->view, sizeof(u16));

    CHIP8Instruction instruction;
    if(!CHIP8Decoder::decode(&rp->view, &instruction)) return;

    if(instruction.mnemonic.empty())
    {
        switch(instruction.op)
        {
            case CHIP8Opcode_Copy:
            case CHIP8Opcode_SetI:
            case CHIP8Opcode_SetTime:
            case CHIP8Opcode_SetTone:
            case CHIP8Opcode_SetPitch:
            case CHIP8Opcode_GetTime:
            case CHIP8Opcode_GetKey:
                CHIP8::renderCopy(&instruction, rp);
                break;

            case CHIP8Opcode_Copy2:
                CHIP8::renderCopy2(&instruction, rp);
                break;

            case CHIP8Opcode_Add:
            case CHIP8Opcode_Sub:
            case CHIP8Opcode_And:
            case CHIP8Opcode_Xor:
            case CHIP8Opcode_Or:
                CHIP8::renderMath(&instruction, rp);
                break;

            default: RDRenderer_Text(rp->renderer, "???"); break;
        }

        return;
    }

    if(instruction.op == CHIP8Opcode_Return) RDRenderer_Mnemonic(rp->renderer, instruction.mnemonic.c_str(), Theme_Ret);
    else RDRenderer_Mnemonic(rp->renderer, instruction.mnemonic.c_str(), Theme_Default);
    RDRenderer_Text(rp->renderer, " ");

    for(size_t i = 0; i < instruction.operands.size(); i++)
    {
        const CHIP8Operand* op = &instruction.operands[i];

        if(IS_TYPE(op, CHIP8Operand_Null)) break;
        if(i) RDRenderer_Text(rp->renderer, ", ");
        CHIP8::renderOperand(rp, op);
    }
}

void CHIP8::emulate(RDContext* ctx, RDEmulateResult* result)
{
    CHIP8Instruction instruction;
    if(!CHIP8Decoder::decode(RDEmulateResult_GetView(result), &instruction)) return;

    if((instruction.opcode == 0x00EE) || (instruction.opcode == 0xF000))
        RDEmulateResult_AddReturn(result);
    else
    {
        rd_address address = RDEmulateResult_GetAddress(result);
        u16 op = instruction.opcode & 0xF000;

        switch(op)
        {
            case 0x1000: RDEmulateResult_AddBranch(result, instruction.opcode & 0x0FFF); break;
            case 0x2000: RDEmulateResult_AddCall(result, instruction.opcode & 0x0FFF); break;

            case 0x3000:
            case 0x4000:
            case 0x5000:
            case 0x9000:
            case 0xE000:
                RDEmulateResult_AddBranchTrue(result, address + (sizeof(u16) * 2));
                RDEmulateResult_AddBranchFalse(result, address + sizeof(u16));
                break;
        }
    }

    RDEmulateResult_SetSize(result, sizeof(u16));
}

void CHIP8::renderOperand(const RDRendererParams* rp, const CHIP8Operand* op)
{
    switch(op->type)
    {
        case CHIP8Operand_Register:       RDRenderer_Register(rp->renderer, ("$v" + std::to_string(op->reg)).c_str()); break;
        case CHIP8Operand_Register_I:     RDRenderer_Register(rp->renderer, "$i");                                     break;
        case CHIP8Operand_Register_MI:    RDRenderer_Register(rp->renderer, "$mi");                                    break;
        case CHIP8Operand_Register_KEY:   RDRenderer_Register(rp->renderer, "KEY");                                    break;
        case CHIP8Operand_Register_TIME:  RDRenderer_Register(rp->renderer, "TIME");                                   break;
        case CHIP8Operand_Register_TONE:  RDRenderer_Register(rp->renderer, "TONE");                                   break;
        case CHIP8Operand_Register_PITCH: RDRenderer_Register(rp->renderer, "PITCH");                                  break;
        case CHIP8Operand_Register_DEQ:   RDRenderer_Register(rp->renderer, "DEQ");                                    break;
        case CHIP8Operand_Register_DSP:   RDRenderer_Register(rp->renderer, "DSP");                                    break;
        case CHIP8Operand_Register_RS485: RDRenderer_Register(rp->renderer, "RS485");                                  break;
        case CHIP8Operand_Register_BAUD:  RDRenderer_Register(rp->renderer, "BAUD");                                   break;
        case CHIP8Operand_Constant:       RDRenderer_Constant(rp->renderer, RD_ToHex(op->cnst));                       break;
        case CHIP8Operand_Address:        RDRenderer_Unsigned(rp->renderer, op->addr);                                 break;

        case CHIP8Operand_Register_Range:
            RDRenderer_Register(rp->renderer, ("$v" + std::to_string(op->reg1)).c_str());
            RDRenderer_Text(rp->renderer, ":");
            RDRenderer_Register(rp->renderer, ("$v" + std::to_string(op->reg2)).c_str());
            break;

        default: RDRenderer_Text(rp->renderer, "???"); break;
    }
}

void CHIP8::renderCopy(CHIP8Instruction* instruction, const RDRendererParams* rp)
{
    CHIP8::renderOperand(rp, &instruction->operands[0]);
    RDRenderer_Text(rp->renderer, " = ");
    CHIP8::renderOperand(rp, &instruction->operands[1]);
}

void CHIP8::renderCopy2(CHIP8Instruction* instruction, const RDRendererParams* rp)
{
    CHIP8::renderOperand(rp, &instruction->operands[0]);
    RDRenderer_Text(rp->renderer, " = ");
    CHIP8::renderOperand(rp, &instruction->operands[1]);
    RDRenderer_Text(rp->renderer, ", ");
    CHIP8::renderOperand(rp, &instruction->operands[2]);
}

void CHIP8::renderMath(CHIP8Instruction* instruction, const RDRendererParams* rp)
{
    CHIP8::renderOperand(rp, &instruction->operands[0]);
    RDRenderer_Text(rp->renderer, " = ");
    CHIP8::renderOperand(rp, &instruction->operands[0]);

    switch(instruction->op)
    {
        case CHIP8Opcode_Add: RDRenderer_Text(rp->renderer, " + "); break;
        case CHIP8Opcode_Sub: RDRenderer_Text(rp->renderer, " - "); break;
        case CHIP8Opcode_And: RDRenderer_Text(rp->renderer, " & "); break;
        case CHIP8Opcode_Xor: RDRenderer_Text(rp->renderer, " ^ "); break;
        case CHIP8Opcode_Or:  RDRenderer_Text(rp->renderer, " | "); break;
        default: RDRenderer_Text(rp->renderer, " ??? "); break;
    }

    CHIP8::renderOperand(rp, &instruction->operands[1]);
}
