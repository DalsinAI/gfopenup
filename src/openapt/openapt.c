/* GFOpenApt 0.1: C:apt, GFOS's parts from the Shell (docs/design/GFOPENAPT.md).
 *
 * Dale, 23 November 2026: "side quest , amiga version of apt"; "c: command"; "noetwortk up"; "needs the repo
 * list"; "github being hte source of most".
 *
 * A second face on GFOpenUpdate, not a second updater: the index is GFOpenUpdate's (src/openupdate.h), what is
 * installed is GFOpenUpTool's manifest, an archive is fetched with GFOpenSocket's httpget, weighed by size and
 * SHA-256, unpacked with GFOpenCompress and put in with GFOpenUpTool INSTALL, all through the code the window
 * uses (src/openupdate_amiga.h). A part comes out with GFOpenUpTool REMOVE.
 *
 *   apt update                       reads every source in the repo list again and keeps each one's index
 *   apt list [installed|upgradable]  from the kept indexes and the manifest; no network needed
 *   apt search <text>                the same, by name and description
 *   apt show <name>
 *   apt install <name>...            what a part needs ("needs part" in its Part file) goes in first
 *   apt upgrade [<name>...]          every installed part with a newer version, or those named
 *   apt remove <name>...             only a part whose Part file is at hand: one apt put in, or the pack's
 *   apt sources                      the repo list
 *
 * The repo list is S:apt.sources and nothing else (src/openapt.h has its lines; without it apt says so and
 * returns 10); the kept indexes are in ENVARC:GFOpenUpdate/Lists. Before anything is fetched apt asks whether
 * the network is up and the source's host answers, and says so when not. Return codes: 0 done, 5 something
 * was left as it was, 10 it couldn't be done, 20 apt couldn't run.
 *
 * MIT, Copyright (c) 2026 AmiCradle Team. */

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/rdargs.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/bsdsocket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>

/* the shared headers carry helpers only the window, the host tests and GFOpenUpTool use */
#pragma GCC diagnostic ignored "-Wunused-function"
#include "../openupdate.h"
#include "../openupdate_sha256.h"
#include "../openupdate_amiga.h"
#include "../openup_lines.h"
#include "../openapt.h"

#define NAME "GFOpenApt"
#define VERSION_TEXT "0.1"
unsigned long __stack = 65536;
static const char version[] __attribute__((used)) = "$VER: " NAME " " VERSION_TEXT " (23.11.2026)";

struct Library *SocketBase;

#define MANIFEST "ENVARC:GFOpenAmiga/GFOpenUp.manifest"
#define MAX_PKGS 256
#define MAX_TEXT (4L * 1024 * 1024)
#define REACH_SECONDS 8

static struct oua_source *sources;
static int nsources;
static struct oua_pkg *cat, *fresh;            /* the catalogue, and one source's packages as they are read */
static struct oud_part *scratch;
static int ncat;
static char *manifest;
static long mlen;

/* ---- small things ---------------------------------------------------------------------------------------- */

static void say(const char *a, const char *b, const char *c)
{
    if (a) PutStr((STRPTR)a);
    if (b) PutStr((STRPTR)b);
    if (c) PutStr((STRPTR)c);
    PutStr((STRPTR)"\n");
}

static void say_num(const char *a, long n, const char *b)
{
    char t[16];
    t[0] = 0;
    oua_num(t, sizeof t, 0, n);
    PutStr((STRPTR)a);
    PutStr((STRPTR)t);
    PutStr((STRPTR)b);
}

