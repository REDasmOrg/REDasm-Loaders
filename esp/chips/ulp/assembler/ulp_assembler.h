#pragma once

#include <rdapi/rdapi.h>
#include "ulp_opcodes.h"
#include <array>

class ULPAssembler
{
    public:
        ULPAssembler() = delete;
        static bool decode(const RDBufferView* view, ULPInstruction* ulpinstr);
        static void renderInstruction(RDContext*, const RDRendererParams* rp);
        static void emulate(RDContext*, RDEmulateResult* result);

    private:
        static std::string regName(u8 reg);

    private:
        static void emulateJmp(const ULPInstruction* ulpinstr, RDEmulateResult* result);

    private:
        static void renderAlu(const ULPInstruction* ulpinstr, const RDRendererParams* rp);
        static void renderAluReg(const ULPInstruction* ulpinstr, const RDRendererParams* rp);
        static void renderAluImm(const ULPInstruction* ulpinstr, const RDRendererParams* rp);
        static void renderAluStage(const ULPInstruction* ulpinstr, const RDRendererParams* rp);
        static void renderStore(const ULPInstruction* ulpinstr, const RDRendererParams* rp);
        static void renderLoad(const ULPInstruction* ulpinstr, const RDRendererParams* rp);
        static void renderJmp(const ULPInstruction* ulpinstr, const RDRendererParams* rp);
        static void renderWakeSleep(const ULPInstruction* ulpinstr, const RDRendererParams* rp);
        static void renderWait(const ULPInstruction* ulpinstr, const RDRendererParams* rp);
        static void renderTSens(const ULPInstruction* ulpinstr, const RDRendererParams* rp);
        static void renderAdc(const ULPInstruction* ulpinstr, const RDRendererParams* rp);
        static void renderI2C(const ULPInstruction* ulpinstr, const RDRendererParams* rp);
        static void renderRegRD(const ULPInstruction* ulpinstr, const RDRendererParams* rp);
        static void renderRegWR(const ULPInstruction* ulpinstr, const RDRendererParams* rp);

    private:
        static std::array<const char*, 7> ALUSEL;
};
