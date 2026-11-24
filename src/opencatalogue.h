/* GFOpenUpdate's reading of gfosdistribution's catalogue (backlog AC-280, "cards pull signed releases from the
 * public channel"). Dale, 24 Nov 2026: "every card is a stranger's card": a card pulls what is released from the
 * public channel, amigagfosports/gfosdistribution on GitHub; nothing is pushed to it and nothing assumes his LAN.
 *
 * The catalogue is software/index.txt there, one release a line, tabs between:
 *
 *   # comments and empty lines are passed over
 *   <Name> <tab> <version> <tab> <bytes> <tab> <sha256, 64 hex> <tab> <address> <tab> <description>
 *
 * The address must start with the releases' own address (OUC_RELEASES_DEFAULT, or the RELEASES tooltype) and be
 * "<tag>/<file>" after it, so a catalogue can't send a download anywhere else. A name listed at more than one
 * version is offered at its newest. A line that is wrong is left out and counted; the rest are used, because each
 * download is checked against its own size and SHA-256 before anything is installed.
 *
 * Weighed against GFOpenUpTool's manifest (ENVARC:GFOpenAmiga/GFOpenUp.manifest) each release is newer than what
 * is installed, not installed, or up to date. A release that isn't a GFOpenUp part (no Part file in its drawer,
 * GFOpenSQLite's zip) gets a Part file written for it by its layout (ouc_dest): C/ to C:, Libs/ to LIBS:, and so
 * on, the rest to SYS:Software/<Name>/, so GFOpenUpTool INSTALL lists every file and UNINSTALL can take it out.
 *
 * Plain C with no OS calls, shared with tests/test_opencatalogue.c. MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef OPENCATALOGUE_H
#define OPENCATALOGUE_H

#include <string.h>
#include "openupdate.h"

#define OUC_CATALOGUE_DEFAULT "https://raw.githubusercontent.com/amigagfosports/gfosdistribution/main/software/index.txt"
#define OUC_RELEASES_DEFAULT "https://github.com/amigagfosports/gfosdistribution/releases/download/"
#define OUC_ENV_CATALOGUE "GFOpenUpdate/Catalogue"          /* ENV:, a setting beside the CATALOGUE tooltype */
#define OUC_SOFTWARE "SYS:Software"                         /* where a release's own files go */
/* AC-280's next step: gfosdistribution's public key, put in the image. While a card has none, the catalogue's
 * SHA-256 of each file is the check; once it has one, an unsigned catalogue is refused (openupdate.c). */
#define OUC_KEY "S:GFOS/gfosdistribution.pub"

enum { OUC_SAME = 0, OUC_NEWER = 1, OUC_NEW = 2 };

struct ouc_item {
    struct oud_part p;              /* name, version, bytes, sha256, file (the address's last part), installed */
    char url[200];
    char desc[160];
    int state;                      /* OUC_SAME, OUC_NEWER or OUC_NEW, from ouc_weigh */
};

/* One tab-separated field at p into out (size bytes; longer is cut when cut is 1, else wrong): its length, or -1.
 * *next is after the tab, or at the line's end. */
static int ouc_field(const char *p, const char *end, char *out, int size, int cut, const char **next)
{
    int n = 0, over = 0;
    while (p < end && *p != '\t' && *p != '\n') {
        if (*p != '\r') { if (n < size - 1) out[n++] = *p; else over = 1; }
        p++;
    }
    out[n] = 0;
    if (p < end && *p == '\t') p++;
    *next = p;
    return over && !cut ? -1 : n;
}

/* The address after the releases' own: "<tag>/<file>", both plain names; the file into file. 1, else 0. */
static int ouc_address(const char *url, const char *releases, char *file, int size)
{
    const char *p, *slash = NULL;
    int n = (int)strlen(releases);
    char tag[80];
    if (n == 0 || strncmp(url, releases, n)) return 0;
    for (p = url; *p; p++)
        if ((unsigned char)*p < 33 || *p == '"' || *p == '*' || (unsigned char)*p > 126) return 0;
    for (p = url + n; *p; p++) if (*p == '/') { if (slash) return 0; slash = p; }
    if (!slash || slash - (url + n) >= (int)sizeof tag || (int)strlen(slash + 1) >= size) return 0;
    memcpy(tag, url + n, slash - (url + n));
    tag[slash - (url + n)] = 0;
    if (!oud_plain_name(tag) || !oud_plain_name(slash + 1)) return 0;
    strcpy(file, slash + 1);
    return 1;
}