static int stopped(void)                       /* Ctrl-C */
{
    if (!(SetSignal(0, SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C)) return 0;
    PutStr((STRPTR)"apt: ***Break\n");
    return 1;
}

static void make_drawer(const char *path)
{
    BPTR l = CreateDir((STRPTR)path);
    if (l) UnLock(l);
}

static int write_file(const char *path, const char *text, long len)
{
    BPTR fh = Open((STRPTR)path, MODE_NEWFILE);
    if (!fh) return 0;
    if (Write(fh, (APTR)text, len) != len) { Close(fh); DeleteFile((STRPTR)path); return 0; }
    return Close(fh) ? 1 : 0;
}

static void load_manifest(void)
{
    if (manifest) FreeVec(manifest);
    manifest = oud_read_file(MANIFEST, &mlen, MAX_TEXT);
}

/* ---- is the network up, and is the source's host there? -------------------------------------------------- */

/* 1 when host answers on port within REACH_SECONDS. Else 0, and the user is told which it was: no TCP/IP stack,
 * a name that can't be looked up, or a host that doesn't answer. A host found once isn't asked again this run. */
static int reach(const char *host, int port)
{
    static char known[100];
    struct hostent *h;
    struct sockaddr_in sa;
    struct timeval tv;
    fd_set wr;
    LONG s, on = 1, err = 0, len = sizeof err;
    int ok = 0;

    if (known[0] && !strcmp(known, host)) return 1;
    if (!(SocketBase = OpenLibrary((STRPTR)"bsdsocket.library", 4))) {
        say("apt: the network isn't up (no TCP/IP stack is running), so nothing can be fetched.", NULL, NULL);
        return 0;
    }
    if (!(h = gethostbyname((STRPTR)host)) || !h->h_addr_list || !h->h_addr_list[0]) {
        say("apt: the network isn't up, or has no name server: ", host, " can't be looked up.");
    } else if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        say("apt: the TCP/IP stack gave no socket.", NULL, NULL);
    } else {
        memset(&sa, 0, sizeof sa);
        sa.sin_len = sizeof sa;
        sa.sin_family = AF_INET;
        sa.sin_port = htons((unsigned short)port);
        memcpy(&sa.sin_addr, h->h_addr_list[0], sizeof sa.sin_addr);
        IoctlSocket(s, FIONBIO, (char *)&on);           /* so a host that is down costs seconds, not minutes */
        if (connect(s, (struct sockaddr *)&sa, sizeof sa) == 0) ok = 1;
        else {
            FD_ZERO(&wr);
            FD_SET(s, &wr);
            tv.tv_sec = REACH_SECONDS;
            tv.tv_usec = 0;
            if (WaitSelect(s + 1, NULL, &wr, NULL, &tv, NULL) > 0
                && getsockopt(s, SOL_SOCKET, SO_ERROR, &err, &len) == 0 && err == 0) ok = 1;
        }
        CloseSocket(s);
        if (!ok) say("apt: can't reach ", host, ". Is the network up?");
    }
    CloseLibrary(SocketBase);
    SocketBase = NULL;
    if (ok && strlen(host) < sizeof known) strcpy(known, host);
    return ok;
}

static int reach_url(const char *url)
{
    char host[100];
    int port;
    if (!oua_host(url, host, sizeof host, &port)) { say("apt: ", url, " isn't an address apt can fetch."); return 0; }
    return reach(host, port);
}

/* ---- the repo list, and the kept indexes ----------------------------------------------------------------- */

/* The repo list read: RETURN_OK, or RETURN_ERROR with the wrong line said. With no file apt says so and stops: it
 * doesn't make a list up. */
static int load_sources(void)
{
    long len;
    int bad = 0;
    char *text = oud_read_file(OUA_SOURCES, &len, 65536);
    nsources = 0;
    if (!text) {
        say("apt: the repo list " OUA_SOURCES " is missing or can't be read, so apt has no sources.", NULL, NULL);
        return RETURN_ERROR;
    }
    nsources = oua_sources(text, len, sources, OUA_MAX_SOURCES, &bad);
    FreeVec(text);
    if (nsources < 0) {
        say_num("apt: line ", bad, " of " OUA_SOURCES " isn't a source apt can read, so the list isn't used.\n");
        say("     A line is: github <owner>/<repo> [<tag>], or index <address>, with \"off\" in front to pass it over.", NULL, NULL);
        nsources = 0;
        return RETURN_ERROR;
    }
    return RETURN_OK;
}

static void list_path(const struct oua_source *s, char *out)
{
    strcpy(out, OUA_LISTS "/");
    oua_list_name(s, out + strlen(out));
}

/* The catalogue from the kept indexes: how many sources had one. A kept index that no longer reads is passed over
 * and said. */
static int load_catalogue(void)
{
    int i, read = 0;
    ncat = 0;
    for (i = 0; i < nsources; i++) {
        char path[80], *text;
        long len;
        int n;
        if (sources[i].off) continue;
        list_path(&sources[i], path);
        if (!(text = oud_read_file(path, &len, MAX_TEXT))) continue;
        n = oua_read_list(text, len, &sources[i], i, scratch, fresh, MAX_PKGS);
        FreeVec(text);
        if (n < 0 || (n = oua_merge(cat, ncat, MAX_PKGS, fresh, n)) < 0) {
            say("apt: the kept index of ", sources[i].label, " can't be read; apt update fetches it again.");
            continue;
        }
        ncat = n;
        read++;
    }
    return read;
}

