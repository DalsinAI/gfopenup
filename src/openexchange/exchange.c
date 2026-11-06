/* GFOpenExchange 1.0: the commodities' Exchange (4 November 2026), its window in GFOpenLayout UI.
 *
 * The window lists every commodity running (its broker's name). Choosing one shows its title and description;
 * Show and Hide open and close its window (for a commodity that has one), Active and Inactive turn it on and
 * off, and Remove quits it. The list follows commodities starting and quitting, read again each second while
 * the window is open (commodities.library tells Exchange of no change: OS 3.2 sends no CXCMD_LIST_CHG, as
 * amicradle-guest's CommoditiesABITest measured).
 *
 * Its calls are commodities.library's private three that Exchange uses (-186 CopyBrokerList, -192
 * FreeBrokerList, -198 BrokerCommand), in OS 3.2's and GFOpenCommodities' library alike.
 *
 * It is a commodity itself, named Exchange as OS 3.2's is, so only one runs: started again, the one running
 * shows its window. Tool types (in its icon, or as KEY=VALUE on the command line, which wins):
 *   CX_POPKEY=ctrl alt help    the hotkey that shows the window; NONE for none
 *   CX_POPUP=YES               NO: start with the window hidden (the hotkey, or Exchange started again, shows it)
 *   CX_PRIORITY=0              the broker's priority
 * The close gadget and Project > Hide hide the window; Project > Quit quits.
 *
 * MIT, Copyright (c) 2026 AmiCradle Team. */

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <devices/timer.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <libraries/commodities.h>
#include <libraries/gadtools.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/commodities.h>
#include <proto/icon.h>
#include <proto/utility.h>
#include <clib/alib_protos.h>
#include <string.h>

#include "gfopenlayout_ui.h"
#include "gfopenlayout_ui_amiga.h"

#define NAME "GFOpenExchange"
#define VERSION_TEXT "1.0"
static const char version[] __attribute__((used)) = "$VER: GFOpenExchange 1.0 (4.11.2026) MIT, Copyright (c) 2026 AmiCradle Team";
unsigned long __stack = 32768;

struct Library *CxBase, *IconBase;
struct Library *UtilityBase;

#ifndef COF_ACTIVE
#define COF_ACTIVE 2            /* a BrokerCopy's bc_Flags: the broker is on */
#endif

/* ---- commodities.library's private calls --------------------------------------------------------- */

struct BrokerCopy {             /* what CopyBrokerList makes, a node a broker */
    struct Node bc_Node;
    char bc_Name[CBD_NAMELEN];
    char bc_Title[CBD_TITLELEN];
    char bc_Descr[CBD_DESCRLEN];
    LONG bc_Task;
    LONG bc_Port;
    UWORD bc_Dummy;
    ULONG bc_Flags;
};

static LONG cx_CopyBrokerList(struct List *l)
{
    register struct Library *a6 __asm("a6") = CxBase;
    register struct List *a0 __asm("a0") = l;
    register LONG d0 __asm("d0");
    __asm__ volatile("jsr -186(%%a6)" : "=r"(d0), "+r"(a0) : "r"(a6) : "d1", "a1", "cc", "memory");
    return d0;
}

static void cx_FreeBrokerList(struct List *l)
{
    register struct Library *a6 __asm("a6") = CxBase;
    register struct List *a0 __asm("a0") = l;
    __asm__ volatile("jsr -192(%%a6)" : "+r"(a0) : "r"(a6) : "d0", "d1", "a1", "cc", "memory");
}

static LONG cx_BrokerCommand(const char *name, LONG id)
{
    register struct Library *a6 __asm("a6") = CxBase;
    register const char *a0 __asm("a0") = name;
    register LONG d0 __asm("d0") = id;
    __asm__ volatile("jsr -198(%%a6)" : "+r"(d0), "+r"(a0) : "r"(a6) : "d1", "a1", "cc", "memory");
    return d0;
}

/* ---- the commodities, as last read ---------------------------------------------------------------- */

typedef struct {
    char name[CBD_NAMELEN], title[CBD_TITLELEN], descr[CBD_DESCRLEN];
    ULONG flags;
} cx_entry;

static cx_entry *cxs;           /* AllocVec'd, ncx of them */
static int ncx;
static char chosen[CBD_NAMELEN];    /* the broker chosen in the list ("" for none) */

static void copy_field(char *to, const char *from, int size)
{
    int i;
    for (i = 0; i < size - 1 && from[i]; i++) to[i] = from[i];
    to[i] = 0;
}

