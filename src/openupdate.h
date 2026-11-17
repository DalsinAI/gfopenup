/* GFOpenUpdate's reading and weighing (Dale, 17 Nov 2026: "a GFOS updater that knows what versions of things are
 * installed and checks dalsinai etc for updates and offers them to the user to update"; "a wbstartup commodity that
 * checks periodically"; "once at start up when the network is up, then periodically that can be set via TOOLTYPE";
 * "store its data in envarc:"). docs/design/GFOPENUPDATE.md.
 *
 * What is installed is GFOpenUpTool's own list, ENVARC:GFOpenAmiga/GFOpenUp.manifest ("part <tab> name <tab>
 * version" lines). What is on offer is the index, one small text file fetched from DalsinAI:
 *
 *   GFOpenUpdate 1
 *   ; comments and empty lines are passed over
 *   part <Name> <version> <bytes> <sha256, 64 hex> <file>
 *   GFOpenUpdate end
 *
 * The last line shows the index came whole: one cut short on the way is never used. <file> is a plain name beside
 * the index (no drawer, no volume), so an index can't point a download anywhere else. Only a part that is installed
 * and older is offered: an update adds no part (as UPDATE AUTO), and a version the user said no to
 * (ENVARC:GFOpenUpdate/Skipped, "skip <Name> <version>" lines) is not offered again until a newer one comes.
 *
 * Plain C with no OS calls, shared with tests/test_openupdate.c. Versions compare by openup_vers.h's ouv_cmp.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef OPENUPDATE_H
#define OPENUPDATE_H

#include <string.h>
#include "openup_vers.h"

#define OUD_DATA "ENVARC:GFOpenUpdate"                  /* its drawer of data */
#define OUD_SKIPPED OUD_DATA "/Skipped"                 /* versions the user said no to */
#define OUD_LAST OUD_DATA "/LastCheck"                  /* when it last checked, seconds */
#define OUD_INDEX_HEAD "GFOpenUpdate 1"
#define OUD_INDEX_END "GFOpenUpdate end"

#define OUD_INTERVAL_DEFAULT 86400L                     /* seconds between checks: a day */
#define OUD_INTERVAL_MIN 600L                           /* never more often than this, whatever the tooltype says */

struct oud_part {
    char name[40];
    char version[24];
    char sha256[65];
    char file[80];
    long bytes;
    char installed[24];                                 /* oud_offers: the version in the manifest */
};

static int oud_blank(char c) { return c == ' ' || c == '\t' || c == '\r'; }

/* The line at p (to the newline or the text's end) cut into up to max words, each copied into w[i] (size bytes,
 * a longer word makes the line bad). The count, or -1 for a bad line; *next is the line after. */
static int oud_words(const char *p, const char *end, char w[][80], int max, const char **next)
{
    int n = 0, bad = 0;
    while (p < end && *p != '\n') {
        int k = 0;
        while (p < end && oud_blank(*p)) p++;
        if (p >= end || *p == '\n') break;
        if (n == max) { bad = 1; while (p < end && *p != '\n') p++; break; }
        while (p < end && *p != '\n' && !oud_blank(*p)) {
            if (k < 79) w[n][k++] = *p; else bad = 1;
            p++;
        }
        w[n++][k] = 0;
    }
    if (p < end) p++;
    *next = p;
    return bad ? -1 : n;
}

static int oud_hex64(const char *s)
{
    int i;
    for (i = 0; i < 64; i++) {
        char c = s[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) return 0;
    }
    return s[64] == 0;
}

/* A file's name beside the index: no drawer, no volume, no leading dot, and something there. */
static int oud_plain_name(const char *s)
{
    if (!*s || *s == '.') return 0;
    for (; *s; s++)
        if (*s == '/' || *s == ':' || *s == '\\' || *s == '?' || *s == '#' || *s == '%' || (unsigned char)*s < 33) return 0;
    return 1;
}

/* All digits, to a long (no more than 9 of them): 1, else 0. */
static int oud_number(const char *s, long *out)
{
    long n = 0;
    int k = 0;
    if (!*s) return 0;
    for (; *s; s++, k++) {
        if (!ouv_digit(*s) || k >= 9) return 0;
        n = n * 10 + (*s - '0');
    }
    *out = n;
    return 1;
}

/* 1 when the index (len bytes) is whole: its first line is the head and its last line the end. */
static int oud_index_whole(const char *text, long len)
{
    const char *end = text + len, *p = text, *next;
    char w[3][80];
    int seen_end = 0, first = 1;
    while (p < end) {
        int n = oud_words(p, end, w, 3, &next);
        p = next;
        if (n == 0) continue;
        if (first) {
            if (n != 2 || strcmp(w[0], "GFOpenUpdate") || strcmp(w[1], "1")) return 0;
            first = 0;
            continue;
        }
        if (seen_end) return 0;                         /* something after the end line */
        if (n == 2 && !strcmp(w[0], "GFOpenUpdate") && !strcmp(w[1], "end")) seen_end = 1;
    }
    return seen_end;
}