/* The catalogue for a command that only reads: RETURN_OK, or RETURN_WARN when no source has been read yet. */
static int need_catalogue(void)
{
    if (load_catalogue()) return RETURN_OK;
    say("apt: no source has been read yet. Run apt update first (it needs the network).", NULL, NULL);
    return RETURN_WARN;
}

/* ---- update ---------------------------------------------------------------------------------------------- */

static int cmd_update(void)
{
    int i, good = 0, tried = 0, n, offers;
    make_drawer(OUD_DATA);
    make_drawer(OUA_LISTS);
    oud_delete_all(OUA_WORK);
    make_drawer(OUA_WORK);
    for (i = 0; i < nsources; i++) {
        const struct oua_source *s = &sources[i];
        char path[80], *text = NULL;
        long len = 0;
        const char *why;
        if (s->off) continue;
        if (stopped()) { oud_delete_all(OUA_WORK); return RETURN_ERROR; }
        tried++;
        if (!reach_url(s->url)) {
            if (tried == 1) { oud_delete_all(OUA_WORK); return RETURN_ERROR; }   /* the first: no network at all, most likely */
            continue;
        }
        why = oud_fetch_checked(s->url, OUA_WORK "/index", 0, NULL);
        if (!why && !(text = oud_read_file(OUA_WORK "/index", &len, MAX_TEXT))) why = "it couldn't be fetched";
        n = why ? -1 : oua_read_list(text, len, s, i, scratch, fresh, MAX_PKGS);
        if (n < 0) {
            PutStr((STRPTR)"Not read  ");
            say(s->label, why ? ": " : ": what came isn't a whole index", why ? why : " (it publishes none yet?)");
        } else {
            list_path(s, path);
            if (!write_file(path, text, len)) {
                PutStr((STRPTR)"Not kept  ");
                say(s->label, ": " OUA_LISTS " can't be written. Is SYS: full or write-protected?", NULL);
            } else {
                PutStr((STRPTR)"Read      ");
                PutStr((STRPTR)s->label);
                say_num(": ", n, n == 1 ? " package\n" : " packages\n");
                good++;
            }
        }
        if (text) FreeVec(text);
        DeleteFile((STRPTR)OUA_WORK "/index");
    }
    oud_delete_all(OUA_WORK);
    if (!tried) { say("apt: the repo list (" OUA_SOURCES ") has no source switched on.", NULL, NULL); return RETURN_WARN; }
    load_catalogue();
    load_manifest();
    offers = oua_offers(cat, ncat, manifest, mlen, scratch);
    say_num("", ncat, ncat == 1 ? " package in all; " : " packages in all; ");
    say_num("", offers, offers == 1 ? " installed part has a newer version" : " installed parts have a newer version");
    say(offers ? " (apt list upgradable, apt upgrade)." : ".", NULL, NULL);
    return !good ? RETURN_ERROR : good < tried ? RETURN_WARN : RETURN_OK;
}

/* ---- list, search, show, sources ------------------------------------------------------------------------- */

static int cmd_list(int which)
{
    static struct oud_row rows[MAX_PKGS];
    char t[120];
    int i, n, offers, rc;
    load_manifest();
    if (which == OUA_INSTALLED) {               /* the manifest's parts: no index is needed, one only adds "new:" */
        if (!manifest) { say("apt: GFOpenUp isn't installed here (no " MANIFEST "), so no part is.", NULL, NULL); return RETURN_WARN; }
        load_catalogue();
        offers = oua_offers(cat, ncat, manifest, mlen, scratch);
        n = oud_rows(manifest, mlen, scratch, offers, rows, MAX_PKGS);
    } else {
        if ((rc = need_catalogue())) return rc;
        if (which == OUA_UPGRADABLE) {          /* the rows GFOpenUpdate's window shows first */
            offers = oua_offers(cat, ncat, manifest, mlen, scratch);
            n = oud_rows(NULL, 0, scratch, offers, rows, MAX_PKGS);
            if (!n) say("Every installed part is the newest version the sources have.", NULL, NULL);
        } else {
            for (n = 0; n < ncat; n++) oua_row(&cat[n], manifest, mlen, &rows[n]);
        }
    }
    for (i = 0; i < n; i++) {
        oud_row_text(&rows[i], t, sizeof t);
        say(t, NULL, NULL);
    }
    return RETURN_OK;
}

