/* Host test of src/openup_lines.h, the Part file lines' conditions GFOpenUpTool reads (0.8.9).
 *
 *   cc -std=c99 -Wall -Werror -Isrc tests/test_openup_lines.c -o test_openup_lines
 *   ./test_openup_lines                    the checks below
 *   ./test_openup_lines PART MACHINE       the Part file's lines as INSTALL acts on them on MACHINE: os323 (OS
 *                                          3.2.3's ROM), openkick100 (an open ROM with prefs.resource 47.100),
 *                                          openkick101 (47.101), each with "-ac" for an AmiCradle instance;
 *                                          "remove" for REMOVE's reading (every condition taken off)
 *   ./test_openup_lines install PART MANIFEST   INSTALL's "needs part" check of the Part file against the manifest
 *                                          (GFOpenUp.manifest's form): "missing <part>" for each part it needs that
 *                                          isn't installed, and 5 (RETURN_WARN: left out); else the manifest as
 *                                          INSTALL leaves it (the part's line, its needpart and startup lines), and 0
 *   ./test_openup_lines install PART MANIFEST fail   the same install with a file that couldn't be put in place:
 *                                          the manifest as INSTALL leaves it then (no part line, the startup and
 *                                          needpart lines as they were), and 10 (RETURN_ERROR)
 *   ./test_openup_lines remove PART MANIFEST    REMOVE's check: "needed by <part>" and 5 while an installed part
 *                                          needs this one; else the manifest as REMOVE leaves it, and 0
 * tests/test_build_pack.py runs these forms over the parts build_pack.py writes.
 *
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "openup_drop.h"
#include "openup_lines.h"

static int fails;

static void same(const char *what, const char *got, const char *want)
{
    if ((!got && !want) || (got && want && !strcmp(got, want))) { printf("ok   %s\n", what); return; }
    printf("FAIL %s: got [%s], wanted [%s]\n", what, got ? got : "(left out)", want ? want : "(left out)");
    fails++;
}

static void truth(const char *what, int got, int want)
{
    if (!got == !want) { printf("ok   %s\n", what); return; }
    printf("FAIL %s: got %d, wanted %d\n", what, got, want);
    fails++;
}

static int need_checks(void);
static int way_checks(void);
static int manifest_checks(void);

/* GFOpenUp's list, for oul_old_line_taken: it names one file */
static int gf_has(const char *path) { return !strcmp(path, "SYS:Developer/x.h"); }

