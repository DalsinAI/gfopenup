/* GFOpenApt's reading and weighing (Dale, 23 Nov 2026: "side quest , amiga version of apt"; "c: command";
 * "needs the repo list"; "github being hte source of most"). docs/design/GFOPENAPT.md.
 *
 * C:apt is a second face on GFOpenUpdate: the same index (src/openupdate.h), the same versions (openup_vers.h),
 * the same SHA-256, the same manifest of what is installed, and GFOpenUpTool to put a part in or take it out.
 * What this file adds is what a Shell command needs and the window didn't:
 *
 *   the repo list   S:apt.sources (Dale, 23 Nov 2026: the list lives in S:), and no other; a line each:
 *                     github <owner>/<repo> <tag>   the release <tag>, whose files include GFOpenUpdate.index
 *                     github <owner>/<repo>         the repository's newest release, the same file on it
 *                     index <address>               an index anywhere: GFOpenUpdate's form, or gfosdistribution's
 *                                                   software/index.txt (tabs: name, version, size, sha256,
 *                                                   address, description); an address ending "/" is a drawer
 *                                                   holding GFOpenUpdate.index
 *                     off <any of those>            listed, not read
 *                   ";" and "#" start a comment.
 *   the lists       each source's index as last fetched, ENVARC:GFOpenUpdate/Lists/<8 hex>.list, so list, search
 *                   and show work with the network down
 *   the commands    update, list [installed|upgradable], search, show, install, upgrade, remove, sources
 *   needs           a part's "needs part <name>" lines (its Part file, src/openup_lines.h) put in order: what is
 *                   needed goes in first, and parts that need each other in a ring are refused
 *
 * Plain C with no OS calls, shared with tests/test_openapt.c.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef OPENAPT_H
#define OPENAPT_H

#include <string.h>
#include "openupdate.h"

#define OUA_SOURCES "S:apt.sources"                     /* the repo list: apt reads this and nothing else */
#define OUA_LISTS OUD_DATA "/Lists"                     /* each source's index as last fetched */
#define OUA_KEPT "SYS:Storage/GFOpenApt"                /* <Name>/Part of each part apt put in, for remove */
#define OUA_PACK "DH1:GFOpenUp/Parts"                   /* the pack Cradle put there, GFOpenUpTool UPDATE's place */
#define OUA_WORK "T:GFOpenApt"                          /* downloads and unpacked parts, deleted after */
#define OUA_GITHUB "https://github.com/"

#define OUA_WORD 240                                    /* the longest word of a line: an address */
#define OUA_MAX_SOURCES 48
#define OUA_MAX_JOBS 32                                 /* parts in one install, with what they need */
#define OUA_MAX_EDGES 96

/* ---- small things ---- */

static char oua_lower(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }

/* 1 when a and b are the same but for the case of their letters, as AmigaDOS reads names. */
static int oua_same(const char *a, const char *b)
{
    for (; *a && *b; a++, b++) if (oua_lower(*a) != oua_lower(*b)) return 0;
    return !*a && !*b;
}

/* 1 when text holds word, whatever the case. An empty word is in everything. */
static int oua_holds(const char *text, const char *word)
{
    int n = (int)strlen(word), i;
    for (; *text; text++) {
        for (i = 0; i < n && text[i] && oua_lower(text[i]) == oua_lower(word[i]); i++) ;
        if (i == n) return 1;
    }
    return n == 0;
}

/* n (0 or more) as digits into out at k: the new length. */
static int oua_num(char *out, int size, int k, long n)
{
    char rev[12], t[12];
    int r = 0, j;
    if (n < 0) n = 0;
    do { rev[r++] = (char)('0' + n % 10); n /= 10; } while (n > 0 && r < 11);
    for (j = 0; j < r; j++) t[j] = rev[r - 1 - j];
    t[r] = 0;
    return oud_put(out, size, k, t);
}

/* ---- the command line: "apt <command> [names]" ---- */

