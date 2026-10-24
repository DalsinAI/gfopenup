/* Host test of src/openup_lines.h, the Part file lines' conditions OpenUpTool reads (0.8.9).
 *
 *   cc -std=c99 -Wall -Werror -Isrc tests/test_openup_lines.c -o test_openup_lines
 *   ./test_openup_lines                    the checks below
 *   ./test_openup_lines PART MACHINE       the Part file's lines as INSTALL acts on them on MACHINE: os323 (OS
 *                                          3.2.3's ROM), openkick100 (an open ROM with prefs.resource 47.100),
 *                                          openkick101 (47.101), each with "-ac" for an AmigaChrome instance;
 *                                          "remove" for REMOVE's reading (every condition taken off)
 * tests/test_build_pack.py runs the second form over the parts build_pack.py writes.
 *
 * MIT, Copyright (c) 2026 AmigaChrome Team. */
#include <stdio.h>
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
    printf(fails ? "%d FAILED\n" : "all passed\n", fails);
    return fails ? 1 : 0;
}

int main(int argc, char **argv)
{
    char line[1024];
    unsigned have = 0;
    int any = 0;
    FILE *f;
    if (argc < 3) return checks();
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