static int checks(void)
{
    const unsigned none = 0, ac = OUL_AMICRADLE, ok = OUL_OPENKICK, both = OUL_AMICRADLE | OUL_OPENKICK;
    /* the ROM: GFOpenKick's mark and prefs.resource 47.101 or later, compared as two numbers */
    truth("OS 3.2.3's ROM (no mark, no prefs.resource): no prefs task", oul_prefs_task(0, 0, 0), 0);
    truth("no mark, prefs.resource 47.101: no prefs task", oul_prefs_task(0, 47, 101), 0);
    truth("GFOpenKick, prefs.resource 47.100: no prefs task", oul_prefs_task(1, 47, 100), 0);
    truth("GFOpenKick, prefs.resource 47.99: no prefs task", oul_prefs_task(1, 47, 99), 0);
    truth("GFOpenKick, prefs.resource 47.101: the prefs task", oul_prefs_task(1, 47, 101), 1);
    truth("GFOpenKick, prefs.resource 47.110: the prefs task", oul_prefs_task(1, 47, 110), 1);
    truth("GFOpenKick, prefs.resource 48.0: the prefs task", oul_prefs_task(1, 48, 0), 1);
    truth("GFOpenKick, prefs.resource 46.200: no prefs task", oul_prefs_task(1, 46, 200), 0);
    truth("GFOpenKick, no prefs.resource: no prefs task", oul_prefs_task(1, 0, 0), 0);
    /* the ACRTG board, each of its sizes (W144, 26 October 2026: Instance-32's 256 MiB board was "RTG: none") */
    truth("ACRTG: product 9, the 16 MiB board", oul_acrtg_product(9), 1);
    truth("ACRTG: product 15, the 64 MiB board", oul_acrtg_product(15), 1);
    truth("ACRTG: product 16, the 256 MiB board", oul_acrtg_product(16), 1);
    truth("ACRTG: product 1 (ACBridge) is not one", oul_acrtg_product(1), 0);
    truth("ACRTG: product 2 (ACStorage) is not one", oul_acrtg_product(2), 0);
    truth("ACRTG: product 6 (the GFOpenSocket card) is not one", oul_acrtg_product(6), 0);
    truth("ACRTG: product 17 is not one (yet)", oul_acrtg_product(17), 0);
    /* lines without a condition stay as they are */
    same("a plain line", oul_strip("copy \"a\" \"SYS:a\"", none, 0), "copy \"a\" \"SYS:a\"");
    same("a startup line", oul_strip("startup Run >NIL: C:GFOpenUpMenu", both, 0), "startup Run >NIL: C:GFOpenUpMenu");
    same("a line starting whenever", oul_strip("whenever x", both, 0), "whenever x");
    /* when AmiCradle, as since 0.6.3 */
    same("when AmiCradle, on AmiCradle", oul_strip("when AmiCradle copy \"a\" \"b\"", ac, 0), "copy \"a\" \"b\"");
    same("when AmiCradle, elsewhere", oul_strip("when AmiCradle copy \"a\" \"b\"", ok, 0), NULL);
    same("unless AmiCradle, on AmiCradle", oul_strip("unless AmiCradle copy \"a\" \"b\"", ac, 0), NULL);
    same("unless AmiCradle, elsewhere", oul_strip("unless AmiCradle copy \"a\" \"b\"", none, 0), "copy \"a\" \"b\"");
    /* GFOpenKick, startup lines and retire lines too */
    same("when GFOpenKick retire, on GFOpenKick", oul_strip("when GFOpenKick retire \"SYS:x\"", ok, 0), "retire \"SYS:x\"");
    same("when GFOpenKick retire, elsewhere", oul_strip("when GFOpenKick retire \"SYS:x\"", ac, 0), NULL);
    same("unless GFOpenKick startup, on GFOpenKick", oul_strip("unless GFOpenKick startup SYS:Preferences/GFOpenPrefs APPLY", both, 0), NULL);
    same("unless GFOpenKick startup, elsewhere", oul_strip("unless GFOpenKick startup SYS:Preferences/GFOpenPrefs APPLY", ac, 0),
         "startup SYS:Preferences/GFOpenPrefs APPLY");
    same("a tab between the words", oul_strip("when\tGFOpenKick\t\tretire x", ok, 0), "retire x");
    /* a machine this GFOpenUpTool doesn't know: the line stays out, as before 0.8.9 */
    same("when PiStorm", oul_strip("when PiStorm copy a b", both, 0), NULL);
    same("unless PiStorm", oul_strip("unless PiStorm copy a b", both, 0), NULL);
    /* REMOVE: every condition comes off, whether it holds or not */
    same("REMOVE: when GFOpenKick", oul_strip("when GFOpenKick retire x", none, 1), "retire x");
    same("REMOVE: unless GFOpenKick startup", oul_strip("unless GFOpenKick startup Path x", ok, 1), "startup Path x");
    same("REMOVE: when PiStorm", oul_strip("when PiStorm copy a b", none, 1), "copy a b");
    /* a condition with nothing after it is no condition */
    same("when GFOpenKick alone", oul_strip("when GFOpenKick", ok, 0), "when GFOpenKick");
    same("when alone", oul_strip("when", ok, 0), "when");
    return need_checks();
}

