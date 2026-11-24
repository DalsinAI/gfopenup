/* GFOpenUpdate 0.3: GFOS's updates and software from gfosdistribution, offered on the Amiga
 * (docs/design/GFOPENUPDATE.md; backlog AC-280).
 *
 * Dale, 17 November 2026: "a GFOS updater that knows what versions of things are installed and checks dalsinai etc
 * for updates and offers them to the user to update"; "make it a wbstartup commodity"; "once at start up when the
 * network is up, then periodically that can be set via TOOLTYPE"; "store its data in envarc:".
 * Dale, 24 November 2026: "now can we update the GFOpenUpdate tool to go look there [gfosdistribution]"; "every
 * card is a stranger's card": what a card gets it pulls from the public channel, nothing is pushed to it.
 *
 * A commodity. Its list is gfosdistribution's catalogue, software/index.txt (src/opencatalogue.h), fetched with
 * GFOpenSocket's httpget through GFOpenTLS and weighed against GFOpenUpTool's manifest: each release newer than
 * what is installed, not installed, or up to date. Install all (or Install for the one picked) fetches each
 * archive, checks its size and SHA-256 against the catalogue, unpacks it with GFOpenCompress and installs it with
 * GFOpenUpTool INSTALL, writing a Part file by the release's layout when it hasn't one. After an update it offers a
 * restart. Nothing is installed unasked. When the catalogue can't be had, GFOpenUp's own update release (the URL
 * tooltype, GFOpenUpdate 0.2's only source) is tried instead, for this one release.
 *
 * Started from its icon or a Shell it opens its window and checks at once (a GFOpenUpdate running already, an
 * older one from SYS:WBStartup, is asked to quit first). From SYS:WBStartup it waits quietly: it checks once at
 * startup when the network is up, then every INTERVAL seconds, and says when something is newer (GFOpenNotifier,
 * Ranchero or Ringhio, else its window).
 * Tool types (its icon, or KEY=VALUE on the command line):
 *   CATALOGUE=<address> gfosdistribution's software/index.txt (also ENV:GFOpenUpdate/Catalogue)
 *   RELEASES=<address>  the only place a download may come from (gfosdistribution's releases)
 *   INTERVAL=86400      seconds between checks after the one at startup; never under 600; 0 = only at startup
 *   URL=<address>       GFOpenUp's own update release, the fallback
 *   CX_PRIORITY=0       the broker's priority
 * From a Shell also: LIST (what the catalogue has against what is installed, then quit) and GET=<Name> (that one
 * fetched, checked and installed, then quit).
 * Its data: ENVARC:GFOpenUpdate/LastCheck (seconds).
 *
 * MIT, Copyright (c) 2026 AmiCradle Team. */

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/var.h>
#include <devices/timer.h>
#include <intuition/intuition.h>
#include <libraries/commodities.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/commodities.h>
#include <proto/icon.h>
#include <proto/utility.h>
#include <proto/rexxsyslib.h>
#include <rexx/storage.h>
#include <rexx/rxslib.h>
#include <rexx/errors.h>
#include <string.h>

#include "gfopenlayout_ui.h"
#include "gfopenlayout_ui_amiga.h"

/* the shared headers carry helpers only the host tests and GFOpenUpTool use */
#pragma GCC diagnostic ignored "-Wunused-function"
#include "../openupdate.h"
#include "../openupdate_sha256.h"
#include "../openupdate_amiga.h"    /* shared with C:apt (src/openapt/openapt.c) */
#include "../opencatalogue.h"

#define NAME "GFOpenUpdate"
#define VERSION_TEXT "0.3"
unsigned long __stack = 32768;
static const char version[] __attribute__((used)) = "$VER: " NAME " " VERSION_TEXT " (24.11.2026)";

struct Library *CxBase, *IconBase;
struct IntuitionBase *IntuitionBase;
struct Library *UtilityBase;
struct RxsLib *RexxSysBase;

#define MANIFEST "ENVARC:GFOpenAmiga/GFOpenUp.manifest"
#define INDEX_FILE OUD_WORK "/GFOpenUpdate.index"
#define CAT_FILE OUD_WORK "/index.txt"
#define UNPACKED OUD_WORK "/x"
#define MAX_PARTS 96
#define MAX_TEXT (4L * 1024 * 1024)

static struct {
    long interval;
    char url[200];
    char catalogue[200];
    char releases[200];
    LONG cxpri;
} set = { OUD_INTERVAL_DEFAULT, OUD_URL_DEFAULT, OUC_CATALOGUE_DEFAULT, OUC_RELEASES_DEFAULT, 0 };

