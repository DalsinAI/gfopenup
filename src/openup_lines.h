/* What a Part file line's condition asks, the ROM test for GFOpenKick, and one part's need of another (the words of
 * a line, the manifest's needpart lines): plain C with no OS calls, so
 * GFOpenUpTool (src/openuptool.c) and its host test (tests/test_openup_lines.c) share it.
 *
 * A condition goes in front of any other line, a startup line too:
 *   when AmiCradle <line>      only on an AmiCradle instance (GFOpenUpTool 0.6.3)
 *   unless AmiCradle <line>    only elsewhere (0.8.9)
 *   when GFOpenKick <line>         only on the open ROM whose intuition applies the boot's settings (0.8.9): the
 *                                GFOpenKick ROM with prefs.resource 47.101 or later, the ROM without ROM: (the
 *                                no-ROM-volume design of 24 October 2026, sections 9 and 10)
 *   unless GFOpenKick <line>       on every other ROM: OS 3.2.3's own, and a GFOpenKick ROM before prefs.resource
 *                                47.101, whose prefs task applies only the system's own settings
 * A condition this GFOpenUpTool doesn't know keeps its line out of an install, as before.
 *
 * And one part needing another (25 October 2026, the GFOffice design's section 12.2):
 *   needs part <name>            INSTALL leaves the part out while the manifest doesn't list <name> as installed,
 *                                and notes the need in the manifest ("needpart <part> <name>"); REMOVE of <name>
 *                                refuses while a part that needs it is installed. The GFOffice Suite's programs
 *                                need the GFOffice part, which holds their shared files (their four libraries,
 *                                z.library, the dictionary, OWConvert), so the shared files have one owner.
 * A GFOpenUpTool before this ignores the line ("needs" with a word it doesn't know).
 *
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef OPENUP_LINES_H
#define OPENUP_LINES_H

#define OUL_AMICRADLE 1u          /* an AmiCradle instance (its ACBridge or ACStorage board) */
#define OUL_OPENKICK    2u          /* the open ROM that applies the boot's settings (oul_prefs_task) */

/* 1 on the open ROM whose intuition applies Sound's levels, Gamepads' patch and the stack caps at boot:
 * the GFOpenKick ROM (okr: its 'OKR1' mark) with prefs.resource (version, revision) at least 47.101 (no-ROM-volume
 * design P5). Compared as two numbers, as okvr.h does: 47.101 is newer than 47.100, and 48.0 than both. */
static int oul_prefs_task(int okr, unsigned version, unsigned revision)
{
    return okr && (version > 47u || (version == 47u && revision >= 101u));
}

/* The ACRTG graphics board's Zorro products (amigachrome-guest common/protocol/acrtg.h, ACRTG_PRODUCT*): 9 the
 * 16 MiB board, 15 the 64 MiB one ("rtgVramMiB": 64, acrtg.card 2.6), 16 the 256 MiB one ("rtgVramMiB": 256,
 * acrtg.card 2.8). DETECT's "RTG:" line, ENV:GFOpenUp/RTG and a Part file's "needs ACRTG" take any of them. Up to
 * GFOpenUpTool 0.7.5 only 9 counted; up to 0.8.11 16 didn't, so Instance-32 (256 MiB) had "RTG: none" and its
 * GFOpenRTG part was left out ("needs an ACRTG graphics board"), at 0.19.0 (W144, 26 October 2026). */
#define OUL_ACRTG_PRODUCTS 3
static const unsigned short oul_acrtg_products[OUL_ACRTG_PRODUCTS] = { 9, 15, 16 };

/* 1 when the Zorro product is one of the ACRTG board's */
static int oul_acrtg_product(unsigned product)
{
    int i;
    for (i = 0; i < OUL_ACRTG_PRODUCTS; i++)
        if (oul_acrtg_products[i] == product) return 1;
    return 0;
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
 * bits in have, NULL when it doesn't (or names a machine this GFOpenUpTool doesn't know), the line itself when
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
    if ((n = oul_word(s, "AmiCradle"))) bit = OUL_AMICRADLE;
    else if ((n = oul_word(s, "GFOpenKick"))) bit = OUL_OPENKICK;
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

/* A line of the manifest, ENVARC:GFOpenAmiga/GFOpenUp.manifest, as GFOpenUpTool keeps it: its kind, then up to two
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

/* ---- INSTALL's way and its end (25 October 2026) ----
 * A file where a part's drawer goes is in its way: Instance-32 has a calculator of its own at SYS:Utilities/GFOpenCalc,
 * where the GFOffice Suite's GFOpenCalc drawer goes. INSTALL keeps that file in SYS:Storage/GFOpenUp-Superseded, as it
 * keeps any original it replaces, and makes the drawer; UNINSTALL, and REMOVE once the drawer is empty, put the file
 * back. oul_way names the drawers a path goes through, for INSTALL to look at each.
 *
 * And a part whose install had an error isn't listed as installed: no "part" line (an older version's stays, so an
 * upgrade is tried again), and its startup and needpart lines stay as they were. What its files did stays listed (a
 * file put in, an original kept), so REMOVE and UNINSTALL can still undo it. While the part installs, its startup
 * lines from before are marked "ostartup" (those its Part file still has go back to "startup") and its needpart lines
 * OUL_OLDNEED; the lines it adds come at first and after (the manifest's count before), and oul_settle ends it. */
