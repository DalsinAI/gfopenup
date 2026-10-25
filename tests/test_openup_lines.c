/* Host test of src/openup_lines.h, the Part file lines' conditions OpenUpTool reads (0.8.9).
 *
 *   cc -std=c99 -Wall -Werror -Isrc tests/test_openup_lines.c -o test_openup_lines
 *   ./test_openup_lines                    the checks below
 *   ./test_openup_lines PART MACHINE       the Part file's lines as INSTALL acts on them on MACHINE: os323 (OS
 *                                          3.2.3's ROM), openkick100 (an open ROM with prefs.resource 47.100),
 *                                          openkick101 (47.101), each with "-ac" for an AmigaChrome instance;
 *                                          "remove" for REMOVE's reading (every condition taken off)
 *   ./test_openup_lines install PART MANIFEST   INSTALL's "needs part" check of the Part file against the manifest
 *                                          (OpenUp.manifest's form): "missing <part>" for each part it needs that
 *                                          isn't installed, and 5 (RETURN_WARN: left out); else the manifest as
 *                                          INSTALL leaves it (the part's line, its needpart lines), and 0
 *   ./test_openup_lines remove PART MANIFEST    REMOVE's check: "needed by <part>" and 5 while an installed part
 *                                          needs this one; else the manifest as REMOVE leaves it, and 0
 * tests/test_build_pack.py runs these forms over the parts build_pack.py writes.
 *
 * MIT, Copyright (c) 2026 AmigaChrome Team. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

static int checks(void)
{
    const unsigned none = 0, ac = OUL_AMIGACHROME, ok = OUL_OPENKICK, both = OUL_AMIGACHROME | OUL_OPENKICK;
    /* the ROM: OpenKick's mark and prefs.resource 47.101 or later, compared as two numbers */
    truth("OS 3.2.3's ROM (no mark, no prefs.resource): no prefs task", oul_prefs_task(0, 0, 0), 0);
    truth("no mark, prefs.resource 47.101: no prefs task", oul_prefs_task(0, 47, 101), 0);
    truth("OpenKick, prefs.resource 47.100: no prefs task", oul_prefs_task(1, 47, 100), 0);
    truth("OpenKick, prefs.resource 47.99: no prefs task", oul_prefs_task(1, 47, 99), 0);
    truth("OpenKick, prefs.resource 47.101: the prefs task", oul_prefs_task(1, 47, 101), 1);
    truth("OpenKick, prefs.resource 47.110: the prefs task", oul_prefs_task(1, 47, 110), 1);
    truth("OpenKick, prefs.resource 48.0: the prefs task", oul_prefs_task(1, 48, 0), 1);
    truth("OpenKick, prefs.resource 46.200: no prefs task", oul_prefs_task(1, 46, 200), 0);
    truth("OpenKick, no prefs.resource: no prefs task", oul_prefs_task(1, 0, 0), 0);
    /* lines without a condition stay as they are */
    same("a plain line", oul_strip("copy \"a\" \"SYS:a\"", none, 0), "copy \"a\" \"SYS:a\"");
    same("a startup line", oul_strip("startup Run >NIL: C:OpenUpMenu", both, 0), "startup Run >NIL: C:OpenUpMenu");
    same("a line starting whenever", oul_strip("whenever x", both, 0), "whenever x");
    /* when AmigaChrome, as since 0.6.3 */
    same("when AmigaChrome, on AmigaChrome", oul_strip("when AmigaChrome copy \"a\" \"b\"", ac, 0), "copy \"a\" \"b\"");
    same("when AmigaChrome, elsewhere", oul_strip("when AmigaChrome copy \"a\" \"b\"", ok, 0), NULL);
    same("unless AmigaChrome, on AmigaChrome", oul_strip("unless AmigaChrome copy \"a\" \"b\"", ac, 0), NULL);
    same("unless AmigaChrome, elsewhere", oul_strip("unless AmigaChrome copy \"a\" \"b\"", none, 0), "copy \"a\" \"b\"");
    /* OpenKick, startup lines and retire lines too */
    same("when OpenKick retire, on OpenKick", oul_strip("when OpenKick retire \"SYS:x\"", ok, 0), "retire \"SYS:x\"");
    same("when OpenKick retire, elsewhere", oul_strip("when OpenKick retire \"SYS:x\"", ac, 0), NULL);
    same("unless OpenKick startup, on OpenKick", oul_strip("unless OpenKick startup SYS:Preferences/OpenPrefs APPLY", both, 0), NULL);
    same("unless OpenKick startup, elsewhere", oul_strip("unless OpenKick startup SYS:Preferences/OpenPrefs APPLY", ac, 0),
         "startup SYS:Preferences/OpenPrefs APPLY");
    same("a tab between the words", oul_strip("when\tOpenKick\t\tretire x", ok, 0), "retire x");
    /* a machine this OpenUpTool doesn't know: the line stays out, as before 0.8.9 */
    same("when PiStorm", oul_strip("when PiStorm copy a b", both, 0), NULL);
    same("unless PiStorm", oul_strip("unless PiStorm copy a b", both, 0), NULL);
    /* REMOVE: every condition comes off, whether it holds or not */
    same("REMOVE: when OpenKick", oul_strip("when OpenKick retire x", none, 1), "retire x");
    same("REMOVE: unless OpenKick startup", oul_strip("unless OpenKick startup Path x", ok, 1), "startup Path x");
    same("REMOVE: when PiStorm", oul_strip("when PiStorm copy a b", none, 1), "copy a b");
    /* a condition with nothing after it is no condition */
    same("when OpenKick alone", oul_strip("when OpenKick", ok, 0), "when OpenKick");
    same("when alone", oul_strip("when", ok, 0), "when");
    return need_checks();
}

