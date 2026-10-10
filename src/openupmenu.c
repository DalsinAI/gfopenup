/* OpenUpMenu: OpenUp's programs and settings in Workbench's Tools menu
 * (DESIGN.md section 7).
 *
 *   OpenUpMenu          adds two sub-menus to Tools, "OpenUp" and
 *                       "Preferences", and waits for them to be chosen.
 *                       Started from OpenUp's block in S:User-Startup.
 *   OpenUpMenu          run again: the running one builds its menus anew
 *                       (after an install, say), and the new one ends
 *   OpenUpMenu QUIT     the running one takes its menus out and ends;
 *                       Ctrl-C (Break in the Shell) does the same
 *   OpenUpMenu LIST     what the menus hold on this Amiga, in the Shell
 *
 * Each menu shows only what is installed: an entry whose program isn't
 * there isn't shown. Choosing one starts the program as a double-click
 * does (OpenWorkbenchObjectA, so its icon's stack and tool types count);
 * icons selected on Workbench at the time go with it as files to open
 * (OpenUp menu only, not OpenEdit's script). "OpenUp Update..." runs OpenUpTool UPDATE in a
 * window, where it shows what changes and asks first.
 *
 * Workbench's sub-menus need workbench.library 44.1511 (OS 3.5 and later;
 * OS 3.2 has them). Older ones get flat entries, "OpenUp: OpenFiles".
 * S:User-Startup runs before LoadWB, so OpenUpMenu tries every two seconds
 * until Workbench takes its entries; after that it only waits on its
 * ports, with no polling.
 *
 * MIT, Copyright (c) 2026 Dalsin Limited. */
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/wb.h>
#include <proto/icon.h>

#include <string.h>

static const char version[] __attribute__((used)) = "$VER: OpenUpMenu 0.1.4 (10.10.2026) MIT, Copyright (c) 2026 Dalsin Limited";

struct Library *WorkbenchBase, *IconBase;

#define PORT_NAME "OpenUpMenu"

enum { M_OPENUP, M_PREFS, NMENUS };
enum { HOW_WB, HOW_CON, HOW_SEP };

/* path: what Workbench opens; cli: the Shell command for a window, or for a
 * Workbench too old for OpenWorkbenchObjectA; needs: what must be there for
 * the entry to show (path when NULL); marker: a string that file must hold. */
struct entry {
    UBYTE menu, how;
    const char *label, *path, *cli, *needs, *marker;
};

static const char *const titles[NMENUS] = { "OpenUp", "Preferences" };

static const struct entry entries[] = {
    { M_OPENUP, HOW_WB,  "OpenFiles",    "SYS:Utilities/OpenFiles",              NULL, NULL, NULL },
    { M_OPENUP, HOW_WB,  "OpenEdit",     "SYS:Tools/OpenEdit",                   "C:OpenEdit", "SYS:C/OpenEdit", NULL },
    { M_OPENUP, HOW_WB,  "OpenWrite",    "SYS:Utilities/OpenWrite/OpenWrite",    NULL, NULL, NULL },
    { M_OPENUP, HOW_WB,  "OpenPlay",     "SYS:Utilities/OpenPlay",               NULL, NULL, NULL },
    { M_OPENUP, HOW_WB,  "OpenView",     "SYS:Utilities/OpenPrint/OpenView",     NULL, NULL, NULL },
    { M_OPENUP, HOW_WB,  "OpenCompress", "SYS:Utilities/OpenCompress",           NULL, NULL, NULL },
    { M_OPENUP, HOW_WB,  "OpenMail",     "SYS:Utilities/OpenMail/OpenMail",      NULL, NULL, NULL },
    { M_OPENUP, HOW_WB,  "OpenBrowser",  "SYS:Utilities/OpenBrowser/OpenBrowser", NULL, NULL, NULL },
    { M_OPENUP, HOW_WB,  "OpenFTP",      "SYS:Utilities/OpenFTP/OpenFTP",        NULL, NULL, NULL },
    { M_OPENUP, HOW_WB,  "Telegram (unofficial)", "SYS:Utilities/UnofficialTelegram", NULL, NULL, NULL },
    { M_OPENUP, HOW_SEP, "---",          NULL, NULL, NULL, NULL },
    /* only with an OpenUpTool that has UPDATE (OpenUp Update, DESIGN.md section 5) */
    { M_OPENUP, HOW_CON, "OpenUp Update...", NULL, "SYS:C/OpenUpTool UPDATE", "SYS:C/OpenUpTool", "UPDATE [CHECK" },
    { M_PREFS,  HOW_WB,  "Look",         "SYS:Prefs/OpenLook",                   NULL, NULL, NULL },   /* 0.1.3: Windows, Title bar, Menus are its pages */
    /* 0.1.4: every editor of ours in Prefs is Open... (the user, 10 October 2026); the OS's own are in Classic */
    { M_PREFS,  HOW_WB,  "Dock",         "SYS:Prefs/OpenDock",                   NULL, NULL, NULL },
    { M_PREFS,  HOW_WB,  "Sound",        "SYS:Prefs/OpenSound",                  NULL, NULL, NULL },
    { M_PREFS,  HOW_WB,  "Date and time", "SYS:Prefs/OpenDateTime",              NULL, NULL, NULL },
    { M_PREFS,  HOW_WB,  "Input",        "SYS:Prefs/OpenInput",                  NULL, NULL, NULL },
    { M_PREFS,  HOW_WB,  "Serial",       "SYS:Prefs/OpenSerial",                 NULL, NULL, NULL },
    { M_PREFS,  HOW_WB,  "Printers",     "SYS:Utilities/OpenPrint/Printers and Queue", NULL, NULL, NULL },
    { M_PREFS,  HOW_WB,  "Network",      "SYS:Tools/Commodities/OpenSocketControl", NULL, NULL, NULL },
    { M_PREFS,  HOW_WB,  "OpenTypes",    "SYS:Prefs/OpenTypes",                  NULL, NULL, NULL },
    { M_PREFS,  HOW_WB,  "Tata",         "SYS:Prefs/OpenTata",                   NULL, NULL, NULL },   /* Gamepads: under Input (OpenInput) since 0.1.3 */
    { M_PREFS,  HOW_WB,  "OpenUp Setup", "SYS:Prefs/OpenUp-Setup",               NULL, NULL, NULL },
};
#define NENTRIES (sizeof entries / sizeof entries[0])
#define MAXARGS 16

