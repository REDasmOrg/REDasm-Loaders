#pragma once

#define ELF_ST_BIND(i)       ((i) >> 4)
#define ELF_ST_TYPE(i)       ((i) & 0xF)
#define ELF_ST_INFO(b,t)     (((b) << 4) + ((t) & 0xF))
#define ELF_ST_VISIBILITY(o) ((o) & 0x3)

#define EI_NIDENT 16

#define EI_MAG0         0
#define EI_MAG1         1
#define EI_MAG2         2
#define EI_MAG3         3
#define EI_CLASS        4
#define EI_DATA         5
#define EI_VERSION      6
#define EI_OSABI        7
#define EI_PAD          8

#define ELFMAG0         0x7f
#define ELFMAG1         'E'
#define ELFMAG2         'L'
#define ELFMAG3         'F'

#define ELFCLASSNONE    0
#define ELFCLASS32      1
#define ELFCLASS64      2

#define ELFDATANONE     0
#define ELFDATA2LSB     1
#define ELFDATA2MSB     2

#define EV_NONE         0
#define EV_CURRENT      1

#define EM_NONE            0 /* No machine */
#define EM_M32             1 /* AT&T WE 32100 */
#define EM_SPARC           2 /* SUN SPARC */
#define EM_386             3 /* Intel 80386 */
#define EM_68K             4 /* Motorola m68k family */
#define EM_88K             5 /* Motorola m88k family */
#define EM_IAMCU           6 /* Intel MCU */
#define EM_860             7 /* Intel 80860 */
#define EM_MIPS            8 /* MIPS R3000 big-endian */
#define EM_S370            9 /* IBM System/370 */
#define EM_MIPS_RS3_LE    10 /* MIPS R3000 little-endian */
/* reserved 11-14 */
#define EM_PARISC         15 /* HPPA */
/* reserved 16 */
#define EM_VPP500         17 /* Fujitsu VPP500 */
#define EM_SPARC32PLUS    18 /* Sun's "v8plus" */
#define EM_960            19 /* Intel 80960 */
#define EM_PPC            20 /* PowerPC */
#define EM_PPC64          21 /* PowerPC 64-bit */
#define EM_S390           22 /* IBM S390 */
#define EM_SPU            23 /* IBM SPU/SPC */
/* reserved 24-35 */
#define EM_V800           36 /* NEC V800 series */
#define EM_FR20           37 /* Fujitsu FR20 */
#define EM_RH32           38 /* TRW RH-32 */
#define EM_RCE            39 /* Motorola RCE */
#define EM_ARM            40 /* ARM */
#define EM_FAKE_ALPHA     41 /* Digital Alpha */
#define EM_SH             42 /* Hitachi SH */
#define EM_SPARCV9        43 /* SPARC v9 64-bit */
#define EM_TRICORE        44 /* Siemens Tricore */
#define EM_ARC            45 /* Argonaut RISC Core */
#define EM_H8_300         46 /* Hitachi H8/300 */
#define EM_H8_300H        47 /* Hitachi H8/300H */
#define EM_H8S            48 /* Hitachi H8S */
#define EM_H8_500         49 /* Hitachi H8/500 */
#define EM_IA_64          50 /* Intel Merced */
#define EM_MIPS_X         51 /* Stanford MIPS-X */
#define EM_COLDFIRE       52 /* Motorola Coldfire */
#define EM_68HC12         53 /* Motorola M68HC12 */
#define EM_MMA            54 /* Fujitsu MMA Multimedia Accelerator */
#define EM_PCP            55 /* Siemens PCP */
#define EM_NCPU           56 /* Sony nCPU embeeded RISC */
#define EM_NDR1           57 /* Denso NDR1 microprocessor */
#define EM_STARCORE       58 /* Motorola Start*Core processor */
#define EM_ME16           59 /* Toyota ME16 processor */
#define EM_ST100          60 /* STMicroelectronic ST100 processor */
#define EM_TINYJ          61 /* Advanced Logic Corp. Tinyj emb.fam */
#define EM_X86_64         62 /* AMD x86-64 architecture */
#define EM_PDSP           63 /* Sony DSP Processor */
#define EM_PDP10          64 /* Digital PDP-10 */
#define EM_PDP11          65 /* Digital PDP-11 */
#define EM_FX66           66 /* Siemens FX66 microcontroller */
#define EM_ST9PLUS        67 /* STMicroelectronics ST9+ 8/16 mc */
#define EM_ST7            68 /* STmicroelectronics ST7 8 bit mc */
#define EM_68HC16         69 /* Motorola MC68HC16 microcontroller */
#define EM_68HC11         70 /* Motorola MC68HC11 microcontroller */
#define EM_68HC08         71 /* Motorola MC68HC08 microcontroller */
#define EM_68HC05         72 /* Motorola MC68HC05 microcontroller */
#define EM_SVX            73 /* Silicon Graphics SVx */
#define EM_ST19           74 /* STMicroelectronics ST19 8 bit mc */
#define EM_VAX            75 /* Digital VAX */
#define EM_CRIS           76 /* Axis Communications 32-bit emb.proc */
#define EM_JAVELIN        77 /* Infineon Technologies 32-bit emb.proc */
#define EM_FIREPATH       78 /* Element 14 64-bit DSP Processor */
#define EM_ZSP            79 /* LSI Logic 16-bit DSP Processor */
#define EM_MMIX           80 /* Donald Knuth's educational 64-bit proc */
#define EM_HUANY          81 /* Harvard University machine-independent object files */
#define EM_PRISM          82 /* SiTera Prism */
#define EM_AVR            83 /* Atmel AVR 8-bit microcontroller */
#define EM_FR30           84 /* Fujitsu FR30 */
#define EM_D10V           85 /* Mitsubishi D10V */
#define EM_D30V           86 /* Mitsubishi D30V */
#define EM_V850           87 /* NEC v850 */
#define EM_M32R           88 /* Mitsubishi M32R */
#define EM_MN10300        89 /* Matsushita MN10300 */
#define EM_MN10200        90 /* Matsushita MN10200 */
#define EM_PJ             91 /* picoJava */
#define EM_OPENRISC       92 /* OpenRISC 32-bit embedded processor */
#define EM_ARC_COMPACT    93 /* ARC International ARCompact */
#define EM_XTENSA         94 /* Tensilica Xtensa Architecture */
#define EM_VIDEOCORE      95 /* Alphamosaic VideoCore */
#define EM_TMM_GPP        96 /* Thompson Multimedia General Purpose Proc */
#define EM_NS32K          97 /* National Semi. 32000 */
#define EM_TPC            98 /* Tenor Network TPC */
#define EM_SNP1K          99 /* Trebia SNP 1000 */
#define EM_ST200         100 /* STMicroelectronics ST200 */
#define EM_IP2K          101 /* Ubicom IP2xxx */
#define EM_MAX           102 /* MAX processor */
#define EM_CR            103 /* National Semi. CompactRISC */
#define EM_F2MC16        104 /* Fujitsu F2MC16 */
#define EM_MSP430        105 /* Texas Instruments msp430 */
#define EM_BLACKFIN      106 /* Analog Devices Blackfin DSP */
#define EM_SE_C33        107 /* Seiko Epson S1C33 family */
#define EM_SEP           108 /* Sharp embedded microprocessor */
#define EM_ARCA          109 /* Arca RISC */
#define EM_UNICORE       110 /* PKU-Unity & MPRC Peking Uni. mc series */
#define EM_EXCESS        111 /* eXcess configurable cpu */
#define EM_DXP           112 /* Icera Semi. Deep Execution Processor */
#define EM_ALTERA_NIOS2  113 /* Altera Nios II */
#define EM_CRX           114 /* National Semi. CompactRISC CRX */
#define EM_XGATE         115 /* Motorola XGATE */
#define EM_C166          116 /* Infineon C16x/XC16x */
#define EM_M16C          117 /* Renesas M16C */
#define EM_DSPIC30F      118 /* Microchip Technology dsPIC30F */
#define EM_CE            119 /* Freescale Communication Engine RISC */
#define EM_M32C          120 /* Renesas M32C */
/* reserved 121-130 */
#define EM_TSK3000       131 /* Altium TSK3000 */
#define EM_RS08          132 /* Freescale RS08 */
#define EM_SHARC         133 /* Analog Devices SHARC family */
#define EM_ECOG2         134 /* Cyan Technology eCOG2 */
#define EM_SCORE7        135 /* Sunplus S+core7 RISC */
#define EM_DSP24         136 /* New Japan Radio (NJR) 24-bit DSP */
#define EM_VIDEOCORE3    137 /* Broadcom VideoCore III */
#define EM_LATTICEMICO32 138 /* RISC for Lattice FPGA */
#define EM_SE_C17        139 /* Seiko Epson C17 */
#define EM_TI_C6000      140 /* Texas Instruments TMS320C6000 DSP */
#define EM_TI_C2000      141 /* Texas Instruments TMS320C2000 DSP */
#define EM_TI_C5500      142 /* Texas Instruments TMS320C55x DSP */
#define EM_TI_ARP32      143 /* Texas Instruments App. Specific RISC */
#define EM_TI_PRU        144 /* Texas Instruments Prog. Realtime Unit */
/* reserved 145-159 */
#define EM_MMDSP_PLUS    160 /* STMicroelectronics 64bit VLIW DSP */
#define EM_CYPRESS_M8C   161 /* Cypress M8C */
#define EM_R32C          162 /* Renesas R32C */
#define EM_TRIMEDIA      163 /* NXP Semi. TriMedia */
#define EM_QDSP6         164 /* QUALCOMM DSP6 */
#define EM_8051          165 /* Intel 8051 and variants */
#define EM_STXP7X        166 /* STMicroelectronics STxP7x */
#define EM_NDS32         167 /* Andes Tech. compact code emb. RISC */
#define EM_ECOG1X        168 /* Cyan Technology eCOG1X */
#define EM_MAXQ30        169 /* Dallas Semi. MAXQ30 mc */
#define EM_XIMO16        170 /* New Japan Radio (NJR) 16-bit DSP */
#define EM_MANIK         171 /* M2000 Reconfigurable RISC */
#define EM_CRAYNV2       172 /* Cray NV2 vector architecture */
#define EM_RX            173 /* Renesas RX */
#define EM_METAG         174 /* Imagination Tech. META */
#define EM_MCST_ELBRUS   175 /* MCST Elbrus */
#define EM_ECOG16        176 /* Cyan Technology eCOG16 */
#define EM_CR16          177 /* National Semi. CompactRISC CR16 */
#define EM_ETPU          178 /* Freescale Extended Time Processing Unit */
#define EM_SLE9X         179 /* Infineon Tech. SLE9X */
#define EM_L10M          180 /* Intel L10M */
#define EM_K10M          181 /* Intel K10M */
/* reserved 182 */
#define EM_AARCH64       183 /* ARM AARCH64 */
/* reserved 184 */
#define EM_AVR32         185 /* Amtel 32-bit microprocessor */
#define EM_STM8          186 /* STMicroelectronics STM8 */
#define EM_TILE64        187 /* Tileta TILE64 */
#define EM_TILEPRO       188 /* Tilera TILEPro */
#define EM_MICROBLAZE    189 /* Xilinx MicroBlaze */
#define EM_CUDA          190 /* NVIDIA CUDA */
#define EM_TILEGX        191 /* Tilera TILE-Gx */
#define EM_CLOUDSHIELD   192 /* CloudShield */
#define EM_COREA_1ST     193 /* KIPO-KAIST Core-A 1st gen. */
#define EM_COREA_2ND     194 /* KIPO-KAIST Core-A 2nd gen. */
#define EM_ARC_COMPACT2  195 /* Synopsys ARCompact V2 */
#define EM_OPEN8         196 /* Open8 RISC */
#define EM_RL78          197 /* Renesas RL78 */
#define EM_VIDEOCORE5    198 /* Broadcom VideoCore V */
#define EM_78KOR         199 /* Renesas 78KOR */
#define EM_56800EX       200 /* Freescale 56800EX DSC */
#define EM_BA1           201 /* Beyond BA1 */
#define EM_BA2           202 /* Beyond BA2 */
#define EM_XCORE         203 /* XMOS xCORE */
#define EM_MCHP_PIC      204 /* Microchip 8-bit PIC(r) */
/* reserved 205-209 */
#define EM_KM32          210 /* KM211 KM32 */
#define EM_KMX32         211 /* KM211 KMX32 */
#define EM_EMX16         212 /* KM211 KMX16 */
#define EM_EMX8          213 /* KM211 KMX8 */
#define EM_KVARC         214 /* KM211 KVARC */
#define EM_CDP           215 /* Paneve CDP */
#define EM_COGE          216 /* Cognitive Smart Memory Processor */
#define EM_COOL          217 /* Bluechip CoolEngine */
#define EM_NORC          218 /* Nanoradio Optimized RISC */
#define EM_CSR_KALIMBA   219 /* CSR Kalimba */
#define EM_Z80           220 /* Zilog Z80 */
#define EM_VISIUM        221 /* Controls and Data Services VISIUMcore */
#define EM_FT32          222 /* FTDI Chip FT32 */
#define EM_MOXIE         223 /* Moxie processor */
#define EM_AMDGPU        224 /* AMD GPU */
/* reserved 225-242 */
#define EM_RISCV         243 /* RISC-V */