/* "needs part" (25 Oct 2026): the words of a line, the Part file's name, and the manifest's needpart lines */
static int need_checks(void)
{
    char l1[] = "copy \"Utilities/Open Write\" \"SYS:Utilities/Open Write\"", l2[] = "  ; a comment", l3[] = "needs part GFOffice",
         l4[] = "needs AmiCradle", l5[] = "needs part", l6[] = "needs parts GFOffice", l7[] = "needs\tpart\tGFOffice ; shared",
         name[16];
    char *w[12];
    int n;
    oul_entry m[] = {
        { "part", "GFOpenUp", "0.8.9" }, { "new", "LIBS:owf.library", "" }, { "part", "GFOffice", "1.1" },
        { "part", "GFOpenWrite", "2.0.1" }, { OUL_NEEDPART, "GFOpenWrite", "GFOffice" },
        { OUL_NEEDPART, "GFOpenCalc", "GFOffice" },       /* GFOpenCalc's part line is gone: REMOVE took it out by hand */
        { OUL_NEEDPART, "Odd", "Odd" }, { "part", "Odd", "1" },
    };
    const int nm = (int)(sizeof m / sizeof m[0]);
    n = oul_words(l1, w, 12);
    truth("words: a quoted word keeps its space", n == 3 && !strcmp(w[1], "Utilities/Open Write") && !strcmp(w[2], "SYS:Utilities/Open Write"), 1);
    truth("words: a comment line has none", oul_words(l2, w, 12) == 0, 1);
    n = oul_words(l3, w, 12);
    same("needs part GFOffice", oul_needs_part(w, n), "GFOffice");
    n = oul_words(l4, w, 12);
    same("needs AmiCradle is no part", oul_needs_part(w, n), NULL);
    n = oul_words(l5, w, 12);
    same("needs part with no name", oul_needs_part(w, n), NULL);
    n = oul_words(l6, w, 12);
    same("needs parts is not needs part", oul_needs_part(w, n), NULL);
    n = oul_words(l7, w, 12);
    same("needs part with tabs and a comment after", oul_needs_part(w, n), "GFOffice");
    /* needs 68040: the CPU floor (9 Nov 2026) */
    truth("68040 with its FPU", oul_full_040(0x0008u | 0x0040u | 0x0007u), 1);
    truth("68060 with its FPU", oul_full_040(0x0080u | 0x0008u | 0x0040u | 0x0007u), 1);
    truth("68LC040 or 68EC040 (no FPU)", oul_full_040(0x0008u | 0x0007u), 0);
    truth("68030 with a 68882", oul_full_040(0x0003u | 0x0010u | 0x0020u), 0);
    truth("68000", oul_full_040(0), 0);
    truth("installed: GFOffice", oul_installed(m, nm, "GFOffice"), 1);
    truth("installed: not GFOpenCalc (a needpart line is not a part line)", oul_installed(m, nm, "GFOpenCalc"), 0);
    truth("installed: not a name's start (GFOffic)", oul_installed(m, nm, "GFOffic"), 0);
    same("REMOVE GFOffice: GFOpenWrite needs it", oul_needed_by(m, nm, "GFOffice"), "GFOpenWrite");
    same("REMOVE GFOpenWrite: nothing needs it", oul_needed_by(m, nm, "GFOpenWrite"), NULL);
    same("REMOVE of a part that names itself: it doesn't stop itself", oul_needed_by(m, nm, "Odd"), NULL);
    m[3].kind[0] = 0;                                   /* REMOVE took GFOpenWrite out */
    same("REMOVE GFOffice once GFOpenWrite is out (GFOpenCalc's old line doesn't count)", oul_needed_by(m, nm, "GFOffice"), NULL);
    strcpy(m[3].kind, "part");
    oul_drop_needs(m, nm, "GFOpenWrite");
    truth("drop GFOpenWrite's needs: its needpart line goes, GFOpenCalc's stays", !m[4].kind[0] && !strcmp(m[5].kind, OUL_NEEDPART)
          && !strcmp(m[0].kind, "part"), 1);
    same("REMOVE GFOffice with GFOpenWrite in but its need dropped", oul_needed_by(m, nm, "GFOffice"), NULL);
    truth("part name: the part line", oul_part_name("part GFOffice 1.1\n; shared\n", name, sizeof name) && !strcmp(name, "GFOffice"), 1);
    truth("part name: after a comment, with tabs and CR LF", oul_part_name("; x\r\n\tpart\tGFOpenWrite\t2.0.1\r\n", name, sizeof name)
          && !strcmp(name, "GFOpenWrite"), 1);
    truth("part name: partial is no part line", oul_part_name("partial x\n", name, sizeof name), 0);
    truth("part name: one too long for the buffer", oul_part_name("part ABCDEFGHIJKLMNOPQRSTUVWXYZ 1\n", name, sizeof name), 0);
    /* REINSTALL's needs first (Ivanova's bundle C, 14 Nov 2026): GFOpenMUI 19.0 needs the GFOpenLayout part */
    {
        static const char mui[] = "part GFOpenMUI 19.0\r\nadd \"Libs/muimaster.library\" \"LIBS:muimaster.library\"\n"
                                  "needs part GFOpenLayout\n; needs part Commented\n\tneeds\tpart\tGFOffice\n";
        char nd[48];
        truth("part needs: the first", oul_part_needs(mui, 0, nd, sizeof nd) && !strcmp(nd, "GFOpenLayout"), 1);
        truth("part needs: the second, with tabs (a commented one isn't)", oul_part_needs(mui, 1, nd, sizeof nd) && !strcmp(nd, "GFOffice"), 1);
        truth("part needs: no third", oul_part_needs(mui, 2, nd, sizeof nd), 0);
        truth("part needs: none in a part without", oul_part_needs("part GFOpenLayout 18.0\ncopy \"a\" \"b\"\n", 0, nd, sizeof nd), 0);
        truth("part needs: a name too long for the buffer", oul_part_needs("needs part GFOpenLayout\n", 0, nd, 8), 0);
    }
    return way_checks();
}