static struct MsgPort *appport;
static struct AppMenuItem *items[NENTRIES], *parents[NMENUS];
static char flat_labels[NENTRIES][40];
static UBYTE shown[NENTRIES];

struct command {
    struct Message msg;
    LONG what;                          /* 1 build anew, 2 quit */
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

/* Which entries show: the installed ones, and a line only between two. */
static int find_shown(void)
{
    int i, j, any = 0;
    for (i = 0; i < (int)NENTRIES; i++) {
        const struct entry *e = &entries[i];
        const char *needs = e->needs ? e->needs : e->path;
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

/* Adds the entries that show. 1 done (or nothing to add), 0 Workbench
 * didn't take them (not open yet): none are left in. */
static int put_in(void)
{
    int m, i, flat = WorkbenchBase->lib_Version < 44;
    take_out();
    if (!find_shown())
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
        in = Open((STRPTR)"CON:40/30/600/220/OpenUp Update/AUTO/CLOSE/WAIT", MODE_OLDFILE);
        out = 0;                            /* the same window (dos.doc, SystemTagList) */
    } else {
        in = Open((STRPTR)"NIL:", MODE_OLDFILE);
        out = Open((STRPTR)"NIL:", MODE_NEWFILE);
    }
    if (!in) { if (out) Close(out); return; }
    if (e->cli) {
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

/* Starts entry i, with the icons that were selected (OpenUp menu only). */
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
    /* OpenEdit's entry opens a script (IconX), which takes no files */
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

/* ---- one OpenUpMenu at a time ---------------------------------------------------------- */

/* Tells a running OpenUpMenu what to do. 1 told, 0 none is running. */
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
    return port != NULL;
}

static void list(void)
{
    int m, i;
    if (!find_shown()) {
        PutStr((STRPTR)"OpenUpMenu: no OpenUp programs or settings editors are installed here.\n");
        return;
    }
    for (m = 0; m < NMENUS; m++)
        for (i = 0; i < (int)NENTRIES; i++)
            if (entries[i].menu == m && shown[i])
                Printf((STRPTR)"Tools > %s > %s\n", (LONG)titles[m], (LONG)entries[i].label);
}

/* The running one: menus in, then wait. */
static int serve(int from_shell)
{
    struct MsgPort *port;
    struct Message *msg;
    ULONG appsig, cmdsig;
    int in = 0, quit = 0;
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
    while (!quit) {
        ULONG got;
        if (!in && !(in = put_in())) {
            /* Workbench isn't open yet: try again in two seconds */
            Delay(100);
            got = SetSignal(0, cmdsig | SIGBREAKF_CTRL_C);
        } else {
            got = Wait(appsig | cmdsig | SIGBREAKF_CTRL_C);
        }
        if (got & SIGBREAKF_CTRL_C)
            quit = 1;
        while ((msg = GetMsg(appport)))
            chosen((struct AppMessage *)msg);
        while ((msg = GetMsg(port))) {
            LONG what = ((struct command *)msg)->what;
            ReplyMsg(msg);
            if (what == 2) quit = 1;
            else if (what == 1) in = 0;     /* build anew: what is installed may have changed */
        }
    }
    take_out();
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
        PutStr((STRPTR)"Usage: OpenUpMenu [QUIT] [LIST]\n");
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
        rc = tell(2) ? RETURN_OK : (PutStr((STRPTR)"OpenUpMenu isn't running.\n"), RETURN_WARN);
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
