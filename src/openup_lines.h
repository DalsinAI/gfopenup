/* What a Part file line's condition asks, the ROM test for OpenKick, and one part's need of another (the words of
 * a line, the manifest's needpart lines): plain C with no OS calls, so
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
 * And one part needing another (25 October 2026, the GFOffice design's section 12.2):
 *   needs part <name>            INSTALL leaves the part out while the manifest doesn't list <name> as installed,
 *                                and notes the need in the manifest ("needpart <part> <name>"); REMOVE of <name>
 *                                refuses while a part that needs it is installed. The GFOffice Suite's programs
 *                                need the GFOffice part, which holds their shared files (owf.library, z.library,
 *                                the dictionary, OWConvert), so the shared files have one owner.
 * An OpenUpTool before this ignores the line ("needs" with a word it doesn't know).
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

/* Splits a line into words, in place; quoted words keep their spaces; ";" or "#" starts a comment. The count. */
static int oul_words(char *line, char **w, int max)
{
    int n = 0;
    char *p = line;
    while (*p && n < max) {
        while (oul_blank(*p)) p++;
        if (!*p || *p == ';' || *p == '#') break;
        if (*p == '"') {
            w[n++] = ++p;
            while (*p && *p != '"') p++;
        } else {
            w[n++] = p;
            while (*p && !oul_blank(*p)) p++;
        }
        if (*p) *p++ = 0;
    }
    return n;
}

static int oul_same(const char *a, const char *b)
{
    while (*a && *a == *b) a++, b++;
    return *a == *b;
}

/* A line of the manifest, ENVARC:OpenAmiga/OpenUp.manifest, as OpenUpTool keeps it: its kind, then up to two
 * fields. A kind emptied is a line taken out (the manifest is written without it). */
typedef struct { char kind[10]; char *a; char *b; } oul_entry;

#define OUL_NEEDPART "needpart"     /* needpart <part> <the part it needs> */

/* The part a line split into words needs: the name in "needs part <name>", else NULL. */
static const char *oul_needs_part(char **w, int n)
{
    return n >= 3 && oul_same(w[0], "needs") && oul_same(w[1], "part") ? w[2] : 0;
}

/* 1 when the manifest lists the part as installed (its "part" line). */
static int oul_installed(const oul_entry *e, int n, const char *part)
{
    int i;
    for (i = 0; i < n; i++)
        if (oul_same(e[i].kind, "part") && oul_same(e[i].a, part)) return 1;
    return 0;
}

/* REMOVE's check: an installed part that needs this one, else NULL. A needpart line whose part is no longer
 * installed doesn't count. */
static const char *oul_needed_by(const oul_entry *e, int n, const char *part)
{
    int i;
    for (i = 0; i < n; i++)
        if (oul_same(e[i].kind, OUL_NEEDPART) && oul_same(e[i].b, part) && !oul_same(e[i].a, part) && oul_installed(e, n, e[i].a))
            return e[i].a;
    return 0;
}

/* A part's needpart lines taken out: when INSTALL notes the needs its Part file has now (an older version's go),
 * and when REMOVE takes the part out. */
static void oul_drop_needs(oul_entry *e, int n, const char *part)
{
    int i;
    for (i = 0; i < n; i++)
        if (oul_same(e[i].kind, OUL_NEEDPART) && oul_same(e[i].a, part)) e[i].kind[0] = 0;
}

/* The name on a Part file's "part" line, copied into name (size bytes): 1, else 0. */
static int oul_part_name(const char *text, char *name, int size)
{
    const char *s = text;
    int n, i;
    while (*s) {
        while (oul_blank(*s)) s++;
        if ((n = oul_word(s, "part"))) {
            s += n;
            while (oul_blank(*s)) s++;
            for (n = 0; s[n] && !oul_blank(s[n]) && s[n] != '\n' && s[n] != '\r'; n++)
                ;
            if (!n || n >= size) return 0;
            for (i = 0; i < n; i++) name[i] = s[i];
            name[n] = 0;
            return 1;
        }
        while (*s && *s != '\n') s++;
        if (*s) s++;
    }
    return 0;
}

#endif