static const char *way(const char *path, int i)
{
    static char out[32];
    return oul_way(path, i, out, sizeof out) ? out : NULL;
}

/* The kinds of a manifest's lines, in order, the taken-out ones left out: "part startup ostartup ..." */
static const char *kinds(const oul_entry *e, int n)
{
    static char out[256];
    int i;
    out[0] = 0;
    for (i = 0; i < n; i++)
        if (e[i].kind[0]) { if (out[0]) strcat(out, " "); strcat(out, e[i].kind); }
    return out;
}

/* INSTALL's way (a file where a part's drawer goes) and its end (a failed install isn't listed), 25 Oct 2026 */
static int way_checks(void)
{
    same("way 0 of SYS:Utilities/GFOpenCalc/GFOpenCalc", way("SYS:Utilities/GFOpenCalc/GFOpenCalc", 0), "SYS:Utilities");
    same("way 1: the GFOpenCalc drawer, where Instance-32's calculator is a file", way("SYS:Utilities/GFOpenCalc/GFOpenCalc", 1),
         "SYS:Utilities/GFOpenCalc");
    same("way 2: none (the file itself isn't a drawer of its way)", way("SYS:Utilities/GFOpenCalc/GFOpenCalc", 2), NULL);
    same("a file at a volume's top has no drawers", way("LIBS:owf.library", 0), NULL);
    same("a quoted name with a space", way("SYS:Utilities/Open Write/GFOpenWrite", 0), "SYS:Utilities");
    same("no volume: the first name", way("Utilities/GFOpenCalc", 0), "Utilities");
    same("a parent's \"/\" right after the volume: nothing is looked at", way("SYS:/x/y", 0), NULL);
    same("\"//\": the drawer before it", way("SYS:a//b/c", 0), "SYS:a");
    same("\"//\": nothing at or after it", way("SYS:a//b/c", 1), NULL);
    same("one that doesn't fit the buffer", way("SYS:AVeryLongDrawerNameIndeedThatGoesOn/x", 0), NULL);
    {
        /* GFOpenMenus 0.4.5 to 0.4.6: one startup line kept, one changed; GFOffice needed before and now */
        oul_entry m[12] = {
            { "part", "GFOpenMenus", "0.4.5" }, { "startup", "Run >NIL: C:GFOpenMenus", "GFOpenMenus" },
            { "startup", "C:GFOpenMenus OLD", "GFOpenMenus" }, { "startup", "Run >NIL: C:GFOpenDock", "GFOpenDock" },
            { OUL_NEEDPART, "GFOpenMenus", "GFOffice" }, { OUL_NEEDPART, "GFOpenWrite", "GFOffice" },
        };
        oul_entry k[12];
        int n = 6, first, i;
        /* pass 1, as openuptool.c install does it: mark, keep the line still there, add the new ones */
        for (i = 0; i < n; i++)
            if (!strcmp(m[i].kind, "startup") && !strcmp(m[i].b, "GFOpenMenus")) strcpy(m[i].kind, "ostartup");
        oul_mark_needs(m, n, "GFOpenMenus");
        same("needs marked: GFOpenMenus's only", kinds(m, n), "part ostartup ostartup startup oneedpart needpart");
        first = n;
        strcpy(m[1].kind, "startup");                           /* still in the Part file: kept, in its place */
        m[n].a = "C:GFOpenMenus NEW"; m[n].b = "GFOpenMenus"; strcpy(m[n++].kind, "startup");
        m[n].a = "GFOpenMenus"; m[n].b = "GFOpenFont"; strcpy(m[n++].kind, OUL_NEEDPART);
        memcpy(k, m, sizeof m);
        oul_settle(m, n, first, "GFOpenMenus", 1);
        same("settled, installed: the old lines go, the kept and new ones stay", kinds(m, n),
             "part startup startup needpart startup needpart");
        truth("installed: GFOpenDock's startup line and GFOpenWrite's need untouched",
              !strcmp(m[3].kind, "startup") && !strcmp(m[5].kind, OUL_NEEDPART), 1);
        oul_settle(k, n, first, "GFOpenMenus", 0);
        same("settled, failed: the lines are as before the install", kinds(k, n), "part startup startup startup needpart needpart");
        truth("failed: the changed line is back, the new ones are out",
              !strcmp(k[2].kind, "startup") && !strcmp(k[4].kind, OUL_NEEDPART) && !k[6].kind[0] && !k[7].kind[0], 1);
    }
    return manifest_checks();
}

