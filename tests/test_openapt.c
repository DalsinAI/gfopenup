/* Host test of src/openapt.h: C:apt's command line, its repo list, the indexes it reads (GFOpenUpdate's and
 * gfosdistribution's), the catalogue against GFOpenUpTool's manifest, what list, search and show print, the order
 * parts go in by their needs, and what remove refuses.
 *   cc -std=c99 -Wall -Werror -Isrc tests/test_openapt.c -o t && ./t
 * tests/test_build_pack.py builds and runs it.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <stdio.h>
#include <string.h>
/* the shared headers carry what the window and GFOpenUpTool use too; their own tests weigh those */
#pragma GCC diagnostic ignored "-Wunused-function"
#include "openup_lines.h"
#include "openapt.h"

static int fails, checks;
#define CHECK(c, msg) do { checks++; if (!(c)) { fails++; printf("FAIL %s\n", msg); } } while (0)

#define SHA_A "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
#define SHA_B "ABCDEF0123456789abcdef0123456789abcdef0123456789abcdef0123456789"

static const char index_ok[] =
    "GFOpenUpdate 1\n"
    "part GFOpenPlay 0.1.7-rc1 123456 " SHA_A " GFOpenPlay-0.1.7-rc1.lha\n"
    "part GFOpenIcons 0.8.31 2048 " SHA_B " GFOpenIcons-0.8.31.lha\n"
    "part GFOpenDock 0.6.15 999 " SHA_A " GFOpenDock-0.6.15.lha\n"
    "part GFOpenSynth 1.0 5 " SHA_A " GFOpenSynth-1.0.lha\n"
    "part KickstartSet 0.8.31 7 " SHA_A " KickstartSet-0.8.31.lha\n"
    "GFOpenUpdate end\n";

static const char software_ok[] =
    "# GFOS software: name, version, size, sha256, address, description (tabs between)\n"
    "\n"
    "LumaVault\t1.2\t4096\t" SHA_A "\thttps://github.com/amigagfosports/gfosdistribution/releases/download/software/LumaVault-1.2.lha\tThe media vault's client\r\n"
    "GFOpenSynth\t1.1\t6\t" SHA_B "\thttps://example.org/s/GFOpenSynth-1.1.lha\n";

static const char manifest[] =
    "GFOpenUp\t0.2\n"
    "part\tGFOpenUp\t0.8.30-rc6\n"
    "file\tC:GFOpenUpTool\n"
    "part\tGFOpenPlay\t0.1.6\n"
    "part\tGFOpenIcons\t0.8.30-rc6\n"
    "part\tGFOpenDock\t0.6.15\n"
    "part\tGFOpenLibs\t1.0\n"
    "part\tOldThing\t9.0\n"
    "needpart\tGFOpenDock\tGFOpenLibs\n"
    "needpart\tGone\tGFOpenPlay\n"
    "GFOpenUp\tend\n";
#define MLEN ((long)strlen(manifest))

static struct oua_source src[OUA_MAX_SOURCES];
static struct oud_part scratch[16];
static struct oua_pkg cat[16], more[16];

static int sources_of(const char *text, int max, int *bad)
{
    return oua_sources(text, (long)strlen(text), src, max, bad);
}

static int parse(const char *cmd, const char *const *names, int n, struct oua_args *a)
{
    const char *why = NULL;
    int ok = oua_parse(cmd, names, n, a, &why);
    CHECK(ok ? why == NULL : (why && *why), "a refused command line has a reason, a taken one has none");
    return ok;
}