static struct oud_part parts[MAX_PARTS];       /* the old index, read into this first */
static struct ouc_item items[MAX_PARTS];       /* what the list shows */
static int nitems, newer, fresh, state;        /* state: 0 not checked yet, 1 checked, -1 nothing could be had */
static const char *source = "";                /* which list it is */
static BPTR shell_out;                         /* LIST and GET: where they print; 0 in the window */

/* ---- small things ---------------------------------------------------------------------------------------- */

static long now_seconds(void)
{
    struct DateStamp ds;
    DateStamp(&ds);
    return ds.ds_Days * 86400L + ds.ds_Minute * 60L + ds.ds_Tick / TICKS_PER_SECOND;
}

static char *read_file(const char *path, long *len)
{
    return oud_read_file(path, len, MAX_TEXT);
}

static int write_file(const char *path, const char *text, long len)
{
    BPTR fh;
    BPTR l = CreateDir((STRPTR)OUD_DATA);
    if (l) UnLock(l);
    if (!(fh = Open((STRPTR)path, MODE_NEWFILE))) return 0;
    if (Write(fh, (APTR)text, len) != len) { Close(fh); return 0; }
    return Close(fh) ? 1 : 0;
}

static LONG run(const char *cmd)
{
    return oud_run(cmd, 0);
}

static void clear_work(void)
{
    run("C:Delete >NIL: \"" OUD_WORK "\" ALL QUIET FORCE");
}

static void print(const char *a, const char *b, const char *c)
{
    if (!shell_out) return;
    if (a) FPuts(shell_out, (STRPTR)a);
    if (b) FPuts(shell_out, (STRPTR)b);
    if (c) FPuts(shell_out, (STRPTR)c);
    FPuts(shell_out, (STRPTR)"\n");
    Flush(shell_out);
}

static struct Window *win;
static LONG ask(const char *body, const char *gadgets, ULONG arg)
{
    struct EasyStruct es = { sizeof(struct EasyStruct), 0, (STRPTR)NAME, (STRPTR)body, (STRPTR)gadgets };
    return EasyRequest(win, &es, NULL, arg);
}

static int exists(const char *path)
{
    BPTR l = Lock((STRPTR)path, ACCESS_READ);
    if (l) UnLock(l);
    return l != 0;
}

/* ---- signing: AC-280's next step ------------------------------------------------------------------------ */

/* NULL when the catalogue may be used, else why not. Today each file is checked against the catalogue's SHA-256
 * and the catalogue itself comes over HTTPS from GitHub. The next step signs the catalogue: the release process
 * signs software/index.txt (Ed25519, a detached software/index.txt.sig beside it), and the image carries the public
 * key at OUC_KEY. A card with that key refuses a catalogue this build can't check, so once keys ship an old
 * GFOpenUpdate never falls back to trusting an unsigned list. */
static const char *catalogue_trusted(const char *text, long len)
{
    (void)text; (void)len;
    if (exists(OUC_KEY)) return "this card wants a signed list, and GFOpenUpdate 0.3 can't check signatures yet";
    return NULL;
}

/* ---- a release's Part file, when it hasn't one ---------------------------------------------------------- */

/* Every file under base/rel (rel "" at the top) as a copy line, depth at most 8. 1, else 0. */
static int part_lines(BPTR fh, const char *name, const char *base, const char *rel, int depth)
{
    char path[300], sub[260], line[600];
    struct FileInfoBlock *fib;
    BPTR lock;
    int ok = 1;
    if (depth > 8 || strlen(base) + strlen(rel) + 2 >= sizeof path) return 0;
    strcpy(path, base);
    if (*rel) { strcat(path, "/"); strcat(path, rel); }
    if (!(lock = Lock((STRPTR)path, ACCESS_READ))) return 0;
    if (!(fib = AllocDosObject(DOS_FIB, NULL))) { UnLock(lock); return 0; }
    if (Examine(lock, fib))
        while (ok && ExNext(lock, fib)) {
            const char *n = (const char *)fib->fib_FileName;
            if (!*rel && !Stricmp((STRPTR)n, (STRPTR)"Part")) continue;
            if (strlen(rel) + strlen(n) + 2 >= sizeof sub) { ok = 0; break; }
            strcpy(sub, rel);
            if (*rel) strcat(sub, "/");
            strcat(sub, n);
            if (fib->fib_DirEntryType > 0) ok = part_lines(fh, name, base, sub, depth + 1);
            else if (!ouc_part_line(name, sub, line, sizeof line) || FPuts(fh, (STRPTR)line)) ok = 0;
        }
    FreeDosObject(DOS_FIB, fib);
    UnLock(lock);
    return ok;
}