/* Reads the brokers into a new array: 1 when they differ from cxs (which is then replaced), 0 when they are the
 * same or memory ran out. */
static int read_brokers(void)
{
    struct List l;
    struct Node *n;
    cx_entry *e = NULL;
    int count = 0, i = 0, changed;
    NewList(&l);
    cx_CopyBrokerList(&l);
    for (n = l.lh_Head; n->ln_Succ; n = n->ln_Succ) count++;
    if (count && !(e = (cx_entry *)AllocVec((ULONG)count * sizeof *e, MEMF_ANY | MEMF_CLEAR))) {
        cx_FreeBrokerList(&l);
        return 0;
    }
    for (n = l.lh_Head; n->ln_Succ && i < count; n = n->ln_Succ, i++) {
        struct BrokerCopy *b = (struct BrokerCopy *)n;
        copy_field(e[i].name, b->bc_Name, CBD_NAMELEN);
        copy_field(e[i].title, b->bc_Title, CBD_TITLELEN);
        copy_field(e[i].descr, b->bc_Descr, CBD_DESCRLEN);
        e[i].flags = b->bc_Flags & (COF_ACTIVE | COF_SHOW_HIDE);
    }
    cx_FreeBrokerList(&l);
    changed = count != ncx || (count && memcmp(e, cxs, (size_t)count * sizeof *e));
    if (!changed) {
        if (e) FreeVec(e);
        return 0;
    }
    if (cxs) FreeVec(cxs);
    cxs = e;
    ncx = count;
    return 1;
}

static const cx_entry *find_chosen(void)
{
    int i;
    if (!chosen[0]) return NULL;
    for (i = 0; i < ncx; i++)
        if (!strcmp(cxs[i].name, chosen)) return &cxs[i];
    return NULL;
}

/* Latin-1 (brokers' names and text) to UTF-8 (olui's) */
static void utf8_of(char *to, int size, const char *from)
{
    int o = 0;
    const unsigned char *s = (const unsigned char *)from;
    for (; *s && o < size - 3; s++) {
        if (*s < 0x80) to[o++] = (char)*s;
        else { to[o++] = (char)(0xC0 | (*s >> 6)); to[o++] = (char)(0x80 | (*s & 0x3F)); }
    }
    to[o] = 0;
}

/* ---- the window --------------------------------------------------------------------------------------- */

static const char markup[] =
    "<row style='flex:1'>"
    "  <list id=list style='flex:1;height:auto' title='The commodities running'></list>"
    "  <column style='width:16em'>"
    "    <fieldset><legend>Information</legend>"
    "      <p id=title>Choose a commodity.</p>"
    "      <p id=descr></p>"
    "    </fieldset>"
    "    <row><button id=show style='flex:1' title='Opens its window'>Show</button>"
    "         <button id=hide style='flex:1' title='Closes its window'>Hide</button></row>"
    "    <select id=state title='Whether it works'><option>Active</option><option>Inactive</option></select>"
    "    <button id=remove title='Quits it'>Remove</button>"
    "  </column>"
    "</row>";

static const char css[] =
    "row{gap:6px} column{gap:6px} #title{font-weight:bold} p{margin:2px 0}";

static struct NewMenu menu_spec[] = {
    { NM_TITLE, (STRPTR)"Project", NULL, 0, 0, NULL },
    { NM_ITEM, (STRPTR)"About...", (STRPTR)"?", 0, 0, NULL },
    { NM_ITEM, NM_BARLABEL, NULL, 0, 0, NULL },
    { NM_ITEM, (STRPTR)"Hide", (STRPTR)"H", 0, 0, NULL },
    { NM_ITEM, (STRPTR)"Quit", (STRPTR)"Q", 0, 0, NULL },
    { NM_END, NULL, NULL, 0, 0, NULL }
};
enum { MI_ABOUT = 0, MI_HIDE = 2, MI_QUIT = 3 };

static struct Window *win;
static struct Menu *menus;
static APTR vi;
static olui *ui;
static olui_amiga *glue;
static int quit;

static struct {
    char popkey[80];
    int popup;
    LONG cxpri;
} set = { "ctrl alt help", 1, 0 };