/* The catalogue's releases into items (max of them), each name once at its newest version: the count. *bad (may
 * be NULL) gets the number of lines left out as wrong. */
static int ouc_parse(const char *text, long len, const char *releases, struct ouc_item *items, int max, int *bad)
{
    const char *end = text + len, *p = text, *next;
    int count = 0, wrong = 0, i;
    while (p < end) {
        struct ouc_item it;
        char size[16], sha[72], url[220];
        const char *q = p, *eol = p;
        while (eol < end && *eol != '\n') eol++;
        while (q < eol && (*q == ' ' || *q == '\r')) q++;
        if (q == eol || *q == '#') { p = eol < end ? eol + 1 : end; continue; }
        memset(&it, 0, sizeof it);
        if (ouc_field(p, eol, it.p.name, sizeof it.p.name, 0, &next) <= 0
            || ouc_field(next, eol, it.p.version, sizeof it.p.version, 0, &next) <= 0
            || ouc_field(next, eol, size, sizeof size, 0, &next) <= 0
            || ouc_field(next, eol, sha, sizeof sha, 0, &next) <= 0
            || ouc_field(next, eol, url, sizeof url, 0, &next) <= 0
            || ouc_field(next, eol, it.desc, sizeof it.desc, 1, &next) < 0
            || !oud_plain_name(it.p.name) || !ouv_digit(it.p.version[0]) || !oud_plain_name(it.p.version)
            || !oud_number(size, &it.p.bytes) || it.p.bytes <= 0 || !oud_hex64(sha) || strlen(url) >= sizeof it.url
            || !ouc_address(url, releases, it.p.file, sizeof it.p.file)) {
            wrong++;
            p = eol < end ? eol + 1 : end;
            continue;
        }
        p = eol < end ? eol + 1 : end;
        strcpy(it.p.sha256, sha);
        strcpy(it.url, url);
        for (i = 0; it.desc[i]; i++)                  /* the window's font: plain letters only */
            if ((unsigned char)it.desc[i] < 32 || (unsigned char)it.desc[i] > 126) it.desc[i] = '?';
        for (i = 0; i < count; i++)
            if (!strcmp(items[i].p.name, it.p.name)) break;
        if (i < count) {                              /* the name again: the newer of the two stays */
            if (ouv_cmp(it.p.version, items[i].p.version) > 0) items[i] = it;
            continue;
        }
        if (count == max) { wrong++; continue; }
        items[count++] = it;
    }
    if (bad) *bad = wrong;
    return count;
}

/* Each release weighed against the manifest (NULL: nothing installed): installed version and state set, and the
 * newer first, then the not installed, then the up to date, each in the catalogue's order. The number newer;
 * *fresh (may be NULL) the number not installed. */
static int ouc_weigh(struct ouc_item *items, int count, const char *manifest, long mlen, int *fresh)
{
    int i, j, k, newer = 0, add = 0;
    static const int order[3] = { OUC_NEWER, OUC_NEW, OUC_SAME };
    for (i = 0; i < count; i++) {
        char have[24];
        if (!oud_installed(manifest, mlen, items[i].p.name, have, sizeof have)) {
            items[i].p.installed[0] = 0;
            items[i].state = OUC_NEW;
            add++;
        } else {
            strcpy(items[i].p.installed, have);
            items[i].state = ouv_cmp(items[i].p.version, have) > 0 ? OUC_NEWER : OUC_SAME;
            newer += items[i].state == OUC_NEWER;
        }
    }
    for (k = 0, j = 0; k < 3; k++)                    /* a stable sort by state, in place */
        for (i = j; i < count; i++)
            if (items[i].state == order[k]) {
                struct ouc_item t = items[i];
                int m;
                for (m = i; m > j; m--) items[m] = items[m - 1];
                items[j++] = t;
            }
    if (fresh) *fresh = add;
    return newer;
}