/* The drawer's Part file: its own when it has one (a GFOpenUp part), else written by its layout. NULL, else why. */
static const char *ensure_part(const char *drawer, const struct ouc_item *it)
{
    char path[300];
    BPTR fh;
    int ok;
    if (strlen(drawer) + 6 >= sizeof path) return "its drawer's name won't fit";
    strcpy(path, drawer);
    strcat(path, "/Part");
    if (exists(path)) return NULL;
    if (!(fh = Open((STRPTR)path, MODE_NEWFILE))) return "its Part file couldn't be written";
    FPuts(fh, (STRPTR)"part ");
    FPuts(fh, (STRPTR)it->p.name);
    FPuts(fh, (STRPTR)" ");
    FPuts(fh, (STRPTR)it->p.version);
    FPuts(fh, (STRPTR)"\n; written by " NAME " " VERSION_TEXT " from the release's layout\n");
    ok = part_lines(fh, it->p.name, drawer, "", 0);
    if (!Close(fh)) ok = 0;
    return ok ? NULL : "its Part file couldn't be written";
}

/* ---- one release: fetch, check, unpack, install --------------------------------------------------------- */

/* NULL when installed; else why not. */
static const char *install_item(const struct ouc_item *it)
{
    char path[160], drawer[80], where[200];
    const char *w;
    BPTR l;
    if ((l = CreateDir((STRPTR)OUD_WORK))) UnLock(l);
    strcpy(path, OUD_WORK "/");
    strcat(path, it->p.file);
    print("  fetching ", it->url, NULL);
    if ((w = oud_fetch_checked(it->url, path, it->p.bytes, it->p.sha256))) return w;
    print("  size and SHA-256 match the catalogue: ", it->p.sha256, NULL);
    if ((w = oud_unpack_part(path, UNPACKED, drawer, sizeof drawer))) return w;
    strcpy(where, UNPACKED "/");
    strcat(where, drawer);
    if ((w = ensure_part(where, it))) return w;
    print("  installing with GFOpenUpTool INSTALL", NULL, NULL);
    return oud_install_part(UNPACKED, drawer, shell_out, NULL);
}

/* ---- a check, and what the window shows ----------------------------------------------------------------- */

static olui *ui;
static olui_amiga *glue;
static int picked = -1;                        /* the row picked in the list, or -1 */

static void show(void);

static void say(const char *text)              /* the status line now, before something slow */
{
    if (!ui) return;
    olui_set_text(ui, olui_find(ui, "status"), text);
    olui_amiga_draw(glue);
}

/* The old index (GFOpenUpdate 0.2's source) as items: the count, or -1. */
static int from_old_index(void)
{
    char url[300], cmd[600], *index;
    long ilen;
    int count = -1;
    if (oud_url(set.url, OUD_INDEX_NAME, url, sizeof url) && oud_fetch_cmd(url, INDEX_FILE, cmd, sizeof cmd) && run(cmd) == 0
        && (index = read_file(INDEX_FILE, &ilen))) {
        count = oud_index_parts(index, ilen, parts, MAX_PARTS);
        FreeVec(index);
    }
    return count < 0 ? -1 : ouc_from_index(parts, count, set.url, items, MAX_PARTS);
}

/* The catalogue fetched and weighed: items set, state 1; or -1 when neither it nor the old index could be had. */
static int check(void)
{
    char cmd[600], *text, *manifest;
    const char *why = NULL;
    long tlen, mlen = 0;
    int count = -1, bad = 0;
    BPTR l;

    say("Checking gfosdistribution...");
    clear_work();
    if ((l = CreateDir((STRPTR)OUD_WORK))) UnLock(l);
    if (oud_fetch_cmd(set.catalogue, CAT_FILE, cmd, sizeof cmd) && run(cmd) == 0 && (text = read_file(CAT_FILE, &tlen))) {
        if (!(why = catalogue_trusted(text, tlen))) count = ouc_parse(text, tlen, set.releases, items, MAX_PARTS, &bad);
        FreeVec(text);
        source = "gfosdistribution";
    }
    if (count <= 0 && !why) { count = from_old_index(); source = "GFOpenUp's update release"; }
    clear_work();
    picked = -1;
    if (count < 0 || why) {
        nitems = newer = fresh = 0;
        state = -1;
        if (why) { say(why); print(NAME ": ", why, NULL); }
        else show();
        return -1;
    }
    manifest = read_file(MANIFEST, &mlen);
    nitems = count;
    newer = ouc_weigh(items, nitems, manifest, mlen, &fresh);
    if (manifest) FreeVec(manifest);
    state = 1;
    show();
    return 0;
}

static void restart_offer(int updated)
{
    if (!updated) return;
    if (ask("Installed.\nSome of it starts only after a restart.\n\nRestart the Amiga now?", "Restart now|Later", 0) == 1) {
        Delay(100);                            /* the disk's buffers written out */
        ColdReboot();
    }
}