#define OUL_OLDNEED "oneedpart"

/* The drawers a path goes through, below its volume, from the top: for "SYS:Utilities/GFOpenCalc/GFOpenCalc", way 0 is
 * "SYS:Utilities" and way 1 "SYS:Utilities/GFOpenCalc". 1 with the i-th in out (size bytes); 0 when there is none,
 * when it doesn't fit, or at an empty name (AmigaDOS's "/" for the parent), where nothing is looked at. */
static int oul_way(const char *path, int i, char *out, int size)
{
    const char *s = path, *p;
    int n = -1, len, k;
    for (p = path; *p; p++)
        if (*p == ':') { s = p + 1; break; }
    for (p = s; *p; p++)
        if (*p == '/') {
            if (p == s || p[-1] == '/' || p[-1] == ':') return 0;
            if (++n == i) break;
        }
    if (!*p) return 0;
    len = (int)(p - path);
    if (len >= size) return 0;
    for (k = 0; k < len; k++) out[k] = path[k];
    out[len] = 0;
    return 1;
}

/* INSTALL's pass 1: the part's needpart lines from before are marked, so a failed install can put them back. */
static void oul_mark_needs(oul_entry *e, int n, const char *part)
{
    int i, k;
    for (i = 0; i < n; i++)
        if (oul_same(e[i].kind, OUL_NEEDPART) && oul_same(e[i].a, part))
            for (k = 0; (e[i].kind[k] = OUL_OLDNEED[k]); k++) ;
}

/* INSTALL's end. ok: the marked lines go (the Part file's own are the ones now). Not ok: the marked lines come back
 * as they were, and the startup and needpart lines this install added (at first and after) go. */
static void oul_settle(oul_entry *e, int n, int first, const char *part, int ok)
{
    int i, k;
    for (i = 0; i < n; i++) {
        char *kind = e[i].kind;
        if ((oul_same(kind, "ostartup") && oul_same(e[i].b, part)) || (oul_same(kind, OUL_OLDNEED) && oul_same(e[i].a, part))) {
            if (ok) kind[0] = 0;
            else for (k = 0; (kind[k] = kind[k + 1]); k++) ;      /* "ostartup" is "startup" again, "oneedpart" "needpart" */
        } else if (!ok && i >= first && ((oul_same(kind, "startup") && oul_same(e[i].b, part))
                                         || (oul_same(kind, OUL_NEEDPART) && oul_same(e[i].a, part)))) {
            kind[0] = 0;
        }
    }
}

/* ---- The manifest's first and last lines (1 November 2026) ----
 * GFOpenUpTool writes the manifest whole or not at all (a new copy that takes the old one's name), and ends it with
 * OUL_MANIFEST_END, so a manifest cut short (by an older GFOpenUpTool on a full disk, say) shows it and is never saved
 * over: a short list would lose the originals it names ("replaced" lines), which UNINSTALL puts back. The first
 * line says which kind it is: OUL_MANIFEST_HEAD has the end line; OUL_MANIFEST_OLD (GFOpenUpTool up to 0.8.11) has
 * none, and is whole when its last line ends, as every line it wrote did. */
#define OUL_MANIFEST_HEAD "GFOpenUp\t0.2"
#define OUL_MANIFEST_OLD "GFOpenUp\t0.1"
#define OUL_MANIFEST_END "GFOpenUp\tend"

/* 1 when the text from s to e (a line without its newline, a '\r' before it allowed) is w. */
static int oul_line_is(const char *s, const char *e, const char *w)
{
    if (e > s && e[-1] == '\r') e--;
    while (s < e && *w && *s == *w) s++, w++;
    return s == e && !*w;
}

/* 1 when the manifest's text (len bytes) is whole, as above. */
static int oul_manifest_whole(const char *text, long len)
{
    const char *end, *head = text, *last;
    if (len <= 0 || text[len - 1] != '\n') return 0;
    end = text + len - 1;
    while (*head != '\n') head++;
    if (oul_line_is(text, head, OUL_MANIFEST_OLD)) return 1;
    if (head - text < 9 || !oul_line_is(text, text + 9, "GFOpenUp\t")) return 0;   /* not a manifest's first line */
    for (last = end; last > text && last[-1] != '\n'; last--)
        ;
    return last > head && oul_line_is(last, end, OUL_MANIFEST_END);
}

#endif
