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
#define OUD_INDEX_NAME "GFOpenUpdate.index"             /* beside the archives on the release */

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
        const char *q = p;
        int n = oud_words(p, end, w, 7, &next);
        p = next;
        while (q < end && oud_blank(*q)) q++;
        if (n == 0 || (q < end && *q == ';')) continue;      /* empty, or a comment however long */
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

/* Whether a part the index has and the manifest doesn't is listed as "not installed". GFOpenUp is always there.
 * KickstartSet goes in with the slim ROM, through GFOpenUpTool KICKSET. */
static int oud_listed_as_new(const char *name)
{
    return strcmp(name, "GFOpenUp") && strcmp(name, "KickstartSet");
}

/* The index's parts from parts[from] on (count in all) that aren't installed move to parts[from], in the index's
 * order, each with installed "". How many. Update never installs them, GFOpenUp's Installer adds a part. */
static int oud_new_parts(struct oud_part *parts, int from, int count, const char *manifest, long mlen)
{
    int i, n = from;
    char have[24];
    if (!manifest) return 0;
    for (i = from; i < count; i++) {
        if (oud_installed(manifest, mlen, parts[i].name, have, sizeof have) || !oud_listed_as_new(parts[i].name))
            continue;
        if (n != i) parts[n] = parts[i];
        parts[n].installed[0] = 0;
        n++;
    }
    return n - from;
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

/* ---- what the commodity says and writes (step 3) ---- */

#define OUD_URL_DEFAULT "https://github.com/DalsinAI/gfopenup/releases/download/updates/"   /* the URL tooltype */
#define OUD_RETRY 60L                                   /* seconds between tries until the first check gets through */

/* base and file as one address into out: base gets a "/" when it hasn't one. 1, or 0 when out is too small or
 * either holds a quote, a blank or a control letter (the address goes on a command line). */
static int oud_url(const char *base, const char *file, char *out, int size)
{
    int n = 0, i;
    const char *part[3];
    part[0] = base; part[1] = (*base && base[strlen(base) - 1] != '/') ? "/" : ""; part[2] = file;
    if (!*base) return 0;
    for (i = 0; i < 3; i++) {
        const char *p;
        for (p = part[i]; *p; p++) {
            if ((unsigned char)*p < 33 || *p == '"' || *p == '*' || (unsigned char)*p > 126 || n >= size - 1) return 0;
            out[n++] = *p;
        }
    }
    out[n] = 0;
    return 1;
}

static int oud_put(char *out, int size, int n, const char *s)
{
    while (*s && n < size - 1) out[n++] = *s++;
    out[n] = 0;
    return n;
}

/* The offers as the requester's lines, "<Name>: <installed> to <version>" each, the first `show` of them and then
 * "and <n> more". The length written. */
static int oud_offer_text(const struct oud_part *parts, int count, int show, char *out, int size)
{
    int i, n = 0;
    out[0] = 0;
    for (i = 0; i < count && i < show; i++) {
        n = oud_put(out, size, n, parts[i].name);
        n = oud_put(out, size, n, ": ");
        n = oud_put(out, size, n, parts[i].installed);
        n = oud_put(out, size, n, " to ");
        n = oud_put(out, size, n, parts[i].version);
        n = oud_put(out, size, n, "\n");
    }
    if (count > show) {
        char num[12];
        int more = count - show, k = 0, j;
        char rev[12];
        while (more > 0 && k < 11) { rev[k++] = (char)('0' + more % 10); more /= 10; }
        for (j = 0; j < k; j++) num[j] = rev[k - 1 - j];
        num[k] = 0;
        n = oud_put(out, size, n, "and ");
        n = oud_put(out, size, n, num);
        n = oud_put(out, size, n, " more\n");
    }
    return n;
}

/* Skipped's new text when the user says no to these offers: a "skip <Name> <version>" line each, then the old
 * text's lines for other parts (old may be NULL). The length written, or -1 when out is too small (then the file
 * is left as it was). */
static int oud_skip_text(const struct oud_part *parts, int count, const char *old, long oldlen, char *out, int size)
{
    int i, n = 0;
    const char *p = old, *end = old ? old + oldlen : NULL, *next;
    char w[4][80];
    for (i = 0; i < count; i++) {
        if (n + (int)strlen(parts[i].name) + (int)strlen(parts[i].version) + 8 >= size) return -1;
        n = oud_put(out, size, n, "skip ");
        n = oud_put(out, size, n, parts[i].name);
        n = oud_put(out, size, n, " ");
        n = oud_put(out, size, n, parts[i].version);
        n = oud_put(out, size, n, "\n");
    }
    while (p && p < end) {
        int words = oud_words(p, end, w, 4, &next), again = 0;
        p = next;
        if (words != 3 || strcmp(w[0], "skip")) continue;
        for (i = 0; i < count; i++) again |= !strcmp(parts[i].name, w[1]);
        if (again) continue;
        if (n + (int)strlen(w[1]) + (int)strlen(w[2]) + 8 >= size) return -1;
        n = oud_put(out, size, n, "skip ");
        n = oud_put(out, size, n, w[1]);
        n = oud_put(out, size, n, " ");
        n = oud_put(out, size, n, w[2]);
        n = oud_put(out, size, n, "\n");
    }
    if (!count && !n) out[0] = 0;
    return n;
}

/* ---- the commands the commodity runs (step 3) ---- */

#define OUD_HTTPGET "SYS:Tools/OpenSocket/httpget"       /* GFOpenSocket's, https:// through GFOpenTLS, follows redirects */
#define OUD_UNPACK "SYS:Utilities/GFOpenCompress"        /* GFOpenCompress x <archive> <drawer> */
#define OUD_TOOL "SYS:C/GFOpenUpTool"
#define OUD_WORK "T:GFOpenUpdate"                        /* downloads and unpacked parts, deleted after */

/* A path fit to go in quotes on a command line: something there, no quote, no newline, no control letter. */
static int oud_quotable(const char *s)
{
    if (!*s) return 0;
    for (; *s; s++) if ((unsigned char)*s < 32 || *s == '"') return 0;
    return 1;
}

static int oud_cat(char *out, int size, const char *const *bits)
{
    int n = 0;
    for (; *bits; bits++) {
        const char *p;
        for (p = *bits; *p; p++) {
            if (n >= size - 1) return 0;
            out[n++] = *p;
        }
    }
    out[n] = 0;
    return 1;
}

/* httpget saving url as the file to: 1, or 0 when it won't fit or either can't go on a command line (url is
 * oud_url's, so it has no blank or quote). */
static int oud_fetch_cmd(const char *url, const char *to, char *out, int size)
{
    const char *bits[] = { OUD_HTTPGET " -q -o \"", to, "\" ", url, NULL };
    const char *p;
    if (!oud_quotable(to) || !*url) return 0;
    for (p = url; *p; p++) if ((unsigned char)*p < 33 || *p == '"' || (unsigned char)*p > 126) return 0;
    return oud_cat(out, size, bits);
}

/* GFOpenCompress unpacking the archive into the drawer. 1, or 0 as oud_fetch_cmd. */
static int oud_unpack_cmd(const char *archive, const char *drawer, char *out, int size)
{
    const char *bits[] = { OUD_UNPACK " x \"", archive, "\" \"", drawer, "\"", NULL };
    if (!oud_quotable(archive) || !oud_quotable(drawer)) return 0;
    return oud_cat(out, size, bits);
}

/* GFOpenUpTool installing the part's drawer, <drawer>/<name>: name is the part's drawer as unpacked ("<name>/..." in the
 * archive, update_index.py). 1, or 0. */
static int oud_install_cmd(const char *drawer, const char *name, char *out, int size)
{
    const char *bits[] = { OUD_TOOL " INSTALL \"", drawer, "/", name, "\"", NULL };
    if (!oud_quotable(drawer) || !oud_plain_name(name)) return 0;
    return oud_cat(out, size, bits);
}

/* The LastCheck file's text (seconds) back as a number: 0 when there is none or it isn't one. */
static long oud_last(const char *text, long len)
{
    char w[1][80];
    const char *next;
    long n;
    if (!text || len <= 0) return 0;
    if (oud_words(text, text + len, w, 1, &next) < 1 || !oud_number(w[0], &n)) return 0;
    return n;
}

/* ---- the window's rows (step 4) ---- */

struct oud_row {
    char name[40];
    char installed[24];
    char newer[24];                                     /* "" when there is none */
};

/* The parts the manifest lists ("part <Name> <version> ..." lines), each with the newer version an offer has (offers
 * from oud_offers, count of them), into rows (max): the offered first, in the offers' order, then the rest in the
 * manifest's order. The count of rows. */
static int oud_rows(const char *manifest, long len, const struct oud_part *offers, int count, struct oud_row *rows, int max)
{
    const char *p = manifest, *end = manifest ? manifest + len : NULL, *next;
    char w[8][80];
    int n = 0, i;
    for (i = 0; i < count && n < max; i++, n++) {
        strcpy(rows[n].name, offers[i].name);
        strcpy(rows[n].installed, offers[i].installed);
        strcpy(rows[n].newer, offers[i].version);
    }
    while (p && p < end && n < max) {
        int words = oud_words(p, end, w, 8, &next), dup = 0;
        p = next;
        if (words < 3 || strcmp(w[0], "part") || strlen(w[1]) >= 40 || strlen(w[2]) >= 24) continue;
        for (i = 0; i < n; i++) dup |= !strcmp(rows[i].name, w[1]);
        if (dup) continue;
        strcpy(rows[n].name, w[1]);
        strcpy(rows[n].installed, w[2]);
        rows[n].newer[0] = 0;
        n++;
    }
    return n;
}

/* The parts oud_new_parts found (fresh, more of them) after the n rows already there: rows with installed "" and
 * the index's version as newer. The count of rows. */
static int oud_rows_add_new(struct oud_row *rows, int n, int max, const struct oud_part *fresh, int more)
{
    int i;
    for (i = 0; i < more && n < max; i++, n++) {
        strcpy(rows[n].name, fresh[i].name);
        rows[n].installed[0] = 0;
        strcpy(rows[n].newer, fresh[i].version);
    }
    return n;
}

/* A row as the list shows it: "<Name>  <installed>  new: <version>", "<Name>  <installed>", or for a part that
 * isn't installed "<Name>  not installed: <version>". The length. */
static int oud_row_text(const struct oud_row *r, char *out, int size)
{
    int n = 0;
    out[0] = 0;
    n = oud_put(out, size, n, r->name);
    if (!r->installed[0]) {
        n = oud_put(out, size, n, "  not installed: ");
        return oud_put(out, size, n, r->newer);
    }
    n = oud_put(out, size, n, "  ");
    n = oud_put(out, size, n, r->installed);
    if (r->newer[0]) {
        n = oud_put(out, size, n, "  new: ");
        n = oud_put(out, size, n, r->newer);
    }
    return n;
}

/* The line under the list: what the last check found. state: 1 checked, 0 never yet, -1 the list couldn't be had. */
static const char *oud_status(int state, int offers)
{
    if (state < 0) return "The list of updates couldn't be fetched. Is the network up?";
    if (state == 0) return "Not checked yet.";
    if (offers == 0) return "GFOS is up to date.";
    return offers == 1 ? "One update is ready." : "Updates are ready.";
}

/* The status line when parts can be added too (more of them, from oud_new_parts): oud_status's line, then that. */
static const char *oud_status_more(int state, int offers, int more)
{
    if (state != 1 || more == 0) return oud_status(state, offers);
    if (offers == 0) return "GFOS is up to date. More parts can be added with GFOpenUp's Installer.";
    return "Updates are ready. More parts can be added with GFOpenUp's Installer.";
}

/* ---- GFOpenNotifier, or Ranchero or Ringhio: "updates are waiting" ---- */

#define OUD_NOTIFY_PORT "RINGHIO"                       /* GFOpenNotifier's, as Ranchero's and Ringhio's */
#define OUD_PORT "GFOPENUPDATE"                         /* ours: a double-click on the notice sends SHOW here */

/* The two commands for the notifier: register (which = 0) and the notice (which = 1), count updates waiting. */
static int oud_notice_cmd(int which, int count, char *out, int size)
{
    char num[12], rev[12];
    int k = 0, j, n = 0;
    if (!which)
        return oud_put(out, size, 0, "REGISTERAPP APP=GFOpenUpdate AREXXPORT=" OUD_PORT " TEXT GFOS updates");
    if (count < 0) count = 0;
    do { rev[k++] = (char)('0' + count % 10); count /= 10; } while (count > 0 && k < 11);
    for (j = 0; j < k; j++) num[j] = rev[k - 1 - j];
    num[k] = 0;
    n = oud_put(out, size, n, "RINGHIO APP=GFOpenUpdate TITLE=\"GFOS updates\" BACKRXMSG=SHOW TEXT ");
    if (!strcmp(num, "1")) n = oud_put(out, size, n, "One update is waiting.");
    else { n = oud_put(out, size, n, num); n = oud_put(out, size, n, " updates are waiting."); }
    n = oud_put(out, size, n, " Double-click to see them.");
    return n;
}

#endif
