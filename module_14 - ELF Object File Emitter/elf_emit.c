/* elf_emit.c — ELF64 relocatable object file emitter
 * Module 14: ELF Object File Emitter
 *
 * Writes a valid ELF64 ET_REL .o file directly in binary.
 * Layout:
 *   [ELF header] [.text bytes] [.symtab entries] [.strtab] [.shstrtab]
 *   [5 section headers: null, .text, .symtab, .strtab, .shstrtab]
 */
#include "elf_emit.h"

#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Little-endian write helpers                                          */
/* ------------------------------------------------------------------ */

static void write_u8(uint8_t *buf, size_t *off, uint8_t val) {
    buf[(*off)++] = val;
}

static void write_u16(uint8_t *buf, size_t *off, uint16_t val) {
    buf[(*off)++] = (uint8_t)(val & 0xff);
    buf[(*off)++] = (uint8_t)((val >> 8) & 0xff);
}

static void write_u32(uint8_t *buf, size_t *off, uint32_t val) {
    buf[(*off)++] = (uint8_t)(val & 0xff);
    buf[(*off)++] = (uint8_t)((val >> 8) & 0xff);
    buf[(*off)++] = (uint8_t)((val >> 16) & 0xff);
    buf[(*off)++] = (uint8_t)((val >> 24) & 0xff);
}

static void write_u64(uint8_t *buf, size_t *off, uint64_t val) {
    buf[(*off)++] = (uint8_t)(val & 0xff);
    buf[(*off)++] = (uint8_t)((val >> 8) & 0xff);
    buf[(*off)++] = (uint8_t)((val >> 16) & 0xff);
    buf[(*off)++] = (uint8_t)((val >> 24) & 0xff);
    buf[(*off)++] = (uint8_t)((val >> 32) & 0xff);
    buf[(*off)++] = (uint8_t)((val >> 40) & 0xff);
    buf[(*off)++] = (uint8_t)((val >> 48) & 0xff);
    buf[(*off)++] = (uint8_t)((val >> 56) & 0xff);
}

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

void elf_init(ElfEmitter *e) {
    memset(e, 0, sizeof(*e));
}

void elf_add_func(ElfEmitter *e, const char *name, const uint8_t *code, size_t code_len) {
    int idx = e->n_syms;
    if (idx >= 64) {
        fprintf(stderr, "elf_add_func: too many functions\n");
        return;
    }
    e->sym_offsets[idx] = e->text_size;
    e->sym_sizes[idx]   = code_len;
    strncpy(e->sym_names[idx], name, 63);
    e->sym_names[idx][63] = '\0';
    memcpy(e->text + e->text_size, code, code_len);
    e->text_size += code_len;
    e->n_syms++;
}

