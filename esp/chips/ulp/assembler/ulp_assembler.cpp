#include "ulp_assembler.h"
#include "ulp_constants.h"

#define WORDS_TO_ADDR(instr) (instr) * sizeof(ULPInstruction)

std::array<const char*, 7> ULPAssembler::ALUSEL = {
    "add", "sub", "and", "or", "move", "lsh", "rsh"
};

bool ULPAssembler::decode(const RDBufferView* view, ULPInstruction* ulpinstr)
{
    if(!ulpinstr || (view->size < sizeof(ULPInstruction))) return false;
    ulpinstr->data = RD_FromLittleEndian32(*reinterpret_cast<u32*>(view->data));
    return true;
}

void ULPAssembler::renderInstruction(RDContext*, const RDRendererParams* rp)
{
    ULPInstruction ulpinstr;
    if(!ULPAssembler::decode(&rp->view, &ulpinstr)) return;

    switch(ulpinstr.unk.opcode)
    {
        case ULP_OP_ALU: ULPAssembler::renderAlu(&ulpinstr, rp); break;
        case ULP_OP_STORE: ULPAssembler::renderStore(&ulpinstr, rp); break;
        case ULP_OP_LOAD: ULPAssembler::renderLoad(&ulpinstr, rp); break;
        case ULP_OP_JUMP: ULPAssembler::renderJmp(&ulpinstr, rp); break;
        case ULP_OP_HALT: RDRenderer_Mnemonic(rp->renderer, "halt", Theme_Ret); break;
        case ULP_OP_WAKESLEEP: ULPAssembler::renderWakeSleep(&ulpinstr, rp); break;
        case ULP_OP_WAIT: ULPAssembler::renderWait(&ulpinstr, rp); break;
        case ULP_OP_TSENS: ULPAssembler::renderTSens(&ulpinstr, rp); break;
        case ULP_OP_ADC: ULPAssembler::renderAdc(&ulpinstr, rp); break;
        case ULP_OP_I2C: ULPAssembler::renderI2C(&ulpinstr, rp); break;
        case ULP_OP_REGRD: ULPAssembler::renderRegRD(&ulpinstr, rp); break;
        case ULP_OP_REGWR: ULPAssembler::renderRegWR(&ulpinstr, rp); break;
        default: RDRenderer_Unknown(rp->renderer); break;
    }
}

void ULPAssembler::emulate(RDContext*, RDEmulateResult* result)
{
    ULPInstruction ulpinstr;
    if(!ULPAssembler::decode(RDEmulateResult_GetView(result), &ulpinstr)) return;
    RDEmulateResult_SetSize(result, sizeof(ULPInstruction));

    switch(ulpinstr.unk.opcode)
    {
        case ULP_OP_JUMP: ULPAssembler::emulateJmp(&ulpinstr, result); break;
        case ULP_OP_HALT: RDEmulateResult_AddReturn(result); break;
        default: break;
    }
}

std::string ULPAssembler::regName(u8 reg) { return "r" + std::to_string(reg); }

void ULPAssembler::emulateJmp(const ULPInstruction* ulpinstr, RDEmulateResult* result)
{
    rd_address address = RDEmulateResult_GetAddress(result);

    switch(ulpinstr->unk.choice)
    {
        case 0: {
            if(!ulpinstr->jump.sel) {
                if(ulpinstr->jump.type) RDEmulateResult_AddBranchTrue(result, ulpinstr->jump.addr);
                else RDEmulateResult_AddBranch(result, WORDS_TO_ADDR(ulpinstr->jump.addr));
            }
            else RDEmulateResult_AddBranchIndirect(result);

            if(ulpinstr->jump.type) RDEmulateResult_AddBranchFalse(result, address + sizeof(ULPInstruction));
            break;
        }

        case 1:
        case 2: {
            rd_address relative = WORDS_TO_ADDR(ulpinstr->jumpr.step & 0x7F);
            int sign = ulpinstr->jumpr.step & 0x80 ? -1 : +1;
            RDEmulateResult_AddBranchTrue(result, address + sign * relative);
            RDEmulateResult_AddBranchFalse(result, address + sizeof(ULPInstruction));
            break;
        }

        default: break;
    }
}

