/* Host test of src/opencatalogue.h: gfosdistribution's software/index.txt read, weighed against GFOpenUpTool's
 * manifest, and a release's files given their places (backlog AC-280).
 *   cc -std=c99 -Wall -Werror -Isrc tests/test_opencatalogue.c -o t && ./t
 * tests/test_build_pack.py builds and runs it.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <stdio.h>
#include <string.h>
/* openupdate.h carries GFOpenUpdate's other helpers, tested in tests/test_openupdate.c */
#pragma GCC diagnostic ignored "-Wunused-function"
#include "opencatalogue.h"

static int fails, checks;
#define CHECK(c, msg) do { checks++; if (!(c)) { fails++; printf("FAIL %s\n", msg); } } while (0)

#define SHA_A "f1e17fa0ef4684cceb43c26c74309be9647a548aaf79b25020c14f9d36c725f6"
#define SHA_B "6ffb89dc0c5e9ee73e94116e208eb85597399fd4e671ad811fdabfa4de73a9eb"
#define SHA_C "550e9a1e5552587abf3a147968aed967521fab3ea673212f9bd0c87619be4693"
#define REL OUC_RELEASES_DEFAULT

/* as gfosdistribution's was on 24 Nov 2026, with wrong lines added */
static const char cat[] =
    "# GFOS software: name, version, size, sha256, address, description (tabs between)\n"
    "AmiPortAtlas\t0.4.0-rc1\t1583077\t" SHA_A "\t" REL "AmiPortAtlas-0.4.0-rc1/AmiPortAtlas-0.4.0-rc1.lha\tRelease candidate.\n"
    "AmiPortAtlas\t0.4.0-rc2\t1583429\t" SHA_B "\t" REL "AmiPortAtlas-0.4.0-rc2/AmiPortAtlas-0.4.0-rc2.lha\tSame program as RC1; \xe2\x80\x9cicons\xe2\x80\x9d.\n"
    "GFOpenSQLite\t3.53.4-2\t1835723\t" SHA_C "\t" REL "GFOpenSQLite-3.53.4-2/GFOpenSQLite-3.53.4-2.zip\tsqlite.library 1.0\r\n"
    "Elsewhere\t1.0\t10\t" SHA_C "\thttps://example.com/x/Elsewhere.lha\tsent away\n"
    "Deeper\t1.0\t10\t" SHA_C "\t" REL "a/b/Deeper.lha\tnot tag/file\n"
    "Dots\t1.0\t10\t" SHA_C "\t" REL "Dots-1.0/../x.lha\tparent\n"
    "NoSha\t1.0\t10\tabc\t" REL "NoSha-1.0/NoSha.lha\tbad hash\n"
    "Cut\t1.0\t10\t" SHA_C "\n"
    "Blanks\t1.0\t10\t" SHA_C "\t" REL "Blanks-1.0/my file.lha\tblank\n"
    "\n"
    "GFOpenUpdate\t0.3\t200000\t" SHA_A "\t" REL "GFOpenUpdate-0.3/GFOpenUpdate-0.3.lha";   /* no newline, no description */

static const char manifest[] =
    "GFOpenUp\t0.2\n"
    "part\tGFOpenUp\t0.8.31-rc1\n"
    "part\tAmiPortAtlas\t0.4.0-rc1\n"
    "part\tGFOpenUpdate\t0.3\n"
    "GFOpenUp\tend\n";

