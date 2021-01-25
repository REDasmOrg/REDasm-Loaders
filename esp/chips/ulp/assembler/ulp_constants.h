#pragma once

#define ULP_OP_REGWR     1
#define ULP_OP_REGRD     2
#define ULP_OP_I2C       3
#define ULP_OP_WAIT      4
#define ULP_OP_ADC       5
#define ULP_OP_STORE     6
#define ULP_OP_ALU       7
#define ULP_OP_JUMP      8
#define ULP_OP_WAKESLEEP 9
#define ULP_OP_TSENS     10
#define ULP_OP_HALT      11
#define ULP_OP_LOAD      13

#define ALU_SEL_ADD  0
#define ALU_SEL_SUB  1
#define ALU_SEL_AND  2
#define ALU_SEL_OR   3
#define ALU_SEL_MOVE 4
#define ALU_SEL_LSH  5
#define ALU_SEL_RSH  6

#define DR_REG_RTCCNTL_BASE 0x3ff48000
#define DR_REG_RTCIO_BASE   0x3ff48400
#define DR_REG_SENS_BASE    0x3ff48800
#define DR_REG_RTC_I2C_BASE 0x3ff48C00