#define DT_NULL                     0 /* Marks end of dynamic section */
#define DT_NEEDED                   1 /* Name of needed library */
#define DT_PLTRELSZ                 2 /* Size in bytes of PLT relocs */
#define DT_PLTGOT                   3 /* Processor defined value */
#define DT_HASH                     4 /* Address of symbol hash table */
#define DT_STRTAB                   5 /* Address of string table */
#define DT_SYMTAB                   6 /* Address of symbol table */
#define DT_RELA                     7 /* Address of Rela relocs */
#define DT_RELASZ                   8 /* Total size of Rela relocs */
#define DT_RELAENT                  9 /* Size of one Rela reloc */
#define DT_STRSZ                   10 /* Size of string table */
#define DT_SYMENT                  11 /* Size of one symbol table entry */
#define DT_INIT                    12 /* Address of init function */
#define DT_FINI                    13 /* Address of termination function */
#define DT_SONAME                  14 /* Name of shared object */
#define DT_RPATH                   15 /* Library search path (deprecated) */
#define DT_SYMBOLIC                16 /* Start symbol search here */
#define DT_REL                     17 /* Address of Rel relocs */
#define DT_RELSZ                   18 /* Total size of Rel relocs */
#define DT_RELENT                  19 /* Size of one Rel reloc */
#define DT_PLTREL                  20 /* Type of reloc in PLT */
#define DT_DEBUG                   21 /* For debugging; unspecified */
#define DT_TEXTREL                 22 /* Reloc might modify .text */
#define DT_JMPREL                  23 /* Address of PLT relocs */
#define DT_BIND_NOW                24 /* Process relocations of object */
#define DT_INIT_ARRAY              25 /* Array with addresses of init fct */
#define DT_FINI_ARRAY              26 /* Array with addresses of fini fct */
#define DT_INIT_ARRAYSZ            27 /* Size in bytes of DT_INIT_ARRAY */
#define DT_FINI_ARRAYSZ            28 /* Size in bytes of DT_FINI_ARRAY */
#define DT_RUNPATH                 29 /* Library search path */
#define DT_FLAGS                   30 /* Flags for the object being loaded */
#define DT_ENCODING                32 /* Start of encoded range */
#define DT_PREINIT_ARRAY           32 /* Array with addresses of preinit fct*/
#define DT_PREINIT_ARRAYSZ         33 /* size in bytes of DT_PREINIT_ARRAY */
#define DT_LOOS            0x6000000d
#define DT_SUNW_RTLDINF    0x6000000e
#define DT_HIOS            0x6ffff000
#define DT_VALRNGLO        0x6ffffd00
#define DT_CHECKSUM        0x6ffffdf8
#define DT_PLTPADSZ        0x6ffffdf9
#define DT_MOVEENT         0x6ffffdfa
#define DT_MOVESZ          0x6ffffdfb
#define DT_FEATURE_1       0x6ffffdfc
#define DT_POSFLAG_1       0x6ffffdfd
#define DT_SYMINSZ         0x6ffffdfe
#define DT_SYMINENT        0x6ffffdff
#define DT_VALRNGHI        0x6ffffdff
#define DT_ADDRRNGLO       0x6ffffe00
#define DT_CONFIG          0x6ffffefa
#define DT_DEPAUDIT        0x6ffffefb
#define DT_AUDIT           0x6ffffefc
#define DT_PLTPAD          0x6ffffefd
#define DT_MOVETAB         0x6ffffefe
#define DT_SYMINFO         0x6ffffeff
#define DT_ADDRRNGHI       0x6ffffeff
#define DT_VERSYM          0x6ffffff0
#define DT_RELACOUNT       0x6ffffff9
#define DT_RELCOUNT        0x6ffffffa
#define DT_FLAGS_1         0x6ffffffb
#define DT_VERDEF          0x6ffffffc
#define DT_VERDEFNUM       0x6ffffffd
#define DT_VERNEED         0x6ffffffe
#define DT_VERNEEDNUM      0x6fffffff
#define DT_LOPROC          0x70000000
#define DT_SPARC_REGISTER  0x70000001
#define DT_AUXILIARY       0x7ffffffd
#define DT_USED            0x7ffffffe
#define DT_FILTER          0x7fffffff
#define DT_HIPROC          0x7fffffff

