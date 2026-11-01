/* GFOpenUpMenu: GFOpenUp's programs and settings in Workbench's Tools menu
 * (DESIGN.md section 7).
 *
 *   GFOpenUpMenu          adds two sub-menus to Tools, "GFOpenUp" and
 *                       "Preferences", and waits for them to be chosen.
 *                       Started from GFOpenUp's block in S:User-Startup.
 *   GFOpenUpMenu          run again: the running one builds its menus anew
 *                       (after an install, say), and the new one ends
 *   GFOpenUpMenu QUIT     the running one takes its menus out and ends;
 *                       Ctrl-C (Break in the Shell) does the same
 *   GFOpenUpMenu LIST     what the menus hold on this Amiga, in the Shell, and
 *                       (0.1.6) whether the running one has them in
 *                       Workbench's Tools menu or waits for a Workbench
 *                       that takes them
 *
 * Each menu shows only what is installed: an entry whose program isn't
 * there isn't shown. Choosing one starts the program as a double-click
 * does (OpenWorkbenchObjectA, so its icon's stack and tool types count);
 * a GFOpenPrefs page opens its icon in SYS:Preferences, else
 * SYS:Preferences/GFOpenPrefs at that page (0.1.5, the no-ROM-volume design,
 * section 10);
 * icons selected on Workbench at the time go with it as files to open
 * (GFOpenUp menu only, not GFOpenEdit's script). "GFOpenUp Update..." runs GFOpenUpTool UPDATE in a
 * window, where it shows what changes and asks first.
 *
 * Workbench's sub-menus need workbench.library 44.1511 (OS 3.5 and later;
 * OS 3.2 has them). Older ones get flat entries, "GFOpenUp: GFOpenFiles".
 *
 * What is installed is looked for once (0.1.6): at the start, and again
 * only when GFOpenUp's list of installed files changes (a notification on
 * ENVARC:GFOpenAmiga/GFOpenUp.manifest, which every install and uninstall
 * writes, where the disk's file system sends one: AmiCradle's host
 * drives don't) or when GFOpenUpMenu is run again, as the Installer does
 * at its end. S:User-Startup runs before
 * LoadWB, so until Workbench takes the entries GFOpenUpMenu offers them
 * again, every two seconds for the first minute, then every minute; that
 * touches no file. Up to 0.1.5
 * every try looked for every entry's file again: on the open ROM, whose
 * desktop (GFOpenBench) takes no menu entries yet, that was 36 Locks and a
 * read of GFOpenUpTool every two seconds for as long as the Amiga ran. Once
 * Workbench has the entries, GFOpenUpMenu only waits on its ports.
 *
 * MIT, Copyright (c) 2026 AmigaChrome Team. */
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/notify.h>
#include <devices/timer.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/wb.h>
#include <proto/icon.h>

#include <string.h>

static const char version[] __attribute__((used)) = "$VER: GFOpenUpMenu 0.1.6 (24.10.2026) MIT, Copyright (c) 2026 AmigaChrome Team";

struct Library *WorkbenchBase, *IconBase;

#define PORT_NAME "GFOpenUpMenu"
#define OPENPREFS "SYS:Preferences/GFOpenPrefs"
#define MANIFEST "ENVARC:GFOpenAmiga/GFOpenUp.manifest"   /* GFOpenUpTool writes it at every install and uninstall */

enum { M_OPENUP, M_PREFS, NMENUS };
enum { HOW_WB, HOW_CON, HOW_SEP, HOW_PAGE };

/* path: what Workbench opens; cli: the Shell command for a window, or for a
 * Workbench too old for OpenWorkbenchObjectA; needs: what must be there for
 * the entry to show (path when NULL); marker: a string that file must hold.
 * HOW_PAGE, a GFOpenPrefs page (0.1.5): path is its project icon in
 * SYS:Preferences (the icon alone, no file), cli its GFOpenPrefs SECTION word;
 * it shows when the icon or SYS:Preferences/GFOpenPrefs is there. */
struct entry {
    UBYTE menu, how;
    const char *label, *path, *cli, *needs, *marker;
};

