#pragma once

#include <rdapi/rdapi.h>

#define PSXEXE_SIGNATURE_SIZE 8
#define PSXEXE_SIGNATURE      "PS-X EXE"

#define PSXEXE_TEXT_OFFSET    0x00000800
#define PSX_USER_RAM_START    0x80000000
#define PSX_USER_RAM_END      0x80200000

struct PsxExeHeader
{
    char id[PSXEXE_SIGNATURE_SIZE];
    u32 text, data;
    u32 pc0, gp0;
    u32 t_addr, t_size;
    u32 d_addr, d_size;
    u32 b_addr, b_size;
    u32 s_addr, s_size;
    u32 SavedSP, SavedFP, SavedGP, SavedRA, SavedS0;
};

class PsxExeLoader
{
    public:
        PsxExeLoader() = default;
        static const char* test(const RDLoaderRequest* request);
        static bool load(RDContext*, RDLoader* loader);
};