#define PT_NULL                  0
#define PT_LOAD                  1
#define PT_DYNAMIC               2
#define PT_INTERP                3
#define PT_NOTE                  4
#define PT_SHLIB                 5
#define PT_PHDR                  6
#define PT_LOOS         0x60000000
#define PT_HIOS         0x6fffffff
#define PT_LOPROC       0x70000000
#define PT_HIPROC       0x7fffffff
#define PT_GNU_EH_FRAME 0x6474e550

#define PF_X            0x1
#define PF_W            0x2
#define PF_R            0x4

#define SHN_UNDEF       0
#define SHN_LORESERVE   0xff00
#define SHN_LOPROC      0xff00
#define SHN_HIPROC      0xff1f
#define SHN_ABS         0xfff1
#define SHN_COMMON      0xfff2
#define SHN_HIRESERVE   0xffff

#define SHT_NULL          0
#define SHT_PROGBITS      1
#define SHT_SYMTAB        2
#define SHT_STRTAB        3
#define SHT_RELA          4
#define SHT_HASH          5
#define SHT_DYNAMIC       6
#define SHT_NOTE          7
#define SHT_NOBITS        8
#define SHT_REL           9
#define SHT_SHLIB         10
#define SHT_DYNSYM        11
#define SHT_NUM           12
#define SHT_INIT_ARRAY    14
#define SHT_FINI_ARRAY    15
#define SHT_PREINIT_ARRAY 16
#define SHT_GROUP         17
#define SHT_SYMTAB_SHNDX  0x60000000
#define SHT_LOPROC        0x70000000
#define SHT_HIPROC        0x7fffffff
#define SHT_LOUSER        0x80000000
#define SHT_HIUSER        0xffffffff