/* "needs part" (25 Oct 2026): the words of a line, the Part file's name, and the manifest's needpart lines */
static int need_checks(void)
{
    char l1[] = "copy \"Utilities/Open Write\" \"SYS:Utilities/Open Write\"", l2[] = "  ; a comment", l3[] = "needs part GFOffice",
         l4[] = "needs AmigaChrome", l5[] = "needs part", l6[] = "needs parts GFOffice", l7[] = "needs\tpart\tGFOffice ; shared",
         name[16];
    char *w[12];
    int n;
    oul_entry m[] = {
        { "part", "OpenUp", "0.8.9" }, { "new", "LIBS:owf.library", "" }, { "part", "GFOffice", "1.1" },
        { "part", "OpenWrite", "2.0.1" }, { OUL_NEEDPART, "OpenWrite", "GFOffice" },
        { OUL_NEEDPART, "OpenCalc", "GFOffice" },       /* OpenCalc's part line is gone: REMOVE took it out by hand */
        { OUL_NEEDPART, "Odd", "Odd" }, { "part", "Odd", "1" },
    };
    const int nm = (int)(sizeof m / sizeof m[0]);
    n = oul_words(l1, w, 12);
    truth("words: a quoted word keeps its space", n == 3 && !strcmp(w[1], "Utilities/Open Write") && !strcmp(w[2], "SYS:Utilities/Open Write"), 1);
    truth("words: a comment line has none", oul_words(l2, w, 12) == 0, 1);
    n = oul_words(l3, w, 12);
    same("needs part GFOffice", oul_needs_part(w, n), "GFOffice");
    n = oul_words(l4, w, 12);
    same("needs AmigaChrome is no part", oul_needs_part(w, n), NULL);
    n = oul_words(l5, w, 12);
    same("needs part with no name", oul_needs_part(w, n), NULL);
    n = oul_words(l6, w, 12);
    same("needs parts is not needs part", oul_needs_part(w, n), NULL);
    n = oul_words(l7, w, 12);
    same("needs part with tabs and a comment after", oul_needs_part(w, n), "GFOffice");
    truth("installed: GFOffice", oul_installed(m, nm, "GFOffice"), 1);
    truth("installed: not OpenCalc (a needpart line is not a part line)", oul_installed(m, nm, "OpenCalc"), 0);
    truth("installed: not a name's start (GFOffic)", oul_installed(m, nm, "GFOffic"), 0);
    same("REMOVE GFOffice: OpenWrite needs it", oul_needed_by(m, nm, "GFOffice"), "OpenWrite");
    same("REMOVE OpenWrite: nothing needs it", oul_needed_by(m, nm, "OpenWrite"), NULL);
    same("REMOVE of a part that names itself: it doesn't stop itself", oul_needed_by(m, nm, "Odd"), NULL);
    m[3].kind[0] = 0;                                   /* REMOVE took OpenWrite out */
    same("REMOVE GFOffice once OpenWrite is out (OpenCalc's old line doesn't count)", oul_needed_by(m, nm, "GFOffice"), NULL);
    strcpy(m[3].kind, "part");
    oul_drop_needs(m, nm, "OpenWrite");
    truth("drop OpenWrite's needs: its needpart line goes, OpenCalc's stays", !m[4].kind[0] && !strcmp(m[5].kind, OUL_NEEDPART)
          && !strcmp(m[0].kind, "part"), 1);
    same("REMOVE GFOffice with OpenWrite in but its need dropped", oul_needed_by(m, nm, "GFOffice"), NULL);
    truth("part name: the part line", oul_part_name("part GFOffice 1.1\n; shared\n", name, sizeof name) && !strcmp(name, "GFOffice"), 1);
    truth("part name: after a comment, with tabs and CR LF", oul_part_name("; x\r\n\tpart\tOpenWrite\t2.0.1\r\n", name, sizeof name)
          && !strcmp(name, "OpenWrite"), 1);
    truth("part name: partial is no part line", oul_part_name("partial x\n", name, sizeof name), 0);
    truth("part name: one too long for the buffer", oul_part_name("part ABCDEFGHIJKLMNOPQRSTUVWXYZ 1\n", name, sizeof name), 0);
    printf(fails ? "%d FAILED\n" : "all passed\n", fails);
    return fails ? 1 : 0;
}