enum { OUA_NONE, OUA_UPDATE, OUA_LIST, OUA_SEARCH, OUA_SHOW, OUA_INSTALL, OUA_UPGRADE, OUA_REMOVE, OUA_REPOS, OUA_HELP };
enum { OUA_ALL, OUA_INSTALLED, OUA_UPGRADABLE };

struct oua_args {
    int cmd;                                            /* OUA_UPDATE... */
    int which;                                          /* list: OUA_ALL, OUA_INSTALLED or OUA_UPGRADABLE */
    int count;                                          /* the names, or search's words */
    const char *const *names;
};

#define OUA_USAGE \
    "apt update                       reads every source in the repo list again\n" \
    "apt list [installed|upgradable]  what the sources have, what is here, what has a newer version\n" \
    "apt search <text>                packages with the text in their name or description\n" \
    "apt show <name>                  what the sources say about one\n" \
    "apt install <name>...            fetches, checks and installs, what they need first\n" \
    "apt upgrade [<name>...]          every installed part that has a newer version, or those named\n" \
    "apt remove <name>...             takes a part out, putting back what it replaced\n" \
    "apt sources                      the repo list (" OUA_SOURCES ")\n"

static int oua_command(const char *w)
{
    static const struct { const char *word; int cmd; } known[] = {
        { "update", OUA_UPDATE }, { "list", OUA_LIST }, { "search", OUA_SEARCH }, { "show", OUA_SHOW },
        { "install", OUA_INSTALL }, { "upgrade", OUA_UPGRADE }, { "remove", OUA_REMOVE },
        { "sources", OUA_REPOS }, { "help", OUA_HELP }, { "?", OUA_HELP } };
    unsigned i;
    for (i = 0; i < sizeof known / sizeof known[0]; i++)
        if (oua_same(w, known[i].word)) return known[i].cmd;
    return OUA_NONE;
}

/* The command word and what follows it into a: 1, or 0 with *why, a line for the user. Commands and list's word are
 * read in any case, as AmigaDOS reads its own. */
static int oua_parse(const char *command, const char *const *names, int count, struct oua_args *a, const char **why)
{
    a->cmd = command ? oua_command(command) : OUA_NONE;
    a->which = OUA_ALL;
    a->count = count;
    a->names = names;
    *why = NULL;
    switch (a->cmd) {
    case OUA_NONE:
        *why = command ? "apt doesn't know that command." : "apt needs a command.";
        return 0;
    case OUA_UPDATE: case OUA_REPOS: case OUA_HELP:
        if (count) { *why = "That command takes nothing after it."; return 0; }
        return 1;
    case OUA_LIST:
        if (count > 1) { *why = "list takes one word: installed or upgradable."; return 0; }
        if (count == 1) {
            if (oua_same(names[0], "installed")) a->which = OUA_INSTALLED;
            else if (oua_same(names[0], "upgradable") || oua_same(names[0], "upgradeable")) a->which = OUA_UPGRADABLE;
            else if (!oua_same(names[0], "all")) { *why = "list takes one word: installed or upgradable."; return 0; }
        }
        return 1;
    case OUA_SEARCH:
        if (!count) { *why = "search needs the text to look for."; return 0; }
        return 1;
    case OUA_SHOW: case OUA_INSTALL: case OUA_REMOVE:
        if (!count) { *why = "That command needs a package's name."; return 0; }
        return 1;
    }
    return 1;                                           /* upgrade: with names or without */
}

/* ---- the repo list ---- */

enum { OUA_KIND_GITHUB = 1, OUA_KIND_INDEX };

struct oua_source {
    int kind;
    int off;                                            /* listed, not read */
    char label[OUA_WORD];                               /* the line as the user reads it, without "off" */
    char url[OUA_WORD];                                 /* its index's address */
    char base[OUA_WORD];                                /* where a plain file name of that index is */
};

/* The line at p cut into up to max words of under OUA_WORD letters: the count, or -1 for a longer word or more
 * words. *next is the line after. */
