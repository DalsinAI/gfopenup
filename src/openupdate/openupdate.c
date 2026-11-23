/* GFOpenUpdate 0.2: GFOS's updates offered on the Amiga (docs/design/GFOPENUPDATE.md, steps 3 and 4).
 *
 * Dale, 17 November 2026: "a GFOS updater that knows what versions of things are installed and checks dalsinai etc
 * for updates and offers them to the user to update"; "make it a wbstartup commodity"; "once at start up when the
 * network is up, then periodically that can be set via TOOLTYPE"; "store its data in envarc:".
 *
 * A commodity in SYS:WBStartup. Once a minute it looks whether a check is due (src/openupdate.h, oud_due): the
 * first once bsdsocket.library opens and the index comes, then every INTERVAL seconds. A check fetches
 * GFOpenUpdate.index from the release with GFOpenSocket's httpget and weighs it against GFOpenUpTool's manifest.
 * When an installed part has a newer version, its window opens (Dale: "lets give gfopenup a ui"): every part
 * installed with its version, the newer version beside each one that has one, and Update, Skip these, Check now
 * and Not now. Update fetches each part's archive, checks its size and SHA-256, unpacks it with GFOpenCompress and
 * installs it with GFOpenUpTool INSTALL. Nothing is installed unasked.
 *
 * A timed check that finds updates says so through GFOpenNotifier (or Ranchero, or Ringhio: the RINGHIO port);
 * a double-click on the notice sends SHOW to its port GFOPENUPDATE, which opens the window. With no notifier
 * running the window opens by itself.
 * Exchange's Show (or GFOpenUpdate started again) opens the window and checks now; Hide closes it.
 * Tool types (its icon, or KEY=VALUE on the command line):
 *   INTERVAL=86400      seconds between checks after the one at startup; never under 600; 0 = only at startup
 *   URL=<address>       where the index and archives are (default gfopenup's release "updates")
 *   CX_PRIORITY=0       the broker's priority
 * Its data: ENVARC:GFOpenUpdate/LastCheck (seconds) and ENVARC:GFOpenUpdate/Skipped ("skip <Name> <version>").
 *
 * MIT, Copyright (c) 2026 AmiCradle Team. */

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dostags.h>
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

#define NAME "GFOpenUpdate"
#define VERSION_TEXT "0.2"
unsigned long __stack = 32768;
static const char version[] __attribute__((used)) = "$VER: " NAME " " VERSION_TEXT " (17.11.2026)";

struct Library *CxBase, *IconBase;
struct IntuitionBase *IntuitionBase;
struct Library *UtilityBase;
struct RxsLib *RexxSysBase;

#define MANIFEST "ENVARC:GFOpenAmiga/GFOpenUp.manifest"
#define INDEX_FILE OUD_WORK "/GFOpenUpdate.index"
#define UNPACKED OUD_WORK "/x"
#define MAX_PARTS 96
#define MAX_TEXT (4L * 1024 * 1024)

static struct {
    long interval;
    char url[200];
    LONG cxpri;
} set = { OUD_INTERVAL_DEFAULT, OUD_URL_DEFAULT, 0 };

static struct oud_part parts[MAX_PARTS];

/* ---- small things ---------------------------------------------------------------------------------------- */

static long now_seconds(void)
{
    struct DateStamp ds;
    DateStamp(&ds);
    return ds.ds_Days * 86400L + ds.ds_Minute * 60L + ds.ds_Tick / TICKS_PER_SECOND;
}

/* A whole file into a new buffer (NUL after it), its length in *len; NULL when there is none or it is over
 * MAX_TEXT (src/openupdate_amiga.h). */
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

static struct Window *win;
static LONG ask(const char *body, const char *gadgets, ULONG arg)
{
    struct EasyStruct es = { sizeof(struct EasyStruct), 0, (STRPTR)NAME, (STRPTR)body, (STRPTR)gadgets };
    return EasyRequest(win, &es, NULL, arg);
}

static int network_up(void)
{
    return oud_network_up();
}

/* ---- one part: fetch, check, unpack, install ------------------------------------------------------------- */

/* 0 when installed; else why not into why. The steps are src/openupdate_amiga.h's, which C:apt shares. */
static int update_part(const struct oud_part *p, char *why, int size)
{
    char url[300], path[160], drawer[80];
    const char *w;

    strcpy(path, OUD_WORK "/");
    strcat(path, p->file);
    if (!oud_url(set.url, p->file, url, sizeof url)) w = "its address won't fit";
    else if (!(w = oud_fetch_checked(url, path, p->bytes, p->sha256))
             && !(w = oud_unpack_part(path, UNPACKED, drawer, sizeof drawer)))
        w = oud_install_part(UNPACKED, drawer, 0, NULL);
    if (w) strncpy(why, w, size);
    return w != NULL;
}