void ULPAssembler::renderAlu(const ULPInstruction* ulpinstr, const RDRendererParams* rp)
{
    switch(ulpinstr->unk.choice)
    {
        case 0: ULPAssembler::renderAluReg(ulpinstr, rp); break;
        case 1: ULPAssembler::renderAluImm(ulpinstr, rp); break;
        case 2: ULPAssembler::renderAluStage(ulpinstr, rp); break;
        default: break;
    }
}

void ULPAssembler::renderAluReg(const ULPInstruction* ulpinstr, const RDRendererParams* rp)
{
    RDRenderer_MnemonicWord(rp->renderer, ALUSEL[ulpinstr->alureg.sel], Theme_Default);
    RDRenderer_Register(rp->renderer, ULPAssembler::regName(ulpinstr->alureg.rdst).c_str());
    RDRenderer_Text(rp->renderer, ", ");
    RDRenderer_Register(rp->renderer, ULPAssembler::regName(ulpinstr->alureg.rsrc1).c_str());

    if(ulpinstr->alureg.sel == ALU_SEL_MOVE) return;

    RDRenderer_Text(rp->renderer, ", ");
    RDRenderer_Register(rp->renderer, ULPAssembler::regName(ulpinstr->alureg.rsrc2).c_str());
}

void ULPAssembler::renderAluImm(const ULPInstruction* ulpinstr, const RDRendererParams* rp)
{
    RDRenderer_MnemonicWord(rp->renderer, ALUSEL[ulpinstr->aluimm.sel], Theme_Default);

    RDRenderer_Register(rp->renderer, ULPAssembler::regName(ulpinstr->aluimm.rdst).c_str());

    if(ulpinstr->aluimm.sel != ALU_SEL_MOVE)
    {
        RDRenderer_Text(rp->renderer, ", ");
        RDRenderer_Register(rp->renderer, ULPAssembler::regName(ulpinstr->aluimm.rsrc1).c_str());
    }

    RDRenderer_Text(rp->renderer, ", ");
    RDRenderer_Unsigned(rp->renderer, ulpinstr->aluimm.imm);
}

void ULPAssembler::renderAluStage(const ULPInstruction* ulpinstr, const RDRendererParams* rp)
{
    static std::array<const char*, 3> STAgeSEL = {
        "stage_inc", "stage_dec", "stage_rst"
    };

    RDRenderer_MnemonicWord(rp->renderer, STAgeSEL[ulpinstr->alustage.sel], Theme_Default);
    if(ulpinstr->alustage.sel == 2) return;
    RDRenderer_Unsigned(rp->renderer, ulpinstr->alustage.imm);
}

void ULPAssembler::renderJmp(const ULPInstruction* ulpinstr, const RDRendererParams* rp)
{
    switch(ulpinstr->unk.choice)
    {
        case 0: {
            RDRenderer_MnemonicWord(rp->renderer, "jump", ulpinstr->jump.type ? Theme_JumpCond : Theme_Jump);

            if(ulpinstr->jump.sel) RDRenderer_Register(rp->renderer, ULPAssembler::regName(ulpinstr->jump.rdest).c_str());
            else RDRenderer_Reference(rp->renderer, WORDS_TO_ADDR(ulpinstr->jump.addr));

            if(ulpinstr->jump.type) {
                RDRenderer_Text(rp->renderer, ", ");
                if(ulpinstr->jump.type == 1) RDRenderer_Text(rp->renderer, "eq");
                else if(ulpinstr->jump.type == 2) RDRenderer_Text(rp->renderer, "ov");
            }

            break;
        }

        case 1: {
            rd_address relative = WORDS_TO_ADDR(ulpinstr->jumpr.step & 0x7F);
            int sign = ulpinstr->jumpr.step & 0x80 ? -1 : +1;

            RDRenderer_MnemonicWord(rp->renderer, "jumpr", Theme_JumpCond);
            RDRenderer_Reference(rp->renderer, rp->address + sign * relative);
            RDRenderer_Text(rp->renderer, ", ");
            RDRenderer_Unsigned(rp->renderer, ulpinstr->jumps.thres);

            RDRenderer_Text(rp->renderer, ", ");
            RDRenderer_Text(rp->renderer, ulpinstr->jumpr.cond ? "ge" : "lt");
            break;
        }

        case 2: {
            rd_address relative = WORDS_TO_ADDR(ulpinstr->jumps.step & 0x7F);
            int sign = ulpinstr->jumps.step & 0x80 ? -1 : +1;

            RDRenderer_MnemonicWord(rp->renderer, "jumps", Theme_JumpCond);
            RDRenderer_Unsigned(rp->renderer, rp->address + (sign * relative));
            RDRenderer_Text(rp->renderer, ", ");
            RDRenderer_Unsigned(rp->renderer, ulpinstr->jumps.thres);

            RDRenderer_Text(rp->renderer, ", ");

            if(ulpinstr->jumps.cond == 0) RDRenderer_Text(rp->renderer, "lt");
            else if(ulpinstr->jumps.cond == 1) RDRenderer_Text(rp->renderer, "gt");
            else RDRenderer_Text(rp->renderer, "eq");
            break;
        }

        default: break;
    }
}