int main(void)
{
    struct ouc_item it[8];
    char t[300], f[80];
    int bad = -1, n, newer, fresh = -1;

    n = ouc_parse(cat, (long)strlen(cat), REL, it, 8, &bad);
    CHECK(n == 3, "three names: AmiPortAtlas once, GFOpenSQLite, GFOpenUpdate");
    CHECK(bad == 6, "the six wrong lines are left out and counted");
    CHECK(!strcmp(it[0].p.name, "AmiPortAtlas") && !strcmp(it[0].p.version, "0.4.0-rc2"), "AmiPortAtlas at its newest");
    CHECK(!strcmp(it[0].p.sha256, SHA_B) && it[0].p.bytes == 1583429, "rc2's own size and SHA-256");
    CHECK(!strcmp(it[0].p.file, "AmiPortAtlas-0.4.0-rc2.lha"), "the file is the address's last part");
    CHECK(strchr(it[0].desc, '?') && !strchr(it[0].desc, '\xe2'), "the description in plain letters");
    CHECK(!strcmp(it[1].p.file, "GFOpenSQLite-3.53.4-2.zip") && !strcmp(it[1].desc, "sqlite.library 1.0"), "CRLF line");
    CHECK(!strcmp(it[2].p.name, "GFOpenUpdate") && it[2].desc[0] == 0, "the last line without newline or description");
    CHECK(ouc_parse(cat, (long)strlen(cat), "https://example.com/", it, 8, &bad) == 1 && bad == 9
          && !strcmp(it[0].p.name, "Elsewhere"), "with RELEASES elsewhere, only that address is taken");

    n = ouc_parse(cat, (long)strlen(cat), REL, it, 8, &bad);
    newer = ouc_weigh(it, n, manifest, (long)strlen(manifest), &fresh);
    CHECK(newer == 1 && fresh == 1, "one newer, one not installed");
    CHECK(it[0].state == OUC_NEWER && !strcmp(it[0].p.name, "AmiPortAtlas"), "the newer first");
    CHECK(it[1].state == OUC_NEW && !strcmp(it[1].p.name, "GFOpenSQLite"), "then the not installed");
    CHECK(it[2].state == OUC_SAME && !strcmp(it[2].p.name, "GFOpenUpdate"), "then the up to date");
    ouc_row_text(&it[0], t, sizeof t);
    CHECK(!strcmp(t, "AmiPortAtlas  0.4.0-rc1 -> 0.4.0-rc2  new"), "a newer row");
    ouc_row_text(&it[1], t, sizeof t);
    CHECK(!strcmp(t, "GFOpenSQLite  3.53.4-2  not installed"), "a not installed row");
    ouc_row_text(&it[2], t, sizeof t);
    CHECK(!strcmp(t, "GFOpenUpdate  0.3  up to date"), "an up to date row");
    n = ouc_parse(cat, (long)strlen(cat), REL, it, 8, NULL);
    CHECK(ouc_weigh(it, n, NULL, 0, &fresh) == 0 && fresh == 3, "no manifest: all not installed");

    CHECK(!strcmp(ouc_status(1, 0, 0), "Nothing new: everything on gfosdistribution is installed and up to date."), "nothing new");
    CHECK(strstr(ouc_status(-1, 0, 0), "couldn't") != NULL, "no catalogue");
    CHECK(strstr(ouc_status(1, 2, 0), "Install all") != NULL, "updates ready");

    CHECK(ouc_dest("GFOpenSQLite", "Libs/sqlite.library", t, sizeof t) && !strcmp(t, "LIBS:sqlite.library"), "Libs");
    CHECK(ouc_dest("GFOpenSQLite", "C/SQLiteLibTest", t, sizeof t) && !strcmp(t, "C:SQLiteLibTest"), "C");
    CHECK(ouc_dest("X", "libs/a/b.library", t, sizeof t) && !strcmp(t, "LIBS:a/b.library"), "any case, deeper");
    CHECK(ouc_dest("X", "WBStartup/X", t, sizeof t) && !strcmp(t, "SYS:WBStartup/X"), "WBStartup");
    CHECK(ouc_dest("GFOpenSQLite", "SDK/Include/sqlite3.h", t, sizeof t)
          && !strcmp(t, "SYS:Software/GFOpenSQLite/SDK/Include/sqlite3.h"), "the rest under SYS:Software");
    CHECK(ouc_dest("GFOpenSQLite", "ReadMe.txt", t, sizeof t) && !strcmp(t, "SYS:Software/GFOpenSQLite/ReadMe.txt"), "top file");
    CHECK(ouc_dest("X", "Classic/x", t, sizeof t) && !strcmp(t, "SYS:Software/X/Classic/x"), "C only as a whole name");
    CHECK(!ouc_dest("X", "a\"b", t, sizeof t), "a quote refused");
    CHECK(!ouc_dest("a/b", "x", t, sizeof t), "a name with a drawer refused");
    CHECK(!ouc_dest("X", "Libs/x", t, 5), "too long refused");
    CHECK(ouc_part_line("GFOpenSQLite", "Libs/sqlite.library", t, sizeof t)
          && !strcmp(t, "copy \"Libs/sqlite.library\" \"LIBS:sqlite.library\"\n"), "a Part line");
    CHECK(ouc_address(REL "T-1/T-1.lha", REL, f, sizeof f) && !strcmp(f, "T-1.lha"), "an address");
    CHECK(!ouc_address(REL "T-1/T-1.lha?x=1", REL, f, sizeof f), "no query");
    CHECK(!ouc_address(REL "T-1/", REL, f, sizeof f), "no file");

    {
        struct oud_part p[1];
        memset(p, 0, sizeof p);
        strcpy(p[0].name, "GFOpenPlay"); strcpy(p[0].version, "0.1.8"); strcpy(p[0].file, "GFOpenPlay-0.1.8.lha");
        p[0].bytes = 5; strcpy(p[0].sha256, SHA_A);
        n = ouc_from_index(p, 1, "https://github.com/DalsinAI/gfopenup/releases/download/updates", it, 8);
        CHECK(n == 1 && !strcmp(it[0].url, "https://github.com/DalsinAI/gfopenup/releases/download/updates/GFOpenPlay-0.1.8.lha"),
              "the old index as the fallback");
    }

    printf("%d checks, %d failed\n", checks, fails);
    return fails != 0;
}
