#pragma once

#define DEXLOADER_USERDATA          "dex_userdata"

#define DEX_FILE_MAGIC              "dex"
#define DEX_ENDIAN_CONSTANT         0x12345678
#define DEX_REVERSE_ENDIAN_CONSTANT 0x78563412
#define DEX_NO_INDEX                static_cast<s32>(-1)
#define DEX_NO_INDEX_U              static_cast<u32>(DEX_NO_INDEX)

enum {
    DexAccessFlag_Public               = 0x1,
    DexAccessFlag_Private              = 0x2,
    DexAccessFlag_Protected            = 0x4,
    DexAccessFlag_Static               = 0x8,
    DexAccessFlag_Final                = 0x10,
    DexAccessFlag_Synchronized         = 0x20,
    DexAccessFlag_Volatile             = 0x40,
    DexAccessFlag_Bridge               = 0x40,
    DexAccessFlag_Transient            = 0x80,
    DexAccessFlag_VarArgs              = 0x80,
    DexAccessFlag_Native               = 0x100,
    DexAccessFlag_Interface            = 0x200,
    DexAccessFlag_Abstract             = 0x400,
    DexAccessFlag_Strict               = 0x800,
    DexAccessFlag_Synthetic            = 0x1000,
    DexAccessFlag_Annotation           = 0x2000,
    DexAccessFlag_Enum                 = 0x8000,
    DexAccessFlag_Constructor          = 0x10000,
    DexAccessFlag_DeclaredSynchronized = 0x20000,
};

enum {
    DexValueFormat_Byte         = 0x00,
    DexValueFormat_Short        = 0x02,
    DexValueFormat_Char         = 0x03,
    DexValueFormat_Int          = 0x04,
    DexValueFormat_Long         = 0x06,
    DexValueFormat_Float        = 0x10,
    DexValueFormat_Double       = 0x11,
    DexValueFormat_MethodType   = 0x15,
    DexValueFormat_MethodHandle = 0x16,

    DexValueFormat_String       = 0x17,
    DexValueFormat_Type         = 0x18,
    DexValueFormat_Field        = 0x19,
    DexValueFormat_Method       = 0x1a,
    DexValueFormat_Enum         = 0x1b,

    DexValueFormat_Array        = 0x1c,
    DexValueFormat_Annotation   = 0x1d,
    DexValueFormat_Null         = 0x1e,
    DexValueFormat_Boolean      = 0x1f,
};

enum {
    DexTypeCode_Header               = 0x0000,
    DexTypeCode_StringId             = 0x0001,
    DexTypeCode_TypeId               = 0x0002,
    DexTypeCode_ProtoId              = 0x0003,
    DexTypeCode_FieldId              = 0x0004,
    DexTypeCode_MethodId             = 0x0005,
    DexTypeCode_ClassDef             = 0x0006,
    DexTypeCode_CallSiteId           = 0x0007,
    DexTypeCode_MethodHandle         = 0x0008,
    DexTypeCode_MapList              = 0x1000,
    DexTypeCode_TypeList             = 0x1001,
    DexTypeCode_AnnotationSetRefList = 0x1002,
    DexTypeCode_AnnotationSet        = 0x1003,
    DexTypeCode_ClassData            = 0x2000,
    DexTypeCode_Code                 = 0x2001,
    DexTypeCode_StringData           = 0x2002,
    DexTypeCode_DebugInfo            = 0x2003,
    DexTypeCode_Annotation           = 0x2004,
    DexTypeCode_EncodedArray         = 0x2005,
    DexTypeCode_AnnotationsDirectory = 0x2006,
};
