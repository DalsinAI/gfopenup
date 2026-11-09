/* GFOpenUpTool STARTUPSEQ's plain C (no OS calls), shared with its host test (tests/test_openup_startupseq.c):
 * the disk's S:Startup-Sequence against the ROM's default (amicradle-guest docs/design/rom-or-disk.md, section 6,
 * approved by Dale on 9 Nov 2026). dos weighs the two (GFOpenDOS's private ODRomDefaultState): the same, newer,
 * older, or no $VER and not the ROM's. A disk file always runs; GFOpenUp only offers the ROM's, and only when the
 * disk's is older or different and the user hasn't kept theirs against this ROM's version.
 *
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef OPENUP_STARTUPSEQ_H
#define OPENUP_STARTUPSEQ_H

/* ODRomDefaultState's answers (amicradle-guest libraries/opendos/dos_core.h, OD_RDS_) */
#define OSS_NONE    0       /* the ROM has no default Startup-Sequence (OS 3.2.3's ROM, an older GFOpenDOS) */
#define OSS_ROM     1       /* none on the disk: the ROM's runs */
#define OSS_SAME    2
#define OSS_NEWER   3
#define OSS_OLDER   4
#define OSS_DIFFERS 5

/* "V.R" of a packed version (version << 16 | revision) into b (12 bytes at least); b */
static char *oss_vr(char *b, unsigned long vr)
{
    char t[12];
    unsigned long parts[2];
    int n = 0, i;
    parts[0] = vr >> 16;
    parts[1] = vr & 0xFFFF;
    for (i = 0; i < 2; i++) {
        unsigned long v = parts[i];
        int m = 0;
        do { t[m++] = (char)('0' + v % 10); v /= 10; } while (v);
        while (m) b[n++] = t[--m];
        if (!i) b[n++] = '.';
    }
    b[n] = 0;
    return b;
}

/* the version kept in ENVARC:GFOpenAmiga/StartupSequence.kept ("V.R", as oss_vr writes it): packed, 0 for none */
static unsigned long oss_kept(const char *text)
{
    unsigned long v = 0, r = 0;
    if (!text || *text < '0' || *text > '9') return 0;
    while (*text >= '0' && *text <= '9') v = v * 10 + (unsigned long)(*text++ - '0');
    if (*text == '.') {
        text++;
        while (*text >= '0' && *text <= '9') r = r * 10 + (unsigned long)(*text++ - '0');
    }
    return v << 16 | (r & 0xFFFF);
}

/* CHECK's verdict: 1 (offer the ROM's: GFOpenUpTool answers 5, WARN) when the disk's is older or not the ROM's and
 * the user hasn't kept theirs against this very ROM version (kept == romvr); else 0 */
static int oss_offer(unsigned long state, unsigned long romvr, unsigned long kept)
{
    if (state != OSS_OLDER && state != OSS_DIFFERS) return 0;
    return kept != romvr;
}

/* CHECK's line for the state */
static const char *oss_says(unsigned long state)
{
    switch (state) {
    case OSS_NONE: return "This ROM has no default Startup-Sequence: nothing to compare.";
    case OSS_ROM: return "There is no S:Startup-Sequence on the disk: the ROM's default runs.";
    case OSS_SAME: return "S:Startup-Sequence is the ROM's default.";
    case OSS_NEWER: return "S:Startup-Sequence is newer than the ROM's default: yours runs.";
    case OSS_OLDER: return "S:Startup-Sequence is older than the ROM's default. Yours runs; the ROM's can take its place.";
    case OSS_DIFFERS: return "S:Startup-Sequence isn't the ROM's default and has no version. Yours runs; the ROM's can take its place.";
    default: return "S:Startup-Sequence: unknown.";
    }
}

/* a "; $VER:" line (after blanks), left out of a comparison */
static int oss_ver_line(const char *p, unsigned long n)
{
    static const char v[] = "; $VER:";
    unsigned long i = 0, k;
    while (i < n && (p[i] == ' ' || p[i] == '\t')) i++;
    for (k = 0; k < 7; k++, i++)
        if (i >= n || p[i] != v[k]) return 0;
    return 1;
}

/* the line at p (to its '\n' or the end, a CR before it left out): its length; *next where the next starts */
static unsigned long oss_line(const char *p, unsigned long n, unsigned long *next)
{
    unsigned long i = 0, len;
    while (i < n && p[i] != '\n') i++;
    *next = i < n ? i + 1 : i;
    len = i;
    if (len && p[len - 1] == '\r') len--;
    return len;
}

/* is the line (s, len) one of text's lines ("; $VER:" lines aside) */
static int oss_has_line(const char *text, unsigned long n, const char *s, unsigned long len)
{
    unsigned long i = 0;
    while (i < n) {
        unsigned long next, l = oss_line(text + i, n - i, &next);
        if (l == len && !oss_ver_line(text + i, l)) {
            unsigned long k = 0;
            while (k < len && text[i + k] == s[k]) k++;
            if (k == len) return 1;
        }
        i += next;
    }
    return 0;
}

/* COMPARE: "- line" for each of the ROM's lines the disk's hasn't, then "+ line" for each of the disk's lines the
 * ROM's hasn't, in their order ("; $VER:" lines aside; blank lines too); put(ctx, text, len) gets each piece. The
 * number of lines that differ. */
static unsigned long oss_compare(const char *rom, unsigned long rn, const char *disk, unsigned long dn,
                                 void (*put)(void *ctx, const char *s, unsigned long len), void *ctx)
{
    unsigned long count = 0;
    int side;
    for (side = 0; side < 2; side++) {
        const char *a = side ? disk : rom, *b = side ? rom : disk;
        unsigned long an = side ? dn : rn, bn = side ? rn : dn, i = 0;
        while (i < an) {
            unsigned long next, l = oss_line(a + i, an - i, &next);
            if (l && !oss_ver_line(a + i, l) && !oss_has_line(b, bn, a + i, l)) {
                put(ctx, side ? "+ " : "- ", 2);
                put(ctx, a + i, l);
                put(ctx, "\n", 1);
                count++;
            }
            i += next;
        }
    }
    return count;
}

#endif