/* Install all (which -1: every newer one and every one not installed) or the one picked. */
static void install(int which)
{
    char *res = AllocVec(4096, MEMF_CLEAR);
    int i, bad = 0, good = 0, updated = 0, len = 0;
    if (!res) return;
    for (i = 0; i < nitems; i++) {
        char line[120];
        const char *why;
        int n;
        if (which >= 0 ? i != which : items[i].state == OUC_SAME) continue;
        n = oud_put(line, sizeof line, 0, "Installing ");
        n = oud_put(line, sizeof line, n, items[i].p.name);
        n = oud_put(line, sizeof line, n, " ");
        n = oud_put(line, sizeof line, n, items[i].p.version);
        oud_put(line, sizeof line, n, "...");
        say(line);
        if ((why = install_item(&items[i]))) {
            bad++;
            len = oud_put(res, 4096, len, items[i].p.name);
            len = oud_put(res, 4096, len, ": ");
            len = oud_put(res, 4096, len, why);
            len = oud_put(res, 4096, len, "\n");
        } else {
            good++;
            updated |= items[i].state == OUC_NEWER;
        }
        clear_work();
    }
    if (bad) ask("Not installed:\n\n%s", "OK", (ULONG)res);
    else if (good && !updated) ask("Installed.", "OK", 0);
    FreeVec(res);
    restart_offer(updated);
    check();
}

/* ---- the window (GFOpenLayout UI, the look every GFOpenUp program has) ---------------------------------- */

static const char markup[] =
    "<column style='flex:1'>"
    "  <p id=head>What gfosdistribution has, against what this Amiga has installed.</p>"
    "  <list id=list style='flex:1;height:auto' title='gfosdistribution'></list>"
    "  <p id=desc> </p>"
    "  <p id=status>Checking gfosdistribution...</p>"
    "  <row>"
    "    <button id=all style='flex:1' title='Downloads, checks and installs everything new'>Install all</button>"
    "    <button id=one style='flex:1' title='Downloads, checks and installs the one picked'>Install</button>"
    "    <button id=check style='flex:1' title='Looks at gfosdistribution again'>Check now</button>"
    "    <button id=later style='flex:1' title='Closes the window'>Close</button>"
    "  </row>"
    "</column>";

static const char css[] = "row{gap:6px} column{gap:6px} #head{font-weight:bold} p{margin:2px 0}";

enum { A_NONE, A_ALL, A_ONE, A_CHECK, A_CLOSE, A_PICK };
static int action;

static void on_all(olui *u, const olui_event *ev, void *d) { (void)u; (void)ev; (void)d; action = A_ALL; }
static void on_one(olui *u, const olui_event *ev, void *d) { (void)u; (void)ev; (void)d; action = A_ONE; }
static void on_check(olui *u, const olui_event *ev, void *d) { (void)u; (void)ev; (void)d; action = A_CHECK; }
static void on_later(olui *u, const olui_event *ev, void *d) { (void)u; (void)ev; (void)d; action = A_CLOSE; }
static void on_pick(olui *u, const olui_event *ev, void *d) { (void)u; (void)ev; (void)d; action = A_PICK; }

static void show_pick(void)
{
    olui_el *sel;
    const char *v;
    char t[220];
    int n = 0;
    if (!ui) return;
    sel = olui_list_selected(olui_find(ui, "list"));
    v = sel ? olui_get_value(sel) : NULL;
    picked = -1;
    if (v && *v) { LONG k; if (StrToLong((STRPTR)v, &k) > 0 && k >= 0 && k < nitems) picked = (int)k; }
    t[0] = 0;
    if (picked >= 0) {
        n = oud_put(t, sizeof t, 0, items[picked].p.name);
        n = oud_put(t, sizeof t, n, ": ");
        oud_put(t, sizeof t, n, items[picked].desc[0] ? items[picked].desc : "(no description)");
    }
    olui_set_text(ui, olui_find(ui, "desc"), t[0] ? t : " ");
    olui_set_disabled(ui, olui_find(ui, "one"), picked < 0 || items[picked].state == OUC_SAME);
    olui_amiga_draw(glue);
}