#define DT_NULL      0
#define DT_NEEDED    1
#define DT_PLTRELSZ  2
#define DT_PLTGOT    3
#define DT_HASH      4
#define DT_STRTAB    5
#define DT_SYMTAB    6
#define DT_RELA      7
#define DT_RELASZ    8
#define DT_RELAENT   9
#define DT_STRSZ    10
#define DT_SYMENT   11
#define DT_INIT     12
#define DT_FINI     13
#define DT_SONAME   14
#define DT_RPATH    15
#define DT_SYMBOLIC 16
#define DT_REL      17
#define DT_RELSZ    18
#define DT_RELENT   19
#define DT_PLTREL   20
#define DT_DEBUG    21
#define DT_TEXTREL  22
#define DT_JMPREL   23
#define DT_ENCODING 32

#define SHF_WRITE       0x1
#define SHF_ALLOC       0x2
#define SHF_EXECINSTR   0x4
#define SHF_MASKPROC    0xf0000000

#define STB_LOCAL       0
#define STB_GLOBAL      1
#define STB_WEAK        2

#define STT_NOTYPE      0
#define STT_OBJECT      1
#define STT_FUNC        2
#define STT_SECTION     3
#define STT_FILE        4
#define STT_COMMON      5
#define STT_TLS         6

#define STV_DEFAULT     0
#define STV_INTERNAL    1
#define STV_HIDDEN      2

#define STV_PROTECTED   3

#define EF_MIPS_ARCH_1     0x00000000
#define EF_MIPS_ARCH_2     0x10000000
#define EF_MIPS_ARCH_3     0x20000000
#define EF_MIPS_ARCH_4     0x30000000
#define EF_MIPS_ARCH_5     0x40000000
#define EF_MIPS_ARCH_32    0x50000000
#define EF_MIPS_ARCH_64    0x60000000
#define EF_MIPS_ARCH_32R2  0x70000000
#define EF_MIPS_ARCH_64R2  0x80000000
#define EF_MIPS_ABI_EABI32 0x00003000
#define EF_MIPS_ABI_EABI64 0x00004000

#define R_386_NONE      0
#define R_386_32        1
#define R_386_PC32      2
#define R_386_GOT32     3
#define R_386_PLT32     4
#define R_386_COPY      5
#define R_386_GLOB_DAT  6
#define R_386_JMP_SLOT  7
#define R_386_RELATIVE  8
#define R_386_GOTOFF    9
#define R_386_GOTPC    10
#define R_386_NUM      11