static const char *const titles[NMENUS] = { "GFOpenUp", "Preferences" };

static const struct entry entries[] = {
    { M_OPENUP, HOW_WB,  "GFOpenFiles",    "SYS:Utilities/GFOpenFiles",              NULL, NULL, NULL },
    { M_OPENUP, HOW_WB,  "GFOpenEdit",     "SYS:Tools/GFOpenEdit",                   "C:GFOpenEdit", "SYS:C/GFOpenEdit", NULL },
    { M_OPENUP, HOW_WB,  "GFOpenWrite",    "SYS:Utilities/GFOpenWrite/GFOpenWrite",    NULL, NULL, NULL },
    { M_OPENUP, HOW_WB,  "GFOpenPlay",     "SYS:Utilities/GFOpenPlay",               NULL, NULL, NULL },
    { M_OPENUP, HOW_WB,  "GFOpenView",     "SYS:Utilities/GFOpenPrint/GFOpenView",     NULL, NULL, NULL },
    { M_OPENUP, HOW_WB,  "GFOpenCompress", "SYS:Utilities/GFOpenCompress",           NULL, NULL, NULL },
    { M_OPENUP, HOW_WB,  "GFOpenMail",     "SYS:Utilities/GFOpenMail/GFOpenMail",      NULL, NULL, NULL },
    { M_OPENUP, HOW_WB,  "OpenBrowser",  "SYS:Utilities/OpenBrowser/OpenBrowser", NULL, NULL, NULL },
    { M_OPENUP, HOW_WB,  "OpenBrowser Lite", "SYS:Utilities/OpenBrowserLite/OpenBrowserLite", NULL, NULL, NULL },
    { M_OPENUP, HOW_WB,  "GFOpenFTP",      "SYS:Utilities/GFOpenFTP/GFOpenFTP",        NULL, NULL, NULL },
    { M_OPENUP, HOW_WB,  "Telegram (unofficial)", "SYS:Utilities/UnofficialTelegram", NULL, NULL, NULL },
    { M_OPENUP, HOW_SEP, "---",          NULL, NULL, NULL, NULL },
    /* only with a GFOpenUpTool that has UPDATE (GFOpenUp Update, DESIGN.md section 5) */
    { M_OPENUP, HOW_CON, "GFOpenUp Update...", NULL, "SYS:C/GFOpenUpTool UPDATE", "SYS:C/GFOpenUpTool", "UPDATE [CHECK" },
    /* 0.1.5: GFOpenPrefs's pages by their plain names, in SYS:Preferences (the no-ROM-volume design, section 8): the
     * old editors' names in SYS:Prefs (GFOpenLook, GFOpenDock, OpenSound...) were stubs of GFOpenPrefs, and GFOpenUp
     * archives them */
    { M_PREFS,  HOW_PAGE, "Theme",       "SYS:Preferences/Theme",                "THEME", NULL, NULL },
    { M_PREFS,  HOW_PAGE, "Dock",        "SYS:Preferences/Dock",                 "DOCK", NULL, NULL },
    { M_PREFS,  HOW_PAGE, "Sound",       "SYS:Preferences/Sound",                "SOUND", NULL, NULL },
    { M_PREFS,  HOW_PAGE, "Date & Time", "SYS:Preferences/Date & Time",          "DATETIME", NULL, NULL },
    { M_PREFS,  HOW_PAGE, "Input",       "SYS:Preferences/Input",                "INPUT", NULL, NULL },
    { M_PREFS,  HOW_PAGE, "Gamepads",    "SYS:Preferences/Gamepads",             "GAMEPADS", NULL, NULL },
    { M_PREFS,  HOW_PAGE, "Serial",      "SYS:Preferences/Serial",               "SERIAL", NULL, NULL },
    { M_PREFS,  HOW_WB,  "Printers",     "SYS:Utilities/GFOpenPrint/Printers and Queue", NULL, NULL, NULL },
    { M_PREFS,  HOW_WB,  "Network",      "SYS:Tools/Commodities/GFOpenSocketControl", NULL, NULL, NULL },
    { M_PREFS,  HOW_PAGE, "File Types",  "SYS:Preferences/File Types",           "TYPES", NULL, NULL },
    { M_PREFS,  HOW_WB,  "Tata",         "SYS:Prefs/GFOpenTata",                   NULL, NULL, NULL },
    { M_PREFS,  HOW_WB,  "GFOpenUp Setup", "SYS:Prefs/GFOpenUp-Setup",               NULL, NULL, NULL },
};
#define NENTRIES (sizeof entries / sizeof entries[0])
#define MAXARGS 16