static void show(void)
{
    olui_el *list;
    int i;
    char t[140], num[8];
    if (shell_out) {                           /* LIST */
        print(NAME " " VERSION_TEXT ": ", state < 0 ? "no list" : source, state < 0 ? "" : " (SHA-256 checked on each download)");
        for (i = 0; i < nitems; i++) { ouc_row_text(&items[i], t, sizeof t); print("  ", t, NULL); }
        print(ouc_status(state, newer, fresh), NULL, NULL);
    }
    if (!ui) return;
    list = olui_find(ui, "list");
    olui_list_clear(ui, list);
    for (i = 0; i < nitems; i++) {
        ouc_row_text(&items[i], t, sizeof t);
        num[0] = (char)('0' + i / 10); num[1] = (char)('0' + i % 10); num[2] = 0;
        olui_list_add(ui, list, NULL, t, num);
    }
    olui_set_text(ui, olui_find(ui, "status"), ouc_status(state, newer, fresh));
    olui_set_disabled(ui, olui_find(ui, "all"), newer + fresh == 0);
    show_pick();
}

static void close_win(void)
{
    if (glue) olui_amiga_free(glue);
    if (ui) olui_free(ui);
    glue = NULL;
    ui = NULL;
    if (win) CloseWindow(win);
    win = NULL;
}

static int open_win(void)
{
    struct Screen *scr;
    int fh, w, h;
    char err[128];
    if (win) { WindowToFront(win); ActivateWindow(win); return 1; }
    if (!(scr = LockPubScreen(NULL))) return 0;
    fh = scr->Font ? scr->Font->ta_YSize : 8;
    w = fh * 50;
    h = fh * 24;
    if (w > scr->Width) w = scr->Width;
    if (h > scr->Height) h = scr->Height;
    win = OpenWindowTags(NULL, WA_PubScreen, (ULONG)scr, WA_Title, (ULONG)"GFOS Updates: gfosdistribution",
                         WA_ScreenTitle, (ULONG)NAME " " VERSION_TEXT,
                         WA_Left, (scr->Width - w) / 2, WA_Top, (scr->Height - h) / 2, WA_Width, w, WA_Height, h,
                         WA_MinWidth, fh * 30, WA_MinHeight, fh * 12, WA_MaxWidth, ~0UL, WA_MaxHeight, ~0UL,
                         WA_DragBar, TRUE, WA_DepthGadget, TRUE, WA_CloseGadget, TRUE, WA_SizeGadget, TRUE,
                         WA_Activate, TRUE, WA_SimpleRefresh, TRUE, WA_ReportMouse, TRUE,
                         WA_IDCMP, olui_amiga_idcmp(), TAG_DONE);
    UnlockPubScreen(NULL, scr);
    if (!win) return 0;
    if (!(ui = olui_new()) || !(glue = olui_amiga_new(ui, win))) goto fail;
    olui_amiga_theme(glue, NULL);
    if (!olui_load(ui, markup, css, err, sizeof err)) goto fail;
    olui_on(ui, "all", OLUI_CLICK, on_all, NULL);
    olui_on(ui, "one", OLUI_CLICK, on_one, NULL);
    olui_on(ui, "check", OLUI_CLICK, on_check, NULL);
    olui_on(ui, "later", OLUI_CLICK, on_later, NULL);
    olui_on(ui, "list", OLUI_SELECT, on_pick, NULL);
    if (!olui_amiga_layout(glue)) goto fail;
    show();
    return 1;
fail:
    close_win();
    return 0;
}

static void save_last(long now)
{
    char t[16], rev[16];
    int k = 0, j;
    long v = now;
    do { rev[k++] = (char)('0' + v % 10); v /= 10; } while (v > 0 && k < 15);
    for (j = 0; j < k; j++) t[j] = rev[k - 1 - j];
    t[k++] = '\n';
    write_file(OUD_LAST, t, k);
}

/* ---- the notice: GFOpenNotifier (or Ranchero, or Ringhio) says updates are waiting ----------------------- */

static struct MsgPort *rxport;                 /* OUD_PORT: SHOW from a double-click on the notice */

/* One command to the notifier's port, waited for: 1 when it answered rc 0. */
static int notifier_cmd(const char *cmd)
{
    struct MsgPort *reply, *to;
    struct RexxMsg *rm;
    int ok = 0;
    if (!RexxSysBase || !(reply = CreateMsgPort())) return 0;
    if ((rm = CreateRexxMsg(reply, NULL, (STRPTR)OUD_NOTIFY_PORT))) {
        if ((rm->rm_Args[0] = (STRPTR)CreateArgstring((STRPTR)cmd, strlen(cmd)))) {
            rm->rm_Action = RXCOMM | RXFF_RESULT;
            Forbid();
            if ((to = FindPort((STRPTR)OUD_NOTIFY_PORT))) PutMsg(to, (struct Message *)rm);
            Permit();
            if (to) {
                WaitPort(reply);
                GetMsg(reply);
                ok = rm->rm_Result1 == 0;
                if (rm->rm_Result1 == 0 && rm->rm_Result2) DeleteArgstring((STRPTR)rm->rm_Result2);
            }
            DeleteArgstring(rm->rm_Args[0]);
        }
        DeleteRexxMsg(rm);
    }
    DeleteMsgPort(reply);
    return ok;
}

