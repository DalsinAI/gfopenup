/* Checks src/startseq.h, OpenUpTool's change to S:Startup-Sequence, on the build machine:
 *   cc -std=c99 -Wall -Werror -I src -o startseq_test tools/startseq_test.c && ./startseq_test [Startup-Sequence...]
 * With no files it checks its own cases; each file given is changed and changed back, as INSTALL and
 * UNINSTALL do it, and must come back byte for byte. MIT, Copyright (c) 2026 AmigaChrome Team. */
#include <stdio.h>
#include "startseq.h"

#define BENCH "SYS:System/OpenBench"
static int fails;

static void check(int ok, const char *what)
{
    printf("%s  %s\n", ok ? "ok  " : "FAIL", what);
    if (!ok) fails++;
}

/* changed, then changed back: the same bytes; changing twice does nothing more */
static void round_trip(const char *name, const char *text, int want)
{
    char *with = NULL, *back = NULL, *again = NULL;
    char what[300];
    int r = ss_with_desktop(text, BENCH, &with);
    snprintf(what, sizeof what, "%s: change gives %d (want %d)", name, r, want);
    check(r == want, what);
    if (r != SS_CHANGED) { free(with); return; }
    snprintf(what, sizeof what, "%s: a second INSTALL leaves it (SS_ALREADY)", name);
    check(ss_with_desktop(with, BENCH, &again) == SS_ALREADY && !again, what);
    snprintf(what, sizeof what, "%s: UNINSTALL gives the original back, byte for byte", name);
    check(ss_without_desktop(with, &back) == SS_CHANGED && back && !strcmp(back, text), what);
    free(with);
    free(back);
}

static char *slurp(const char *path)
{
    FILE *f = fopen(path, "rb");
    long n;
    char *t;
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (!(t = malloc(n + 1)) || fread(t, 1, n, f) != (size_t)n) { fclose(f); free(t); return NULL; }
    t[n] = 0;
    fclose(f);
    return t;
}

int main(int argc, char **argv)
{
    static const char stock_tail[] =
        "IF EXISTS S:User-Startup\n  Execute S:User-Startup\nEndIF\n\nLoadWB\nEndCLI >NIL:\n";
    char *with = NULL, *back = NULL;
    int i;
    round_trip("stock tail", stock_tail, SS_CHANGED);
    round_trip("no final line end", "SetPatch >NIL:\nLoadWB", SS_CHANGED);
    round_trip("CR LF lines", "SetPatch >NIL:\r\nLoadWB DELAY\r\nEndCLI >NIL:\r\n", SS_CHANGED);
    round_trip("C:LoadWB, indented, with arguments", "If EXISTS S:x\n  C:LoadWB NEWPATH\nEndIf\n", SS_CHANGED);
    round_trip("only a comment names it", "; LoadWB is started below\nloadwb\n", SS_CHANGED);
    round_trip("no LoadWB", "SetPatch >NIL:\nEndCLI >NIL:\n", SS_NO_LOADWB);
    round_trip("two LoadWB lines", "If A\n LoadWB\nElse\n LoadWB DELAY\nEndIf\n", SS_MANY_LOADWB);
    round_trip("LoadWBx is another program", "LoadWBx\n", SS_NO_LOADWB);
    round_trip("set up by hand (Instance-32's)",
               "If EXISTS SYS:System/OpenBench\n  Run >NIL: SYS:System/OpenBench\nElse\n  LoadWB\nEndIf\n", SS_BY_HAND);
    /* the block itself */
    ss_with_desktop(stock_tail, BENCH, &with);
    printf("---- the stock tail, changed:\n%s----\n", with ? with : "(nothing)");
    check(with && strstr(with, "\n" SEQ_BEGIN "\n") && strstr(with, "If EXISTS " BENCH "\n  Run >NIL: " BENCH "\nElse\nLoadWB\nEndIf\n" SEQ_END "\nEndCLI >NIL:\n"),
          "the block: If EXISTS, Run, Else, the original line, EndIf, the end marker, then the rest");
    /* someone else's change after OpenUp's: kept, and the LoadWB line still comes back */
    if (with) {
        char *edited = malloc(strlen(with) + 64);
        strcpy(edited, "; my own line\n");
        strcat(edited, with);
        check(ss_without_desktop(edited, &back) == SS_CHANGED && back && !strcmp(back + 14, stock_tail) && !strncmp(back, "; my own line\n", 14),
              "an edit made since: kept, the block still undone");
        free(back);
        free(edited);
    }
    check(ss_without_desktop(stock_tail, &back) == SS_NONE && !back, "no block: UNINSTALL leaves the file");
    free(with);
    for (i = 1; i < argc; i++) {
        char *t = slurp(argv[i]);
        char name[300];
        if (!t) { printf("FAIL  can't read %s\n", argv[i]); fails++; continue; }
        snprintf(name, sizeof name, "%s", argv[i]);
        {
            char *w = NULL;
            int r = ss_with_desktop(t, BENCH, &w);
            printf("      %s: %s\n", argv[i], r == SS_CHANGED ? "changed" : r == SS_BY_HAND ? "already runs OpenBench (left as it is)" :
                   r == SS_ALREADY ? "already has the block" : r == SS_NO_LOADWB ? "no LoadWB line (left)" : "more than one LoadWB line (left)");
            free(w);
            round_trip(name, t, r);
        }
        free(t);
    }
    printf("%s\n", fails ? "startseq: FAILED" : "startseq: all passed");
    return fails != 0;
}