static void show_chosen(void)
{
    const cx_entry *c = find_chosen();
    char t[CBD_TITLELEN * 2], d[CBD_DESCRLEN * 2];
    if (!ui) return;
    utf8_of(t, sizeof t, c ? c->title : "Choose a commodity.");
    utf8_of(d, sizeof d, c ? c->descr : "");
    olui_set_text(ui, olui_find(ui, "title"), t);
    olui_set_text(ui, olui_find(ui, "descr"), d);
    olui_set_disabled(ui, olui_find(ui, "show"), !c || !(c->flags & COF_SHOW_HIDE));
    olui_set_disabled(ui, olui_find(ui, "hide"), !c || !(c->flags & COF_SHOW_HIDE));
    olui_set_disabled(ui, olui_find(ui, "state"), !c);
    olui_set_disabled(ui, olui_find(ui, "remove"), !c);
    if (c) olui_select_set(ui, olui_find(ui, "state"), (c->flags & COF_ACTIVE) ? 0 : 1);
}

/* The list again from cxs, the chosen one kept chosen while it runs */
static void fill_list(void)
{
    olui_el *list = olui_find(ui, "list"), *keep = NULL;
    int i;
    char text[CBD_NAMELEN * 2];
    olui_list_clear(ui, list);
    for (i = 0; i < ncx; i++) {
        olui_el *it;
        utf8_of(text, sizeof text, cxs[i].name);
        it = olui_list_add(ui, list, NULL, text, NULL);
        if (it) olui_set_user(it, (void *)(cxs + i));
        if (it && !strcmp(cxs[i].name, chosen)) keep = it;
    }
    if (keep) olui_list_select(ui, list, keep);
    else chosen[0] = 0;
    show_chosen();
}

static void refresh(int force)
{
    if (read_brokers() || force) {
        if (ui) fill_list();
    }
}

static void on_select(olui *u, const olui_event *ev, void *userdata)
{
    const cx_entry *c = ev->item ? (const cx_entry *)olui_user(ev->item) : NULL;
    (void)u; (void)userdata;
    if (c) copy_field(chosen, c->name, CBD_NAMELEN);
    else chosen[0] = 0;
    show_chosen();
}

static void command(LONG id)
{
    if (!chosen[0]) return;
    cx_BrokerCommand(chosen, id);
    Delay(5);                   /* the broker hears it on its own time: a moment, then read the list again */
    refresh(0);
}

static void on_show(olui *u, const olui_event *ev, void *userdata) { (void)u; (void)ev; (void)userdata; command(CXCMD_APPEAR); }
static void on_hide(olui *u, const olui_event *ev, void *userdata) { (void)u; (void)ev; (void)userdata; command(CXCMD_DISAPPEAR); }
static void on_remove(olui *u, const olui_event *ev, void *userdata) { (void)u; (void)ev; (void)userdata; command(CXCMD_KILL); }
static void on_state(olui *u, const olui_event *ev, void *userdata)
{
    (void)u; (void)userdata;
    command(ev->index == 1 ? CXCMD_DISABLE : CXCMD_ENABLE);
}

static void close_win(void)
{
    if (glue) olui_amiga_free(glue);
    if (ui) olui_free(ui);
    glue = NULL;
    ui = NULL;
    if (win) {
        if (menus) ClearMenuStrip(win);
        CloseWindow(win);
        win = NULL;
    }
    if (menus) FreeMenus(menus);
    if (vi) FreeVisualInfo(vi);
    menus = NULL;
    vi = NULL;
}

static int open_win(void)
{
    struct Screen *scr;
    int fh, w, h;
    char err[128];
    if (win) {
        WindowToFront(win);
        ActivateWindow(win);
        return 1;
    }
    if (!(scr = LockPubScreen(NULL))) return 0;
    fh = scr->Font ? scr->Font->ta_YSize : 8;
    w = fh * 44;
    h = fh * 18;
    if (w > scr->Width) w = scr->Width;
    if (h > scr->Height) h = scr->Height;
    win = OpenWindowTags(NULL, WA_PubScreen, (ULONG)scr, WA_Title, (ULONG)"Commodities Exchange",
                         WA_ScreenTitle, (ULONG)NAME " " VERSION_TEXT,
                         WA_Left, (scr->Width - w) / 2, WA_Top, (scr->Height - h) / 2, WA_Width, w, WA_Height, h,
                         WA_MinWidth, fh * 28, WA_MinHeight, fh * 12, WA_MaxWidth, ~0UL, WA_MaxHeight, ~0UL,
                         WA_DragBar, TRUE, WA_DepthGadget, TRUE, WA_CloseGadget, TRUE, WA_SizeGadget, TRUE,
                         WA_Activate, TRUE, WA_SimpleRefresh, TRUE, WA_ReportMouse, TRUE, WA_NewLookMenus, TRUE,
                         WA_IDCMP, olui_amiga_idcmp() | IDCMP_MENUPICK, TAG_DONE);
    if (win && (vi = GetVisualInfo(scr, TAG_DONE)) && (menus = CreateMenus(menu_spec, TAG_DONE))
        && LayoutMenus(menus, vi, GTMN_NewLookMenus, TRUE, TAG_DONE))
        SetMenuStrip(win, menus);
    UnlockPubScreen(NULL, scr);
    if (!win) return 0;
    if (!(ui = olui_new()) || !(glue = olui_amiga_new(ui, win))) goto fail;
    olui_amiga_theme(glue, NULL);
    if (!olui_load(ui, markup, css, err, sizeof err)) goto fail;
    olui_on(ui, "list", OLUI_SELECT, on_select, NULL);
    olui_on(ui, "show", OLUI_CLICK, on_show, NULL);
    olui_on(ui, "hide", OLUI_CLICK, on_hide, NULL);
    olui_on(ui, "remove", OLUI_CLICK, on_remove, NULL);
    olui_on(ui, "state", OLUI_CHANGE, on_state, NULL);
    read_brokers();
    fill_list();
    if (!olui_amiga_layout(glue)) goto fail;
    return 1;
fail:
    close_win();
    return 0;
}