static int whole(const char *text)
{
    return oul_manifest_whole(text, (long)strlen(text));
}

/* The manifest whole or cut short (1 Nov 2026): a short one is never saved over, so its "replaced" lines, the
 * originals UNINSTALL puts back, aren't lost */
static int manifest_checks(void)
{
    const char *now = OUL_MANIFEST_HEAD "\npart\tGFOpenUp\t0.8.12\nreplaced\tLIBS:x.library\tSYS:Storage/GFOpenUp-Superseded/LIBS/x.library\n"
                      OUL_MANIFEST_END "\n";
    char cut[256];
    size_t i;
    int all_short = 1;
    truth("a manifest as GFOpenUpTool writes it now is whole", whole(now), 1);
    truth("an empty list (the first line and the end) is whole", whole(OUL_MANIFEST_HEAD "\n" OUL_MANIFEST_END "\n"), 1);
    for (i = 0; i < strlen(now); i++) {             /* every shorter copy, at a line's end too */
        memcpy(cut, now, i);
        cut[i] = 0;
        if (oul_manifest_whole(cut, (long)i)) all_short = 0;
    }
    truth("every copy of it cut short, at a line's end too, is short", all_short, 1);
    truth("nothing at all (a file cut to nothing) is short", oul_manifest_whole("", 0), 0);
    truth("an end line that isn't the last line doesn't count",
          whole(OUL_MANIFEST_HEAD "\n" OUL_MANIFEST_END "\nnew\tC:x\n"), 0);
    truth("Windows line ends are read too", whole("GFOpenUp\t0.2\r\nnew\tC:x\r\nGFOpenUp\tend\r\n"), 1);
    truth("an older GFOpenUpTool's manifest (0.1, no end line) is whole when its last line ends",
          whole(OUL_MANIFEST_OLD "\npart\tGFOpenUp\t0.8.9\nnew\tSYS:C/GFOpenUpTool\n"), 1);
    truth("an older one cut in a line is short", whole(OUL_MANIFEST_OLD "\npart\tGFOpenUp\t0.8.9\nnew\tSYS:C/Open"), 0);
    truth("a later one (0.3) needs the end line", whole("GFOpenUp\t0.3\nnew\tC:x\n"), 0);
    truth("a later one with it is whole", whole("GFOpenUp\t0.3\nnew\tC:x\n" OUL_MANIFEST_END "\n"), 1);
    truth("no first line: not a manifest", whole("new\tC:x\n" OUL_MANIFEST_END "\n"), 0);

    /* a "new" line's mark: only what GFOpenUp wrote is taken out (9 Nov 2026) */
    {
        char mark[24];
        unsigned long size = 0, crc = 0, c1;
        const unsigned char *txt = (const unsigned char *)"123456789";
        same("CRC-32 of \"123456789\" (the check value)", oul_crc32(0, txt, 9) == 0xCBF43926UL ? "cbf43926" : "wrong", "cbf43926");
        c1 = oul_crc32(oul_crc32(0, txt, 4), txt + 4, 5);
        truth("CRC-32 a chunk at a time is the same", c1 == 0xCBF43926UL, 1);
        oul_mark(mark, 9, 0xCBF43926UL);
        same("the mark of a 9-byte file", mark, "=9:cbf43926");
        oul_mark(mark, 0, 0);
        same("the mark of an empty file", mark, "=0:00000000");
        oul_mark(mark, 4000000000UL, 0xFFFFFFFFUL);
        same("the mark of a big file", mark, "=4000000000:ffffffff");
        truth("a mark reads back", oul_mark_read("=9:cbf43926", &size, &crc) && size == 9 && crc == 0xCBF43926UL, 1);
        truth("no field: not a mark", oul_mark_read("", &size, &crc), 0);
        truth("a replaced line's backup path: not a mark", oul_mark_read("SYS:Storage/GFOpenUp-Superseded/SYS/C/x", &size, &crc), 0);
        truth("a mark cut short: not a mark", oul_mark_read("=9:cbf4392", &size, &crc), 0);
        truth("a mark with more after it: not a mark", oul_mark_read("=9:cbf43926x", &size, &crc), 0);
        truth("the file as GFOpenUp wrote it is taken out", oul_still_ours("=9:cbf43926", 1, 9, 0xCBF43926UL), 1);
        truth("the file written over since (other bytes) stays", oul_still_ours("=9:cbf43926", 1, 9, 0x12345678UL), 0);
        truth("the file written over since (other size) stays", oul_still_ours("=9:cbf43926", 1, 10, 0xCBF43926UL), 0);
        truth("a line from before marks is taken out as before", oul_still_ours("", 1, 10, 1), 1);
        truth("a file that can't be read is left to DeleteFile", oul_still_ours("=9:cbf43926", 0, 0, 0), 1);
    }
    /* the old OpenUp's list: its lines for GFOpenUp's files go before its UNINSTALL (9 Nov 2026) */
    truth("an old new line for a file GFOpenUp lists is taken out", oul_old_line_taken("new\tSYS:Developer/x.h", gf_has), 1);
    truth("an old new line for a file GFOpenUp doesn't list stays", oul_old_line_taken("new\tSYS:C/OpenDock", gf_has), 0);
    truth("an old replaced line stays", oul_old_line_taken("replaced\tSYS:Developer/x.h\tSYS:Storage/x", gf_has), 0);
    truth("an old dir line stays", oul_old_line_taken("dir\tSYS:Developer", gf_has), 0);
    truth("Windows line ends are read too", oul_old_line_taken("new\tSYS:Developer/x.h\r", gf_has), 1);
    printf(fails ? "%d FAILED\n" : "all passed\n", fails);
    return fails ? 1 : 0;
}

