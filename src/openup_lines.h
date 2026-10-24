/* What a Part file line's condition asks, and the ROM test for OpenKick: plain C with no OS calls, so
 * OpenUpTool (src/openuptool.c) and its host test (tests/test_openup_lines.c) share it.
 *
 * A condition goes in front of any other line, a startup line too:
 *   when AmigaChrome <line>      only on an AmigaChrome instance (OpenUpTool 0.6.3)
 *   unless AmigaChrome <line>    only elsewhere (0.8.9)
 *   when OpenKick <line>         only on the open ROM whose intuition applies the boot's settings (0.8.9): the
 *                                OpenKick ROM with prefs.resource 47.101 or later, the ROM without ROM: (the
 *                                no-ROM-volume design of 24 October 2026, sections 9 and 10)
 *   unless OpenKick <line>       on every other ROM: OS 3.2.3's own, and an OpenKick ROM before prefs.resource
 *                                47.101, whose prefs task applies only the system's own settings
 * A condition this OpenUpTool doesn't know keeps its line out of an install, as before.
 *
 * MIT, Copyright (c) 2026 AmigaChrome Team. */
#ifndef OPENUP_LINES_H
#define OPENUP_LINES_H

#define OUL_AMIGACHROME 1u          /* an AmigaChrome instance (its ACBridge or ACStorage board) */
#define OUL_OPENKICK    2u          /* the open ROM that applies the boot's settings (oul_prefs_task) */

/* 1 on the open ROM whose intuition applies Sound's levels, Gamepads' patch and the stack caps at boot:
 * the OpenKick ROM (okr: its 'OKR1' mark) with prefs.resource (version, revision) at least 47.101 (no-ROM-volume
 * design P5). Compared as two numbers, as okvr.h does: 47.101 is newer than 47.100, and 48.0 than both. */
static int oul_prefs_task(int okr, unsigned version, unsigned revision)
{
    return okr && (version > 47u || (version == 47u && revision >= 101u));
}

static int oul_blank(char c) { return c == ' ' || c == '\t'; }

/* The word at s is w, followed by a blank: the length of w, else 0. */
static int oul_word(const char *s, const char *w)
{
    int n = 0;
    while (w[n] && s[n] == w[n]) n++;
    return !w[n] && oul_blank(s[n]) ? n : 0;
}

/* The line without its condition: the rest of the line when its condition holds on a machine with the OUL_
 * bits in have, NULL when it doesn't (or names a machine this OpenUpTool doesn't know), the line itself when
 * it has none. With any set, a condition is taken off whether it holds or not (REMOVE takes out whatever the
 * part put in). A "when" or "unless" with nothing after its word is no condition: the line stays as it is. */
static const char *oul_strip(const char *line, unsigned have, int any)
{
    const char *s = line, *rest;
    unsigned bit = 0;
    int unless = 0, n;
    while (oul_blank(*s)) s++;
    if ((n = oul_word(s, "when"))) s += n;
    else if ((n = oul_word(s, "unless"))) { s += n; unless = 1; }
    else return line;
    while (oul_blank(*s)) s++;
    if ((n = oul_word(s, "AmigaChrome"))) bit = OUL_AMIGACHROME;
    else if ((n = oul_word(s, "OpenKick"))) bit = OUL_OPENKICK;
    else for (n = 0; s[n] && !oul_blank(s[n]); n++) ;
    rest = s + n;
    while (oul_blank(*rest)) rest++;
    if (!*rest || !n) return line;
    if (any) return rest;
    if (!bit) return 0;
    return ((have & bit) != 0) != unless ? rest : 0;
}

#endif