static int cmd_search(const struct oua_args *a)
{
    struct oud_row row;
    char t[120];
    int i, found = 0, rc = need_catalogue();
    if (rc) return rc;
    load_manifest();
    for (i = 0; i < ncat; i++) {
        if (!oua_match(&cat[i], a->names, a->count)) continue;
        oua_row(&cat[i], manifest, mlen, &row);
        oud_row_text(&row, t, sizeof t);
        say(t, cat[i].desc[0] ? "  " : NULL, cat[i].desc[0] ? cat[i].desc : NULL);
        found++;
    }
    if (!found) { say("No package has that in its name or description.", NULL, NULL); return RETURN_WARN; }
    return RETURN_OK;
}

static int cmd_show(const struct oua_args *a)
{
    static char t[1200];
    char have[24];
    int i, k, rc = need_catalogue();
    if (rc) return rc;
    load_manifest();
    for (i = 0; i < a->count; i++) {
        if ((k = oua_find(cat, ncat, a->names[i])) < 0) {
            say("apt: no source has a package named ", a->names[i], ".");
            rc = RETURN_WARN;
            continue;
        }
        oua_weigh(&cat[k], manifest, mlen, have, sizeof have);
        oua_show_text(&cat[k], have, sources[cat[k].source].label, t, sizeof t);
        if (i) PutStr((STRPTR)"\n");
        PutStr((STRPTR)t);
    }
    return rc;
}

static int cmd_sources(void)
{
    int i;
    for (i = 0; i < nsources; i++) {
        char path[80];
        BPTR l;
        list_path(&sources[i], path);
        if ((l = Lock((STRPTR)path, ACCESS_READ))) UnLock(l);
        PutStr((STRPTR)(sources[i].off ? "off       " : l ? "read      " : "not read  "));
        say(sources[i].label, NULL, NULL);
    }
    if (!nsources) say("The repo list (" OUA_SOURCES ") is empty.", NULL, NULL);
    return RETURN_OK;
}

/* ---- install and upgrade --------------------------------------------------------------------------------- */

static struct job {
    int pkg;                                   /* its place in cat */
    char where[40];                            /* the drawer it is unpacked in */
    char drawer[80];                           /* the part's own drawer there */
} jobs[OUA_MAX_JOBS];
static short edges[OUA_MAX_EDGES][2];
static int njobs, nedges;

static int job_of(int pkg)
{
    int i;
    for (i = 0; i < njobs; i++) if (jobs[i].pkg == pkg) return i;
    return -1;
}

static int add_job(int pkg)
{
    if (njobs == OUA_MAX_JOBS) return -1;
    jobs[njobs].pkg = pkg;
    jobs[njobs].where[0] = jobs[njobs].drawer[0] = 0;
    return njobs++;
}

/* The part's Part file kept as OUA_KEPT/<Name>/Part, so apt remove has the lines of the version that is installed. */
static void keep_part(const struct job *j)
{
    char from[200], to[120], *text;
    long len;
    strcpy(from, j->where); strcat(from, "/"); strcat(from, j->drawer); strcat(from, "/Part");
    if (!(text = oud_read_file(from, &len, MAX_TEXT))) return;
    make_drawer(OUA_KEPT);
    strcpy(to, OUA_KEPT "/"); strcat(to, cat[j->pkg].p.name);
    make_drawer(to);
    strcat(to, "/Part");
    if (!write_file(to, text, len))
        say("apt: couldn't keep ", to, ", so apt remove won't know this part. Is SYS: full or write-protected?");
    FreeVec(text);
}

/* Job i fetched, weighed and unpacked, and the parts it needs that aren't installed added as jobs: RETURN_OK, or
 * RETURN_ERROR with the reason said. Nothing is installed here. */