/* ---- the manifest, as GFOpenUpTool keeps it, for "install" and "remove" ---- */

#define MAXT 512
static oul_entry ents[MAXT];
static int nents;

static char *dup_text(const char *s)
{
    char *d = malloc(strlen(s) + 1);
    if (d) strcpy(d, s);
    return d;
}

static void add(const char *kind, const char *a, const char *b)
{
    if (nents >= MAXT) return;
    snprintf(ents[nents].kind, sizeof ents[0].kind, "%s", kind);
    ents[nents].a = dup_text(a);
    ents[nents].b = dup_text(b ? b : "");
    nents++;
}

static char *read_all(const char *path)
{
    FILE *f = fopen(path, "rb");
    long n;
    char *t = NULL;
    if (!f) { perror(path); return NULL; }
    if (!fseek(f, 0, SEEK_END) && (n = ftell(f)) >= 0 && !fseek(f, 0, SEEK_SET) && (t = malloc(n + 1))) {
        if (fread(t, 1, n, f) != (size_t)n) { free(t); t = NULL; }
        else t[n] = 0;
    }
    fclose(f);
    return t;
}

/* GFOpenUp.manifest's lines: kind, then up to two tab-separated fields; the "GFOpenUp" line first */
static int load(const char *path)
{
    char *text = read_all(path), *p;
    if (!text) return 0;
    for (p = text; *p; ) {
        char *f[3] = { p, NULL, NULL };
        int n = 1;
        while (*p && *p != '\n') {
            if (*p == '\t' && n < 3) { *p = 0; f[n++] = p + 1; }
            p++;
        }
        if (*p) *p++ = 0;
        f[0][strcspn(f[0], "\r")] = 0;
        if (n > 1) f[n - 1][strcspn(f[n - 1], "\r")] = 0;
        if (f[0][0] && strcmp(f[0], "GFOpenUp")) add(f[0], f[1] ? f[1] : "", f[2]);
    }
    return 1;
}