/* "n updates are waiting" as a notice: 1 when the notifier took it, else 0 (the window opens instead). */
static int notify(int count)
{
    char cmd[200];
    if (!rxport || !RexxSysBase) return 0;
    oud_notice_cmd(0, 0, cmd, sizeof cmd);
    notifier_cmd(cmd);                         /* "already registered" after the first time: fine */
    oud_notice_cmd(1, count, cmd, sizeof cmd);
    return notifier_cmd(cmd);
}

/* ---- the one running already: commodities.library's private BrokerCommand (-198), as GFOpenExchange's ---- */

static LONG cx_BrokerCommand(const char *name, LONG id)
{
    register struct Library *a6 __asm("a6") = CxBase;
    register const char *a0 __asm("a0") = name;
    register LONG d0 __asm("d0") = id;
    __asm__ volatile("jsr -198(%%a6)" : "+r"(d0), "+r"(a0) : "r"(a6) : "d1", "a1", "cc", "memory");
    return d0;
}

/* ---- settings -------------------------------------------------------------------------------------------- */

static int address_ok(const char *value, int size)
{
    char t[300];
    return value && (int)strlen(value) < size && oud_url(value, "x", t, sizeof t) && !strncmp(value, "https://", 8);
}

static void take(const char *key, const char *value)
{
    if (!value) return;
    if (!Stricmp((STRPTR)key, (STRPTR)"INTERVAL")) set.interval = oud_interval(value);
    else if (!Stricmp((STRPTR)key, (STRPTR)"URL")) { if (address_ok(value, sizeof set.url)) strcpy(set.url, value); }
    else if (!Stricmp((STRPTR)key, (STRPTR)"CATALOGUE")) { if (address_ok(value, sizeof set.catalogue)) strcpy(set.catalogue, value); }
    else if (!Stricmp((STRPTR)key, (STRPTR)"RELEASES")) { if (address_ok(value, sizeof set.releases)) strcpy(set.releases, value); }
    else if (!Stricmp((STRPTR)key, (STRPTR)"CX_PRIORITY")) { LONG v; if (StrToLong((STRPTR)value, &v) > 0) set.cxpri = v; }
}

static const char *const KEYS[] = { "INTERVAL", "URL", "CX_PRIORITY", "CATALOGUE", "RELEASES" };
#define NKEYS (sizeof KEYS / sizeof KEYS[0])

static void from_icon(struct DiskObject *d)
{
    unsigned i;
    if (!d || !d->do_ToolTypes) return;
    for (i = 0; i < NKEYS; i++)
        take(KEYS[i], (const char *)FindToolType((CONST_STRPTR *)d->do_ToolTypes, (STRPTR)KEYS[i]));
}

static int quiet;                              /* from SYS:WBStartup: no window until something is newer */
static int list_only;
static char get_name[40];

/* 1 when the drawer is SYS:WBStartup (its name ends so) */
static int is_wbstartup(BPTR dir)
{
    char p[256];
    int n;
    if (!dir || !NameFromLock(dir, (STRPTR)p, sizeof p)) return 0;
    n = (int)strlen(p);
    return n >= 9 && !Stricmp((STRPTR)(p + n - 9), (STRPTR)"WBStartup");
}

static void settings(int argc, char **argv)
{
    char v[200];
    if (GetVar((STRPTR)OUC_ENV_CATALOGUE, (STRPTR)v, sizeof v, GVF_GLOBAL_ONLY) > 0) take("CATALOGUE", v);
    if (argc == 0) {                        /* from Workbench: its icon */
        struct WBStartup *wbs = (struct WBStartup *)argv;
        if (IconBase && wbs && wbs->sm_NumArgs > 0) {
            BPTR old = CurrentDir(wbs->sm_ArgList[0].wa_Lock);
            struct DiskObject *d = GetDiskObject((STRPTR)wbs->sm_ArgList[0].wa_Name);
            quiet = is_wbstartup(wbs->sm_ArgList[0].wa_Lock);
            from_icon(d);
            if (d) FreeDiskObject(d);
            CurrentDir(old);
        }
    } else {                                /* from a Shell: the icon beside it, then the line */
        char prog[256], icon[300];
        LONG a[NKEYS + 2] = { 0, 0, 0, 0, 0, 0, 0 };
        struct RDArgs *rd;
        unsigned i;
        if (IconBase && GetProgramName((STRPTR)prog, sizeof prog)) {
            struct DiskObject *d;
            strcpy(icon, "PROGDIR:");
            strncat(icon, (const char *)FilePart((STRPTR)prog), sizeof icon - 10);
            if ((d = GetDiskObject((STRPTR)icon))) { from_icon(d); FreeDiskObject(d); }
        }
        if ((rd = ReadArgs((STRPTR)"INTERVAL/K,URL/K,CX_PRIORITY/K,CATALOGUE/K,RELEASES/K,LIST/S,GET/K", a, NULL))) {
            for (i = 0; i < NKEYS; i++) take(KEYS[i], (const char *)a[i]);
            list_only = a[NKEYS] != 0;
            if (a[NKEYS + 1] && strlen((const char *)a[NKEYS + 1]) < sizeof get_name) strcpy(get_name, (const char *)a[NKEYS + 1]);
            FreeArgs(rd);
        }
    }
}