/* A release as the list shows it: "<Name>  <installed> -> <version>  new", "<Name>  <version>  not installed",
 * or "<Name>  <version>  up to date". The length. */
static int ouc_row_text(const struct ouc_item *it, char *out, int size)
{
    int n = oud_put(out, size, 0, it->p.name);
    n = oud_put(out, size, n, "  ");
    if (it->state == OUC_NEWER) {
        n = oud_put(out, size, n, it->p.installed);
        n = oud_put(out, size, n, " -> ");
        n = oud_put(out, size, n, it->p.version);
        return oud_put(out, size, n, "  new");
    }
    n = oud_put(out, size, n, it->p.version);
    return oud_put(out, size, n, it->state == OUC_NEW ? "  not installed" : "  up to date");
}

/* The line under the list. state: 1 checked, 0 not yet, -1 the catalogue couldn't be had. */
static const char *ouc_status(int state, int newer, int fresh)
{
    if (state < 0) return "gfosdistribution's list couldn't be fetched. Is the network up?";
    if (state == 0) return "Checking gfosdistribution...";
    if (newer == 0 && fresh == 0) return "Nothing new: everything on gfosdistribution is installed and up to date.";
    if (newer == 0) return "Up to date. There is more on gfosdistribution to install.";
    return newer == 1 ? "One update is ready. Install all, or pick one." : "Updates are ready. Install all, or pick one.";
}

/* Where a file of a release's drawer goes: rel is its path in the drawer ("Libs/sqlite.library"). The system's
 * drawers by name, any case (C, Libs, Devs, L, S, Fonts, Locale, Classes, WBStartup), the rest under
 * SYS:Software/<name>/. 1, else 0 (it won't fit, or rel or name can't be quoted). */
static int ouc_dest(const char *name, const char *rel, char *out, int size)
{
    static const char *const sys[][2] = {
        { "C", "C:" }, { "Libs", "LIBS:" }, { "Devs", "DEVS:" }, { "L", "L:" }, { "S", "S:" },
        { "Fonts", "FONTS:" }, { "Locale", "LOCALE:" }, { "Classes", "SYS:Classes/" }, { "WBStartup", "SYS:WBStartup/" }
    };
    const char *slash = strchr(rel, '/'), *bits[6];
    unsigned i;
    int n;
    if (!*rel || !oud_quotable(rel) || !oud_plain_name(name)) return 0;
    if (slash && slash[1])
        for (i = 0; i < sizeof sys / sizeof sys[0]; i++) {
            n = (int)strlen(sys[i][0]);
            if (slash - rel == n) {
                int k;
                for (k = 0; k < n && (rel[k] | 0x20) == (sys[i][0][k] | 0x20); k++)
                    ;
                if (k == n) {
                    bits[0] = sys[i][1]; bits[1] = slash + 1; bits[2] = NULL;
                    return oud_cat(out, size, bits);
                }
            }
        }
    bits[0] = OUC_SOFTWARE "/"; bits[1] = name; bits[2] = "/"; bits[3] = rel; bits[4] = NULL;
    return oud_cat(out, size, bits);
}

/* The Part file's line for one file: copy "<rel>" "<dest>". 1, else 0. */
static int ouc_part_line(const char *name, const char *rel, char *out, int size)
{
    char dest[260];
    const char *bits[] = { "copy \"", rel, "\" \"", dest, "\"\n", NULL };
    if (!ouc_dest(name, rel, dest, sizeof dest)) return 0;
    return oud_cat(out, size, bits);
}

/* The old index's parts (oud_index_parts, from the URL tooltype's release) as releases, each address made from
 * base: GFOpenUpdate 0.2's source, kept for one release as the fallback. The count. */
static int ouc_from_index(const struct oud_part *parts, int n, const char *base, struct ouc_item *items, int max)
{
    int i, count = 0;
    for (i = 0; i < n && count < max; i++) {
        memset(&items[count], 0, sizeof items[count]);
        items[count].p = parts[i];
        if (!oud_url(base, parts[i].file, items[count].url, sizeof items[count].url)) continue;
        strcpy(items[count].desc, "From GFOpenUp's own update release.");
        count++;
    }
    return count;
}

#endif