static void print_manifest(void)
{
    int i;
    puts(OUL_MANIFEST_HEAD);
    for (i = 0; i < nents; i++) {
        if (!ents[i].kind[0]) continue;                /* taken out */
        printf("%s\t%s", ents[i].kind, ents[i].a);
        if (ents[i].b[0]) printf("\t%s", ents[i].b);
        putchar('\n');
    }
    puts(OUL_MANIFEST_END);
}

/* INSTALL's "needs part" and startup lines (openuptool.c install): pass 0 refuses, pass 1 notes; the files aren't
 * copied here. With fail, a file couldn't be put in place: the part isn't listed, and its startup and needpart
 * lines stay as they were (oul_settle; 25 Oct 2026), and 10 (RETURN_ERROR). The lines' conditions are read as on
 * an AmiCradle instance on OS 3.2.3's ROM (Instance-32). */
static int install_part(const char *part, const char *manifest, int fail)
{
    char *text = read_all(part), partname[64] = "", partver[32] = "";
    int pass, refused = 0, i, found = 0, first = 0;
    if (!text || !load(manifest)) return 2;
    for (pass = 0; pass < 2 && !refused; pass++) {
        char *copy = dup_text(text), *p;
        if (pass == 1) first = nents;
        for (p = copy; p && *p; ) {
            char *line = p, *w[12];
            const char *need, *dt, *up, *ut;
            int n;
            while (*p && *p != '\n') p++;
            if (*p) *p++ = 0;
            line[strcspn(line, "\r")] = 0;
            if (!(line = (char *)oul_strip(line, OUL_AMICRADLE, 0))) continue;
            if (!strncmp(line, "startup ", 8)) {
                if (pass == 1) {
                    int kept = 0, have = 0;
                    for (i = 0; i < nents; i++) {
                        if (!strcmp(ents[i].kind, "ostartup") && !strcmp(ents[i].a, line + 8) && !strcmp(ents[i].b, partname)) {
                            strcpy(ents[i].kind, "startup");            /* kept, in its place */
                            kept = 1;
                            break;
                        }
                        if (!strcmp(ents[i].kind, "startup") && !strcmp(ents[i].a, line + 8)) have = 1;
                    }
                    if (!kept && !have) add("startup", line + 8, partname);
                }
                continue;
            }
            if (!(n = oul_words(line, w, 12))) continue;
            if (!strcmp(w[0], "part") && n >= 3) {
                snprintf(partname, sizeof partname, "%s", w[1]);
                snprintf(partver, sizeof partver, "%s", w[2]);
                if (pass == 1) {
                    for (i = 0; i < nents; i++)
                        if (!strcmp(ents[i].kind, "startup") && !strcmp(ents[i].b, partname)) strcpy(ents[i].kind, "ostartup");
                    oul_mark_needs(ents, nents, partname);
                }
            } else if (pass == 0 && (need = oul_needs_part(w, n))) {
                if (!oul_installed(ents, nents, need)) { printf("missing %s\n", need); refused = 1; }
            } else if (pass == 1 && (need = oul_needs_part(w, n))) {
                add(OUL_NEEDPART, partname, need);
            } else if (pass == 1 && !strcmp(w[0], "drop") && n >= 2 && (oud_words(w, n, &dt, &up, &ut), !dt)) {
                /* with <text> it reads the file: not here; an unless guard's file isn't here either, so it goes */
                /* openuptool.c drop_file: a listed file leaves the manifest (here taken as still GFOpenUp's bytes) */
                for (i = 0; i < nents; i++)
                    if (ents[i].kind[0] && !strcmp(ents[i].a, w[1])) {
                        if (oud_action(ents[i].kind, 1) != OUD_NONE) ents[i].kind[0] = 0;
                        break;
                    }
            }
        }
        free(copy);
    }
    free(text);
    if (refused) return 5;                              /* RETURN_WARN: left out, nothing changed */
    oul_settle(ents, nents, first, partname, !fail);
    if (!fail) {
        for (i = 0; i < nents; i++)
            if (!strcmp(ents[i].kind, "part") && !strcmp(ents[i].a, partname)) { ents[i].b = dup_text(partver); found = 1; }
        if (!found) add("part", partname, partver);
    }
    print_manifest();
    return fail ? 10 : 0;
}