static int prepare(int i)
{
    const struct oua_pkg *p = &cat[jobs[i].pkg];
    char path[160], part[200], need[64], *text;
    const char *why;
    long len;
    int k, at, to;

    if (!reach_url(p->url)) return RETURN_ERROR;
    PutStr((STRPTR)"Fetching ");
    PutStr((STRPTR)p->p.name);
    PutStr((STRPTR)" ");
    PutStr((STRPTR)p->p.version);
    say_num(" (", p->p.bytes, " bytes)...\n");
    strcpy(path, OUA_WORK "/");
    strcat(path, p->p.file);
    strcpy(jobs[i].where, OUA_WORK "/x");
    oua_num(jobs[i].where, sizeof jobs[i].where, (int)strlen(jobs[i].where), i);
    why = oud_fetch_checked(p->url, path, p->p.bytes, p->p.sha256);
    if (!why) why = oud_unpack_part(path, jobs[i].where, jobs[i].drawer, sizeof jobs[i].drawer);
    DeleteFile((STRPTR)path);                  /* the archive: right or wrong, it isn't kept */
    if (why) {
        say("apt: ", p->p.name, " wasn't installed:");
        say("     ", why, ". Nothing was changed.");
        return RETURN_ERROR;
    }
    strcpy(part, jobs[i].where); strcat(part, "/"); strcat(part, jobs[i].drawer); strcat(part, "/Part");
    if (!(text = oud_read_file(part, &len, MAX_TEXT))) {
        say("apt: ", p->p.name, " wasn't installed: its archive holds no Part file, so GFOpenUpTool can't put it in.");
        return RETURN_ERROR;
    }
    for (k = 0; oul_part_needs(text, k, need, sizeof need); k++) {
        char have[24];
        if (oud_installed(manifest, mlen, need, have, sizeof have)) continue;
        if ((at = oua_find(cat, ncat, need)) < 0) {
            say("apt: ", p->p.name, " needs a part that isn't installed and that no source has:");
            say("     ", need, ". Nothing was changed.");
            FreeVec(text);
            return RETURN_ERROR;
        }
        if ((to = job_of(at)) < 0) {
            if ((to = add_job(at)) < 0) { say("apt: too many parts for one go; install some first.", NULL, NULL); FreeVec(text); return RETURN_ERROR; }
            PutStr((STRPTR)p->p.name);
            say(" needs ", cat[at].p.name, ", which goes in first.");
        }
        if (nedges == OUA_MAX_EDGES) { say("apt: too many needs for one go; install some first.", NULL, NULL); FreeVec(text); return RETURN_ERROR; }
        edges[nedges][0] = (short)i;
        edges[nedges][1] = (short)to;
        nedges++;
    }
    FreeVec(text);
    return RETURN_OK;
}

/* The jobs fetched and checked, every one, then installed in the order their needs give. */
static int run_jobs(void)
{
    short order[OUA_MAX_JOBS];
    int i, ring = 0, rc = RETURN_OK, done = 0;

    oud_delete_all(OUA_WORK);
    make_drawer(OUA_WORK);
    for (i = 0; i < njobs && rc == RETURN_OK; i++) {    /* njobs grows as needs are found */
        if (stopped()) rc = RETURN_ERROR;
        else rc = prepare(i);
    }
    if (rc == RETURN_OK && !oua_order(njobs, (const short (*)[2])edges, nedges, order, &ring)) {
        say("apt: ", cat[jobs[ring].pkg].p.name, " and a part it needs each need the other, so neither can go in first.");
        say("     Nothing was changed.", NULL, NULL);
        rc = RETURN_ERROR;
    }
    for (i = 0; i < njobs && rc == RETURN_OK; i++) {
        const struct job *j = &jobs[order[i]];
        const struct oua_pkg *p = &cat[j->pkg];
        char have[24];
        if (stopped()) { rc = RETURN_ERROR; break; }
        say("Installing ", p->p.name, "...");
        oud_install_part(j->where, j->drawer, Output(), NULL);
        load_manifest();                        /* GFOpenUpTool's own list says whether it went in */
        if (!oud_installed(manifest, mlen, p->p.name, have, sizeof have) || strcmp(have, p->p.version)) {
            say("apt: ", p->p.name, " wasn't installed; GFOpenUpTool's lines above and T:GFOpenUp.log say why.");
            if (i + 1 < njobs) say("     The parts after it were left as they were.", NULL, NULL);
            rc = RETURN_ERROR;
            break;
        }
        keep_part(j);
        done++;
    }
    oud_delete_all(OUA_WORK);
    if (done) say("Some changes need a reboot to start.", NULL, NULL);
    return rc;
}

