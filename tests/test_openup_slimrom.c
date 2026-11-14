/* Host test of src/openup_slimrom.h: DETECT's SlimROM on each kind of ROM.
 *
 *   cc -std=c99 -Wall -Werror -Isrc tests/test_openup_slimrom.c -o test_openup_slimrom && ./test_openup_slimrom
 *
 * tests/test_build_pack.py builds and runs it.
 *
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <stdio.h>
#include <string.h>
#include "openup_slimrom.h"

static int fails;

static void is(const char *what, int got, int want)
{
    if (got == want) { printf("ok   %s\n", what); return; }
    printf("FAIL %s: got %d, want %d\n", what, got, want);
    fails++;
}

/* A ROM's first 64 bytes as tools/openkickrom writes them: magic, JMP, entry, version words, 'OKR1' at 20, the id
 * at $18 (mark 0 for an older header, or an OS 3.2.3 ROM's bytes) */
static void rom(unsigned char *b, int mark, const char *id)
{
    memset(b, 0, 64);
    b[0] = 0x11; b[1] = 0x14; b[2] = 0x4E; b[3] = 0xF9;
    if (mark) { b[20] = 'O'; b[21] = 'K'; b[22] = 'R'; b[23] = '1'; }
    if (id) memcpy(b + OUS_ID_AT, id, strlen(id) + 1);
}

int main(void)
{
    unsigned char b[64];

    /* the thin ROM, packed boot image: kickload and every module run from Fast RAM, no workbench.library */
    rom(b, 1, "GFOpenKick 47.112 amicradle-thin");
    is("thin, packed, before the set", ous_slim_rom(1, b, 0, 0, 0), 1);
    is("thin, packed, the set's workbench.library linked", ous_slim_rom(1, b, 0, 1, 0), 1);
    is("thin, kickload in the ROM window", ous_slim_rom(1, b, 1, 0, 0), 1);
    rom(b, 1, "GFOpenKick 47.112 trial-thin");
    is("thin, another ROM name", ous_slim_rom(1, b, 0, 0, 0), 1);

    /* full ROMs of ours: workbench.library their own */
    rom(b, 1, "GFOpenKick 47.112 amicradle-amicradle");
    is("amicradle profile, packed (workbench.library in RAM)", ous_slim_rom(1, b, 0, 1, 0), 0);
    is("amicradle profile, plain", ous_slim_rom(1, b, 1, 1, 1), 0);
    rom(b, 1, "GFOpenKick 47.112 card-amicradle-card");
    is("amicradle-card profile", ous_slim_rom(1, b, 0, 1, 0), 0);
    rom(b, 1, "GFOpenKick 47.112 trial-thinner");
    is("a profile that only starts with thin", ous_slim_rom(1, b, 0, 0, 0), 0);

    /* a header with no NUL in the id field is no id */
    rom(b, 1, NULL);
    memset(b + OUS_ID_AT, 'x', 0x40 - OUS_ID_AT);
    memcpy(b + 0x40 - 5, "-thin", 5);
    is("an id field with no NUL", ous_slim_rom(1, b, 0, 0, 0), 0);

    /* OS 3.2.3's ROM: no kickload */
    rom(b, 0, NULL);
    is("OS 3.2.3", ous_slim_rom(0, b, 0, 1, 1), 0);

    /* an older ROM of ours without the header: the earlier rule */
    rom(b, 0, NULL);
    is("old header, kickload in ROM, no workbench.library", ous_slim_rom(1, b, 1, 0, 0), 1);
    is("old header, kickload in ROM, the set's workbench.library in RAM", ous_slim_rom(1, b, 1, 1, 0), 1);
    is("old header, workbench.library in the ROM", ous_slim_rom(1, b, 1, 1, 1), 0);

    printf("%s\n", fails ? "FAIL" : "PASS");
    return fails ? 1 : 0;
}