static int oua_split(const char *p, const char *end, char w[][OUA_WORD], int max, const char **next)
{
    int n = 0, bad = 0;
    while (p < end && *p != '\n') {
        int k = 0;
        while (p < end && oud_blank(*p)) p++;
        if (p >= end || *p == '\n') break;
        if (n == max) { bad = 1; while (p < end && *p != '\n') p++; break; }
        while (p < end && *p != '\n' && !oud_blank(*p)) {
            if (k < OUA_WORD - 1) w[n][k++] = *p; else bad = 1;
            p++;
        }
        w[n++][k] = 0;
    }
    if (p < end) p++;
    *next = p;
    return bad ? -1 : n;
}

/* An address fit to fetch and to go on a command line: http:// or https://, a host, a "/" after it, and no blank,
 * quote, star or letter outside ASCII. */
static int oua_address(const char *s)
{
    const char *p;
    int slash = 0;
    if (!strncmp(s, "https://", 8)) p = s + 8;
    else if (!strncmp(s, "http://", 7)) p = s + 7;
    else return 0;
    if (!*p || *p == '/') return 0;
    for (; *p; p++) {
        if ((unsigned char)*p < 33 || (unsigned char)*p > 126 || *p == '"' || *p == '*') return 0;
        if (*p == '/') slash = 1;
    }
    return slash;
}

/* "<owner>/<repo>" as GitHub writes them: letters, digits, ".", "_" and "-", one "/" between two names. */
static int oua_repo(const char *s)
{
    int slashes = 0, len = 0;
    if (*s == '.' || *s == '/') return 0;
    for (; *s; s++) {
        char c = *s;
        if (c == '/') {
            if (!len || slashes || !s[1] || s[1] == '.') return 0;
            slashes++;
            len = 0;
            continue;
        }
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || ouv_digit(c) || c == '.' || c == '_' || c == '-')) return 0;
        len++;
    }
    return slashes == 1 && len > 0;
}

/* One line's words as a source: 1, or 0 for a line that isn't one. */
static int oua_source_words(char w[][OUA_WORD], int n, struct oua_source *s)
{
    int i = 0, k;
    s->off = 0;
    if (n > 0 && oua_same(w[0], "off")) { s->off = 1; i = 1; }
    if (n - i < 2) return 0;
    if (oua_same(w[i], "github")) {
        const char *tag = n - i == 3 ? w[i + 2] : NULL;
        const char *bits[8];
        if (n - i > 3 || !oua_repo(w[i + 1]) || (tag && !oud_plain_name(tag))) return 0;
        s->kind = OUA_KIND_GITHUB;
        k = 0;
        bits[k++] = OUA_GITHUB; bits[k++] = w[i + 1];
        if (tag) { bits[k++] = "/releases/download/"; bits[k++] = tag; bits[k++] = "/"; }
        else bits[k++] = "/releases/latest/download/";
        bits[k] = NULL;
        if (!oud_cat(s->base, sizeof s->base, bits)) return 0;
        if (!oud_url(s->base, OUD_INDEX_NAME, s->url, sizeof s->url)) return 0;
        k = oud_put(s->label, sizeof s->label, 0, "github ");
        k = oud_put(s->label, sizeof s->label, k, w[i + 1]);
        if (tag) { k = oud_put(s->label, sizeof s->label, k, " "); oud_put(s->label, sizeof s->label, k, tag); }
        return 1;
    }
    if (oua_same(w[i], "index")) {
        const char *u = w[i + 1];
        if (n - i != 2 || !oua_address(u)) return 0;
        s->kind = OUA_KIND_INDEX;
        if (u[strlen(u) - 1] == '/') {                  /* a drawer: GFOpenUpdate.index in it */
            strcpy(s->base, u);
            if (!oud_url(u, OUD_INDEX_NAME, s->url, sizeof s->url)) return 0;
        } else {
            k = (int)(strrchr(u, '/') - u) + 1;         /* oua_address: there is one after the host */
            strcpy(s->url, u);
            memcpy(s->base, u, k);
            s->base[k] = 0;
        }
        k = oud_put(s->label, sizeof s->label, 0, "index ");
        oud_put(s->label, sizeof s->label, k, u);
        return 1;
    }
    return 0;
}