/* LIST and GET, from a Shell: the return code. */
static int shell_mode(void)
{
    int i;
    shell_out = Output();
    if (!oud_network_up()) { print(NAME ": the network isn't up (no bsdsocket.library)", NULL, NULL); return RETURN_FAIL; }
    if (check() < 0) return RETURN_ERROR;
    if (!get_name[0]) return RETURN_OK;
    for (i = 0; i < nitems; i++)
        if (!Stricmp((STRPTR)items[i].p.name, (STRPTR)get_name)) {
            const char *why;
            print("Installing ", items[i].p.name, NULL);
            why = install_item(&items[i]);
            clear_work();
            print(items[i].p.name, why ? ": not installed: " : ": installed", why);
            if (!why) check();
            return why ? RETURN_ERROR : RETURN_OK;
        }
    print(NAME ": the catalogue has no ", get_name, NULL);
    return RETURN_WARN;
}

/* ---- the commodity --------------------------------------------------------------------------------------- */

static struct NewBroker nb = {
    NB_VERSION, (STRPTR)NAME, (STRPTR)NAME " " VERSION_TEXT, (STRPTR)"GFOS updates from gfosdistribution (Show: its window)",
    NBU_UNIQUE | NBU_NOTIFY, COF_SHOW_HIDE, 0, NULL, 0
};

