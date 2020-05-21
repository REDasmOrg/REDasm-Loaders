#pragma once

#include <rdapi/types.h>
#include <rdapi/utils.h>
#include "elf_common.h"

#define ELF_ARG(...)    __VA_ARGS__
#define ELF32_R_SYM(i) ((i) >> 8)
#define ELF64_R_SYM(i) ((i) >> 32)

template<size_t bits> struct elf_unsigned_t { };
template<> struct elf_unsigned_t<32> { typedef u32 type; };
template<> struct elf_unsigned_t<64> { typedef u64 type; };

template<size_t bits> struct elf_signed_t { };
template<> struct elf_signed_t<32> { typedef s32 type; };
template<> struct elf_signed_t<64> { typedef s64 type; };

template<typename T> T e_valT(T t, size_t e)
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

template<size_t bits> struct Elf_Ehdr
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

template<size_t bits> struct Elf_Shdr
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
    typename elf_unsigned_t<bits>::type sh_endsize;
};

template<size_t bits> struct Elf_Rel
{
    typename elf_unsigned_t<bits>::type r_offset;
    typename elf_unsigned_t<bits>::type r_info;
};

template<size_t bits> struct Elf_Rela
{
    typename elf_unsigned_t<bits>::type r_offset;
    typename elf_unsigned_t<bits>::type r_info;
    typename elf_signed_t<bits>::type r_addend;
};

struct Elf32_Phdr
{
    u32 p_type;
    u32 p_offset;
    u32 p_vaddr;
    u32 p_paddr;
    u32 p_filesz;
    u32 p_memsz;
    u32 p_flags;
    u32 p_align;
};

struct Elf64_Phdr
{
    u32 p_type;
    u32 p_flags;
    u64 p_offset;
    u64 p_vaddr;
    u64 p_paddr;
    u64 p_filesz;
    u64 p_memsz;
    u64 p_align;
};

struct Elf32_Sym
{
    u32 st_name;
    u32 st_value;
    u32 st_size;
    u8 st_info;
    u8 st_other;
    u16 st_shndx;
};

struct Elf64_Sym
{
    u32 st_name;
    u8 st_info;
    u8 st_other;
    u16 st_shndx;
    u64 st_value;
    u64 st_size;
};