/* The repo list's sources into out (max): the count, or -1 with *bad the number of the line that is wrong (a word
 * it doesn't know, an address that isn't one, the same index twice, more sources than the table holds). A list
 * with a wrong line is not used at all: half a repo list would quietly drop a source. */
static int oua_sources(const char *text, long len, struct oua_source *out, int max, int *bad)
{
    const char *end = text + len, *p = text, *next;
    static char w[5][OUA_WORD];
    int count = 0, line = 0, i;
    while (p < end) {
        const char *q = p;
        int n;
        line++;
        while (q < end && oud_blank(*q)) q++;
        if (q < end && (*q == ';' || *q == '#')) {      /* a comment however long */
            while (p < end && *p != '\n') p++;
            if (p < end) p++;
            continue;
        }
        n = oua_split(p, end, w, 5, &next);
        p = next;
        if (n == 0) continue;
        *bad = line;
        if (n < 0 || count == max || !oua_source_words(w, n, &out[count])) return -1;
        for (i = 0; i < count; i++)
            if (!strcmp(out[i].url, out[count].url)) return -1;
        count++;
    }
    *bad = 0;
    return count;
}

/* The name of a source's saved list in OUA_LISTS: 8 hex digits made from its index's address, then ".list". The
 * same source keeps its list wherever it stands in the repo list. out holds 14 letters. */
static void oua_list_name(const struct oua_source *s, char *out)
{
    unsigned long h = 2166136261UL;
    const char *p;
    int i;
    for (p = s->url; *p; p++) h = ((h ^ (unsigned char)*p) * 16777619UL) & 0xFFFFFFFFUL;
    for (i = 0; i < 8; i++) out[i] = "0123456789abcdef"[(h >> (28 - 4 * i)) & 15];
    strcpy(out + 8, ".list");
}

/* The host an address names and its port (80 or 443, or the one written): 1, else 0. For "is it there?". */
static int oua_host(const char *url, char *host, int size, int *port)
{
    const char *p;
    int n = 0;
    if (!strncmp(url, "https://", 8)) { p = url + 8; *port = 443; }
    else if (!strncmp(url, "http://", 7)) { p = url + 7; *port = 80; }
    else return 0;
    for (; *p && *p != '/' && *p != ':'; p++) {
        if (n >= size - 1) return 0;
        host[n++] = *p;
    }
    host[n] = 0;
    if (*p == ':') {
        long v = 0;
        int digits = 0;
        for (p++; ouv_digit(*p) && digits < 5; p++, digits++) v = v * 10 + (*p - '0');
        if (!digits || (*p && *p != '/') || v < 1 || v > 65535) return 0;
        *port = (int)v;
    }
    return n > 0;
}

/* ---- the packages a source lists ---- */

struct oua_pkg {
    struct oud_part p;                                  /* name, version, bytes, sha256, file: as GFOpenUpdate's */
    char url[OUA_WORD];                                 /* the archive's address */
    char desc[100];                                     /* what it is, when the source says */
    int source;                                         /* which line of the repo list it came from, from 0 */
};

/* One line of gfosdistribution's software/index.txt (to its end, no newline) into pkg: 1, or 0 for a wrong one.
 * Tabs between: name, version, size, sha256, address, description. The address is whole (the files are on a
 * release, not beside the index); its last name is the file kept while it is checked. */