static struct MsgPort *appport;
static struct AppMenuItem *items[NENTRIES], *parents[NMENUS];
static char flat_labels[NENTRIES][40];
static UBYTE shown[NENTRIES];

struct command {
    struct Message msg;
    LONG what;                          /* 1 build anew, 2 quit, 3 (0.1.6) say where the entries are */
    LONG answer;                        /* to 3: 1 in Workbench's menu, 2 waiting for a Workbench; 0 from 0.1.5;
                                         * | 4 when the list's notification is on */
};

/* ---- what is installed ------------------------------------------------------ */

static int exists(const char *path)
{
    BPTR l = Lock((STRPTR)path, ACCESS_READ);
    if (l) UnLock(l);
    return l != 0;
}

/* 1 when the file holds the marker; read in pieces, so it stays small. */
static int holds(const char *path, const char *marker)
{
    static char buf[2048];
    LONG n, keep = 0, ml = strlen(marker), k;
    BPTR fh = Open((STRPTR)path, MODE_OLDFILE);
    int found = 0;
    if (!fh)
        return 0;
    while (!found && (n = Read(fh, buf + keep, sizeof buf - keep)) > 0) {
        n += keep;
        for (k = 0; k + ml <= n; k++)
            if (buf[k] == marker[0] && !memcmp(buf + k, marker, ml)) { found = 1; break; }
        keep = n < ml ? n : ml - 1;            /* a marker across two pieces */
        memmove(buf, buf + n - keep, keep);
    }
    Close(fh);
    return found;
}

/* A GFOpenPrefs page's project icon is there (path.info). */
static int page_icon(const struct entry *e)
{
    char info[80];
    strcpy(info, e->path);
    strcat(info, ".info");
    return exists(info);
}

/* Which entries show: the installed ones, and a line only between two. It looks at the disk (each entry's file,
 * GFOpenUpTool's UPDATE): build() calls it once, and again only when the list of installed files changes. */
static int find_shown(void)
{
    int i, j, any = 0, openprefs = exists(OPENPREFS);
    for (i = 0; i < (int)NENTRIES; i++) {
        const struct entry *e = &entries[i];
        const char *needs = e->needs ? e->needs : e->path;
        if (e->how == HOW_PAGE) { shown[i] = openprefs || page_icon(e); continue; }
        shown[i] = e->how != HOW_SEP && (!e->path || exists(e->path)) && exists(needs)
                   && (!e->marker || holds(needs, e->marker));
    }
    for (i = 0; i < (int)NENTRIES; i++) {
        int before = 0, after = 0;
        if (entries[i].how != HOW_SEP)
            continue;
        for (j = 0; j < i; j++) before |= entries[j].menu == entries[i].menu && shown[j];
        for (j = i + 1; j < (int)NENTRIES; j++) after |= entries[j].menu == entries[i].menu && shown[j];
        shown[i] = before && after;
    }
    for (i = 0; i < (int)NENTRIES; i++) any |= shown[i];
    return any;
}

/* ---- the menus ---------------------------------------------------------------- */

static void take_out(void)
{
    int i;
    for (i = 0; i < (int)NENTRIES; i++)
        if (items[i]) { RemoveAppMenuItem(items[i]); items[i] = NULL; }
    for (i = 0; i < NMENUS; i++)
        if (parents[i]) { RemoveAppMenuItem(parents[i]); parents[i] = NULL; }
}

static int built, any_shown;            /* shown[] holds what is installed; any shows */