void ULPAssembler::renderWakeSleep(const ULPInstruction* ulpinstr, const RDRendererParams* rp)
{
    if(ulpinstr->wakesleep.wakeorsleep)
    {
        RDRenderer_MnemonicWord(rp->renderer, "sleep", Theme_Default);
        RDRenderer_Unsigned(rp->renderer, ulpinstr->wakesleep.reg);
    }
    else
        RDRenderer_Mnemonic(rp->renderer, "wake", Theme_Default);
}

void ULPAssembler::renderWait(const ULPInstruction* ulpinstr, const RDRendererParams* rp)
{
    if(ulpinstr->wait.cycles)
    {
        RDRenderer_MnemonicWord(rp->renderer, "wait", Theme_Default);
        RDRenderer_Unsigned(rp->renderer, ulpinstr->wait.cycles);
    }
    else
        RDRenderer_Mnemonic(rp->renderer, "nop", Theme_Nop);
}

void ULPAssembler::renderTSens(const ULPInstruction* ulpinstr, const RDRendererParams* rp)
{
    RDRenderer_MnemonicWord(rp->renderer, "tsens", Theme_Default);
    RDRenderer_Register(rp->renderer, ULPAssembler::regName(ulpinstr->tsens.rdst).c_str());
    RDRenderer_Text(rp->renderer, ", ");
    RDRenderer_Unsigned(rp->renderer, ulpinstr->tsens.delay);
}

void ULPAssembler::renderAdc(const ULPInstruction* ulpinstr, const RDRendererParams* rp)
{
    RDRenderer_MnemonicWord(rp->renderer, "adc", Theme_Default);
    RDRenderer_Register(rp->renderer, ULPAssembler::regName(ulpinstr->adc.rdst).c_str());
    RDRenderer_Text(rp->renderer, ", ");
    RDRenderer_Unsigned(rp->renderer, ulpinstr->adc.sel);
    RDRenderer_Text(rp->renderer, ", ");
    RDRenderer_Unsigned(rp->renderer, ulpinstr->adc.sarmux);
}

void ULPAssembler::renderI2C(const ULPInstruction* ulpinstr, const RDRendererParams* rp)
{
    if(ulpinstr->i2c.rw)
    {
        RDRenderer_MnemonicWord(rp->renderer, "i2c_wr", Theme_Default);
        RDRenderer_Unsigned(rp->renderer, ulpinstr->i2c.subaddr);
        RDRenderer_Text(rp->renderer, ", ");
        RDRenderer_Unsigned(rp->renderer, ulpinstr->i2c.data);
        RDRenderer_Text(rp->renderer, ", ");
        RDRenderer_Unsigned(rp->renderer, ulpinstr->i2c.high);
        RDRenderer_Text(rp->renderer, ", ");
        RDRenderer_Unsigned(rp->renderer, ulpinstr->i2c.low);
        RDRenderer_Text(rp->renderer, ", ");
        RDRenderer_Unsigned(rp->renderer, ulpinstr->i2c.sel);
    }
    else
    {
        RDRenderer_MnemonicWord(rp->renderer, "i2c_rd", Theme_Default);
        RDRenderer_Unsigned(rp->renderer, ulpinstr->i2c.subaddr);
        RDRenderer_Text(rp->renderer, ", ");
        RDRenderer_Unsigned(rp->renderer, ulpinstr->i2c.high);
        RDRenderer_Text(rp->renderer, ", ");
        RDRenderer_Unsigned(rp->renderer, ulpinstr->i2c.low);
        RDRenderer_Text(rp->renderer, ", ");
        RDRenderer_Unsigned(rp->renderer, ulpinstr->i2c.sel);
    }
}

