#pragma once

#include <rdapi/types.h>
#include <rdapi/utils.h>
#include "elf_common.h"

/*
 * FROM: https://man7.org/linux/man-pages/man5/elf.5.html
 *
 * The following types are used for N-bit architectures (N=32,64, ElfN
 * stands for Elf32 or Elf64, uintN_t stands for uint32_t or uint64_t):

 * ElfN_Addr       Unsigned program address, uintN_t
 * ElfN_Off        Unsigned file offset, uintN_t
 * ElfN_Section    Unsigned section index, uint16_t
 * ElfN_Versym     Unsigned version symbol information, uint16_t
 * Elf_Byte        unsigned char
 * ElfN_Half       uint16_t
 * ElfN_Sword      int32_t
 * ElfN_Word       uint32_t
 * ElfN_Sxword     int64_t
 * ElfN_Xword      uint64_t
 */

template<size_t bits> struct elf_unsigned_t { };
template<> struct elf_unsigned_t<32> { typedef u32 type; };
template<> struct elf_unsigned_t<64> { typedef u64 type; };

template<size_t bits> struct elf_signed_t { };
template<> struct elf_signed_t<32> { typedef s32 type; };
template<> struct elf_signed_t<64> { typedef s64 type; };

template<size_t bits>
auto elf_r_sym(typename elf_unsigned_t<bits>::type info) {
    if constexpr(bits == 64) return info >> 32;
    else return info >> 8;
}

template<size_t bits>
auto elf_r_type(typename elf_unsigned_t<bits>::type info) {
    if constexpr(bits == 64) return info & 0xFFFFFFFFL;
    else return static_cast<u8>(info);
}

template<typename T>
T e_valT(T t, size_t e)
{
    if constexpr(sizeof(T) == 2) {
        if(e == Endianness_Big) return RD_FromBigEndian16(t);
        return RD_FromLittleEndian16(t);
    }
    else if constexpr(sizeof(T) == 4) {
        if(e == Endianness_Big) return RD_FromBigEndian32(t);
        return RD_FromLittleEndian32(t);
    }
    else if constexpr(sizeof(T) == 8) {
        if(e == Endianness_Big) return RD_FromBigEndian64(t);
        return RD_FromLittleEndian64(t);
    }
    else return t;
}

struct Elf_EhdrBase
{
    u8 e_ident[EI_NIDENT];
    u16 e_type;
    u16 e_machine;
    u16 e_version;
};

template<size_t bits>
struct Elf_Ehdr
{
    u8 e_ident[EI_NIDENT];
    u16 e_type;
    u16 e_machine;
    u16 e_version;
    typename elf_unsigned_t<bits>::type e_entry;
    typename elf_unsigned_t<bits>::type e_phoff;
    typename elf_unsigned_t<bits>::type e_shoff;
    u32 e_flags;
    u16 e_ehsize;
    u16 e_phentsize;
    u16 e_phnum;
    u16 e_shentsize;
    u16 e_shnum;
    u16 e_shstrndx;
};

template<size_t bits>
struct Elf_Shdr
{
    u32 sh_name;
    u32 sh_type;
    typename elf_unsigned_t<bits>::type sh_flags;
    typename elf_unsigned_t<bits>::type sh_addr;
    typename elf_unsigned_t<bits>::type sh_offset;
    typename elf_unsigned_t<bits>::type sh_size;
    u32 sh_link;
    u32 sh_info;
    typename elf_unsigned_t<bits>::type sh_addralign;
    typename elf_unsigned_t<bits>::type sh_entsize;
};

template<size_t bits>
struct Elf_Dyn {
    typename elf_signed_t<bits>::type d_tag;

    union {
        typename elf_unsigned_t<bits>::type d_val;
        typename elf_unsigned_t<bits>::type d_ptr;
    };
};

template<size_t bits>
struct Elf_Rel
{
    typename elf_unsigned_t<bits>::type r_offset;
    typename elf_unsigned_t<bits>::type r_info;
};

template<size_t bits>
struct Elf_Rela
{
    typename elf_unsigned_t<bits>::type r_offset;
    typename elf_unsigned_t<bits>::type r_info;
    typename elf_signed_t<bits>::type r_addend;
};

struct Elf32_Phdr { u32 p_type; u32 p_offset; u32 p_vaddr; u32 p_paddr; u32 p_filesz; u32 p_memsz; u32 p_flags; u32 p_align; };
struct Elf64_Phdr { u32 p_type; u32 p_flags; u64 p_offset; u64 p_vaddr; u64 p_paddr; u64 p_filesz; u64 p_memsz; u64 p_align; };

struct Elf32_Sym { u32 st_name; u32 st_value; u32 st_size; u8 st_info; u8 st_other; u16 st_shndx; };
struct Elf64_Sym { u32 st_name; u8 st_info; u8 st_other; u16 st_shndx; u64 st_value; u64 st_size; };

struct Elf_Verneed { u16 vn_version; u16 vn_cnt; u32 vn_file; u32 vn_aux; u32 vn_next; };
struct Elf_Vernaux { u32 vna_hash; u16 vna_flags; u16 vna_other; u32 vna_name; u32 vna_next; };