int main(void)
{
    struct oua_args a;
    int n, m, bad;
    (void)ouv_ver; (void)ouv_keep_newer;

    /* the command line */
    {
        const char *one[] = { "GFOpenPlay" }, *two[] = { "GFOpenPlay", "GFOpenDock" };
        const char *inst[] = { "INSTALLED" }, *upg[] = { "upgradable" }, *junk[] = { "new" };
        CHECK(parse("update", NULL, 0, &a) && a.cmd == OUA_UPDATE, "update");
        CHECK(parse("UPDATE", NULL, 0, &a) && a.cmd == OUA_UPDATE, "a command in capitals, as AmigaDOS reads its own");
        CHECK(!parse("update", one, 1, &a), "update takes nothing after it");
        CHECK(parse("list", NULL, 0, &a) && a.cmd == OUA_LIST && a.which == OUA_ALL, "list");
        CHECK(parse("list", inst, 1, &a) && a.which == OUA_INSTALLED, "list installed, in any case");
        CHECK(parse("list", upg, 1, &a) && a.which == OUA_UPGRADABLE, "list upgradable");
        CHECK(!parse("list", junk, 1, &a), "list with a word it doesn't know");
        CHECK(!parse("list", two, 2, &a), "list with two words");
        CHECK(!parse("search", NULL, 0, &a), "search needs its text");
        CHECK(parse("search", two, 2, &a) && a.cmd == OUA_SEARCH && a.count == 2, "search with two words");
        CHECK(!parse("show", NULL, 0, &a) && !parse("install", NULL, 0, &a) && !parse("remove", NULL, 0, &a),
              "show, install and remove need a name");
        CHECK(parse("install", two, 2, &a) && a.cmd == OUA_INSTALL && a.count == 2 && a.names == two, "install two");
        CHECK(parse("upgrade", NULL, 0, &a) && a.cmd == OUA_UPGRADE && a.count == 0, "upgrade everything");
        CHECK(parse("upgrade", one, 1, &a) && a.count == 1, "upgrade one");
        CHECK(parse("remove", one, 1, &a) && a.cmd == OUA_REMOVE, "remove");
        CHECK(parse("sources", NULL, 0, &a) && a.cmd == OUA_REPOS, "sources");
        CHECK(parse("help", NULL, 0, &a) && a.cmd == OUA_HELP, "help");
        CHECK(!parse("dist-upgrade", NULL, 0, &a) && a.cmd == OUA_NONE, "a command it doesn't know");
        CHECK(!parse(NULL, NULL, 0, &a), "no command");
        CHECK(strstr(OUA_USAGE, "apt install") && strstr(OUA_USAGE, "apt remove") && strstr(OUA_USAGE, OUA_SOURCES), "the usage names them");
    }

    /* the repo list */
    {
        static const char list[] =
            "; GFOS's repo list\n"
            "github DalsinAI/gfopenup updates\n"
            "\n"
            "# a repository's newest release\n"
            "github DalsinAI/gfopenbrowser\r\n"
            "  off github DalsinAI/gfoffice\n"
            "index https://github.com/amigagfosports/gfosdistribution/releases/download/software/index.txt\n"
            "INDEX https://example.org/parts/\n";
        char name[14], other[14];
        n = sources_of(list, OUA_MAX_SOURCES, &bad);
        CHECK(n == 5 && bad == 0, "five sources, comments and empty lines passed over");
        CHECK(n == 5 && src[0].kind == OUA_KIND_GITHUB && !src[0].off && !strcmp(src[0].label, "github DalsinAI/gfopenup updates")
              && !strcmp(src[0].base, OUD_URL_DEFAULT) && !strcmp(src[0].url, OUD_URL_DEFAULT OUD_INDEX_NAME),
              "a release by its tag is where GFOpenUpdate itself looks");
        CHECK(n == 5 && !strcmp(src[1].url, "https://github.com/DalsinAI/gfopenbrowser/releases/latest/download/GFOpenUpdate.index")
              && !strcmp(src[1].base, "https://github.com/DalsinAI/gfopenbrowser/releases/latest/download/"),
              "a repository alone: its newest release's GFOpenUpdate.index");
        CHECK(n == 5 && src[2].off && !strcmp(src[2].label, "github DalsinAI/gfoffice"), "off: listed, not read");
        CHECK(n == 5 && src[3].kind == OUA_KIND_INDEX
              && !strcmp(src[3].url, "https://github.com/amigagfosports/gfosdistribution/releases/download/software/index.txt")
              && !strcmp(src[3].base, "https://github.com/amigagfosports/gfosdistribution/releases/download/software/"),
              "an index by its address; its drawer is where plain file names are");
        CHECK(n == 5 && !strcmp(src[4].url, "https://example.org/parts/GFOpenUpdate.index") && !strcmp(src[4].base, "https://example.org/parts/"),
              "an address ending in a slash is a drawer holding GFOpenUpdate.index");
        oua_list_name(&src[0], name);
        oua_list_name(&src[1], other);
        CHECK(strlen(name) == 13 && !strcmp(name + 8, ".list") && strcmp(name, other), "a kept list's name: 8 hex and .list, its own");
        n = sources_of("github DalsinAI/gfopenbrowser\ngithub DalsinAI/gfopenup updates\n", OUA_MAX_SOURCES, &bad);
        oua_list_name(&src[1], other);
        CHECK(n == 2 && !strcmp(name, other), "the same source keeps its list wherever it stands in the repo list");

        CHECK(!strcmp(OUA_SOURCES, "S:apt.sources"), "the repo list is S:apt.sources");
        CHECK(sources_of("", OUA_MAX_SOURCES, &bad) == 0 && bad == 0, "an empty repo list has no source");
        CHECK(sources_of("github DalsinAI/gfopenup updates\napt https://x.example/\n", 8, &bad) == -1 && bad == 2, "a kind it doesn't know: the line is named");
        CHECK(sources_of("github gfopenup\n", 8, &bad) == -1 && bad == 1, "github without an owner");
        CHECK(sources_of("github DalsinAI/gf/openup\n", 8, &bad) == -1, "github with a drawer too many");
        CHECK(sources_of("github DalsinAI/../x\n", 8, &bad) == -1, "github with dots for a name");
        CHECK(sources_of("github Dalsin\"AI/x\n", 8, &bad) == -1, "a quote in a repository's name");
        CHECK(sources_of("github DalsinAI/gfopenup up/dates\n", 8, &bad) == -1, "a tag with a drawer");
        CHECK(sources_of("github DalsinAI/gfopenup updates more\n", 8, &bad) == -1, "a word too many");
        CHECK(sources_of("github\n", 8, &bad) == -1 && sources_of("off\n", 8, &bad) == -1, "a line cut short");
        CHECK(sources_of("index ftp://example.org/index.txt\n", 8, &bad) == -1, "an index that isn't http or https");
        CHECK(sources_of("index https://example.org\n", 8, &bad) == -1, "an index with a host and no file");
        CHECK(sources_of("index SYS:index.txt\n", 8, &bad) == -1, "an index on a volume");
        CHECK(sources_of("github a/b\ngithub a/b\n", 8, &bad) == -1 && bad == 2, "the same source twice");
        CHECK(sources_of("github a/b\ngithub a/c\ngithub a/d\n", 2, &bad) == -1 && bad == 3, "more sources than the table holds");
        {
            char longline[400] = "index https://example.org/";
            memset(longline + strlen(longline), 'a', 300);
            strcat(longline, "\n");
            CHECK(sources_of(longline, 8, &bad) == -1 && bad == 1, "an address too long for its place");
        }
    }

    /* the host to ask "is it there?" */
    {
        char host[40];
        int port = 0;
        CHECK(oua_host(OUD_URL_DEFAULT, host, sizeof host, &port) && !strcmp(host, "github.com") && port == 443, "https: the host, port 443");
        CHECK(oua_host("http://bigguy.lan:3000/x/index.txt", host, sizeof host, &port) && !strcmp(host, "bigguy.lan") && port == 3000, "a port written");
        CHECK(oua_host("http://example.org/x", host, sizeof host, &port) && port == 80, "http: port 80");
        CHECK(!oua_host("ftp://example.org/x", host, sizeof host, &port), "not http: no host");
        CHECK(!oua_host("https://example.org:0/x", host, sizeof host, &port) && !oua_host("https://example.org:x/", host, sizeof host, &port), "a port that isn't one");
        CHECK(!oua_host("https://a-very-long-host-name.example.org/x", host, 10, &port), "a host too long for its place");
    }

    /* the indexes */
    n = sources_of("github DalsinAI/gfopenup updates\nindex https://example.org/software/index.txt\n", OUA_MAX_SOURCES, &bad);
    n = oua_read_list(index_ok, (long)strlen(index_ok), &src[0], 0, scratch, cat, 16);
    CHECK(n == 5, "GFOpenUpdate's index: five packages");
    CHECK(n == 5 && !strcmp(cat[0].p.name, "GFOpenPlay") && !strcmp(cat[0].p.version, "0.1.7-rc1") && cat[0].p.bytes == 123456
          && !strcmp(cat[0].url, OUD_URL_DEFAULT "GFOpenPlay-0.1.7-rc1.lha") && cat[0].source == 0 && !cat[0].desc[0],
          "a part's fields, its address beside the index");
    CHECK(oua_read_list(index_ok, (long)strlen(index_ok) - 10, &src[0], 0, scratch, cat, 16) == -1, "an index cut short: none of it");
    CHECK(oua_read_list(index_ok, (long)strlen(index_ok), &src[0], 0, scratch, cat, 3) == -1, "more parts than the table holds: none of it");
    CHECK(oua_read_list("<html>Not Found</html>\n", 23, &src[0], 0, scratch, cat, 16) == -1, "a web page is no index");
    CHECK(oua_read_list("Not Found", 9, &src[0], 0, scratch, cat, 16) == -1, "GitHub's answer for a file that isn't there is no index");
    CHECK(oua_read_list("", 0, &src[0], 0, scratch, cat, 16) == -1, "nothing is no index");
    CHECK(oua_read_list("GFOpenUpdate 2\nGFOpenUpdate end\n", 32, &src[0], 0, scratch, cat, 16) == -1, "a later kind of index is left alone");

    m = oua_read_list(software_ok, (long)strlen(software_ok), &src[1], 1, scratch, more, 16);
    CHECK(m == 2, "gfosdistribution's index: two packages");
    CHECK(m == 2 && !strcmp(more[0].p.name, "LumaVault") && !strcmp(more[0].p.version, "1.2") && more[0].p.bytes == 4096
          && !strcmp(more[0].p.sha256, SHA_A) && !strcmp(more[0].p.file, "LumaVault-1.2.lha")
          && !strcmp(more[0].url, "https://github.com/amigagfosports/gfosdistribution/releases/download/software/LumaVault-1.2.lha")
          && !strcmp(more[0].desc, "The media vault's client") && more[0].source == 1, "its fields, the CR gone from the description");
    CHECK(m == 2 && !more[1].desc[0], "a line without a description");
    CHECK(oua_read_list("# GFOS software: nothing yet\n", 29, &src[1], 1, scratch, more, 16) == 0, "only its comment: an empty catalogue");
    m = oua_read_list(software_ok, (long)strlen(software_ok), &src[1], 1, scratch, more, 16);
    {
        struct oua_pkg p[4];
#define SOFT(line) oua_read_list(line, (long)strlen(line), &src[1], 1, scratch, p, 4)
        CHECK(SOFT("A\t1.0\t10\t" SHA_A "\n") == -1, "a field missing");
        CHECK(SOFT("A\t1.0\t10\t0123\thttps://x.example/a.lha\n") == -1, "a short SHA-256");
        CHECK(SOFT("A\t1.0\t0\t" SHA_A "\thttps://x.example/a.lha\n") == -1, "no bytes");
        CHECK(SOFT("A\tnew\t10\t" SHA_A "\thttps://x.example/a.lha\n") == -1, "a version without a number");
        CHECK(SOFT("A B\t1.0\t10\t" SHA_A "\thttps://x.example/a.lha\n") == -1, "a blank in a name");
        CHECK(SOFT("../A\t1.0\t10\t" SHA_A "\thttps://x.example/a.lha\n") == -1, "a drawer in a name");
        CHECK(SOFT("A\t1.0\t10\t" SHA_A "\tSYS:a.lha\n") == -1, "an address on a volume");
        CHECK(SOFT("A\t1.0\t10\t" SHA_A "\thttps://x.example/a b.lha\n") == -1, "a blank in an address");
        CHECK(SOFT("A\t1.0\t10\t" SHA_A "\thttps://x.example/\"a.lha\n") == -1, "a quote in an address");
        CHECK(SOFT("A\t1.0\t10\t" SHA_A "\thttps://x.example/dl?file=a.lha\n") == -1, "an address whose last name isn't a file's");
        CHECK(SOFT("A\t1.0\t10\t" SHA_A "\thttps://x.example/a.lha\nA\t1.1\t10\t" SHA_A "\thttps://x.example/b.lha\n") == -1, "a name listed twice");
        CHECK(SOFT("A\t1.0\t10\t" SHA_A "\thttps://x.example/a.lha\ttabs\tin\tits words\n") == 1 && !strcmp(p[0].desc, "tabs in its words"),
              "tabs in a description are blanks");
        {
            char longd[400] = "A\t1.0\t10\t" SHA_A "\thttps://x.example/a.lha\t";
            memset(longd + strlen(longd), 'd', 300);
            strcat(longd, "\n");
            CHECK(SOFT(longd) == 1 && strlen(p[0].desc) == sizeof p[0].desc - 1, "a long description is cut to its place");
        }
#undef SOFT
    }

    /* the catalogue: sources merged */
    n = oua_read_list(index_ok, (long)strlen(index_ok), &src[0], 0, scratch, cat, 16);
    n = oua_merge(cat, n, 16, more, m);
    CHECK(n == 6, "two sources as one catalogue, a name both have listed once");
    CHECK(n == 6 && !strcmp(cat[3].p.name, "GFOpenSynth") && !strcmp(cat[3].p.version, "1.1") && cat[3].source == 1,
          "the newer version of the two, from its own source");
    CHECK(n == 6 && !strcmp(cat[5].p.name, "LumaVault"), "a new name goes after");
    {
        struct oua_pkg same = cat[0];
        same.source = 1;
        CHECK(oua_merge(cat, n, 16, &same, 1) == 6 && cat[0].source == 0, "the same version twice: the earlier source keeps it");
        CHECK(oua_merge(cat, n, 6, &same, 1) == 6, "a name already there needs no room");
        strcpy(same.p.name, "Another");
        CHECK(oua_merge(cat, n, 6, &same, 1) == -1, "a full catalogue");
    }
    CHECK(oua_find(cat, n, "GFOpenDock") == 2 && oua_find(cat, n, "gfopendock") == 2 && oua_find(cat, n, "GFOpen") == -1,
          "a package by its name, in any case, never by a part of it");

    /* against the manifest */
    {
        char have[24], t[120], show[700];
        struct oud_row row, rows[16];
        const char *w1[] = { "synth" }, *w2[] = { "vault", "CLIENT" }, *w3[] = { "vault", "synth" }, *w4[] = { "media" };
        int offers, r;
        CHECK(oua_weigh(&cat[0], manifest, MLEN, have, sizeof have) == OUA_NEWER && !strcmp(have, "0.1.6"), "installed, a newer one in the sources");
        CHECK(oua_weigh(&cat[2], manifest, MLEN, have, sizeof have) == OUA_SAME && !strcmp(have, "0.6.15"), "installed, the same");
        CHECK(oua_weigh(&cat[3], manifest, MLEN, have, sizeof have) == OUA_NEW && !have[0], "not installed");
        CHECK(oua_weigh(&cat[3], NULL, 0, have, sizeof have) == OUA_NEW, "no manifest: nothing is installed");
        {
            struct oua_pkg old = cat[2];
            strcpy(old.p.version, "0.6.14");
            CHECK(oua_weigh(&old, manifest, MLEN, have, sizeof have) == OUA_OLDER, "what is installed is newer than the source's");
        }
        oua_row(&cat[0], manifest, MLEN, &row); oud_row_text(&row, t, sizeof t);
        CHECK(!strcmp(t, "GFOpenPlay  0.1.6  new: 0.1.7-rc1"), "list: an upgradable row, in the window's words");
        oua_row(&cat[2], manifest, MLEN, &row); oud_row_text(&row, t, sizeof t);
        CHECK(!strcmp(t, "GFOpenDock  0.6.15"), "list: an up-to-date row");
        oua_row(&cat[5], manifest, MLEN, &row); oud_row_text(&row, t, sizeof t);
        CHECK(!strcmp(t, "LumaVault  not installed: 1.2"), "list: a row not installed");

        offers = oua_offers(cat, n, manifest, MLEN, scratch);
        CHECK(offers == 2 && !strcmp(scratch[0].name, "GFOpenPlay") && !strcmp(scratch[1].name, "GFOpenIcons")
              && !strcmp(scratch[1].installed, "0.8.30-rc6"), "upgrade: the installed parts with a newer version, no others");
        CHECK(oua_find(cat, n, scratch[1].name) == 1, "an offer found again in the catalogue");
        CHECK(oua_offers(cat, n, NULL, 0, scratch) == 0, "upgrade with no manifest: nothing");
        offers = oua_offers(cat, n, manifest, MLEN, scratch);
        r = oud_rows(NULL, 0, scratch, offers, rows, 16);
        CHECK(r == 2 && !strcmp(rows[0].name, "GFOpenPlay") && !strcmp(rows[0].newer, "0.1.7-rc1"), "list upgradable: the offers alone");
        r = oud_rows(manifest, MLEN, scratch, offers, rows, 16);
        CHECK(r == 6 && !strcmp(rows[0].name, "GFOpenPlay") && !strcmp(rows[2].name, "GFOpenUp") && !strcmp(rows[5].name, "OldThing") && !rows[5].newer[0],
              "list installed: every part the manifest has, one no source lists too");

        CHECK(oua_match(&cat[3], w1, 1) && !oua_match(&cat[0], w1, 1), "search by a part of a name, in any case");
        CHECK(oua_match(&cat[5], w2, 2), "search: every word, in the name or the description");
        CHECK(!oua_match(&cat[5], w3, 2), "search: one word missing");
        CHECK(oua_match(&cat[5], w4, 1), "search in the description alone");

        oua_weigh(&cat[5], manifest, MLEN, have, sizeof have);
        oua_show_text(&cat[5], have, src[cat[5].source].label, show, sizeof show);
        CHECK(!strcmp(show,
              "Name: LumaVault\nVersion: 1.2\nInstalled: no\nSize: 4096 bytes\nSHA-256: " SHA_A "\n"
              "From: index https://example.org/software/index.txt\n"
              "Address: https://github.com/amigagfosports/gfosdistribution/releases/download/software/LumaVault-1.2.lha\n"
              "About: The media vault's client\n"), "show: a package not installed");
        oua_weigh(&cat[0], manifest, MLEN, have, sizeof have);
        oua_show_text(&cat[0], have, src[0].label, show, sizeof show);
        CHECK(strstr(show, "\nInstalled: 0.1.6\n") && strstr(show, "\nFrom: github DalsinAI/gfopenup updates\n") && !strstr(show, "About:"),
              "show: an installed one, with no description");
        CHECK(oua_show_text(&cat[5], "", "x", show, 20) < 20 && strlen(show) < 20, "show never writes past its place");
    }

    /* what needs what */
    {
        static const char part[] =
            "part GFOpenWrite 2.0.25\n"
            "needs part GFOffice\n"
            "needs 68040\n"
            "copy Files/GFOpenWrite SYS:Utilities/GFOpenWrite\n"
            "needs part GFOpenLayout\n";
        char need[64];
        short order[OUA_MAX_JOBS];
        int ring = -1;
        CHECK(oul_part_needs(part, 0, need, sizeof need) && !strcmp(need, "GFOffice"), "a Part file's first need (src/openup_lines.h)");
        CHECK(oul_part_needs(part, 1, need, sizeof need) && !strcmp(need, "GFOpenLayout"), "its second; a machine it needs is no part");
        CHECK(!oul_part_needs(part, 2, need, sizeof need), "and no third");
        {   /* asked for: Write(0), Calc(1); found on the way: GFOffice(2), which needs GFOpenLayout(3) */
            static const short e[][2] = { { 0, 2 }, { 1, 2 }, { 2, 3 }, { 0, 3 } };
            CHECK(oua_order(4, e, 4, order, &ring) && order[0] == 3 && order[1] == 2 && order[2] == 0 && order[3] == 1,
                  "what is needed goes in first, the rest as asked");
        }
        {
            static const short none[1][2] = { { 0, 0 } };
            CHECK(oua_order(3, none, 0, order, &ring) && order[0] == 0 && order[1] == 1 && order[2] == 2, "no needs: as asked");
            CHECK(oua_order(0, none, 0, order, &ring), "nothing to order");
        }
        {
            static const short e[][2] = { { 0, 1 }, { 1, 2 }, { 2, 0 } };
            CHECK(!oua_order(3, e, 3, order, &ring) && ring >= 0 && ring < 3, "three parts in a ring: refused, one of them named");
        }
        {
            static const short e[][2] = { { 1, 1 } };
            CHECK(!oua_order(2, e, 1, order, &ring) && ring == 1, "a part that needs itself: refused");
        }
        {
            static const short e[][2] = { { 0, 1 }, { 2, 1 }, { 0, 1 } };
            CHECK(oua_order(3, e, 3, order, &ring) && order[0] == 1 && order[1] == 0 && order[2] == 2, "a need named twice, and by two parts: once");
        }
        CHECK(!oua_order(OUA_MAX_JOBS + 1, NULL, 0, order, &ring), "more parts than one go holds");
    }

    /* remove */
    {
        char why[200], name[40], ver[24], cmd[200];
        CHECK(oua_can_remove("GFOpenIcons", manifest, MLEN, why, sizeof why) && !why[0], "an installed part nothing needs may go");
        CHECK(!oua_can_remove("GFOpenSynth", manifest, MLEN, why, sizeof why) && !strcmp(why, "GFOpenSynth isn't installed."), "not installed");
        CHECK(!oua_can_remove("GFOpenLibs", manifest, MLEN, why, sizeof why)
              && !strcmp(why, "GFOpenLibs stays: GFOpenDock needs it. Remove GFOpenDock first."), "a part an installed part needs stays");
        CHECK(oua_can_remove("GFOpenPlay", manifest, MLEN, why, sizeof why), "a need noted for a part that is gone doesn't hold it");
        CHECK(!oua_can_remove("GFOpenUp", manifest, MLEN, why, sizeof why) && strstr(why, "Uninstall-GFOpenUp"), "GFOpenUp itself: its own uninstaller");
        CHECK(!oua_can_remove("GFOpenPlay", NULL, 0, why, sizeof why), "no manifest: nothing to remove");
        CHECK(!oua_can_remove("GFOpenLibs", manifest, MLEN, why, 12) && strlen(why) < 12, "a reason never written past its place");
        CHECK(oua_part_head("; a part\npart GFOpenPlay 0.1.6\ncopy a b\n", 39, name, sizeof name, ver, sizeof ver)
              && !strcmp(name, "GFOpenPlay") && !strcmp(ver, "0.1.6"), "a Part file's name and version");
        CHECK(!oua_part_head("copy a b\n", 9, name, sizeof name, ver, sizeof ver), "a Part file without its part line");
        CHECK(oua_remove_cmd("SYS:Storage/GFOpenApt/GFOpenPlay", cmd, sizeof cmd)
              && !strcmp(cmd, "SYS:C/GFOpenUpTool REMOVE \"SYS:Storage/GFOpenApt/GFOpenPlay\""), "the remove");
        CHECK(!oua_remove_cmd("SYS:a\"b", cmd, sizeof cmd) && !oua_remove_cmd("SYS:Storage/GFOpenApt/GFOpenPlay", cmd, 20), "a drawer that can't be quoted, or won't fit");
    }

    /* the return code of a run, and numbers as text */
    CHECK(oua_worst(0, 5) == 5 && oua_worst(10, 5) == 10 && oua_worst(0, 0) == 0, "the worst return code of a run");
    {
        char t[20];
        t[0] = 0;
        CHECK(oua_num(t, sizeof t, 0, 0) == 1 && !strcmp(t, "0"), "0 as digits");
        CHECK(oua_num(t, sizeof t, 1, 123456) == 7 && !strcmp(t, "0123456"), "a number after what is there");
    }

    printf("%d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