void ULPAssembler::renderRegRD(const ULPInstruction* ulpinstr, const RDRendererParams* rp)
{
    unsigned int address = WORDS_TO_ADDR(ulpinstr->regwr.addr) + DR_REG_RTCCNTL_BASE;
    unsigned int base;

    if(address >= DR_REG_RTC_I2C_BASE) base = DR_REG_RTC_I2C_BASE;
    else if(address >= DR_REG_SENS_BASE) base = DR_REG_SENS_BASE;
    else if(address >= DR_REG_RTCIO_BASE) base = DR_REG_RTCIO_BASE;
    else base = DR_REG_RTCCNTL_BASE;

    unsigned int offset = address - base;

    RDRenderer_MnemonicWord(rp->renderer, "reg_rd", Theme_Default);

    if(offset) RDRenderer_Constant(rp->renderer, (rd_tohex(base) + "+" + rd_tohex(offset)).c_str());
    else RDRenderer_Unsigned(rp->renderer, base);

    RDRenderer_Text(rp->renderer, ", ");
    RDRenderer_Unsigned(rp->renderer, ulpinstr->regwr.high);
    RDRenderer_Text(rp->renderer, ", ");
    RDRenderer_Unsigned(rp->renderer, ulpinstr->regwr.low);
}

void ULPAssembler::renderRegWR(const ULPInstruction* ulpinstr, const RDRendererParams* rp)
{
    unsigned int address = WORDS_TO_ADDR(ulpinstr->regwr.addr) + DR_REG_RTCCNTL_BASE;
    unsigned int base;

    if(address >= DR_REG_RTC_I2C_BASE) base = DR_REG_RTC_I2C_BASE;
    else if(address >= DR_REG_SENS_BASE) base = DR_REG_SENS_BASE;
    else if(address >= DR_REG_RTCIO_BASE) base = DR_REG_RTCIO_BASE;
    else base = DR_REG_RTCCNTL_BASE;

    unsigned int offset = address - base;

    RDRenderer_MnemonicWord(rp->renderer, "reg_wr", Theme_Default);

    if(offset) RDRenderer_Constant(rp->renderer, (rd_tohex(base) + "+" + rd_tohex(offset)).c_str());
    else RDRenderer_Unsigned(rp->renderer, base);

    RDRenderer_Text(rp->renderer, ", ");
    RDRenderer_Unsigned(rp->renderer, ulpinstr->regwr.high);
    RDRenderer_Text(rp->renderer, ", ");
    RDRenderer_Unsigned(rp->renderer, ulpinstr->regwr.low);
    RDRenderer_Text(rp->renderer, ", ");
    RDRenderer_Unsigned(rp->renderer, ulpinstr->regwr.data);
}

void ULPAssembler::renderStore(const ULPInstruction* ulpinstr, const RDRendererParams* rp)
{
    RDRenderer_MnemonicWord(rp->renderer, "st", Theme_Default);
    RDRenderer_Register(rp->renderer, ULPAssembler::regName(ulpinstr->ld.rdst).c_str());
    RDRenderer_Text(rp->renderer, ", ");
    RDRenderer_Register(rp->renderer, ULPAssembler::regName(ulpinstr->ld.rsrc).c_str());
    RDRenderer_Text(rp->renderer, ", ");
    RDRenderer_Signed(rp->renderer, ulpinstr->ld.offset);
}

void ULPAssembler::renderLoad(const ULPInstruction* ulpinstr, const RDRendererParams* rp)
{
    RDRenderer_MnemonicWord(rp->renderer, "ld", Theme_Default);
    RDRenderer_Register(rp->renderer, ULPAssembler::regName(ulpinstr->ld.rdst).c_str());
    RDRenderer_Text(rp->renderer, ", ");
    RDRenderer_Register(rp->renderer, ULPAssembler::regName(ulpinstr->ld.rsrc).c_str());
    RDRenderer_Text(rp->renderer, ", ");
    RDRenderer_Signed(rp->renderer, ulpinstr->ld.offset);
}