/* The index's parts into out (max of them): the count, or -1 when the index isn't whole or a part line is wrong
 * (then none of it is used: half an index is no index). A name listed twice is wrong too. */
static int oud_index_parts(const char *text, long len, struct oud_part *out, int max)
{
    const char *end = text + len, *p = text, *next;
    char w[7][80];
    int count = 0, i;
    if (!oud_index_whole(text, len)) return -1;
    while (p < end) {
        int n = oud_words(p, end, w, 7, &next);
        p = next;
        if (n == 0 || (n > 0 && w[0][0] == ';')) continue;
        if (n > 0 && !strcmp(w[0], "GFOpenUpdate")) continue;
        if (n != 6 || strcmp(w[0], "part")) return -1;
        if (count == max) return -1;
        if (strlen(w[1]) >= sizeof out->name || strlen(w[2]) >= sizeof out->version) return -1;
        if (!ouv_digit(w[2][0]) || !oud_plain_name(w[1])) return -1;
        if (!oud_number(w[3], &out[count].bytes) || out[count].bytes <= 0) return -1;
        if (!oud_hex64(w[4]) || !oud_plain_name(w[5])) return -1;
        for (i = 0; i < count; i++)
            if (!strcmp(out[i].name, w[1])) return -1;
        strcpy(out[count].name, w[1]);
        strcpy(out[count].version, w[2]);
        strcpy(out[count].sha256, w[4]);
        strcpy(out[count].file, w[5]);
        out[count].installed[0] = 0;
        count++;
    }
    return count;
}

/* The second field of the first "<kind> <name> <value>" line in text (the manifest's "part" lines, tabs between;
 * Skipped's "skip" lines, blanks between) into out: 1, else 0. Names match whole and by case, as the manifest's. */
static int oud_lookup(const char *text, long len, const char *kind, const char *name, char *out, int size)
{
    const char *end = text + len, *p = text, *next;
    char w[4][80];
    while (p < end) {
        int n = oud_words(p, end, w, 4, &next);
        p = next;
        if (n == 3 && !strcmp(w[0], kind) && !strcmp(w[1], name) && (int)strlen(w[2]) < size) {
            strcpy(out, w[2]);
            return 1;
        }
    }
    return 0;
}

/* The version of the part the manifest lists as installed: 1, else 0 (not installed). */
static int oud_installed(const char *manifest, long len, const char *name, char *out, int size)
{
    return manifest ? oud_lookup(manifest, len, "part", name, out, size) : 0;
}

/* What to offer: the index's parts that are installed, older than the index's, and not said no to at that version
 * or a later one. They move to the front of parts (count of them, from oud_index_parts), each with its installed
 * version; the number offered comes back. skipped may be NULL. */
static int oud_offers(struct oud_part *parts, int count, const char *manifest, long mlen, const char *skipped, long slen)
{
    int i, n = 0;
    for (i = 0; i < count; i++) {
        char have[24], no[24];
        if (!oud_installed(manifest, mlen, parts[i].name, have, sizeof have)) continue;
        if (ouv_cmp(parts[i].version, have) <= 0) continue;
        if (skipped && oud_lookup(skipped, slen, "skip", parts[i].name, no, sizeof no)
            && ouv_cmp(parts[i].version, no) <= 0) continue;
        if (n != i) parts[n] = parts[i];
        strcpy(parts[n].installed, have);
        n++;
    }
    return n;
}

/* The INTERVAL tooltype's value, seconds between checks after the one at startup. NULL or empty is the default;
 * 0 is "only at startup"; anything else under OUD_INTERVAL_MIN is raised to it; a value that isn't a number is the
 * default. */
static long oud_interval(const char *value)
{
    long n;
    if (!value || !*value || !oud_number(value, &n)) return OUD_INTERVAL_DEFAULT;
    if (n == 0) return 0;
    return n < OUD_INTERVAL_MIN ? OUD_INTERVAL_MIN : n;
}

/* 1 when a check is due: the one at startup (checked_this_run 0) once the network is up, then each interval
 * seconds after the last one (now and last in seconds; a clock set back counts as due, so a wrong date never stops
 * the checks). Never without the network, and never again with interval 0. */
static int oud_due(int network_up, int checked_this_run, long now, long last, long interval)
{
    if (!network_up) return 0;
    if (!checked_this_run) return 1;
    if (interval <= 0) return 0;
    return now < last || now - last >= interval;
}

#endif