static int oua_software_line(const char *p, const char *end, struct oua_pkg *pkg)
{
    char f[6][OUA_WORD];
    int n = 0, k = 0, i;
    const char *name;
    for (; p < end; p++) {
        if (*p == '\r') continue;
        if (*p == '\t' && n < 5) { f[n++][k] = 0; k = 0; continue; }
        if (k >= OUA_WORD - 1) { if (n < 5) return 0; continue; }     /* a long description is cut, a long field is wrong */
        f[n][k++] = (unsigned char)*p < 32 ? ' ' : *p;
    }
    f[n++][k] = 0;
    if (n < 5) return 0;
    if (!oud_plain_name(f[0]) || strlen(f[0]) >= sizeof pkg->p.name) return 0;
    if (!ouv_digit(f[1][0]) || !oud_plain_name(f[1]) || strlen(f[1]) >= sizeof pkg->p.version) return 0;
    if (!oud_number(f[2], &pkg->p.bytes) || pkg->p.bytes <= 0) return 0;
    if (!oud_hex64(f[3]) || !oua_address(f[4])) return 0;
    name = strrchr(f[4], '/') + 1;
    if (!oud_plain_name(name) || strlen(name) >= sizeof pkg->p.file) return 0;
    strcpy(pkg->p.name, f[0]);
    strcpy(pkg->p.version, f[1]);
    strcpy(pkg->p.sha256, f[3]);
    strcpy(pkg->p.file, name);
    pkg->p.installed[0] = 0;
    strcpy(pkg->url, f[4]);
    pkg->desc[0] = 0;
    if (n == 6) {
        for (i = 0; f[5][i] && i < (int)sizeof pkg->desc - 1; i++) pkg->desc[i] = f[5][i];
        pkg->desc[i] = 0;
    }
    return 1;
}

/* A fetched index's packages into out (max), for the source src (the srcno-th): the count, or -1 when it isn't a
 * whole, right index, and then none of it is used. Two forms are read, told apart by the first word:
 *   GFOpenUpdate's ("GFOpenUpdate 1" ... "GFOpenUpdate end", oud_index_parts): files beside the index
 *   gfosdistribution's software/index.txt: a line each, tabs between, "#" comments; it must hold a line of some
 *   kind, so an empty answer isn't taken for an empty catalogue
 * scratch is room for max of GFOpenUpdate's parts. */
static int oua_read_list(const char *text, long len, const struct oua_source *src, int srcno,
                         struct oud_part *scratch, struct oua_pkg *out, int max)
{
    const char *end = text + len, *p = text;
    int count = 0, i, lines = 0;
    while (p < end && (oud_blank(*p) || *p == '\n')) p++;
    if (end - p >= 12 && !strncmp(p, "GFOpenUpdate", 12)) {
        count = oud_index_parts(text, len, scratch, max);
        for (i = 0; i < count; i++) {
            out[i].p = scratch[i];
            out[i].desc[0] = 0;
            out[i].source = srcno;
            if (!oud_url(src->base, scratch[i].file, out[i].url, sizeof out[i].url)) return -1;
        }
        return count;
    }
    while (p < end) {
        const char *q = p, *e;
        while (p < end && *p != '\n') p++;
        e = p;
        if (p < end) p++;
        while (q < e && oud_blank(*q)) q++;
        if (q == e) continue;
        lines++;
        if (*q == '#' || *q == ';') continue;
        if (count == max || !oua_software_line(q, e, &out[count])) return -1;
        for (i = 0; i < count; i++)
            if (!strcmp(out[i].p.name, out[count].p.name)) return -1;
        out[count++].source = srcno;
    }
    return lines ? count : -1;
}

/* m more packages (add) into the catalogue of n (room for max): the new count, or -1 when it is full. A name two
 * sources have is listed once, at its newer version; at the same version the earlier source in the repo list
 * keeps it. */
static int oua_merge(struct oua_pkg *cat, int n, int max, const struct oua_pkg *add, int m)
{
    int i, j;
    for (i = 0; i < m; i++) {
        for (j = 0; j < n; j++)
            if (!strcmp(cat[j].p.name, add[i].p.name)) break;
        if (j < n) {
            if (ouv_cmp(add[i].p.version, cat[j].p.version) > 0) cat[j] = add[i];
            continue;
        }
        if (n == max) return -1;
        cat[n++] = add[i];
    }
    return n;
}