/* ---- the manifest, as OpenUpTool keeps it, for "install" and "remove" ---- */

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

/* OpenUp.manifest's lines: kind, then up to two tab-separated fields; the "OpenUp" line first */
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
        if (f[0][0] && strcmp(f[0], "OpenUp")) add(f[0], f[1] ? f[1] : "", f[2]);
    }
    return 1;
}

static void print_manifest(void)
{
    int i;
    puts("OpenUp\t0.1");
    for (i = 0; i < nents; i++) {
        if (!ents[i].kind[0]) continue;                /* taken out */
        printf("%s\t%s", ents[i].kind, ents[i].a);
        if (ents[i].b[0]) printf("\t%s", ents[i].b);
        putchar('\n');
    }
}

/* INSTALL's "needs part" lines (openuptool.c install): pass 0 refuses, pass 1 notes; the files aren't copied here.
 * The lines' conditions are read as on an AmigaChrome instance on OS 3.2.3's ROM (Instance-32). */
static int install_part(const char *part, const char *manifest)
{
    char *text = read_all(part), partname[64] = "", partver[32] = "";
    int pass, refused = 0, i, found = 0;
    if (!text || !load(manifest)) return 2;
    for (pass = 0; pass < 2 && !refused; pass++) {
        char *copy = dup_text(text), *p;
        for (p = copy; p && *p; ) {
            char *line = p, *w[12];
            const char *need;
            int n;
            while (*p && *p != '\n') p++;
            if (*p) *p++ = 0;
            line[strcspn(line, "\r")] = 0;
            if (!(line = (char *)oul_strip(line, OUL_AMIGACHROME, 0)) || !strncmp(line, "startup ", 8)) continue;
            if (!(n = oul_words(line, w, 12))) continue;
            if (!strcmp(w[0], "part") && n >= 3) {
                snprintf(partname, sizeof partname, "%s", w[1]);
                snprintf(partver, sizeof partver, "%s", w[2]);
                if (pass == 1) oul_drop_needs(ents, nents, partname);
            } else if (pass == 0 && (need = oul_needs_part(w, n))) {
                if (!oul_installed(ents, nents, need)) { printf("missing %s\n", need); refused = 1; }
            } else if (pass == 1 && (need = oul_needs_part(w, n))) {
                add(OUL_NEEDPART, partname, need);
            }
        }
        free(copy);
    }
    free(text);
    if (refused) return 5;                              /* RETURN_WARN: left out, nothing changed */
    for (i = 0; i < nents; i++)
        if (!strcmp(ents[i].kind, "part") && !strcmp(ents[i].a, partname)) { ents[i].b = dup_text(partver); found = 1; }
    if (!found) add("part", partname, partver);
    print_manifest();
    return 0;
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
    if (argc == 4 && !strcmp(argv[1], "install")) return install_part(argv[2], argv[3]);
    if (argc == 4 && !strcmp(argv[1], "remove")) return remove_part(argv[2], argv[3]);
    if (!strncmp(argv[2], "openkick101", 11)) have |= OUL_OPENKICK;   /* oul_prefs_task(1, 47, 101) */
    else if (!strcmp(argv[2], "remove")) any = 1;
    else if (strncmp(argv[2], "openkick100", 11) && strncmp(argv[2], "os323", 5)) { fprintf(stderr, "machine?\n"); return 2; }
    if (strstr(argv[2], "-ac")) have |= OUL_AMIGACHROME;
    if (!(f = fopen(argv[1], "r"))) { perror(argv[1]); return 2; }
    while (fgets(line, sizeof line, f)) {
        const char *rest;
        line[strcspn(line, "\r\n")] = 0;
        if ((rest = oul_strip(line, have, any))) puts(rest);
    }
    fclose(f);
    return 0;
}
