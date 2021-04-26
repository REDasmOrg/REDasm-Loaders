#pragma once

#include <rdapi/types.h>

enum: u32 {
    DalvikOp_VNormal = 0,
    DalvikOp_VMethodIndex,
    DalvikOp_VTypeIndex,
    DalvikOp_VStringIndex,
    DalvikOp_VFieldIndex,
    DalvikOp_VPackedSwitchTable,
    DalvikOp_VSparseSwitchTable,
    DalvikOp_VFillArrayData,

    DalvikOp_ParameterFirst = 0x1000,
    DalvikOp_ParameterLast  = 0x2000,
    DalvikOp_ParameterThis  = 0x4000
};