static void about(void)
{
    struct EasyStruct es = { sizeof(struct EasyStruct), 0, (STRPTR)NAME,
                             (STRPTR)NAME " " VERSION_TEXT "\nThe commodities' Exchange\n\n"
                                     "Designed, tested and delivered by The AmiCradle Team\n"
                                     "MIT, Copyright (c) 2026 AmiCradle Team",
                             (STRPTR)"OK" };
    EasyRequestArgs(win, &es, NULL, NULL);
}

/* 1 when the window is to close */
static int menu_picked(UWORD code)
{
    int hide = 0;
    while (code != MENUNULL && win) {
        struct MenuItem *item = ItemAddress(menus, code);
        if (MENUNUM(code) == 0) switch (ITEMNUM(code)) {
        case MI_ABOUT: about(); break;
        case MI_HIDE: hide = 1; break;
        case MI_QUIT: quit = 1; break;
        }
        if (!item) break;
        code = item->NextSelect;
    }
    return hide;
}

/* ---- settings: the icon's tool types, then the command line's KEY=VALUE ------------------------------- */

static void take(const char *key, const char *value)
{
    if (!value) return;
    if (!Stricmp((STRPTR)key, (STRPTR)"CX_POPKEY")) copy_field(set.popkey, value, sizeof set.popkey);
    else if (!Stricmp((STRPTR)key, (STRPTR)"CX_POPUP")) set.popup = !(value[0] == 'n' || value[0] == 'N' || value[0] == '0');
    else if (!Stricmp((STRPTR)key, (STRPTR)"CX_PRIORITY")) { LONG v; if (StrToLong((STRPTR)value, &v) > 0) set.cxpri = v; }
}

static const char *const KEYS[] = { "CX_POPKEY", "CX_POPUP", "CX_PRIORITY" };
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
    } else {                                /* from a Shell or S:User-Startup: the icon beside it, then the line */
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
        if ((rd = ReadArgs((STRPTR)"CX_POPKEY/K,CX_POPUP/K,CX_PRIORITY/K", a, NULL))) {
            for (i = 0; i < NKEYS; i++) take(KEYS[i], (const char *)a[i]);
            FreeArgs(rd);
        }
    }
}

/* ---- the commodity -------------------------------------------------------------------------------------- */

#define EV_POPKEY 1

static struct NewBroker nb = {
    NB_VERSION, (STRPTR)"Exchange", (STRPTR)NAME " " VERSION_TEXT, (STRPTR)"Controls the commodities",
    NBU_UNIQUE | NBU_NOTIFY, COF_SHOW_HIDE, 0, NULL, 0
};