/* What is installed, looked for on the disk (0.1.6: once, and again when built is cleared) */
static void build(void)
{
    any_shown = find_shown();
    built = 1;
}

/* Adds the entries that show, as build() found them: no file is looked at here. 1 done (or nothing to add),
 * 0 Workbench didn't take them (not open yet, or a desktop that takes no menu entries): none are left in. */
static int put_in(void)
{
    int m, i, flat = WorkbenchBase->lib_Version < 44;
    take_out();
    if (!built)
        build();
    if (!any_shown)
        return 1;
    for (m = 0; m < NMENUS; m++) {
        ULONG key = 0;                      /* must start at 0 (wb.doc) */
        int any = 0;
        for (i = 0; i < (int)NENTRIES; i++) any |= entries[i].menu == m && shown[i];
        if (!any)
            continue;
        if (!flat) {
            /* the title shows once its first entry is attached */
            parents[m] = AddAppMenuItem(0xFFFF, 0, (STRPTR)titles[m], appport, WBAPPMENUA_GetKey, (ULONG)&key, TAG_DONE);
            if (!parents[m]) { take_out(); return 0; }
            if (!key) {                     /* workbench.library before 44.1511: no sub-menus */
                RemoveAppMenuItem(parents[m]);
                parents[m] = NULL;
                flat = 1;
            }
        }
        for (i = 0; i < (int)NENTRIES; i++) {
            if (entries[i].menu != m || !shown[i])
                continue;
            if (flat) {
                if (entries[i].how == HOW_SEP)
                    continue;
                strcpy(flat_labels[i], titles[m]);
                strcat(flat_labels[i], ": ");
                strcat(flat_labels[i], entries[i].label);
                items[i] = AddAppMenuItemA(i, 0, (STRPTR)flat_labels[i], appport, NULL);
            } else {
                items[i] = AddAppMenuItem(i, 0, (STRPTR)entries[i].label, appport, WBAPPMENUA_UseKey, key, TAG_DONE);
            }
            if (!items[i]) { take_out(); return 0; }
        }
    }
    return 1;
}

/* ---- starting a program ----------------------------------------------------------- */

/* The stack its icon asks for, at least 8 KB. */
static LONG icon_stack(const char *path)
{
    LONG stack = 8192;
    struct DiskObject *dob;
    if (IconBase && (dob = GetDiskObject((STRPTR)path))) {
        if (dob->do_StackSize > stack) stack = dob->do_StackSize;
        FreeDiskObject(dob);
    }
    return stack;
}

/* As the Shell would: in the program's own drawer, in the background. */
static void start_cli(const struct entry *e)
{
    char cmd[200], dir[160];
    BPTR in, out, lock, old;
    const char *path = e->path ? e->path : e->needs;
    LONG stack = icon_stack(path);
    if (e->how == HOW_CON) {
        in = Open((STRPTR)"CON:40/30/600/220/GFOpenUp Update/AUTO/CLOSE/WAIT", MODE_OLDFILE);
        out = 0;                            /* the same window (dos.doc, SystemTagList) */
    } else {
        in = Open((STRPTR)"NIL:", MODE_OLDFILE);
        out = Open((STRPTR)"NIL:", MODE_NEWFILE);
    }
    if (!in) { if (out) Close(out); return; }
    if (e->how == HOW_PAGE) {               /* GFOpenPrefs at its page: GFOpenPrefs SOUND */
        path = OPENPREFS;
        stack = icon_stack(path);
        strcpy(cmd, OPENPREFS " ");
        strcat(cmd, e->cli);
    } else if (e->cli) {
        strncpy(cmd, e->cli, sizeof cmd - 1);
        cmd[sizeof cmd - 1] = 0;
    } else {
        strcpy(cmd, "\"");
        strncat(cmd, path, sizeof cmd - 3);
        strcat(cmd, "\"");
    }
    strncpy(dir, path, sizeof dir - 1);
    dir[sizeof dir - 1] = 0;
    *PathPart((STRPTR)dir) = 0;
    lock = Lock((STRPTR)dir, ACCESS_READ);
    old = lock ? CurrentDir(lock) : 0;      /* the new Shell takes a copy */
    if (SystemTags((STRPTR)cmd, SYS_Input, in, SYS_Output, out, SYS_Asynch, TRUE,
                   SYS_UserShell, TRUE, NP_StackSize, stack, TAG_DONE) == -1) {
        Close(in);
        if (out) Close(out);
    }
    if (lock) { CurrentDir(old); UnLock(lock); }
}