/* install names, or upgrade (names or all). What is already right is said and left; rc 5 when something asked
 * for was left for a reason the user should read. */
static int cmd_install(const struct oua_args *a)
{
    int i, k, rc = need_catalogue(), upgrade = a->cmd == OUA_UPGRADE;
    char have[24];
    if (rc) return rc;
    load_manifest();
    if (!manifest) {
        say("apt: GFOpenUp isn't installed here (no " MANIFEST ").", NULL, NULL);
        say("     apt puts parts in through GFOpenUpTool; install GFOpenUp first.", NULL, NULL);
        return RETURN_ERROR;
    }
    njobs = nedges = 0;
    if (upgrade && !a->count) {
        int offers = oua_offers(cat, ncat, manifest, mlen, scratch);
        for (i = 0; i < offers; i++)
            if (add_job(oua_find(cat, ncat, scratch[i].name)) < 0) {
                say("apt: more upgrades than one go holds; the rest come with the next apt upgrade.", NULL, NULL);
                rc = RETURN_WARN;
                break;
            }
    }
    for (i = 0; i < a->count; i++) {
        if ((k = oua_find(cat, ncat, a->names[i])) < 0) {
            say("apt: no source has a package named ", a->names[i], ". Nothing was changed.");
            say("     apt search finds names; apt update reads the sources again.", NULL, NULL);
            return RETURN_ERROR;
        }
        switch (oua_weigh(&cat[k], manifest, mlen, have, sizeof have)) {
        case OUA_SAME:
            say(cat[k].p.name, " is already the newest version, ", have);
            continue;
        case OUA_OLDER:
            say(cat[k].p.name, " is installed at a newer version than the sources have; left: ", have);
            rc = oua_worst(rc, RETURN_WARN);
            continue;
        case OUA_NEW:
            if (upgrade) {
                PutStr((STRPTR)cat[k].p.name);
                say(" isn't installed (apt install ", cat[k].p.name, " puts it in).");
                rc = oua_worst(rc, RETURN_WARN);
                continue;
            }
            if (!oud_listed_as_new(cat[k].p.name)) {
                say(cat[k].p.name, " goes in with GFOpenUp's own Installer (or GFOpenUpTool KICKSET), not apt; left.", NULL);
                rc = oua_worst(rc, RETURN_WARN);
                continue;
            }
            break;
        }
        if (job_of(k) < 0 && add_job(k) < 0) { say("apt: too many parts for one go; name fewer.", NULL, NULL); return RETURN_ERROR; }
    }
    if (!njobs) {
        if (upgrade && !a->count) say("Every installed part is the newest version the sources have.", NULL, NULL);
        return rc;
    }
    return oua_worst(rc, run_jobs());
}

/* ---- remove ---------------------------------------------------------------------------------------------- */

/* The drawer holding the Part file of the version that is installed: the one apt kept, else the pack's. 1, else 0. */
static int part_at_hand(const char *name, const char *have, char *drawer, int size)
{
    static const char *const places[] = { OUA_KEPT, OUA_PACK };
    unsigned i;
    for (i = 0; i < 2; i++) {
        char path[200], n[40], v[24], *text;
        long len;
        int same;
        if (strlen(places[i]) + strlen(name) + 8 >= sizeof path || (int)(strlen(places[i]) + strlen(name) + 2) > size) continue;
        strcpy(drawer, places[i]); strcat(drawer, "/"); strcat(drawer, name);
        strcpy(path, drawer); strcat(path, "/Part");
        if (!(text = oud_read_file(path, &len, MAX_TEXT))) continue;
        same = oua_part_head(text, len, n, sizeof n, v, sizeof v) && !strcmp(n, name) && !strcmp(v, have);
        FreeVec(text);
        if (same) return 1;
    }
    return 0;
}