/* REMOVE's check (openuptool.c remove_part), then the part's own lines out of the manifest (its files aren't
 * touched here) */
static int remove_part(const char *part, const char *manifest)
{
    char *text = read_all(part), name[64] = "";
    const char *by;
    int i;
    if (!text || !load(manifest)) return 2;
    if (oul_part_name(text, name, sizeof name) && oul_installed(ents, nents, name) && (by = oul_needed_by(ents, nents, name))) {
        printf("needed by %s\n", by);
        return 5;                                       /* RETURN_WARN: it stays, nothing changed */
    }
    for (i = 0; i < nents; i++)
        if (!strcmp(ents[i].kind, "part") && !strcmp(ents[i].a, name)) ents[i].kind[0] = 0;
    oul_drop_needs(ents, nents, name);
    free(text);
    print_manifest();
    return 0;
}

int main(int argc, char **argv)
{
    char line[1024];
    unsigned have = 0;
    int any = 0;
    FILE *f;
    if (argc < 3) return checks();
    if ((argc == 4 || (argc == 5 && !strcmp(argv[4], "fail"))) && !strcmp(argv[1], "install"))
        return install_part(argv[2], argv[3], argc == 5);
    if (argc == 4 && !strcmp(argv[1], "remove")) return remove_part(argv[2], argv[3]);
    if (!strncmp(argv[2], "openkick101", 11)) have |= OUL_OPENKICK;   /* oul_prefs_task(1, 47, 101) */
    else if (!strcmp(argv[2], "remove")) any = 1;
    else if (strncmp(argv[2], "openkick100", 11) && strncmp(argv[2], "os323", 5)) { fprintf(stderr, "machine?\n"); return 2; }
    if (strstr(argv[2], "-ac")) have |= OUL_AMICRADLE;
    if (!(f = fopen(argv[1], "r"))) { perror(argv[1]); return 2; }
    while (fgets(line, sizeof line, f)) {
        const char *rest;
        line[strcspn(line, "\r\n")] = 0;
        if ((rest = oul_strip(line, have, any))) puts(rest);
    }
    fclose(f);
    return 0;
}