/* Starts entry i, with the icons that were selected (GFOpenUp menu only). */
static void start(int i, BPTR *locks, char **names, int nargs)
{
    const struct entry *e = &entries[i];
    if (e->how == HOW_WB && WorkbenchBase->lib_Version >= 44) {
        struct TagItem tags[2 * MAXARGS + 1];
        int k, t = 0;
        for (k = 0; k < nargs; k++) {
            tags[t].ti_Tag = WBOPENA_ArgLock; tags[t++].ti_Data = (ULONG)locks[k];
            tags[t].ti_Tag = WBOPENA_ArgName; tags[t++].ti_Data = (ULONG)names[k];
        }
        tags[t].ti_Tag = TAG_DONE;
        if (OpenWorkbenchObjectA((STRPTR)e->path, tags))
            return;
    }
    /* a GFOpenPrefs page: its project icon, as a double-click on it; else (no icon, or a Workbench that didn't
     * open it) GFOpenPrefs at that page */
    if (e->how == HOW_PAGE && WorkbenchBase->lib_Version >= 44 && page_icon(e)
        && OpenWorkbenchObjectA((STRPTR)e->path, NULL))
        return;
    if (e->how != HOW_SEP)
        start_cli(e);
}

/* An AppMessage: what was chosen, and the icons selected with it, copied
 * (their locks are Workbench's until the reply). */
static void chosen(struct AppMessage *am)
{
    BPTR locks[MAXARGS];
    char *names[MAXARGS];
    int i, n = 0, id = am->am_Type == AMTYPE_APPMENUITEM ? (int)am->am_ID : -1;
    /* GFOpenEdit's entry opens a script (IconX), which takes no files */
    if (id >= 0 && id < (int)NENTRIES && entries[id].menu == M_OPENUP && entries[id].how == HOW_WB && !entries[id].cli)
        for (i = 0; i < am->am_NumArgs && n < MAXARGS; i++) {
            struct WBArg *a = &am->am_ArgList[i];
            const char *name = a->wa_Name ? (const char *)a->wa_Name : "";
            if (!a->wa_Lock || !(names[n] = AllocVec(strlen(name) + 1, MEMF_ANY)))
                continue;
            strcpy(names[n], name);
            if (!(locks[n] = DupLock(a->wa_Lock))) { FreeVec(names[n]); continue; }
            n++;
        }
    ReplyMsg((struct Message *)am);
    if (id >= 0 && id < (int)NENTRIES)
        start(id, locks, names, n);
    for (i = 0; i < n; i++) { UnLock(locks[i]); FreeVec(names[i]); }
}

/* ---- one GFOpenUpMenu at a time ---------------------------------------------------------- */

/* Tells a running GFOpenUpMenu what to do. 1 told, 0 none is running. */
static LONG told_answer;                /* the running one's answer to tell() */

static int tell(LONG what)
{
    struct MsgPort *reply = CreateMsgPort(), *port;
    struct command c;
    if (!reply)
        return 0;
    memset(&c, 0, sizeof c);
    c.msg.mn_ReplyPort = reply;
    c.msg.mn_Length = sizeof c;
    c.what = what;
    Forbid();
    if ((port = FindPort((STRPTR)PORT_NAME)))
        PutMsg(port, &c.msg);
    Permit();
    if (port)
        WaitPort(reply), GetMsg(reply);
    DeleteMsgPort(reply);
    told_answer = c.answer;
    return port != NULL;
}