static int cmd_remove(const struct oua_args *a)
{
    int i, rc = RETURN_OK;
    load_manifest();
    if (!manifest) { say("apt: GFOpenUp isn't installed here (no " MANIFEST "), so there is nothing to remove.", NULL, NULL); return RETURN_WARN; }
    load_catalogue();                           /* only for a name typed in another case; no index is needed */
    for (i = 0; i < a->count; i++) {
        char why[200], have[24], drawer[120], cmd[300], kept[160];
        const char *name = a->names[i];
        int k = oua_find(cat, ncat, name);
        if (stopped()) return RETURN_ERROR;
        if (k >= 0) name = cat[k].p.name;
        if (!oua_can_remove(name, manifest, mlen, why, sizeof why)) {
            say("apt: ", why, NULL);
            rc = oua_worst(rc, RETURN_WARN);
            continue;
        }
        oud_installed(manifest, mlen, name, have, sizeof have);
        if (!part_at_hand(name, have, drawer, sizeof drawer)) {
            say("apt: ", name, " stays: apt hasn't the Part file of the version installed, and GFOpenUpTool");
            say("     takes out only what a Part file lists. apt keeps one for each part it puts in; this part came with", NULL, NULL);
            say("     the pack. From the pack's drawer: GFOpenUpTool REMOVE Parts/", name, NULL);
            rc = oua_worst(rc, RETURN_WARN);
            continue;
        }
        if (!oua_remove_cmd(drawer, cmd, sizeof cmd)) { rc = oua_worst(rc, RETURN_ERROR); continue; }
        say("Removing ", name, "...");
        oud_run(cmd, Output());
        load_manifest();
        if (!manifest || oud_installed(manifest, mlen, name, have, sizeof have)) {
            say("apt: ", name, " is still installed; GFOpenUpTool's lines above and T:GFOpenUp.log say why.");
            rc = oua_worst(rc, RETURN_ERROR);
            if (!manifest) return rc;
            continue;
        }
        strcpy(kept, OUA_KEPT "/"); strcat(kept, name); strcat(kept, "/Part");
        DeleteFile((STRPTR)kept);
        kept[strlen(kept) - 5] = 0;
        DeleteFile((STRPTR)kept);
    }
    return rc;
}

/* ---- the command ----------------------------------------------------------------------------------------- */

int main(void)
{
    LONG arg[2] = { 0, 0 };
    struct RDArgs *rd;
    struct oua_args a;
    const char *why;
    const char *const *names;
    int count = 0, rc;

    if (!(rd = ReadArgs((STRPTR)"COMMAND,NAMES/M", arg, NULL))) {
        PrintFault(IoErr(), (STRPTR)"apt");
        return RETURN_FAIL;
    }
    names = (const char *const *)arg[1];
    while (names && names[count]) count++;
    if (!oua_parse((const char *)arg[0], names, count, &a, &why)) {
        say("apt: ", why, NULL);
        PutStr((STRPTR)OUA_USAGE);
        FreeArgs(rd);
        return RETURN_WARN;
    }
    if (a.cmd == OUA_HELP) {
        PutStr((STRPTR)NAME " " VERSION_TEXT ": GFOS's parts from the Shell.\n" OUA_USAGE);
        FreeArgs(rd);
        return RETURN_OK;
    }
    sources = AllocVec(sizeof *sources * OUA_MAX_SOURCES, MEMF_CLEAR);
    cat = AllocVec(sizeof *cat * MAX_PKGS, MEMF_CLEAR);
    fresh = AllocVec(sizeof *fresh * MAX_PKGS, MEMF_CLEAR);
    scratch = AllocVec(sizeof *scratch * MAX_PKGS, MEMF_CLEAR);
    if (!sources || !cat || !fresh || !scratch) {
        say("apt: not enough memory.", NULL, NULL);
        rc = RETURN_FAIL;
    } else if ((rc = load_sources()) == RETURN_OK) {
        switch (a.cmd) {
        case OUA_UPDATE: rc = cmd_update(); break;
        case OUA_LIST: rc = cmd_list(a.which); break;
        case OUA_SEARCH: rc = cmd_search(&a); break;
        case OUA_SHOW: rc = cmd_show(&a); break;
        case OUA_INSTALL: case OUA_UPGRADE: rc = cmd_install(&a); break;
        case OUA_REMOVE: rc = cmd_remove(&a); break;
        case OUA_REPOS: rc = cmd_sources(); break;
        }
    }
    if (manifest) FreeVec(manifest);
    if (scratch) FreeVec(scratch);
    if (fresh) FreeVec(fresh);
    if (cat) FreeVec(cat);
    if (sources) FreeVec(sources);
    FreeArgs(rd);
    return rc;
}