/* ---- a check, and what the window shows ----------------------------------------------------------------- */

static struct oud_row rows[MAX_PARTS];
static int nrows, offers, state;               /* state: 0 not checked yet, 1 checked, -1 the index couldn't be had */
static int fresh, more;                        /* parts[fresh] on: the more parts not installed */

static olui *ui;
static olui_amiga *glue;

static void show(void);

static void say(const char *text)              /* the status line now, before something slow */
{
    if (!ui) return;
    olui_set_text(ui, olui_find(ui, "status"), text);
    olui_amiga_draw(glue);
}

/* The index fetched and weighed: offers and rows set, state 1; or -1 when it couldn't be had (try again later).
 * asked: the user asked (Check now), so versions said no to are offered again. */
static int check(int asked)
{
    char url[300], cmd[600], *index, *manifest, *skipped;
    long ilen, mlen = 0, slen = 0;
    int count;
    BPTR l;

    say("Checking for updates...");
    clear_work();
    if ((l = CreateDir((STRPTR)OUD_WORK))) UnLock(l);
    index = NULL;
    if (oud_url(set.url, OUD_INDEX_NAME, url, sizeof url) && oud_fetch_cmd(url, INDEX_FILE, cmd, sizeof cmd) && run(cmd) == 0)
        index = read_file(INDEX_FILE, &ilen);
    count = index ? oud_index_parts(index, ilen, parts, MAX_PARTS) : -1;
    if (index) FreeVec(index);
    clear_work();
    manifest = read_file(MANIFEST, &mlen);
    if (count < 0) {                           /* the parts installed are still shown, with nothing newer */
        offers = more = 0;
        nrows = oud_rows(manifest, mlen, parts, 0, rows, MAX_PARTS);
        if (manifest) FreeVec(manifest);
        state = -1;
        show();
        return -1;
    }
    skipped = asked ? NULL : read_file(OUD_SKIPPED, &slen);
    offers = manifest ? oud_offers(parts, count, manifest, mlen, skipped, slen) : 0;
    fresh = offers;
    more = oud_new_parts(parts, fresh, count, manifest, mlen);
    nrows = oud_rows(manifest, mlen, parts, offers, rows, MAX_PARTS);
    nrows = oud_rows_add_new(rows, nrows, MAX_PARTS, parts + fresh, more);
    if (manifest) FreeVec(manifest);
    if (skipped) FreeVec(skipped);
    state = 1;
    show();
    return 0;
}

/* Update: each offer fetched, checked, unpacked and installed; then what didn't go, and the list again. */
static void update_all(void)
{
    char *res = AllocVec(4096, MEMF_CLEAR);
    int i, bad = 0, len = 0;
    if (!res) return;
    for (i = 0; i < offers; i++) {
        char why[80], line[120];
        strcpy(line, "Updating ");
        strncat(line, parts[i].name, 40);
        strcat(line, "...");
        say(line);
        why[0] = 0;
        if (update_part(&parts[i], why, sizeof why - 1)) {
            why[sizeof why - 1] = 0;
            bad++;
            len = oud_put(res, 4096, len, parts[i].name);
            len = oud_put(res, 4096, len, ": ");
            len = oud_put(res, 4096, len, why);
            len = oud_put(res, 4096, len, "\n");
        }
    }
    clear_work();
    if (bad) ask("Not updated:\n\n%s", "OK", (ULONG)res);
    else ask("GFOS is updated.\nSome changes need a reboot to start.", "OK", 0);
    FreeVec(res);
    check(1);
}

/* Skip these versions: not offered again until a newer one comes. */
static void skip_all(void)
{
    long slen = 0, mlen = 0;
    char *old = read_file(OUD_SKIPPED, &slen), *text = AllocVec(4096, MEMF_ANY), *manifest;
    int n;
    if (text && (n = oud_skip_text(parts, offers, old, slen, text, 4096)) >= 0) write_file(OUD_SKIPPED, text, n);
    if (text) FreeVec(text);
    if (old) FreeVec(old);
    manifest = read_file(MANIFEST, &mlen);
    offers = 0;
    nrows = oud_rows(manifest, mlen, parts, 0, rows, MAX_PARTS);
    nrows = oud_rows_add_new(rows, nrows, MAX_PARTS, parts + fresh, more);
    if (manifest) FreeVec(manifest);
    show();
}