/* The package named: by its exact name, else by the name in any case (apt install gfopenplay). Its place, or -1. */
static int oua_find(const struct oua_pkg *cat, int n, const char *name)
{
    int i;
    for (i = 0; i < n; i++) if (!strcmp(cat[i].p.name, name)) return i;
    for (i = 0; i < n; i++) if (oua_same(cat[i].p.name, name)) return i;
    return -1;
}

/* ---- a package against what is installed ---- */

enum { OUA_OLDER = -1, OUA_SAME, OUA_NEW, OUA_NEWER };

/* How the package stands against the manifest: OUA_NEW (not installed), OUA_NEWER (installed, the source's is
 * newer), OUA_SAME, or OUA_OLDER (what is installed is newer than the source's). have gets the installed version,
 * "" when there is none. */
static int oua_weigh(const struct oua_pkg *pkg, const char *manifest, long mlen, char *have, int size)
{
    int c;
    have[0] = 0;
    if (!oud_installed(manifest, mlen, pkg->p.name, have, size)) { have[0] = 0; return OUA_NEW; }
    c = ouv_cmp(pkg->p.version, have);
    return c > 0 ? OUA_NEWER : c < 0 ? OUA_OLDER : OUA_SAME;
}

/* The package as a row of the list, in the words GFOpenUpdate's window uses (oud_row_text). */
static void oua_row(const struct oua_pkg *pkg, const char *manifest, long mlen, struct oud_row *row)
{
    char have[24];
    int how = oua_weigh(pkg, manifest, mlen, have, sizeof have);
    strcpy(row->name, pkg->p.name);
    strcpy(row->installed, have);
    row->newer[0] = 0;
    if (how == OUA_NEW || how == OUA_NEWER) strcpy(row->newer, pkg->p.version);
}

/* The catalogue's packages that are installed and older, as oud_offers finds them, into parts (room for n): how
 * many. No version is passed over: the user asked. */
static int oua_offers(const struct oua_pkg *cat, int n, const char *manifest, long mlen, struct oud_part *parts)
{
    int i;
    for (i = 0; i < n; i++) parts[i] = cat[i].p;
    return manifest ? oud_offers(parts, n, manifest, mlen, NULL, 0) : 0;
}

/* 1 when every one of the words is in the package's name or description, in any case. */
static int oua_match(const struct oua_pkg *pkg, const char *const *words, int count)
{
    int i;
    for (i = 0; i < count; i++)
        if (!oua_holds(pkg->p.name, words[i]) && !oua_holds(pkg->desc, words[i])) return 0;
    return 1;
}

/* What show prints for one package: have is its installed version ("" none), from the source's label. The length. */
static int oua_show_text(const struct oua_pkg *pkg, const char *have, const char *from, char *out, int size)
{
    int n = 0;
    out[0] = 0;
    n = oud_put(out, size, n, "Name: ");        n = oud_put(out, size, n, pkg->p.name);
    n = oud_put(out, size, n, "\nVersion: ");   n = oud_put(out, size, n, pkg->p.version);
    n = oud_put(out, size, n, "\nInstalled: "); n = oud_put(out, size, n, *have ? have : "no");
    n = oud_put(out, size, n, "\nSize: ");      n = oua_num(out, size, n, pkg->p.bytes);
    n = oud_put(out, size, n, " bytes\nSHA-256: "); n = oud_put(out, size, n, pkg->p.sha256);
    n = oud_put(out, size, n, "\nFrom: ");      n = oud_put(out, size, n, from);
    n = oud_put(out, size, n, "\nAddress: ");   n = oud_put(out, size, n, pkg->url);
    if (pkg->desc[0]) { n = oud_put(out, size, n, "\nAbout: "); n = oud_put(out, size, n, pkg->desc); }
    return oud_put(out, size, n, "\n");
}

/* ---- what needs what: the order parts go in ---- */

static int oua_visit(int i, const short (*edge)[2], int edges, char *mark, short *order, int *k)
{
    int j;
    if (mark[i] == 2) return 1;
    if (mark[i] == 1) return 0;                         /* back where we started: a ring */
    mark[i] = 1;
    for (j = 0; j < edges; j++)
        if (edge[j][0] == i && !oua_visit(edge[j][1], edge, edges, mark, order, k)) return 0;
    mark[i] = 2;
    order[(*k)++] = (short)i;
    return 1;
}