static void list(void)
{
    int m, i;
    if (!find_shown()) {
        PutStr((STRPTR)"GFOpenUpMenu: no GFOpenUp programs or settings editors are installed here.\n");
        return;
    }
    for (m = 0; m < NMENUS; m++)
        for (i = 0; i < (int)NENTRIES; i++)
            if (entries[i].menu == m && shown[i])
                Printf((STRPTR)"Tools > %s > %s\n", (LONG)titles[m], (LONG)entries[i].label);
    if (!tell(3))
        PutStr((STRPTR)"GFOpenUpMenu isn't running.\n");
    else if (told_answer) {
        PutStr((STRPTR)((told_answer & 3) == 1 ? "The running GFOpenUpMenu has them in Workbench's Tools menu.\n"
                        : "The running GFOpenUpMenu waits for a Workbench that takes menu entries (GFOpenBench doesn't yet).\n"));
        PutStr((STRPTR)(told_answer & 4 ? "It looks again when GFOpenUp's list of installed files changes, or when GFOpenUpMenu runs again.\n"
                        : "It looks again when GFOpenUpMenu runs again (this disk sends no notification when GFOpenUp's list changes).\n"));
    }
}

/* The running one: menus in, then wait. */
/* Starts the timer for ticks fiftieths of a second (none running before) */
static void timer_start(struct timerequest *tr, ULONG ticks)
{
    tr->tr_node.io_Command = TR_ADDREQUEST;
    tr->tr_time.tv_secs = ticks / 50;
    tr->tr_time.tv_micro = (ticks % 50) * 20000;
    SendIO((struct IORequest *)tr);
}

/* The running one: menus in, then wait. 0.1.6: no polling of the disk. What is installed is looked for once
 * (build), and again only when GFOpenUp's list changes (a notification on MANIFEST) or GFOpenUpMenu is run again.
 * Until Workbench takes the entries they are offered again on a timer that touches no file: every two seconds
 * for the first minute (the boot: S:User-Startup runs before LoadWB), then every minute. On the open ROM,
 * GFOpenBench is the desktop and takes no menu entries yet (amigachrome-guest libraries/openworkbench): there a
 * minute's timer is all that runs. */
