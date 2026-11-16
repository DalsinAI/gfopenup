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

#include <string.h>

#define OUL_AMICRADLE 1u          /* an AmiCradle instance (its ACBridge or ACStorage board) */
#define OUL_OPENKICK    2u          /* the open ROM that applies the boot's settings (oul_prefs_task) */

/* 1 on the open ROM whose intuition applies Sound's levels, Gamepads' patch and the stack caps at boot:
 * the GFOpenKick ROM (okr: its 'OKR1' mark) with prefs.resource (version, revision) at least 47.101 (no-ROM-volume
 * design P5). Compared as two numbers, as okvr.h does: 47.101 is newer than 47.100, and 48.0 than both. */
static int oul_prefs_task(int okr, unsigned version, unsigned revision)
{
    return okr && (version > 47u || (version == 47u && revision >= 101u));
}

/* The ACRTG graphics board's Zorro products (amicradle-guest common/protocol/acrtg.h, ACRTG_PRODUCT*): 9 the
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

/* "needs 68040" (9 November 2026, the user's CPU floor: a 68040 or 68060 with its FPU and MMU; nothing less, even
 * for handy things): 1 when exec's AttnFlags say so. AttnFlags has no MMU bit, but a 68040 or 68060 with its FPU is
 * the full chip: the EC parts have neither FPU nor MMU, the LC parts an MMU and no FPU. The bits are exec's
 * AFF_68040 (1 << 3), AFF_FPU40 (1 << 6) and AFF_68060 (1 << 7), written out so the host test needs no NDK. */