/* n parts (no more than OUA_MAX_JOBS) and what each needs (edge[j][0] needs edge[j][1]) into order: what is needed
 * before what needs it, otherwise as they were asked for. 1, or 0 when parts need each other in a ring; then
 * *ring is one of them and nothing should be installed. */
static int oua_order(int n, const short (*edge)[2], int edges, short *order, int *ring)
{
    char mark[OUA_MAX_JOBS];
    int i, k = 0;
    if (n > OUA_MAX_JOBS) { *ring = 0; return 0; }
    memset(mark, 0, sizeof mark);
    for (i = 0; i < n; i++)
        if (!oua_visit(i, edge, edges, mark, order, &k)) { *ring = i; return 0; }
    return 1;
}

/* ---- remove ---- */

/* The name and version on a Part file's first line that is one ("part <Name> <version>"): 1, else 0. */
static int oua_part_head(const char *text, long len, char *name, int nsize, char *ver, int vsize)
{
    const char *end = text + len, *p = text, *next;
    char w[4][80];
    while (p < end) {
        int n = oud_words(p, end, w, 4, &next);
        p = next;
        if (n >= 3 && !strcmp(w[0], "part") && (int)strlen(w[1]) < nsize && (int)strlen(w[2]) < vsize) {
            strcpy(name, w[1]);
            strcpy(ver, w[2]);
            return 1;
        }
    }
    return 0;
}

/* Whether apt may ask GFOpenUpTool to take the part out: 1, or 0 with the reason in why. Not GFOpenUp itself or
 * the Kickstart set (they have their own way out), not a part that isn't installed, and not one an installed part
 * needs (the manifest's "needpart <part> <needs>" lines, as GFOpenUpTool REMOVE reads them). */
static int oua_can_remove(const char *name, const char *manifest, long mlen, char *why, int size)
{
    const char *end = manifest ? manifest + mlen : NULL, *p = manifest, *next;
    char w[4][80], have[24];
    int n;
    why[0] = 0;
    if (!oud_installed(manifest, mlen, name, have, sizeof have)) {
        n = oud_put(why, size, 0, name);
        oud_put(why, size, n, " isn't installed.");
        return 0;
    }
    if (!oud_listed_as_new(name)) {
        n = oud_put(why, size, 0, name);
        oud_put(why, size, n, " is GFOpenUp's own: it goes with Uninstall-GFOpenUp, not apt.");
        return 0;
    }
    while (p && p < end) {
        int words = oud_words(p, end, w, 4, &next);
        p = next;
        if (words == 3 && !strcmp(w[0], "needpart") && !strcmp(w[2], name) && strcmp(w[1], name)
            && oud_installed(manifest, mlen, w[1], have, sizeof have)) {
            n = oud_put(why, size, 0, name);
            n = oud_put(why, size, n, " stays: ");
            n = oud_put(why, size, n, w[1]);
            n = oud_put(why, size, n, " needs it. Remove ");
            n = oud_put(why, size, n, w[1]);
            oud_put(why, size, n, " first.");
            return 0;
        }
    }
    return 1;
}

/* GFOpenUpTool taking out the part whose Part file is in drawer. 1, or 0 when it won't fit or can't be quoted. */
static int oua_remove_cmd(const char *drawer, char *out, int size)
{
    const char *bits[] = { OUD_TOOL " REMOVE \"", drawer, "\"", NULL };
    if (!oud_quotable(drawer)) return 0;
    return oud_cat(out, size, bits);
}

/* ---- how a run ended ---- */

/* apt's return code, AmigaDOS's: 0 all done, 5 (WARN) something was left as it was and said so, 10 (ERROR) what
 * was asked couldn't be done, 20 (FAIL) apt itself couldn't run. The worst of a and b. */
static int oua_worst(int a, int b) { return a > b ? a : b; }

#endif
