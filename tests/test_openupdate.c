/* Host test of src/openupdate.h: GFOpenUpdate's index, what it offers against GFOpenUpTool's manifest, the versions
 * said no to, the INTERVAL tooltype and when a check is due.
 *   cc -std=c99 -Wall -Werror -Isrc tests/test_openupdate.c -o t && ./t
 * tests/test_build_pack.py builds and runs it.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <stdio.h>
#include <string.h>
#include "openupdate.h"

static int fails, checks;
#define CHECK(c, msg) do { checks++; if (!(c)) { fails++; printf("FAIL %s\n", msg); } } while (0)

#define SHA_A "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
#define SHA_B "ABCDEF0123456789abcdef0123456789abcdef0123456789abcdef0123456789"

static const char index_ok[] =
    "GFOpenUpdate 1\n"
    "; made by a cut\n"
    "\n"
    "part GFOpenPlay 0.1.7-rc1 123456 " SHA_A " GFOpenPlay-0.1.7-rc1.lha\n"
    "part GFOpenIcons\t0.8.31 2048 " SHA_B " GFOpenIcons-0.8.31.lha\r\n"
    "part GFOpenDock 0.6.15 999 " SHA_A " GFOpenDock-0.6.15.lha\n"
    "part GFOpenSynth 1.0 5 " SHA_A " GFOpenSynth-1.0.lha\n"
    "GFOpenUpdate end\n";

static const char manifest[] =
    "GFOpenUp\t0.2\n"
    "part\tGFOpenUp\t0.8.30-rc6\n"
    "file\tC:GFOpenUpTool\n"
    "part\tGFOpenPlay\t0.1.6\n"
    "part\tGFOpenIcons\t0.8.30-rc6\n"
    "part\tGFOpenDock\t0.6.15\n"
    "needpart\tGFOpenDock\tGFOpenLibs\n"
    "GFOpenUp\tend\n";

static int parts_of(const char *text, struct oud_part *p, int max)
{
    return oud_index_parts(text, (long)strlen(text), p, max);
}

int main(void)
{
    struct oud_part p[8];
    char v[24];
    int n;
    (void)ouv_ver; (void)ouv_keep_newer;          /* openup_vers.h's, not weighed here */

    /* the index */
    CHECK(oud_index_whole(index_ok, (long)strlen(index_ok)), "a whole index");
    CHECK(!oud_index_whole(index_ok, (long)strlen(index_ok) - 10), "cut short: no end line");
    CHECK(!oud_index_whole("<html>404</html>\n", 17), "a web page is no index");
    CHECK(!oud_index_whole("", 0), "nothing is no index");
    CHECK(!oud_index_whole("GFOpenUpdate 2\nGFOpenUpdate end\n", 32), "a later kind of index is left alone");
    CHECK(!oud_index_whole("GFOpenUpdate 1\nGFOpenUpdate end\npart x 1 1 a b\n", 47), "lines after the end");
    n = parts_of(index_ok, p, 8);
    CHECK(n == 4, "four parts");
    CHECK(n == 4 && !strcmp(p[0].name, "GFOpenPlay") && !strcmp(p[0].version, "0.1.7-rc1") && p[0].bytes == 123456
          && !strcmp(p[0].sha256, SHA_A) && !strcmp(p[0].file, "GFOpenPlay-0.1.7-rc1.lha"), "a part's fields");
    CHECK(n == 4 && !strcmp(p[1].name, "GFOpenIcons") && !strcmp(p[1].file, "GFOpenIcons-0.8.31.lha"), "tabs and a CR");
    CHECK(parts_of(index_ok, p, 3) == -1, "more parts than the table holds: none used");
    CHECK(parts_of("GFOpenUpdate 1\nGFOpenUpdate end\n", p, 8) == 0, "an empty index offers nothing");
    CHECK(parts_of("GFOpenUpdate 1\npart A 1.0 10 " SHA_A "\nGFOpenUpdate end\n", p, 8) == -1, "a field missing");
    CHECK(parts_of("GFOpenUpdate 1\npart A 1.0 10 " SHA_A " a.lha more\nGFOpenUpdate end\n", p, 8) == -1, "a field too many");
    CHECK(parts_of("GFOpenUpdate 1\npart A 1.0 10 0123 a.lha\nGFOpenUpdate end\n", p, 8) == -1, "a short SHA-256");
    CHECK(parts_of("GFOpenUpdate 1\npart A 1.0 0 " SHA_A " a.lha\nGFOpenUpdate end\n", p, 8) == -1, "no bytes");
    CHECK(parts_of("GFOpenUpdate 1\npart A 1.0 1x " SHA_A " a.lha\nGFOpenUpdate end\n", p, 8) == -1, "bytes not a number");
    CHECK(parts_of("GFOpenUpdate 1\npart A new 10 " SHA_A " a.lha\nGFOpenUpdate end\n", p, 8) == -1, "a version without a number");
    CHECK(parts_of("GFOpenUpdate 1\npart A 1.0 10 " SHA_A " ../a.lha\nGFOpenUpdate end\n", p, 8) == -1, "a file in another drawer");
    CHECK(parts_of("GFOpenUpdate 1\npart A 1.0 10 " SHA_A " SYS:a.lha\nGFOpenUpdate end\n", p, 8) == -1, "a file on a volume");
    CHECK(parts_of("GFOpenUpdate 1\npart A 1.0 10 " SHA_A " http://x/a.lha\nGFOpenUpdate end\n", p, 8) == -1, "a file elsewhere");
    CHECK(parts_of("GFOpenUpdate 1\npart A 1.0 10 " SHA_A " a.lha\npart A 1.1 10 " SHA_A " b.lha\nGFOpenUpdate end\n", p, 8) == -1,
          "a part listed twice");
    CHECK(parts_of("GFOpenUpdate 1\nfile A 1.0 10 " SHA_A " a.lha\nGFOpenUpdate end\n", p, 8) == -1, "a line of another kind");

    /* the manifest */
    CHECK(oud_installed(manifest, (long)strlen(manifest), "GFOpenPlay", v, sizeof v) && !strcmp(v, "0.1.6"), "an installed part's version");
    CHECK(!oud_installed(manifest, (long)strlen(manifest), "GFOpenSynth", v, sizeof v), "a part not installed");
    CHECK(!oud_installed(manifest, (long)strlen(manifest), "GFOpen", v, sizeof v), "a name matches whole");
    CHECK(!oud_installed(manifest, (long)strlen(manifest), "GFOpenLibs", v, sizeof v), "a needpart line is no part line");
    CHECK(!oud_installed(NULL, 0, "GFOpenPlay", v, sizeof v), "no manifest: nothing installed");

    /* what is offered */
    n = parts_of(index_ok, p, 8);
    n = oud_offers(p, n, manifest, (long)strlen(manifest), NULL, 0);
    CHECK(n == 2, "two offers: the same version and a part not installed are left out");
    CHECK(n == 2 && !strcmp(p[0].name, "GFOpenPlay") && !strcmp(p[0].installed, "0.1.6") && !strcmp(p[0].version, "0.1.7-rc1"),
          "GFOpenPlay 0.1.6 to 0.1.7-rc1");
    CHECK(n == 2 && !strcmp(p[1].name, "GFOpenIcons") && !strcmp(p[1].installed, "0.8.30-rc6") && !strcmp(p[1].version, "0.8.31"),
          "a release over its candidate");
    {
        static const char skip[] = "skip GFOpenPlay 0.1.7-rc1\nskip GFOpenIcons 0.8.30\n";
        n = parts_of(index_ok, p, 8);
        n = oud_offers(p, n, manifest, (long)strlen(manifest), skip, (long)strlen(skip));
        CHECK(n == 1 && !strcmp(p[0].name, "GFOpenIcons"), "a version said no to isn't offered; a newer one than that is");
    }
    {
        static const char older[] = "GFOpenUpdate 1\npart GFOpenPlay 0.1.5 10 " SHA_A " p.lha\nGFOpenUpdate end\n";
        n = parts_of(older, p, 8);
        CHECK(n == 1 && oud_offers(p, n, manifest, (long)strlen(manifest), NULL, 0) == 0, "an older part is never offered");
    }
    n = parts_of(index_ok, p, 8);
    CHECK(oud_offers(p, n, NULL, 0, NULL, 0) == 0, "GFOpenUp not installed: nothing offered");

    /* the INTERVAL tooltype */
    CHECK(oud_interval(NULL) == 86400, "no tooltype: a day");
    CHECK(oud_interval("") == 86400, "an empty tooltype: a day");
    CHECK(oud_interval("3600") == 3600, "an hour");
    CHECK(oud_interval("60") == 600, "under ten minutes is ten minutes");
    CHECK(oud_interval("0") == 0, "0: only at startup");
    CHECK(oud_interval("daily") == 86400, "not a number: a day");
    CHECK(oud_interval("-5") == 86400, "a minus: a day");
    CHECK(oud_interval("99999999999") == 86400, "too long a number: a day");

    /* when a check is due */
    CHECK(!oud_due(0, 0, 1000, 0, 3600), "no network: not at startup");
    CHECK(oud_due(1, 0, 1000, 990, 3600), "at startup once the network is up, whenever the last one was");
    CHECK(!oud_due(1, 1, 1000 + 3599, 1000, 3600), "not before the interval");
    CHECK(oud_due(1, 1, 1000 + 3600, 1000, 3600), "at the interval");
    CHECK(!oud_due(0, 1, 1000 + 7200, 1000, 3600), "no network: not later either");
    CHECK(!oud_due(1, 1, 1000000, 1000, 0), "interval 0: only the one at startup");
    CHECK(oud_due(1, 1, 500, 1000, 3600), "the clock set back: due");

    printf("%d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
