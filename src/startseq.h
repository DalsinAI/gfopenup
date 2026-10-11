/* S:Startup-Sequence's desktop line (DESIGN.md section 3): a part whose program starts the desktop in
 * LoadWB's place (OpenBench) changes S:Startup-Sequence, the one other marked change there beside C:OpenRTG's.
 * The LoadWB line becomes a marked block that runs the program when it is on the disk and keeps the
 * original line, word for word, as the Else:
 *
 *   ;BEGIN OpenUp desktop
 *   ; Added by OpenUp: SYS:System/OpenBench starts the desktop, and runs LoadWB itself when Shift is held
 *   ; or it can't start. OpenUpTool UNINSTALL puts the LoadWB line back.
 *   If EXISTS SYS:System/OpenBench
 *     Run >NIL: SYS:System/OpenBench
 *   Else
 *   LoadWB
 *   EndIf
 *   ;END OpenUp desktop
 *
 * Taking the block out puts the original line back where it was, so the file comes back byte for byte.
 * Plain C on text, with no AmigaOS calls: OpenUpTool includes it, and tools/startseq_test.c checks it on
 * the build machine. MIT, Copyright (c) 2026 AmigaChrome Team. */
#ifndef OPENUP_STARTSEQ_H
#define OPENUP_STARTSEQ_H

#include <string.h>
#include <stdlib.h>

#define SEQ_FILE "S:Startup-Sequence"
#define SEQ_BEGIN ";BEGIN OpenUp desktop"
#define SEQ_END ";END OpenUp desktop"

enum { SS_NOMEM = -1, SS_NONE = 0, SS_CHANGED = 1, SS_ALREADY = 2, SS_BY_HAND = 3, SS_NO_LOADWB = 4, SS_MANY_LOADWB = 5 };

/* The line from p to e (no '\n') is this marker: trailing blanks and a CR allowed. */
static int ss_is_marker(const char *p, const char *e, const char *marker)
{
    size_t ml = strlen(marker);
    while (e > p && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r')) e--;
    return (size_t)(e - p) == ml && !strncmp(p, marker, ml);
}

/* The line from p to e runs LoadWB: its first word is LoadWB, C:LoadWB or SYS:C/LoadWB, in any case. */
static int ss_is_loadwb(const char *p, const char *e)
{
    static const char *const names[] = { "loadwb", "c:loadwb", "sys:c/loadwb" };
    const char *w;
    size_t i, n, k;
    while (p < e && (*p == ' ' || *p == '\t')) p++;
    w = p;
    while (p < e && *p != ' ' && *p != '\t' && *p != '\r') p++;
    n = p - w;
    for (i = 0; i < sizeof names / sizeof names[0]; i++) {
        if (strlen(names[i]) != n) continue;
        for (k = 0; k < n && (w[k] | 0x20) == names[i][k]; k++) ;
        if (k == n) return 1;
    }
    return 0;
}

/* 1 when the text from p to e holds s (a program's path), in any case. */
static int ss_holds(const char *p, const char *e, const char *s)
{
    size_t n = strlen(s), k;
    for (; p + n <= e; p++) {
        for (k = 0; k < n && (p[k] | 0x20) == (s[k] | 0x20); k++) ;
        if (k == n) return 1;
    }
    return 0;
}

/* The Startup-Sequence with the LoadWB line turned into the block for program, in *out (malloc'd, the
 * caller frees it) when SS_CHANGED. Left alone, *out NULL: SS_ALREADY (the block is there), SS_BY_HAND
 * (a line outside a block of ours already names the program: someone set it up), SS_NO_LOADWB or
 * SS_MANY_LOADWB (not exactly one LoadWB line: which one to change is not ours to guess). */
static int ss_with_desktop(const char *text, const char *program, char **out)
{
    const char *p = text, *line = NULL, *line_end = NULL;
    int loadwbs = 0;
    size_t len;
    char *o;
    *out = NULL;
    while (*p) {
        const char *e = strchr(p, '\n');
        const char *q = p;
        if (!e) e = p + strlen(p);
        while (q < e && (*q == ' ' || *q == '\t')) q++;
        if (ss_is_marker(p, e, SEQ_BEGIN)) return SS_ALREADY;
        if (q < e && *q != ';') {
            if (ss_holds(q, e, program)) return SS_BY_HAND;
            if (ss_is_loadwb(q, e)) { loadwbs++; line = p; line_end = e; }
        }
        p = *e ? e + 1 : e;
    }
    if (!loadwbs) return SS_NO_LOADWB;
    if (loadwbs > 1) return SS_MANY_LOADWB;
    len = strlen(text) + 4 * strlen(program) + 400;
    if (!(o = malloc(len))) return SS_NOMEM;
    memcpy(o, text, line - text);
    o[line - text] = 0;
    strcat(o, SEQ_BEGIN "\n; Added by OpenUp: ");
    strcat(o, program);
    strcat(o, " starts the desktop, and runs LoadWB itself when Shift is held\n"
              "; or it can't start. OpenUpTool UNINSTALL puts the LoadWB line back.\nIf EXISTS ");
    strcat(o, program);
    strcat(o, "\n  Run >NIL: ");
    strcat(o, program);
    strcat(o, "\nElse\n");
    strncat(o, line, line_end - line);                 /* the original line, as it was */
    strcat(o, "\nEndIf\n" SEQ_END);
    strcat(o, line_end);                                 /* its line end, and the rest */
    *out = o;
    return SS_CHANGED;
}

/* The Startup-Sequence without the block: the original line (the one after the block's Else) back in its
 * place, in *out (malloc'd) when SS_CHANGED; SS_NONE (*out NULL) when there is no block. */
static int ss_without_desktop(const char *text, char **out)
{
    const char *p = text, *begin = NULL, *end = NULL, *orig = NULL, *orig_end = NULL;
    int after_else = 0;
    char *o;
    *out = NULL;
    while (*p) {
        const char *e = strchr(p, '\n');
        if (!e) e = p + strlen(p);
        if (!begin) {
            if (ss_is_marker(p, e, SEQ_BEGIN)) begin = p;
        } else if (ss_is_marker(p, e, SEQ_END)) {
            end = e;
            break;
        } else if (after_else && !orig) {
            orig = p; orig_end = e;
        } else if (ss_is_marker(p, e, "Else")) {
            after_else = 1;
        }
        p = *e ? e + 1 : e;
    }
    if (!begin || !end) return SS_NONE;
    if (!(o = malloc(strlen(text) + 16))) return SS_NOMEM;
    memcpy(o, text, begin - text);
    o[begin - text] = 0;
    if (orig) strncat(o, orig, orig_end - orig);
    else strcat(o, "LoadWB");                            /* the block was edited by hand: the stock line */
    strcat(o, end);
    *out = o;
    return SS_CHANGED;
}

#endif