static int serve(int from_shell)
{
    struct MsgPort *port, *tport = NULL;
    struct timerequest *tr = NULL;
    struct NotifyRequest nr;
    struct Message *msg;
    ULONG appsig, cmdsig, tsig = 0, nsig = 0;
    LONG nbit = -1;
    int in = 0, quit = 0, ticking = 0, notifying = 0, tries = 0;
    BPTR sys;
    if (!(appport = CreateMsgPort()) || !(port = CreateMsgPort())) {
        if (appport) DeleteMsgPort(appport);
        return RETURN_FAIL;
    }
    port->mp_Node.ln_Name = (char *)PORT_NAME;
    Forbid();
    if (FindPort((STRPTR)PORT_NAME)) {      /* another started at the same moment */
        Permit();
        DeleteMsgPort(port);
        DeleteMsgPort(appport);
        return tell(1) ? RETURN_OK : RETURN_FAIL;
    }
    AddPort(port);
    Permit();
    /* Run from the Installer, the current drawer is the pack's: hold SYS:
     * instead, so the pack's disk or drawer isn't kept busy. */
    if (from_shell && (sys = Lock((STRPTR)"SYS:", ACCESS_READ)))
        UnLock(CurrentDir(sys));
    appsig = 1UL << appport->mp_SigBit;
    cmdsig = 1UL << port->mp_SigBit;
    if ((tport = CreateMsgPort()) && (tr = (struct timerequest *)CreateIORequest(tport, sizeof *tr))
        && !OpenDevice((STRPTR)TIMERNAME, UNIT_VBLANK, (struct IORequest *)tr, 0))
        tsig = 1UL << tport->mp_SigBit;
    else {
        if (tr) DeleteIORequest((struct IORequest *)tr);
        tr = NULL;
    }
    /* GFOpenUp's list of installed files: written at every install and uninstall, so a change means the menus may */
    memset(&nr, 0, sizeof nr);
    if ((nbit = AllocSignal(-1)) >= 0) {
        nr.nr_Name = (STRPTR)MANIFEST;
        nr.nr_Flags = NRF_SEND_SIGNAL;
        nr.nr_stuff.nr_Signal.nr_Task = FindTask(NULL);
        nr.nr_stuff.nr_Signal.nr_SignalNum = (UBYTE)nbit;
        if (StartNotify(&nr)) { notifying = 1; nsig = 1UL << nbit; }
    }
    while (!quit) {
        ULONG got;
        if (!in && !(in = put_in()) && !ticking) {
            /* Workbench hasn't taken them: offered again later, with what is installed known already */
            if (tr) { timer_start(tr, tries < 30 ? 100 : 3000); ticking = 1; }
            tries++;
        }
        if (in && ticking) {                /* in: no more tries */
            AbortIO((struct IORequest *)tr);
            WaitIO((struct IORequest *)tr);
            ticking = 0;
        }
        if (!in && !tr) {                   /* no timer.device: as before 0.1.6, two seconds at a time */
            Delay(100);
            got = SetSignal(0, cmdsig | nsig | SIGBREAKF_CTRL_C);
        } else
            got = Wait(appsig | cmdsig | tsig | nsig | SIGBREAKF_CTRL_C);
        if (got & SIGBREAKF_CTRL_C)
            quit = 1;
        if ((got & tsig) && ticking && GetMsg(tport))
            ticking = 0;                    /* the time to offer them again (at the top of the loop) */
        if (got & nsig) { built = 0; in = 0; tries = 0; }   /* installed or uninstalled: looked for again */
        while ((msg = GetMsg(appport)))
            chosen((struct AppMessage *)msg);
        while ((msg = GetMsg(port))) {
            LONG what = ((struct command *)msg)->what;
            if (what == 3) ((struct command *)msg)->answer = (in ? 1 : 2) | (notifying ? 4 : 0);
            ReplyMsg(msg);
            if (what == 2) quit = 1;
            else if (what == 1) { built = 0; in = 0; tries = 0; }   /* build anew: what is installed may have changed */
        }
    }
    take_out();
    if (notifying) EndNotify(&nr);
    if (nbit >= 0) FreeSignal(nbit);
    if (tr) {
        if (ticking) { AbortIO((struct IORequest *)tr); WaitIO((struct IORequest *)tr); }
        CloseDevice((struct IORequest *)tr);
        DeleteIORequest((struct IORequest *)tr);
    }
    if (tport) DeleteMsgPort(tport);
    Forbid();
    RemPort(port);
    Permit();
    while ((msg = GetMsg(port))) ReplyMsg(msg);
    DeleteMsgPort(port);
    while ((msg = GetMsg(appport))) ReplyMsg(msg);   /* chosen while leaving: Workbench gets its messages back */
    DeleteMsgPort(appport);
    return RETURN_OK;
}

int main(int argc, char **argv)
{
    LONG args[2] = { 0, 0 };
    struct RDArgs *rda = NULL;
    int rc;
    (void)argv;
    if (argc > 0 && !(rda = ReadArgs((STRPTR)"QUIT/S,LIST/S", args, NULL))) {
        PutStr((STRPTR)"Usage: GFOpenUpMenu [QUIT] [LIST]\n");
        return RETURN_FAIL;
    }
    if (!(WorkbenchBase = OpenLibrary((STRPTR)"workbench.library", 37))) {
        if (rda) FreeArgs(rda);
        return RETURN_FAIL;
    }
    IconBase = OpenLibrary((STRPTR)"icon.library", 37);
    if (args[1])
        list(), rc = RETURN_OK;
    else if (args[0])
        rc = tell(2) ? RETURN_OK : (PutStr((STRPTR)"GFOpenUpMenu isn't running.\n"), RETURN_WARN);
    else if (tell(1))
        rc = RETURN_OK;                     /* the running one builds its menus anew */
    else
        rc = serve(argc > 0);
    if (IconBase) CloseLibrary(IconBase);
    CloseLibrary(WorkbenchBase);
    if (rda) FreeArgs(rda);
    (void)version;
    return rc;
}
