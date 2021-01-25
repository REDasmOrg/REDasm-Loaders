#pragma once

/* References:
 *  - https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/ulp_instruction_set.html
 *  - https://github.com/espressif/binutils-esp32ulp/blob/733446e3186d46f8a292e5e9e153c78b82e3ad23/opcodes/esp32ulp-dis.c
 *  - https://github.com/espressif/esp-idf/blob/master/components/ulp/include/esp32/ulp.h
 *  - https://www.texim-europe.com/getfile.ashx?id=114310
 */

#include <rdapi/rdapi.h>

struct UnknownFormat {
    unsigned operand: 25;
    unsigned choice: 3;
    unsigned opcode: 4;
};

struct ALUREGFormat {
    unsigned rdst: 2;
    unsigned rsrc1: 2;
    unsigned rsrc2: 2;
    unsigned dummy: 15;
    unsigned sel: 4;
    unsigned choice: 3;
    unsigned opcode: 4;
};

struct ALUIMMFormat {
    unsigned rdst: 2;
    unsigned rsrc1: 2;
    unsigned imm: 16;
    unsigned dummy1: 1;
    unsigned sel: 4;
    unsigned choice: 3;
    unsigned opcode: 4;
};

struct ALUSTAGEFormat {
    unsigned dummy: 4;
    unsigned imm: 8;
    unsigned dummy2: 9;
    unsigned sel: 4;
    unsigned choice: 3;
    unsigned opcode: 4;
};

struct LDFormat {
    unsigned int rdst: 2;
    unsigned int rsrc: 2;
    unsigned int dummy: 6;
    unsigned int offset: 11;
    unsigned int dummy2: 7;
    unsigned opcode: 4;
};

struct STFormat {
    unsigned int rdst: 2;
    unsigned int rsrc: 2;
    unsigned int null: 6;
    unsigned int offset: 11;
    unsigned int null2: 4;
    unsigned int cent: 3;
    unsigned opcode: 4;
};

struct JUMPFormat {
    unsigned rdest: 2;
    unsigned addr: 11;
    unsigned dummy: 8;
    unsigned sel: 1;
    unsigned type: 3;
    unsigned choice: 3;
    unsigned opcode: 4;
};

struct JUMPRFormat {
    unsigned thres: 16;
    unsigned cond: 1;
    unsigned step: 8;
    unsigned choice: 3;
    unsigned opcode: 4;
};

struct JUMPSFormat {
    unsigned thres: 8;
    unsigned dummy: 7;
    unsigned cond: 2;
    unsigned step: 8;
    unsigned choice: 3;
    unsigned opcode: 4;
};

struct WAKESLEEPFormat {
    unsigned reg: 4;
    unsigned dummy: 21;
    unsigned wakeorsleep: 3;
    unsigned opcode: 4;
};

struct WAITFormat {
    unsigned cycles: 16;
    unsigned dummy: 12;
    unsigned opcode: 4;
};

struct TSENSFormat {
    unsigned rdst: 2;
    unsigned delay: 14;
    unsigned dummy: 12;
    unsigned opcode: 4;
};

struct ADCFormat {
    unsigned rdst: 2;
    unsigned sarmux: 4;
    unsigned sel: 1;
    unsigned dummy: 21;
    unsigned opcode: 4;
};

struct I2CFormat {
    unsigned subaddr: 8;
    unsigned data: 8;
    unsigned low: 3;
    unsigned high: 3;
    unsigned sel: 4;
    unsigned dummy: 1;
    unsigned rw: 1;
    unsigned opcode: 4;
};

struct REGRDFormat {
    unsigned addr: 10;
    unsigned dummy: 8;
    unsigned low: 5;
    unsigned high: 5;
    unsigned opcode: 4;
};

struct REGWRFormat {
    unsigned addr: 10;
    unsigned data: 8;
    unsigned low: 5;
    unsigned high: 5;
    unsigned opcode: 4;
};

union ULPInstruction {
    u32 data;
    UnknownFormat unk;

    ALUREGFormat alureg;
    ALUIMMFormat aluimm;
    ALUSTAGEFormat alustage;

    STFormat st;
    LDFormat ld;

    JUMPFormat jump;
    JUMPRFormat jumpr;
    JUMPSFormat jumps;

    WAKESLEEPFormat wakesleep;
    WAITFormat wait;

    TSENSFormat tsens;
    ADCFormat adc;

    I2CFormat i2c;

    REGWRFormat regwr;
    REGRDFormat regrd;
};

static_assert(sizeof(u32) == sizeof(ULPInstruction));