/* ---- the window (GFOpenLayout UI, the look every GFOpenUp program has) ---------------------------------- */

static const char markup[] =
    "<column style='flex:1'>"
    "  <p id=head>The GFOS parts installed, the newer versions there are, and the parts not installed.</p>"
    "  <list id=list style='flex:1;height:auto' title='Parts installed'></list>"
    "  <p id=status>Not checked yet.</p>"
    "  <row>"
    "    <button id=update style='flex:1' title='Downloads, checks and installs the newer versions'>Update</button>"
    "    <button id=skip style='flex:1' title='Not offered again until a newer version comes'>Skip these</button>"
    "    <button id=check style='flex:1' title='Looks for updates now'>Check now</button>"
    "    <button id=later style='flex:1' title='Closes the window'>Not now</button>"
    "  </row>"
    "</column>";

static const char css[] = "row{gap:6px} column{gap:6px} #head{font-weight:bold} p{margin:2px 0}";

enum { A_NONE, A_UPDATE, A_SKIP, A_CHECK, A_CLOSE };
static int action;

static void on_update(olui *u, const olui_event *ev, void *d) { (void)u; (void)ev; (void)d; action = A_UPDATE; }
static void on_skip(olui *u, const olui_event *ev, void *d) { (void)u; (void)ev; (void)d; action = A_SKIP; }
static void on_check(olui *u, const olui_event *ev, void *d) { (void)u; (void)ev; (void)d; action = A_CHECK; }
static void on_later(olui *u, const olui_event *ev, void *d) { (void)u; (void)ev; (void)d; action = A_CLOSE; }