int main(int argc, char **argv)
{
    struct MsgPort *port = NULL, *tport = NULL;
    struct timerequest *tr = NULL;
    CxObj *broker = NULL;
    LONG err = 0;
    int timer = 0, tpending = 0, rc = RETURN_FAIL;
    (void)version;

    CxBase = OpenLibrary((STRPTR)"commodities.library", 37);
    IconBase = OpenLibrary((STRPTR)"icon.library", 37);
    UtilityBase = (void *)OpenLibrary((STRPTR)"utility.library", 37);
    if (!CxBase || !UtilityBase) goto out;
    settings(argc, argv);

    if (!(port = CreateMsgPort())) goto out;
    nb.nb_Port = port;
    nb.nb_Pri = (BYTE)set.cxpri;
    if (!(broker = CxBroker(&nb, &err))) {     /* running already: that one shows its window (CXCMD_UNIQUE) */
        if (err == CBERR_DUP) {
            rc = RETURN_OK;
            if (argc) PutStr((STRPTR)"Exchange is running already: its window is shown.\n");
        }
        goto out;
    }
    if (set.popkey[0] && Stricmp((STRPTR)set.popkey, (STRPTR)"NONE")) {
        CxObj *hk = HotKey((STRPTR)set.popkey, port, EV_POPKEY);
        if (hk) {
            AttachCxObj(broker, hk);
            if (CxObjError(hk)) {
                ClearCxObjError(hk);
                if (argc) Printf((STRPTR)NAME ": \"%s\" isn't a hotkey commodities knows\n", (ULONG)set.popkey);
            }
        }
    }
    ActivateCxObj(broker, 1);
    if ((tport = CreateMsgPort()) && (tr = (struct timerequest *)CreateIORequest(tport, sizeof *tr)))
        timer = !OpenDevice((STRPTR)TIMERNAME, UNIT_VBLANK, (struct IORequest *)tr, 0);
    if (set.popup) open_win();
    rc = RETURN_OK;

    while (!quit) {
        ULONG sigs, got;
        CxMsg *m;
        int hide = 0;
        if (win && timer && !tpending) {        /* the list again in a second */
            tr->tr_node.io_Command = TR_ADDREQUEST;
            tr->tr_time.tv_secs = 1;
            tr->tr_time.tv_micro = 0;
            SendIO((struct IORequest *)tr);
            tpending = 1;
        }
        sigs = (1UL << port->mp_SigBit) | SIGBREAKF_CTRL_C | (timer ? 1UL << tport->mp_SigBit : 0)
               | (win ? 1UL << win->UserPort->mp_SigBit : 0);
        got = Wait(sigs);
        if (got & SIGBREAKF_CTRL_C) quit = 1;
        if (tpending && CheckIO((struct IORequest *)tr)) {
            WaitIO((struct IORequest *)tr);
            tpending = 0;
            if (win) refresh(0);
        }
        while ((m = (CxMsg *)GetMsg(port))) {
            ULONG type = CxMsgType(m), id = CxMsgID(m);
            ReplyMsg((struct Message *)m);
            if (type == CXM_IEVENT && id == EV_POPKEY) open_win();
            else if (type == CXM_COMMAND) switch (id) {
            case CXCMD_DISABLE: ActivateCxObj(broker, 0); break;
            case CXCMD_ENABLE: ActivateCxObj(broker, 1); break;
            case CXCMD_APPEAR: case CXCMD_UNIQUE: open_win(); break;
            case CXCMD_DISAPPEAR: hide = 1; break;
            case CXCMD_KILL: quit = 1; break;
            }
        }
        if (win) {
            struct IntuiMessage *im;
            while (win && (im = (struct IntuiMessage *)GetMsg(win->UserPort))) {
                int r;
                if (im->Class == IDCMP_MENUPICK) {
                    UWORD code = im->Code;
                    ReplyMsg(&im->ExecMessage);
                    if (menu_picked(code)) hide = 1;
                    continue;
                }
                r = olui_amiga_event(glue, im);
                ReplyMsg(&im->ExecMessage);
                if (r == OLUI_AMIGA_CLOSE) hide = 1;
            }
            if (win && !hide && !quit) olui_amiga_draw(glue);
        }
        if ((hide || quit) && win) close_win();
        if (!win && tpending) {
            AbortIO((struct IORequest *)tr);
            WaitIO((struct IORequest *)tr);
            tpending = 0;
        }
    }
out:
    close_win();
    if (tpending) { AbortIO((struct IORequest *)tr); WaitIO((struct IORequest *)tr); }
    if (timer) CloseDevice((struct IORequest *)tr);
    if (tr) DeleteIORequest((struct IORequest *)tr);
    if (tport) DeleteMsgPort(tport);
    if (broker) DeleteCxObjAll(broker);
    if (port) { struct Message *msg; while ((msg = GetMsg(port))) ReplyMsg(msg); DeleteMsgPort(port); }
    if (cxs) FreeVec(cxs);
    if (UtilityBase) CloseLibrary(UtilityBase);
    if (IconBase) CloseLibrary(IconBase);
    if (CxBase) CloseLibrary(CxBase);
    return rc;
}