void elf_write(ElfEmitter *e, const char *path) {
    uint8_t *buf = e->buf;
    size_t   pos = 0;

    /* ---- Build .strtab ---- */
    /* null entry for index 0, then each function name */
    uint8_t strtab[4096];
    size_t  strtab_size = 0;
    uint32_t sym_name_off[64];

    strtab[strtab_size++] = '\0';  /* mandatory null entry */
    for (int i = 0; i < e->n_syms; i++) {
        sym_name_off[i] = (uint32_t)strtab_size;
        size_t nlen = strlen(e->sym_names[i]) + 1;
        memcpy(strtab + strtab_size, e->sym_names[i], nlen);
        strtab_size += nlen;
    }

    /* ---- Build .shstrtab ---- */
    /* "\0.text\0.symtab\0.strtab\0.shstrtab\0" */
    static const char shstrtab_data[] =
        "\0.text\0.symtab\0.strtab\0.shstrtab";
    size_t shstrtab_size = sizeof(shstrtab_data); /* includes final \0 */

    /* offsets of each name within shstrtab */
    uint32_t sh_text_name    = 1;
    uint32_t sh_symtab_name  = 7;
    uint32_t sh_strtab_name  = 15;
    uint32_t sh_shstrtab_name= 23;

    /* ---- Compute file layout ---- */
    size_t elf_hdr_size  = sizeof(Elf64_Ehdr);
    size_t text_off      = elf_hdr_size;
    size_t text_size     = e->text_size;

    /* .symtab: 1 null entry + n_syms function entries, each Elf64_Sym */
    size_t symtab_off    = text_off + text_size;
    size_t n_sym_entries = 1 + (size_t)e->n_syms;
    size_t symtab_size   = n_sym_entries * sizeof(Elf64_Sym);

    size_t strtab_off    = symtab_off + symtab_size;
    size_t shstrtab_off  = strtab_off + strtab_size;
    size_t shdr_off      = shstrtab_off + shstrtab_size;
    /* Align shdr_off to 8 bytes */
    if (shdr_off % 8 != 0) shdr_off += 8 - (shdr_off % 8);

    /* ---- ELF64 header ---- */
    /* e_ident */
    buf[pos++] = ELFMAG0; buf[pos++] = ELFMAG1;
    buf[pos++] = ELFMAG2; buf[pos++] = ELFMAG3;
    buf[pos++] = ELFCLASS64;
    buf[pos++] = ELFDATA2LSB;
    buf[pos++] = EV_CURRENT;
    buf[pos++] = ELFOSABI_NONE;
    /* padding */
    for (int i = 0; i < 8; i++) buf[pos++] = 0;

    write_u16(buf, &pos, ET_REL);          /* e_type */
    write_u16(buf, &pos, EM_X86_64);       /* e_machine */
    write_u32(buf, &pos, EV_CURRENT);      /* e_version */
    write_u64(buf, &pos, 0);               /* e_entry */
    write_u64(buf, &pos, 0);               /* e_phoff */
    write_u64(buf, &pos, (uint64_t)shdr_off); /* e_shoff */
    write_u32(buf, &pos, 0);               /* e_flags */
    write_u16(buf, &pos, (uint16_t)sizeof(Elf64_Ehdr)); /* e_ehsize */
    write_u16(buf, &pos, 0);               /* e_phentsize */
    write_u16(buf, &pos, 0);               /* e_phnum */
    write_u16(buf, &pos, (uint16_t)sizeof(Elf64_Shdr)); /* e_shentsize */
    write_u16(buf, &pos, 5);               /* e_shnum: null,.text,.symtab,.strtab,.shstrtab */
    write_u16(buf, &pos, 4);               /* e_shstrndx: index of .shstrtab */

    /* ---- .text bytes ---- */
    /* pos should now == text_off */
    memcpy(buf + pos, e->text, text_size);
    pos += text_size;

    /* ---- .symtab ---- */
    /* null entry */
    for (size_t i = 0; i < sizeof(Elf64_Sym); i++) buf[pos++] = 0;

    /* one entry per function */
    for (int i = 0; i < e->n_syms; i++) {
        write_u32(buf, &pos, sym_name_off[i]);         /* st_name */
        /* st_info: STB_GLOBAL | STT_FUNC */
        write_u8 (buf, &pos, (uint8_t)((STB_GLOBAL << 4) | STT_FUNC));
        write_u8 (buf, &pos, STV_DEFAULT);             /* st_other */
        write_u16(buf, &pos, 1);                       /* st_shndx: section 1 = .text */
        write_u64(buf, &pos, (uint64_t)e->sym_offsets[i]); /* st_value */
        write_u64(buf, &pos, (uint64_t)e->sym_sizes[i]);   /* st_size */
    }

    /* ---- .strtab ---- */
    memcpy(buf + pos, strtab, strtab_size);
    pos += strtab_size;

    /* ---- .shstrtab ---- */
    memcpy(buf + pos, shstrtab_data, shstrtab_size);
    pos += shstrtab_size;

    /* ---- Pad to shdr_off ---- */
    while (pos < shdr_off) buf[pos++] = 0;

    /* ---- Section headers ---- */
    /* Helper macro: write one Elf64_Shdr */
#define SHDR(name_off, type, flags, addr, offset, size, link, info, align, entsize) \
    write_u32(buf, &pos, (uint32_t)(name_off));   \
    write_u32(buf, &pos, (uint32_t)(type));        \
    write_u64(buf, &pos, (uint64_t)(flags));       \
    write_u64(buf, &pos, (uint64_t)(addr));        \
    write_u64(buf, &pos, (uint64_t)(offset));      \
    write_u64(buf, &pos, (uint64_t)(size));        \
    write_u32(buf, &pos, (uint32_t)(link));        \
    write_u32(buf, &pos, (uint32_t)(info));        \
    write_u64(buf, &pos, (uint64_t)(align));       \
    write_u64(buf, &pos, (uint64_t)(entsize))

    /* Section 0: null */
    SHDR(0, SHT_NULL, 0, 0, 0, 0, 0, 0, 0, 0);

    /* Section 1: .text */
    SHDR(sh_text_name, SHT_PROGBITS,
         SHF_ALLOC | SHF_EXECINSTR,
         0, text_off, text_size,
         0, 0, 16, 0);

    /* Section 2: .symtab  (link=3=.strtab, info=1=first global sym idx) */
    SHDR(sh_symtab_name, SHT_SYMTAB,
         0,
         0, symtab_off, symtab_size,
         3,   /* link → .strtab (section 3) */
         1,   /* info → index of first global symbol (null entry is local) */
         8, sizeof(Elf64_Sym));

    /* Section 3: .strtab */
    SHDR(sh_strtab_name, SHT_STRTAB,
         0,
         0, strtab_off, strtab_size,
         0, 0, 1, 0);

    /* Section 4: .shstrtab */
    SHDR(sh_shstrtab_name, SHT_STRTAB,
         0,
         0, shstrtab_off, shstrtab_size,
         0, 0, 1, 0);

#undef SHDR

    /* ---- Write to file ---- */
    FILE *fp = fopen(path, "wb");
    if (!fp) {
        perror(path);
        exit(1);
    }
    fwrite(buf, 1, pos, fp);
    fclose(fp);
}
