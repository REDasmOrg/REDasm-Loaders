#pragma once

#include <redasm/redasm.h>
#include <redasm/libs/visit_struct/visit_struct.hpp>

struct GUID
{
    u32 data1;
    u16 data2, data3;
    u8 data4[8];
};

typedef u32 LCID;

VISITABLE_STRUCT(GUID, data1, data2, data3, data4);