static void show(void)
{
    olui_el *list;
    int i;
    char t[120];
    if (!ui) return;
    list = olui_find(ui, "list");
    olui_list_clear(ui, list);
    for (i = 0; i < nrows; i++) {
        oud_row_text(&rows[i], t, sizeof t);
        olui_list_add(ui, list, NULL, t, NULL);
    }
    olui_set_text(ui, olui_find(ui, "status"), oud_status_more(state, offers, more));
    olui_set_disabled(ui, olui_find(ui, "update"), offers == 0);
    olui_set_disabled(ui, olui_find(ui, "skip"), offers == 0);
    olui_amiga_draw(glue);
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
    w = fh * 40;
    h = fh * 22;
    if (w > scr->Width) w = scr->Width;
    if (h > scr->Height) h = scr->Height;
    win = OpenWindowTags(NULL, WA_PubScreen, (ULONG)scr, WA_Title, (ULONG)"GFOS Updates",
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
    olui_on(ui, "update", OLUI_CLICK, on_update, NULL);
    olui_on(ui, "skip", OLUI_CLICK, on_skip, NULL);
    olui_on(ui, "check", OLUI_CLICK, on_check, NULL);
    olui_on(ui, "later", OLUI_CLICK, on_later, NULL);
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

/* ---- settings -------------------------------------------------------------------------------------------- */

static void take(const char *key, const char *value)
{
    if (!value) return;
    if (!Stricmp((STRPTR)key, (STRPTR)"INTERVAL")) set.interval = oud_interval(value);
    else if (!Stricmp((STRPTR)key, (STRPTR)"URL")) {
        char t[300];
        if (oud_url(value, "x", t, sizeof t) && strlen(value) < sizeof set.url) strcpy(set.url, value);
    } else if (!Stricmp((STRPTR)key, (STRPTR)"CX_PRIORITY")) { LONG v; if (StrToLong((STRPTR)value, &v) > 0) set.cxpri = v; }
}

static const char *const KEYS[] = { "INTERVAL", "URL", "CX_PRIORITY" };
#define NKEYS (sizeof KEYS / sizeof KEYS[0])

static void from_icon(struct DiskObject *d)
{
    unsigned i;
    if (!d || !d->do_ToolTypes) return;
    for (i = 0; i < NKEYS; i++)
        take(KEYS[i], (const char *)FindToolType((CONST_STRPTR *)d->do_ToolTypes, (STRPTR)KEYS[i]));
}

static void settings(int argc, char **argv)
{
    if (argc == 0) {                        /* from Workbench: its icon */
        struct WBStartup *wbs = (struct WBStartup *)argv;
        if (IconBase && wbs && wbs->sm_NumArgs > 0) {
            BPTR old = CurrentDir(wbs->sm_ArgList[0].wa_Lock);
            struct DiskObject *d = GetDiskObject((STRPTR)wbs->sm_ArgList[0].wa_Name);
            from_icon(d);
            if (d) FreeDiskObject(d);
            CurrentDir(old);
        }
    } else {                                /* from a Shell: the icon beside it, then the line */
        char prog[256], icon[300];
        LONG a[NKEYS] = { 0, 0, 0 };
        struct RDArgs *rd;
        unsigned i;
        if (IconBase && GetProgramName((STRPTR)prog, sizeof prog)) {
            struct DiskObject *d;
            strcpy(icon, "PROGDIR:");
            strncat(icon, (const char *)FilePart((STRPTR)prog), sizeof icon - 10);
            if ((d = GetDiskObject((STRPTR)icon))) { from_icon(d); FreeDiskObject(d); }
        }
        if ((rd = ReadArgs((STRPTR)"INTERVAL/K,URL/K,CX_PRIORITY/K", a, NULL))) {
            for (i = 0; i < NKEYS; i++) take(KEYS[i], (const char *)a[i]);
            FreeArgs(rd);
        }
    }
}

/* ---- the commodity --------------------------------------------------------------------------------------- */

static struct NewBroker nb = {
    NB_VERSION, (STRPTR)NAME, (STRPTR)NAME " " VERSION_TEXT, (STRPTR)"Offers GFOS's updates (Show: its window)",
    NBU_UNIQUE | NBU_NOTIFY, COF_SHOW_HIDE, 0, NULL, 0
};

int main(int argc, char **argv)
{
    struct MsgPort *port = NULL, *tport = NULL;
    struct timerequest *tr = NULL;
    CxObj *broker = NULL;
    LONG err = 0;
    int timer = 0, tpending = 0, quit = 0, active = 1, checked = 0, first = 1, rc = RETURN_FAIL;
    long last = 0;

    CxBase = OpenLibrary((STRPTR)"commodities.library", 37);
    IconBase = OpenLibrary((STRPTR)"icon.library", 37);
    IntuitionBase = (struct IntuitionBase *)OpenLibrary((STRPTR)"intuition.library", 37);
    UtilityBase = OpenLibrary((STRPTR)"utility.library", 37);
    if (!CxBase || !IntuitionBase || !UtilityBase) goto out;
    settings(argc, argv);
    {
        long len;
        char *t = read_file(OUD_LAST, &len);
        if (t) { last = oud_last(t, len); FreeVec(t); }
    }

    RexxSysBase = (struct RxsLib *)OpenLibrary((STRPTR)"rexxsyslib.library", 0);   /* only for the notice */
    if (RexxSysBase && (rxport = CreateMsgPort())) {
        rxport->mp_Node.ln_Name = (char *)OUD_PORT;
        rxport->mp_Node.ln_Pri = 0;
        Forbid();
        if (FindPort((STRPTR)OUD_PORT)) { Permit(); DeleteMsgPort(rxport); rxport = NULL; }
        else { AddPort(rxport); Permit(); }
    }
    if (!(port = CreateMsgPort())) goto out;
    nb.nb_Port = port;
    nb.nb_Pri = (BYTE)set.cxpri;
    if (!(broker = CxBroker(&nb, &err))) {     /* running already: that one checks now (CXCMD_UNIQUE) */
        if (err == CBERR_DUP) rc = RETURN_OK;
        goto out;
    }
    ActivateCxObj(broker, 1);
    if (!(tport = CreateMsgPort()) || !(tr = (struct timerequest *)CreateIORequest(tport, sizeof *tr))) goto out;
    if (OpenDevice((STRPTR)TIMERNAME, UNIT_VBLANK, (struct IORequest *)tr, 0)) goto out;
    timer = 1;
    rc = RETURN_OK;

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
            if (active && oud_due(network_up(), checked, now, last, set.interval) && check(0) == 0) {
                save_last(now);
                checked = 1;
                last = now;
                if (offers && !win && !notify(offers)) open_win();   /* never installs: it says so, and waits */
            }
        }
        if (now_asked) {                           /* Exchange's Show, or started again: the window, checked now */
            if (open_win() && check(1) == 0) { checked = 1; last = now_seconds(); save_last(last); }
        }
        switch (action) {
        case A_UPDATE: action = A_NONE; update_all(); break;
        case A_SKIP: action = A_NONE; skip_all(); break;
        case A_CHECK: action = A_NONE; if (check(1) == 0) { checked = 1; last = now_seconds(); save_last(last); } break;
        case A_CLOSE: action = A_NONE; close_win(); break;
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