int main(int argc, char **argv)
{
    struct MsgPort *port = NULL, *tport = NULL;
    struct timerequest *tr = NULL;
    CxObj *broker = NULL;
    LONG err = 0;
    int timer = 0, tpending = 0, quit = 0, active = 1, checked = 0, first = 1, rc = RETURN_FAIL, tries;
    long last = 0;

    CxBase = OpenLibrary((STRPTR)"commodities.library", 37);
    IconBase = OpenLibrary((STRPTR)"icon.library", 37);
    IntuitionBase = (struct IntuitionBase *)OpenLibrary((STRPTR)"intuition.library", 37);
    UtilityBase = OpenLibrary((STRPTR)"utility.library", 37);
    if (!CxBase || !IntuitionBase || !UtilityBase) goto out;
    settings(argc, argv);
    if (list_only || get_name[0]) { rc = shell_mode(); goto out; }
    {
        long len;
        char *t = read_file(OUD_LAST, &len);
        if (t) { last = oud_last(t, len); FreeVec(t); }
    }

    RexxSysBase = (struct RxsLib *)OpenLibrary((STRPTR)"rexxsyslib.library", 0);   /* only for the notice */
    if (!(port = CreateMsgPort())) goto out;
    nb.nb_Port = port;
    nb.nb_Pri = (BYTE)set.cxpri;
    for (tries = 0; !(broker = CxBroker(&nb, &err)) && err == CBERR_DUP && !quiet && tries < 10; tries++) {
        cx_BrokerCommand(NAME, CXCMD_KILL);    /* an older one (SYS:WBStartup's 0.2) steps aside for this one */
        Delay(25);
    }
    if (!broker) {                             /* running already: that one checks now (CXCMD_UNIQUE) */
        if (err == CBERR_DUP) rc = RETURN_OK;
        goto out;
    }
    if (RexxSysBase && (rxport = CreateMsgPort())) {
        rxport->mp_Node.ln_Name = (char *)OUD_PORT;
        rxport->mp_Node.ln_Pri = 0;
        Forbid();
        if (FindPort((STRPTR)OUD_PORT)) { Permit(); DeleteMsgPort(rxport); rxport = NULL; }
        else { AddPort(rxport); Permit(); }
    }
    ActivateCxObj(broker, 1);
    if (!(tport = CreateMsgPort()) || !(tr = (struct timerequest *)CreateIORequest(tport, sizeof *tr))) goto out;
    if (OpenDevice((STRPTR)TIMERNAME, UNIT_VBLANK, (struct IORequest *)tr, 0)) goto out;
    timer = 1;
    rc = RETURN_OK;
    if (!quiet && open_win()) {                /* from its icon: the window, and the list at once */
        if (check() == 0) { checked = 1; last = now_seconds(); save_last(last); }
    }

    while (!quit) {
        ULONG got;
        CxMsg *m;
        int now_asked = 0;
        if (!tpending) {                       /* a look each minute (OUD_RETRY): due, and the network up? */
            tr->tr_node.io_Command = TR_ADDREQUEST;
            tr->tr_time.tv_secs = first ? 5 : OUD_RETRY;     /* the first look soon after startup */
            tr->tr_time.tv_micro = 0;
            first = 0;
            SendIO((struct IORequest *)tr);
            tpending = 1;
        }
        got = Wait((1UL << port->mp_SigBit) | (1UL << tport->mp_SigBit) | SIGBREAKF_CTRL_C
                   | (win ? 1UL << win->UserPort->mp_SigBit : 0) | (rxport ? 1UL << rxport->mp_SigBit : 0));
        if (rxport) {                          /* SHOW, from a double-click on the notice: the window */
            struct RexxMsg *rm;
            int show_win = 0;
            while ((rm = (struct RexxMsg *)GetMsg(rxport))) {
                show_win = 1;
                rm->rm_Result1 = 0;
                rm->rm_Result2 = 0;
                ReplyMsg((struct Message *)rm);
            }
            if (show_win) open_win();
        }
        if (got & SIGBREAKF_CTRL_C) quit = 1;
        while ((m = (CxMsg *)GetMsg(port))) {
            ULONG type = CxMsgType(m), id = CxMsgID(m);
            ReplyMsg((struct Message *)m);
            if (type == CXM_COMMAND) switch (id) {
            case CXCMD_DISABLE: ActivateCxObj(broker, 0); active = 0; break;
            case CXCMD_ENABLE: ActivateCxObj(broker, 1); active = 1; break;
            case CXCMD_APPEAR: case CXCMD_UNIQUE: now_asked = 1; break;
            case CXCMD_DISAPPEAR: action = A_CLOSE; break;
            case CXCMD_KILL: quit = 1; break;
            }
        }
        if (quit) break;
        if (win) {
            struct IntuiMessage *im;
            while (win && (im = (struct IntuiMessage *)GetMsg(win->UserPort))) {
                int r = olui_amiga_event(glue, im);
                ReplyMsg(&im->ExecMessage);
                if (r == OLUI_AMIGA_CLOSE) action = A_CLOSE;
            }
            if (win && action == A_NONE) olui_amiga_draw(glue);
        }
        if (tpending && CheckIO((struct IORequest *)tr)) {
            long now = now_seconds();
            WaitIO((struct IORequest *)tr);
            tpending = 0;
            if (active && oud_due(oud_network_up(), checked, now, last, set.interval) && check() == 0) {
                save_last(now);
                checked = 1;
                last = now;
                if (newer && !win && !notify(newer)) open_win();   /* never installs: it says so, and waits */
            }
        }
        if (now_asked) {                           /* Exchange's Show, or started again: the window, checked now */
            if (open_win() && check() == 0) { checked = 1; last = now_seconds(); save_last(last); }
        }
        switch (action) {
        case A_ALL: action = A_NONE; install(-1); break;
        case A_ONE: action = A_NONE; if (picked >= 0) install(picked); break;
        case A_CHECK: action = A_NONE; if (check() == 0) { checked = 1; last = now_seconds(); save_last(last); } break;
        case A_PICK: action = A_NONE; show_pick(); break;
        case A_CLOSE: action = A_NONE; close_win(); if (!quiet) quit = 1; break;
        }
    }
out:
    close_win();
    if (rxport) {
        struct Message *msg;
        Forbid();
        RemPort(rxport);
        Permit();
        while ((msg = GetMsg(rxport))) { ((struct RexxMsg *)msg)->rm_Result1 = RC_ERROR; ReplyMsg(msg); }
        DeleteMsgPort(rxport);
    }
    if (tpending) { AbortIO((struct IORequest *)tr); WaitIO((struct IORequest *)tr); }
    if (timer) CloseDevice((struct IORequest *)tr);
    if (tr) DeleteIORequest((struct IORequest *)tr);
    if (tport) DeleteMsgPort(tport);
    if (broker) DeleteCxObjAll(broker);
    if (port) { struct Message *msg; while ((msg = GetMsg(port))) ReplyMsg(msg); DeleteMsgPort(port); }
    if (RexxSysBase) CloseLibrary((struct Library *)RexxSysBase);
    if (UtilityBase) CloseLibrary(UtilityBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    if (IconBase) CloseLibrary(IconBase);
    if (CxBase) CloseLibrary(CxBase);
    return rc;
}