#define OUL_AFF_68040 0x0008u
#define OUL_AFF_FPU40 0x0040u
#define OUL_AFF_68060 0x0080u
static int oul_full_040(unsigned attn)
{
    return (attn & (OUL_AFF_68040 | OUL_AFF_68060)) && (attn & OUL_AFF_FPU40);
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

/* The k-th (from 0) "needs part <name>" line of a Part file's text, into name: 1, or 0 when there is none. REINSTALL
 * puts a part's needed part in first when the disk hasn't got it yet: GFOpenMUI 19.0 needs the GFOpenLayout part,
 * which no earlier pack had (Ivanova's lab, bundle C, 14 Nov 2026), and INSTALL leaves a part out without its needs. */
static int oul_part_needs(const char *text, int k, char *name, int size)
{
    const char *s = text;
    while (*s) {
        char line[160], *w[4];
        int len = 0, n;
        while (s[len] && s[len] != '\n') len++;
        if (len < (int)sizeof line) {
            const char *need;
            memcpy(line, s, (size_t)len);
            line[len] = 0;
            if (len && line[len - 1] == '\r') line[len - 1] = 0;
            n = oul_words(line, w, 4);
            if ((need = oul_needs_part(w, n)) && k-- == 0) {
                if ((int)strlen(need) >= size) return 0;
                strcpy(name, need);
                return 1;
            }
        }
        s += len;
        if (*s) s++;
    }
    return 0;
}

/* 1 when a Part file's text puts a file at dest (a copy, add, default or face line, whatever its condition), AmigaDOS's
 * way: case doesn't count. REMOVE leaves a file another installed part puts there too: SYS:Libs/gfopenlayout.library
 * is the GFOpenLayout part's and the KickstartSet part's, one manifest line, and REMOVE GFOpenLayout took it from
 * the set (Ivanova's lab, bundle D row 8, 14 Nov 2026). */
static int oul_lower(int c) { return c >= 'A' && c <= 'Z' ? c + 32 : c; }

static int oul_part_writes(const char *text, const char *dest)
{
    const char *s = text;
    while (*s) {
        char line[320], *w[6];
        const char *r;
        int len = 0, n;
        while (s[len] && s[len] != '\n') len++;
        if (len < (int)sizeof line) {
            memcpy(line, s, (size_t)len);
            line[len] = 0;
            if (len && line[len - 1] == '\r') line[len - 1] = 0;
            r = oul_strip(line, 0, 1);
            if (r != line) memmove(line, r, strlen(r) + 1);
            n = oul_words(line, w, 6);
            if (n >= 3 && (oul_same(w[0], "copy") || oul_same(w[0], "add") || oul_same(w[0], "default") ||
                           oul_same(w[0], "face"))) {
                const char *a = w[2], *b = dest;
                while (*a && oul_lower(*a) == oul_lower(*b)) a++, b++;
                if (!*a && !*b) return 1;
            }
        }
        s += len;
        if (*s) s++;
    }
    return 0;
}

/* REMOVE's check for each file: another part the manifest lists as installed whose Part file (texts[i], the pack's
 * part names[i]) puts the file at dest too; NULL when none does, and the file can go. */
static const char *oul_kept_by(const oul_entry *e, int n, const char *self, const char *const *names,
                               const char *const *texts, int nparts, const char *dest)
{
    int i;
    for (i = 0; i < nparts; i++)
        if (texts[i] && !oul_same(names[i], self) && oul_installed(e, n, names[i]) && oul_part_writes(texts[i], dest))
            return names[i];
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

/* ---- What GFOpenUp wrote, so only that is taken out (9 November 2026) ----
 * A "new" line names a file GFOpenUp put where there was none. UNINSTALL and REMOVE deleted every one of them,
 * whatever had happened to the file since: written over by the OS, another program or a card builder, it went all
 * the same (Dale's PiStorm card, GFOS on GFOpenKick full38, 9 Nov 2026: 2,080 of its system drive's 2,170 files went
 * at 02:07 while the GFOpenUp pack's Install and Uninstall were being run). Now INSTALL notes each "new" file's size and CRC-32 as the line's second field,
 * "=<size>:<crc, 8 hex digits>", and a file that is no longer those bytes is someone else's: it stays, and leaves the
 * list. A line without the field (GFOpenUpTool 0.8.16 and earlier) is checked against nothing; INSTALL adds the field
 * the next time it finds GFOpenUp's bytes there (an upgrade, a reinstall). */
static unsigned long oul_crc_table[256];

/* the CRC-32 (IEEE 802.3, as zip's) of n more bytes, from crc (0 to start) */
static unsigned long oul_crc32(unsigned long crc, const unsigned char *p, long n)
{
    if (!oul_crc_table[1]) {
        unsigned long c, i;
        int k;
        for (i = 0; i < 256; i++) {
            for (c = i, k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320UL ^ (c >> 1) : c >> 1;
            oul_crc_table[i] = c;
        }
    }
    crc = ~crc & 0xFFFFFFFFUL;
    while (n-- > 0) crc = oul_crc_table[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
    return ~crc & 0xFFFFFFFFUL;
}

/* the field for a file of size bytes and that CRC, in out (at least 21 bytes) */
static void oul_mark(char *out, unsigned long size, unsigned long crc)
{
    static const char hex[] = "0123456789abcdef";
    char digits[12];
    int n = 0, i;
    *out++ = '=';
    do { digits[n++] = (char)('0' + size % 10); size /= 10; } while (size && n < 11);
    while (n) *out++ = digits[--n];
    *out++ = ':';
    for (i = 7; i >= 0; i--) *out++ = hex[(crc >> (4 * i)) & 15];
    *out = 0;
}

/* 1 when the field is a mark (oul_mark), with its size and CRC */
static int oul_mark_read(const char *s, unsigned long *size, unsigned long *crc)
{
    unsigned long v = 0, c = 0;
    int digits = 0;
    if (!s || *s++ != '=') return 0;
    while (*s >= '0' && *s <= '9' && digits < 11) { v = v * 10 + (unsigned long)(*s++ - '0'); digits++; }
    if (!digits || *s++ != ':') return 0;
    for (digits = 0; digits < 8; digits++, s++) {
        int d = *s >= '0' && *s <= '9' ? *s - '0' : *s >= 'a' && *s <= 'f' ? *s - 'a' + 10 : *s >= 'A' && *s <= 'F' ? *s - 'A' + 10 : -1;
        if (d < 0) return 0;
        c = c << 4 | (unsigned long)d;
    }
    if (*s) return 0;
    *size = v;
    *crc = c & 0xFFFFFFFFUL;
    return 1;
}

/* What UNINSTALL and REMOVE do with a "new" line's file, from its field (b) and the file as it is (there: 1 when it
 * is a file that could be read whole, with its size and CRC): 1 take it out, 0 leave it (someone else's now). A line
 * without a mark is taken out as before, as is a file that couldn't be read (DeleteFile says what is wrong). */
static int oul_still_ours(const char *b, int there, unsigned long size, unsigned long crc)
{
    unsigned long msize, mcrc;
    if (!oul_mark_read(b, &msize, &mcrc) || !there) return 1;
    return msize == size && mcrc == crc;
}

/* ---- the OpenUp from before the GF names (9 November 2026) ----
 * Its OpenUpTool UNINSTALL (0.8.11 and earlier, not GFOpenUp's to change) deletes every "new" line of its list,
 * ENVARC:OpenAmiga/OpenUp.manifest. Where GFOpenUp has put its own file at the same name since, that file is
 * GFOpenUp's (its manifest lists it), and the old UNINSTALL would delete it: the backdrops, the developer files, the
 * datatypes and the rest that both put in the same places, nearly all of a GFOS disk. OLDNAMES takes those lines out
 * of the old list first (its copy in SYS:Storage/GFOpenUp-Superseded/OpenUp-before-GF keeps them). 1 when the old
 * list's line (without its newline) is a "new" line for a path the GFOpenUp list names (listed(path) != 0). */
static int oul_old_line_taken(const char *line, int (*listed)(const char *path))
{
    char path[260];
    int n = 0;
    if (strncmp(line, "new\t", 4)) return 0;
    line += 4;
    while (line[n] && line[n] != '\t' && line[n] != '\r' && n < (int)sizeof path - 1) { path[n] = line[n]; n++; }
    path[n] = 0;
    return n > 0 && listed(path);
}

#endif
