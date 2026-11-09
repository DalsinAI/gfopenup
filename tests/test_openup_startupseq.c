/* Host test of src/openup_startupseq.h: GFOpenUpTool STARTUPSEQ's verdict, the kept version and COMPARE's lines
 * (amicradle-guest docs/design/rom-or-disk.md, section 6).
 *
 *   cc -std=c99 -Wall -Werror -Isrc tests/test_openup_startupseq.c -o test_openup_startupseq && ./test_openup_startupseq
 *
 * tests/test_build_pack.py builds and runs it.
 *
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <stdio.h>
#include <string.h>
#include "openup_startupseq.h"

static int fails;
static char out[2048];
static size_t outn;

static void put(void *ctx, const char *s, unsigned long len)
{
    (void)ctx;
    if (outn + len < sizeof out) { memcpy(out + outn, s, len); outn += len; out[outn] = 0; }
}

static void same(const char *what, const char *got, const char *want)
{
    if (!strcmp(got, want)) { printf("ok   %s\n", what); return; }
    printf("FAIL %s: got <%s>, want <%s>\n", what, got, want);
    fails++;
}

static void is(const char *what, long got, long want)
{
    if (got == want) { printf("ok   %s\n", what); return; }
    printf("FAIL %s: got %ld, want %ld\n", what, got, want);
    fails++;
}

int main(void)
{
    char b[16];
    const char *rom = "; $VER: Startup-Sequence 47.1 (9.11.2026)\nSetPatch QUIET\nAssign C: SYS:C\nLoadWB\nEndCLI >NIL:\n";
    const char *disk = "SetPatch QUIET\r\nAssign C: SYS:C\r\nMyTool\r\nLoadWB\r\n";
    unsigned long rv = 47ul << 16 | 1;
    same("oss_vr 47.1", oss_vr(b, rv), "47.1");
    same("oss_vr 2.10", oss_vr(b, 2ul << 16 | 10), "2.10");
    is("oss_kept \"47.1\\n\"", (long)oss_kept("47.1\n"), (long)rv);
    is("oss_kept \"47\" is 47.0", (long)oss_kept("47"), 47l << 16);
    is("oss_kept nothing", (long)oss_kept(""), 0);
    /* offered: older or different, unless kept against this very version */
    is("older: offered", oss_offer(OSS_OLDER, rv, 0), 1);
    is("different: offered", oss_offer(OSS_DIFFERS, rv, 0), 1);
    is("different, kept against 47.1: not offered", oss_offer(OSS_DIFFERS, rv, rv), 0);
    is("different, kept against 47.0: offered again", oss_offer(OSS_DIFFERS, rv, 47ul << 16), 1);
    is("the same: not offered", oss_offer(OSS_SAME, rv, 0), 0);
    is("newer: not offered", oss_offer(OSS_NEWER, rv, 0), 0);
    is("none on the disk: not offered", oss_offer(OSS_ROM, rv, 0), 0);
    is("no ROM default: not offered", oss_offer(OSS_NONE, 0, 0), 0);
    /* COMPARE: the ROM's lines the disk's lacks, then the disk's the ROM's lacks; $VER lines and CRs aside */
    is("COMPARE counts", (long)oss_compare(rom, strlen(rom), disk, strlen(disk), put, NULL), 2);
    same("COMPARE lines", out, "- EndCLI >NIL:\n+ MyTool\n");
    outn = 0;
    out[0] = 0;
    is("COMPARE of the ROM's against itself (its $VER line off)", (long)oss_compare(rom, strlen(rom), rom + 42, strlen(rom + 42), put, NULL), 0);
    same("COMPARE says nothing", out, "");
    same("CHECK's line, older", oss_says(OSS_OLDER),
         "S:Startup-Sequence is older than the ROM's default. Yours runs; the ROM's can take its place.");
    same("CHECK's line, none on the disk", oss_says(OSS_ROM), "There is no S:Startup-Sequence on the disk: the ROM's default runs.");
    printf(fails ? "%d failed\n" : "all passed\n", fails);
    return fails ? 1 : 0;
}
