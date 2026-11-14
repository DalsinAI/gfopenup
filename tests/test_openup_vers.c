/* Host test of src/openup_vers.h: a file's $VER, and that INSTALL and REINSTALL never put an older file over a
 * newer one without FORCE.
 *   cc -std=c99 -Wall -Werror -Isrc tests/test_openup_vers.c -o t && ./t
 * tests/test_build_pack.py builds and runs it.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <stdio.h>
#include <string.h>
#include "openup_vers.h"

static int fails, checks;
#define CHECK(c, msg) do { checks++; if (!(c)) { fails++; printf("FAIL %s\n", msg); } } while (0)

static int ver(const char *s, char *out)
{
    return ouv_ver(s, (long)strlen(s) + 1, out, 24);
}

int main(void)
{
    char v[24];
    static const char bin[] = "\0\0\x03\xf3junk\0$VER: GFOpenBench 0.1.62 (14.11.2026) AmiCradle Team\0more";
    CHECK(ouv_ver(bin, (long)sizeof bin, v, sizeof v) && !strcmp(v, "0.1.62"), "a binary's $VER among NULs");
    CHECK(ver("$VER: gfopeninput.library 1.3 (17.10.2026)", v) && !strcmp(v, "1.3"), "a library's");
    CHECK(ver("$VER: GFOpenWrite 2.0.35 (14.11.2026)", v) && !strcmp(v, "2.0.35"), "three numbers");
    CHECK(ver("$VER:GFOpenCalc 1.4. (1.11.2026)", v) && !strcmp(v, "1.4"), "no blank after the colon; a trailing dot");
    CHECK(!ver("$VER: Thing (1.11.2026)", v), "only a date: no version");
    CHECK(!ver("no version string here 1.2", v), "no $VER");
    CHECK(ver("x$VER: GFOffice 3.1 (1.1.2026) uses $VER: z.library 1.9", v) && !strcmp(v, "3.1"), "the first $VER");

    CHECK(ouv_cmp("47.10", "47.9") > 0, "47.10 is newer than 47.9");
    CHECK(ouv_cmp("2.0.35", "2.0.26") > 0, "2.0.35 over 2.0.26");
    CHECK(ouv_cmp("1.3", "1.3.0") == 0, "1.3 is 1.3.0");
    CHECK(ouv_cmp("0.4.0", "0.3.3") > 0, "0.4.0 over 0.3.3");
    /* the daily release cycle (14 Nov 2026): 0.8.29-rc1 < -rc2 < 0.8.29 < 0.8.30-rc1 */
    CHECK(ouv_cmp("0.8.29-rc2", "0.8.29-rc1") > 0, "rc2 is newer than rc1");
    CHECK(ouv_cmp("0.8.29", "0.8.29-rc2") > 0, "the release is newer than its candidates");
    CHECK(ouv_cmp("0.8.29-rc1", "0.8.29") < 0, "a candidate is older than its release");
    CHECK(ouv_cmp("0.8.30-rc1", "0.8.29") > 0, "the next day's candidate is newer than the release");
    CHECK(ouv_cmp("0.8.29-rc10", "0.8.29-rc9") > 0, "rc10 over rc9, as numbers");
    CHECK(ouv_cmp("0.8.29-rc1", "0.8.29-rc1") == 0, "the same candidate");
    CHECK(ouv_cmp("0.8.28", "0.8.29-rc1") < 0, "yesterday's release is older than today's candidate");
    CHECK(ouv_cmp("0.8.29-x", "0.8.29") == 0, "another suffix is ignored, as before");

    CHECK(ouv_keep_newer("0.1.58", "0.1.62", 0), "REINSTALL: an older GFOpenBench stays out");
    CHECK(!ouv_keep_newer("0.1.58", "0.1.62", 1), "FORCE puts it in");
    CHECK(!ouv_keep_newer("0.1.62", "0.1.58", 0), "a newer one goes in");
    CHECK(!ouv_keep_newer("0.1.62", "0.1.62", 0), "the same version: written as before");
    CHECK(!ouv_keep_newer("", "0.1.62", 0), "the pack's file has no version: as before");
    CHECK(!ouv_keep_newer("0.1.62", "", 0), "the disk's file has no version: as before");
    printf("%s   %d checks (GFOpenUpTool: an older file never over a newer one)\n", fails ? "FAIL" : "ok", checks);
    return fails != 0;
}
