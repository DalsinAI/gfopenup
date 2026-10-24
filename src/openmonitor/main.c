/* OpenMonitor 0.8: what the machine is doing, as Task Manager on Windows or Resources on Ubuntu show it, grown
 * into the Amiga's task manager (the user, 24 October 2026: "a true task manager, a blend of what it is today,
 * SysInfo, ShowConfig and SnoopDos"; the design of that day, agreed at 15:05).
 *
 * This file: the start (one window, leaving the Shell), the window and its look, the tabs, the status bar,
 * the second's tick, and the settings. Each view is its own file.
 *
 * The ROM keeps OpenMonitor 0.3.3 (../openmonitor.c) until 1.0 goes into it (the design's P6): this one is
 * the disk's, SYS:Utilities/OpenMonitor, which OpenUp installs on every ROM until then.
 *
 * MIT, Copyright (c) 2026 AmigaChrome Team. */

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <devices/timer.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <dos/var.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <intuition/gadgetclass.h>
#include <devices/inputevent.h>
#include <libraries/gadtools.h>
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "om.h"
#include "ogt_look.h"
#include "ogt_font.h"
#include "ogt_openfont.h"

static const char version[] __attribute__((used)) =
    "$VER: OpenMonitor " OM_VERSION " (" OM_DATE ") MIT, Copyright (c) 2026 AmigaChrome Team";

/* The window runs on 32 KB, however OpenMonitor is started (0.3.1: a Shell's 4 KB rebooted the machine
 * when the window closed). libnix's start code swaps to a stack of this size when it was given less. */
unsigned long __stack = 32768;

struct om_ui U;
char om_first_task[40];
static char pubscreen[40];             /* PUBSCREEN/K: a public screen by its name, else the default one */
const struct om_view *om_views[V_N] = { &om_tasks_view, &om_memory_view, &om_libs_view, &om_ports_view, &om_ints_view,
                                        &om_hw_view, &om_speed_view, &om_calls_view };

/* ---- settings: ENV:OpenMonitor/Settings, "key value" lines, kept in ENVARC: too ------------------------- */

#define MAXPREFS 40
/* allocated: the view's variables (a4's 64 KB, built resident) are kept for what must be there */
static struct prefs { char key[MAXPREFS][24], val[MAXPREFS][96]; } *P;
#define pref_key (P->key)
#define pref_val (P->val)
static int npref;

static void prefs_load(void)
{
    char buf[3200], *p, *line;
    LONG n = GetVar((STRPTR)"OpenMonitor/Settings", (STRPTR)buf, sizeof buf, GVF_GLOBAL_ONLY | GVF_BINARY_VAR);
    npref = 0;
    if (!P && !(P = AllocVec(sizeof *P, MEMF_ANY | MEMF_CLEAR))) return;
    if (n <= 0) return;
    buf[n < (LONG)sizeof buf ? n : (LONG)sizeof buf - 1] = 0;
    for (p = buf; p && *p && npref < MAXPREFS; ) {
        char *sp;
        line = p;
        if ((p = strchr(p, '\n'))) *p++ = 0;
        if (*line == ';' || !*line || !(sp = strchr(line, ' '))) continue;
        *sp++ = 0;
        strncpy(pref_key[npref], line, sizeof pref_key[0] - 1);
        strncpy(pref_val[npref], sp, sizeof pref_val[0] - 1);
        pref_key[npref][sizeof pref_key[0] - 1] = 0;
        pref_val[npref][sizeof pref_val[0] - 1] = 0;
        npref++;
    }
}

const char *om_pref(const char *key)
{
    int i;
    if (!P) return NULL;
    for (i = 0; i < npref; i++) if (!strcmp(pref_key[i], key)) return pref_val[i];
    return NULL;
}

static void prefs_save(void)
{
    char buf[3200];
    int i, len = snprintf(buf, sizeof buf, "; OpenMonitor %s's settings\n", OM_VERSION);
    for (i = 0; i < npref && len < (int)sizeof buf - 130; i++)
        len += snprintf(buf + len, sizeof buf - len, "%s %s\n", pref_key[i], pref_val[i]);
    SetVar((STRPTR)"OpenMonitor/Settings", (STRPTR)buf, len, GVF_GLOBAL_ONLY | GVF_SAVE_VAR);
}

void om_set_pref(const char *key, const char *value)
{
    int i;
    if (!P) return;
    for (i = 0; i < npref; i++) if (!strcmp(pref_key[i], key)) break;
    if (i == npref) {
        if (npref >= MAXPREFS) return;
        strncpy(pref_key[i], key, sizeof pref_key[0] - 1);
        pref_key[i][sizeof pref_key[0] - 1] = 0;
        npref++;
    } else if (!strcmp(pref_val[i], value)) return;
    strncpy(pref_val[i], value, sizeof pref_val[0] - 1);
    pref_val[i][sizeof pref_val[0] - 1] = 0;
    prefs_save();
}

/* ---- small helpers -------------------------------------------------------------------------------------- */

void om_text(struct RastPort *rp, struct TextFont *f, LONG pen, int x, int y, const char *s, int maxw)
{
    if (f && rp->Font != f) SetFont(rp, f);
    ogt_text(rp, pen, x, y, s, maxw);
}

int om_text_width(struct TextFont *f, const char *s)
{
    struct RastPort rp;
    InitRastPort(&rp);
    if (f) SetFont(&rp, f);
    return TextLength(&rp, (STRPTR)s, strlen(s));
}

void om_sunk(int x, int y, int w, int h)
{
    struct RastPort *rp = U.win->RPort;
    ogt_box(rp, U.pen_back, x + 1, y + 1, w - 2, h - 2);
    ogt_bevel(rp, U.pen_shadow, U.pen_shine, x, y, w, h);
}

void om_heading(int x, int y, const char *s)
{
    struct RastPort *rp = U.win->RPort;
    SetFont(rp, U.font);
    ogt_bold(rp, 1);
    ogt_text(rp, U.pen_text, x, y, s, U.pw);
    ogt_bold(rp, 0);
}

const char *om_commas(ULONG v, char *buf)
{
    char t[16];
    int n = snprintf(t, sizeof t, "%lu", (unsigned long)v), i, o = 0;
    for (i = 0; i < n; i++) {
        if (i && (n - i) % 3 == 0) buf[o++] = ',';
        buf[o++] = t[i];
    }
    buf[o] = 0;
    return buf;
}

/* Where code is: the ROM (Kickstart's places, or the open ROM's image unpacked into RAM at boot, which the
 * romunpack resource names: common/openunpack/okrz_format.h), a group of the ROM's unpacked on demand, or a
 * program's (RAM). */
const char *om_where(APTR code)
{
    static ULONG *table;
    static int looked;
    ULONG a = (ULONG)code;
    if (!a) return "-";
    if ((a >= 0x00f80000UL && a < 0x01000000UL) || (a >= 0x00e00000UL && a < 0x00e80000UL) ||
        (a >= 0x00f00000UL && a < 0x00f80000UL)) return "ROM";
    if (!looked) {
        UBYTE *res = OpenResource((STRPTR)"romunpack");
        looked = 1;
        if (res && *(ULONG *)(res + 16) == 0x4f4b5254UL) table = (ULONG *)(res + 16);     /* 'OKRT' */
    }
    if (table) {
        UWORD ver = *(UWORD *)((UBYTE *)table + 4), count = *(UWORD *)((UBYTE *)table + 20), i;
        ULONG base = table[3], size = table[4];
        ULONG *e = (ULONG *)((UBYTE *)table + (ver >= 3 ? 32 : 24));      /* 3 added find and load */
        if (a >= base && a < base + size) return "ROM";
        for (i = 0; i < count && i < 1024; i++, e += 3)
            if (e[1] && (e[1] < base || e[1] >= base + size) && a >= e[1] && a < e[1] + e[2]) return "ROM group";
    }
    return "RAM";
}

void om_status(int field, const char *fmt, ...)
{
    char s[OGT_STATUS_LEN];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(s, sizeof s, fmt, ap);
    va_end(ap);
    if (U.win) {
        SetFont(U.win->RPort, U.font);          /* the bar draws in the window's RastPort's font */
        ogt_status_set(&U.status, field, s);
    }
}

/* An Intuition requester: its text as it is (a task's name may hold a %), its buttons; Intuition's numbers
 * (the last button is 0). */
LONG om_ask(const char *title, const char *buttons, const char *fmt, ...)
{
    static char text[900];
    struct EasyStruct es;
    APTR args[1];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(text, sizeof text, fmt, ap);
    va_end(ap);
    args[0] = (APTR)text;
    es.es_StructSize = sizeof es;
    es.es_Flags = 0;
    es.es_Title = (UBYTE *)title;
    es.es_TextFormat = (UBYTE *)"%s";
    es.es_GadgetFormat = (UBYTE *)buttons;
    {
        LONG r = EasyRequestArgs(U.win, &es, NULL, args);
        if (U.win) ActivateWindow(U.win);       /* the keys come back to OpenMonitor */
        return r;
    }
}

/* ---- the clipboard: text as IFF FTXT on unit 0 ---------------------------------------------------------- */

#include <devices/clipboard.h>

int om_clip_text(const char *s)
{
    struct MsgPort *port;
    struct IOClipReq *io = NULL;
    ULONG len = strlen(s), pad = len & 1, form = 4 + 8 + len + pad;
    UBYTE head[20];
    int ok = 0;
    if (!(port = CreateMsgPort())) return 0;
    if ((io = (struct IOClipReq *)CreateIORequest(port, sizeof *io)) && !OpenDevice((STRPTR)"clipboard.device", 0, (struct IORequest *)io, 0)) {
        memcpy(head, "FORM", 4);
        head[4] = form >> 24; head[5] = form >> 16; head[6] = form >> 8; head[7] = form;
        memcpy(head + 8, "FTXTCHRS", 8);
        head[16] = len >> 24; head[17] = len >> 16; head[18] = len >> 8; head[19] = len;
        io->io_Offset = 0;
        io->io_ClipID = 0;
        io->io_Command = CMD_WRITE; io->io_Data = (STRPTR)head; io->io_Length = 20;
        ok = !DoIO((struct IORequest *)io);
        if (ok) { io->io_Command = CMD_WRITE; io->io_Data = (STRPTR)s; io->io_Length = len; ok = !DoIO((struct IORequest *)io); }
        if (ok && pad) { io->io_Command = CMD_WRITE; io->io_Data = (STRPTR)"\0"; io->io_Length = 1; ok = !DoIO((struct IORequest *)io); }
        if (ok) { io->io_Command = CMD_UPDATE; ok = !DoIO((struct IORequest *)io); }
        CloseDevice((struct IORequest *)io);
    }
    if (io) DeleteIORequest((struct IORequest *)io);
    DeleteMsgPort(port);
    return ok;
}

/* ---- the second's tick ---------------------------------------------------------------------------------- */

static struct MsgPort *tport;
static struct timerequest *tr;
static int timer;

static void timer_send(void)
{
    tr->tr_node.io_Command = TR_ADDREQUEST;
    tr->tr_time.tv_secs = 1;
    tr->tr_time.tv_micro = 0;
    SendIO((struct IORequest *)tr);
}

void om_tick(void)
{
    const struct om_view *v = om_views[U.view];
    om_tasks_take();                    /* every second: the CPU graph needs it whichever view is open */
    om_graphs_take();
    om_memory_take();                   /* and the Memory view's last minute */
    if (v->tick) v->tick();
    SetFont(U.win->RPort, U.font);
    if (v->status) v->status();
}

/* 1 when the second was up (the tick has run); for the modal loops (menus, the priority window) too */
int om_timer_due(void)
{
    if (!timer || !CheckIO((struct IORequest *)tr)) return 0;
    WaitIO((struct IORequest *)tr);
    om_tick();
    timer_send();
    return 1;
}

ULONG om_timer_sig(void) { return timer ? 1UL << tport->mp_SigBit : 0; }

/* ---- the window: layout, gadgets, drawing -------------------------------------------------------------- */

static const char *view_names[V_N + 1];
static struct Gadget *cycle_gad;

static int inner_w(void) { return U.win->Width - U.win->BorderLeft - U.win->BorderRight; }
static int inner_h(void) { return U.win->Height - U.win->BorderTop - U.win->BorderBottom; }

static void layout(void)
{
    int l = U.win->BorderLeft, t = U.win->BorderTop, w = inner_w(), h = inner_h(), lh = U.fh + 6, sh = U.fh + 8;
    struct RastPort *rp = U.win->RPort;
    int i;
    for (i = 0; i < V_N; i++) view_names[i] = om_views[i]->name;
    view_names[V_N] = NULL;
    SetFont(rp, U.font);
    ogt_status_init(&U.status, U.win, &U.ctx, 3);
    ogt_status_layout(&U.status, l, t + h - sh, w, sh);
    if (U.compact) {
        /* no tabs on a small screen: a cycle gadget (when there is more than one view), the page below it */
        U.px = l + 4;
        U.py = t + (V_N > 1 ? lh + 6 : 3);
        U.pw = w - 8;
        U.ph = h - sh - (U.py - t) - 3;
    } else {
        ogt_tabs_set(&U.tabs, view_names, V_N, U.view);
        ogt_tabs_layout(&U.tabs, rp, l + 12, t + 8, w - 24, lh);
        U.px = l + 16;
        U.py = U.tabs.y + U.tabs.h + 10;
        U.pw = w - 32;
        U.ph = h - sh - (U.py - t) - 12;
    }
    om_views[U.view]->layout(U.px, U.py, U.pw, U.ph);
}

static struct Gadget *make_gadgets(void)
{
    struct Gadget *g;
    U.glist = NULL;
    cycle_gad = NULL;
    g = CreateContext(&U.glist);
    if (U.compact && V_N > 1 && g) {
        struct NewGadget ng;
        memset(&ng, 0, sizeof ng);
        ng.ng_TextAttr = &U.ta; ng.ng_VisualInfo = U.vi;
        ng.ng_LeftEdge = U.win->BorderLeft + 4; ng.ng_TopEdge = U.win->BorderTop + 3;
        ng.ng_Width = om_text_width(U.font, "Interrupts") + 40; ng.ng_Height = U.fh + 6; ng.ng_GadgetID = GID_TABS;
        g = cycle_gad = CreateGadget(CYCLE_KIND, g, &ng, GTCY_Labels, (ULONG)view_names, GTCY_Active, (ULONG)U.view, TAG_DONE);
    }
    if (g) g = om_views[U.view]->gadgets(g);
    if (g && !U.compact) g = ogt_tabs_gadgets(&U.tabs, g, GID_TABS);
    return g;
}

static void draw_all(void)
{
    struct RastPort *rp = U.win->RPort;
    SetFont(rp, U.font);
    if (!U.compact) ogt_tabs_draw(&U.tabs, U.win, U.status.y - 6);
    om_views[U.view]->draw();
    ogt_status_draw(&U.status);
}

void om_redraw_page(void)
{
    EraseRect(U.win->RPort, U.px - 1, U.py - 1, U.px + U.pw, U.py + U.ph);
    om_views[U.view]->draw();
}

/* All the gadgets again (a view changed its own, or the window its size), and everything drawn. */
void om_rebuild_gadgets(void)
{
    if (U.glist) {
        RemoveGList(U.win, U.glist, -1);
        FreeGadgets(U.glist);
        U.glist = NULL;
    }
    EraseRect(U.win->RPort, U.win->BorderLeft, U.win->BorderTop, U.win->Width - U.win->BorderRight - 1,
              U.win->Height - U.win->BorderBottom - 1);
    layout();
    if (make_gadgets()) {
        AddGList(U.win, U.glist, (UWORD)~0, -1, NULL);
        RefreshGList(U.glist, U.win, NULL, -1);
        GT_RefreshWindow(U.win, NULL);
    }
    draw_all();
}

void om_open_view(int v)
{
    int was = U.view;
    if (v < 0 || v >= V_N || v == was || was < 0 || was >= V_N) return;
    if (om_views[was]->shown) om_views[was]->shown(0);
    U.view = v;
    if (om_views[v]->shown) om_views[v]->shown(1);
    om_set_pref("View", om_views[v]->name);
    om_rebuild_gadgets();
    if (om_views[v]->status) om_views[v]->status();
}

/* ---- fonts and pens ------------------------------------------------------------------------------------- */

static struct TextFont *disk_font(struct TextAttr *ta);

/* DejaVu Sans Mono 11, the system default text since amigachrome-guest #472: from the font list (the open
 * ROM adds it), else openfont.library or diskfont.library (FONTS:, OpenUp's ShellFont), else the system
 * default text itself. */
static struct TextFont *open_table_font(void)
{
    struct TextFont *f;
    struct Library *of;
    U.tta.ta_Name = (STRPTR)"DejaVuSansMono.font";
    U.tta.ta_YSize = 11;
    U.tta.ta_Style = 0;
    U.tta.ta_Flags = 0;
    if ((f = OpenFont(&U.tta)) && f->tf_YSize == 11) return f;
    if (f) CloseFont(f);
    if ((of = ogt_openfont_open())) {
        f = ogt_openfont_font(of, &U.tta);
        CloseLibrary(of);
        if (f) return f;
    }
    if ((f = disk_font(&U.tta))) return f;
    {
        struct TextFont *d = GfxBase->DefaultFont;
        U.tta.ta_Name = (STRPTR)d->tf_Message.mn_Node.ln_Name;
        U.tta.ta_YSize = d->tf_YSize;
        return OpenFont(&U.tta);
    }
}

/* diskfont.library's OpenDiskFont (-30), by its LVO: no global DiskfontBase */
static struct TextFont *disk_font(struct TextAttr *ta)
{
    struct Library *df = OpenLibrary((STRPTR)"diskfont.library", 36);
    struct TextFont *f = NULL;
    if (df) {
        register ULONG d0_ __asm("d0");
        register struct Library *a6_ __asm("a6") = df;
        register struct TextAttr *a0_ __asm("a0") = ta;
        __asm volatile ("jsr -30(%%a6)" : "=r"(d0_), "+r"(a0_) : "r"(a6_) : "d1", "a1", "cc", "memory");
        f = (struct TextFont *)d0_;
        CloseLibrary(df);
    }
    return f;
}

/* The labels' font when the theme's couldn't open (no openfont.library) and ogt_open_font gave the screen's
 * own, which is large on a tall screen (CGTriumvirate 23 on a 1920 x 1200 Workbench): the same family at
 * the theme's size through diskfont (bullet scales CGTriumvirate), else helvetica 13. */
static void smaller_label_font(void)
{
    static char name[40];
    static const char *const families[] = { "CGTriumvirate.font", "helvetica.font", NULL };
    int i, size = U.theme->font_size > 0 ? U.theme->font_size : 13;
    if (U.compact || !U.font || U.font->tf_YSize <= 16) return;
    for (i = 0; families[i]; i++) {
        struct TextAttr ta;
        struct TextFont *f;
        strcpy(name, families[i]);
        ta.ta_Name = (STRPTR)name; ta.ta_YSize = size; ta.ta_Style = 0; ta.ta_Flags = 0;
        if ((f = disk_font(&ta)) && f->tf_YSize <= 16 && (f->tf_Flags & FPF_PROPORTIONAL)) {
            CloseFont(U.font);
            U.font = f;
            U.ta = ta;
            return;
        }
        if (f) CloseFont(f);
    }
}

static LONG rgb_pen(ULONG rgb)
{
    ogt_rgb c;
    c.r = rgb >> 16; c.g = rgb >> 8; c.b = rgb;
    return ogt_pen_rgb(&U.ctx, c);
}

static void pens(void)
{
    UWORD *p = U.dri->dri_Pens;
    U.pen_text = ogt_pen(&U.ctx, "text");
    U.pen_muted = ogt_pen(&U.ctx, "muted");
    U.pen_accent = ogt_pen(&U.ctx, "accent");
    U.pen_list = ogt_pen(&U.ctx, "list");
    U.pen_alt = ogt_pen(&U.ctx, "list.alternate");
    U.pen_head = ogt_pen(&U.ctx, "list.header");
    U.pen_fill = ogt_pen(&U.ctx, "fill");
    U.pen_filltext = ogt_pen(&U.ctx, "fill.text");
    U.pen_shine = p[SHINEPEN];
    U.pen_shadow = p[SHADOWPEN];
    U.pen_back = p[BACKGROUNDPEN];
    if (U.ctx.four) {
        U.pen_danger = U.pen_amber = p[TEXTPEN];
        U.graph_pen[0] = U.graph_pen[1] = U.graph_pen[2] = U.graph_pen[3] = p[FILLPEN];
        /* 0.8.1: the graphs as dark as four colours go: black, the grid grey, the lines white and blue */
        U.gauge_bg = p[SHADOWPEN]; U.gauge_grid = p[BACKGROUNDPEN]; U.gauge_text = p[SHINEPEN];
        U.gauge_line[0] = p[SHINEPEN]; U.gauge_line[1] = p[FILLPEN]; U.gauge_line[2] = p[SHINEPEN];
    } else {
        U.pen_danger = rgb_pen(0xb02828);
        U.pen_amber = rgb_pen(0xa86400);
        U.graph_pen[0] = rgb_pen(0x3d7fd6);
        U.graph_pen[1] = rgb_pen(0x4caf50);
        U.graph_pen[2] = rgb_pen(0xf08c28);
        U.graph_pen[3] = rgb_pen(0x965ac8);
        /* 0.8.1 (the user, 24 October 2026, 22:03: "the gauges to be very very dark grey backgrounds and little
         * whites, reds, blues"): the graphs' insides very dark grey, the grid a little lighter, the title light,
         * and the lines CPU white, memory blue, GPU red */
        U.gauge_bg = rgb_pen(0x161616);
        U.gauge_grid = rgb_pen(0x3a3a3a);
        U.gauge_text = rgb_pen(0xe6e6e6);
        U.gauge_line[0] = rgb_pen(0xf4f4f4);
        U.gauge_line[1] = rgb_pen(0x3d8bff);
        U.gauge_line[2] = rgb_pen(0xe5383b);
    }
}

/* ---- the window ----------------------------------------------------------------------------------------- */

static struct MsgPort *front_port;

/* a view's name as typed (VIEW memory, "ports"): its first letters, any case */
static int same_word(const char *a, const char *b)
{
    int n = 0;
    for (; *a && *b; a++, b++, n++) {
        char x = *a, y = *b;
        if (x >= 'A' && x <= 'Z') x += 32;
        if (y >= 'A' && y <= 'Z') y += 32;
        if (x != y) return 0;
    }
    return n >= 3 && !*a;
}

static int gui(STRPTR first_view)
{
    struct Screen *scr = LockPubScreen(pubscreen[0] ? (STRPTR)pubscreen : NULL);
    int quit = 0, rc = RETURN_OK, i, W, H, X, Y, minw, minh;
    const char *v;
    if (!scr) return RETURN_FAIL;
    memset(&U, 0, sizeof U);
    U.scr = scr;
    prefs_load();
    U.vi = GetVisualInfoA(scr, NULL);
    U.dri = GetScreenDrawInfo(scr);
    U.theme = AllocVec(sizeof *U.theme, MEMF_ANY | MEMF_CLEAR);
    if (!U.vi || !U.dri || !U.theme) { rc = RETURN_FAIL; goto out; }
    ogt_look_load(U.theme, &U.mode);
    if (!ogt_theme_get(U.theme, U.mode, "panel")) {
        /* the status bar's key: a theme without it is drawn in the window's colour, not pen 1 */
        ULONG rgb[3];
        char hex[12];
        GetRGB32(scr->ViewPort.ColorMap, U.dri->dri_Pens[BACKGROUNDPEN], 1, rgb);
        snprintf(hex, sizeof hex, "#%02lx%02lx%02lx", (unsigned long)(rgb[0] >> 24), (unsigned long)(rgb[1] >> 24), (unsigned long)(rgb[2] >> 24));
        ogt_theme_override(U.theme, "panel", hex);
    }
    if (!(U.ctx_ok = ogt_ctx_init(&U.ctx, scr, U.theme, U.mode))) { rc = RETURN_FAIL; goto out; }
    U.compact = scr->Height < 400;
    if (U.compact) {
        /* everything in the screen's font: topaz 8 on a PAL HighRes Workbench (the design, section 8) */
        U.ta = *scr->Font;
        U.font = OpenFont(&U.ta);
        U.tta = *scr->Font;
        U.tfont = OpenFont(&U.tta);
    } else {
        U.font = ogt_open_font(U.theme, scr, &U.ta);
        smaller_label_font();
        U.tfont = open_table_font();
    }
    if (!U.font || !U.tfont) { rc = RETURN_FAIL; goto out; }
    U.fh = U.font->tf_YSize;
    U.tfh = U.tfont->tf_YSize;
    pens();
    U.view = 0;
    if ((v = first_view ? (const char *)first_view : om_pref("View")))
        for (i = 0; i < V_N; i++) if (same_word(v, om_views[i]->name)) U.view = i;
    om_graphs_init();
    if (!om_tasks_init()) { rc = RETURN_FAIL; goto out; }
    om_memory_init();
    om_libs_init();
    om_ports_init();
    om_ints_init();
    om_hw_init();
    om_speed_init();
    om_calls_init();

    /* the size: kept from the last run, else 1000 x 780 (outer) on a big screen, the whole screen on a small one */
    minw = U.compact ? 400 : 720;
    minh = U.compact ? 180 : 480;
    W = U.compact ? scr->Width : 1000;
    H = U.compact ? scr->Height - scr->BarHeight - 1 : 780;
    X = -1; Y = -1;
    if ((v = om_pref("Window")) && !U.compact) sscanf(v, "%d %d %d %d", &X, &Y, &W, &H);
    if (W > scr->Width) W = scr->Width;
    if (H > scr->Height - scr->BarHeight - 1) H = scr->Height - scr->BarHeight - 1;
    if (W < minw) W = minw;
    if (H < minh) H = minh;
    if (X < 0 || X + W > scr->Width) X = (scr->Width - W) / 2;
    if (Y < scr->BarHeight + 1 || Y + H > scr->Height) Y = scr->BarHeight + 1 + (scr->Height - scr->BarHeight - 1 - H) / 2;

    U.win = OpenWindowTags(NULL, WA_Title, (ULONG)"OpenMonitor", WA_ScreenTitle, (ULONG)("OpenMonitor " OM_VERSION),
                           WA_PubScreen, (ULONG)scr, WA_Left, X, WA_Top, Y, WA_Width, W, WA_Height, H,
                           WA_MinWidth, minw, WA_MinHeight, minh, WA_MaxWidth, ~0, WA_MaxHeight, ~0,
                           WA_DragBar, TRUE, WA_DepthGadget, TRUE, WA_CloseGadget, TRUE, WA_SizeGadget, TRUE,
                           WA_SizeBBottom, TRUE, WA_Activate, TRUE, WA_SmartRefresh, TRUE,
                           WA_RMBTrap, TRUE,    /* no menus of its own: the right button opens the views' menus */
                           WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW | IDCMP_NEWSIZE | IDCMP_GADGETUP | IDCMP_GADGETDOWN |
                                     IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE | IDCMP_IDCMPUPDATE | IDCMP_VANILLAKEY | IDCMP_RAWKEY |
                                     IDCMP_INTUITICKS | BUTTONIDCMP | CYCLEIDCMP | STRINGIDCMP | CHECKBOXIDCMP,
                           TAG_DONE);
    if (!U.win) { rc = RETURN_FAIL; goto out; }
    SetFont(U.win->RPort, U.font);
    for (i = 0; i < V_N; i++) if (i == U.view && om_views[i]->shown) om_views[i]->shown(1);
    if ((tport = CreateMsgPort()) && (tr = (struct timerequest *)CreateIORequest(tport, sizeof *tr)))
        timer = !OpenDevice((STRPTR)TIMERNAME, UNIT_VBLANK, (struct IORequest *)tr, 0);
    om_tasks_take();
    om_rebuild_gadgets();
    if (om_views[U.view]->status) om_views[U.view]->status();
    if (timer) timer_send();

    while (!quit) {
        struct IntuiMessage *m;
        ULONG tsig = om_timer_sig(), fsig = front_port ? 1UL << front_port->mp_SigBit : 0;
        ULONG got = Wait((1UL << U.win->UserPort->mp_SigBit) | tsig | fsig | SIGBREAKF_CTRL_C);
        if (got & SIGBREAKF_CTRL_C) quit = 1;
        if (got & fsig) {
            struct Message *fm;
            while ((fm = GetMsg(front_port))) ReplyMsg(fm);
            ScreenToFront(U.win->WScreen);
            WindowToFront(U.win);
            ActivateWindow(U.win);
        }
        if (got & tsig) om_timer_due();
        while (!quit && (m = GT_GetIMsg(U.win->UserPort))) {
            ULONG cls = m->Class, secs = m->Seconds, micros = m->Micros;
            UWORD code = m->Code, qual = m->Qualifier;
            WORD mx = m->MouseX, my = m->MouseY;
            APTR ia = m->IAddress;
            GT_ReplyIMsg(m);
            if (cls == IDCMP_CLOSEWINDOW) { quit = 1; break; }
            if (cls == IDCMP_INTUITICKS) continue;
            if (cls == IDCMP_REFRESHWINDOW) {
                GT_BeginRefresh(U.win);
                draw_all();
                GT_EndRefresh(U.win, TRUE);
                continue;
            }
            if (cls == IDCMP_NEWSIZE) {
                char s[48];
                om_rebuild_gadgets();
                snprintf(s, sizeof s, "%d %d %d %d", U.win->LeftEdge, U.win->TopEdge, U.win->Width, U.win->Height);
                if (!U.compact) om_set_pref("Window", s);
                continue;
            }
            if (cls == IDCMP_GADGETUP && ia) {
                struct Gadget *g = (struct Gadget *)ia;
                int tab = U.compact ? (g == cycle_gad ? (int)code : -1) : ogt_tabs_index(&U.tabs, g->GadgetID);
                if (tab >= 0) { om_open_view(tab); continue; }
            }
            if (cls == IDCMP_VANILLAKEY) {
                if (code == 27 || code == 'q' || code == 'Q') { quit = 1; break; }
                if (code >= '1' && code < '1' + V_N) { om_open_view(code - '1'); continue; }
                if (code == 9) { om_open_view((U.view + (qual & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT) ? V_N - 1 : 1)) % V_N); continue; }
            }
            if (om_views[U.view]->event) om_views[U.view]->event(cls, code, qual, ia, mx, my, secs, micros);
        }
    }
    {
        char s[48];
        snprintf(s, sizeof s, "%d %d %d %d", U.win->LeftEdge, U.win->TopEdge, U.win->Width, U.win->Height);
        if (!U.compact) om_set_pref("Window", s);
    }
out:
    if (timer) {
        if (!CheckIO((struct IORequest *)tr)) AbortIO((struct IORequest *)tr);
        WaitIO((struct IORequest *)tr);
        CloseDevice((struct IORequest *)tr);
        timer = 0;
    }
    if (tr) DeleteIORequest((struct IORequest *)tr);
    if (tport) DeleteMsgPort(tport);
    tr = NULL; tport = NULL;
    for (i = 0; i < V_N; i++) if (i == U.view && U.win && om_views[i]->shown) om_views[i]->shown(0);
    om_calls_free();                    /* the wedges out, every call through them back */
    om_tasks_free();                    /* every frozen task resumed, the sampler out */
    om_libs_free();
    om_ports_free();
    om_hw_free();
    om_ints_free();
    om_graphs_free();
    if (U.win) {
        if (U.glist) RemoveGList(U.win, U.glist, -1);
        CloseWindow(U.win);
        U.win = NULL;
    }
    if (U.glist) FreeGadgets(U.glist);
    if (U.ctx_ok) ogt_ctx_free(&U.ctx);
    if (U.dri) FreeScreenDrawInfo(scr, U.dri);
    if (U.vi) FreeVisualInfo(U.vi);
    if (U.font) CloseFont(U.font);
    if (U.tfont) CloseFont(U.tfont);
    if (U.theme && U.theme->text) ogt_theme_free(U.theme);
    if (U.theme) FreeVec(U.theme);
    U.theme = NULL;
    UnlockPubScreen(NULL, scr);
    if (P) FreeVec(P);
    P = NULL;
    return rc;
}

/* ---- one window, and leaving the Shell (0.3.2, unchanged) ----------------------------------------------- */

#define FRONT_PORT "OpenMonitor"

static int send_to_front(void)
{
    struct MsgPort *reply = CreateMsgPort(), *p;
    struct Message msg;
    int sent = 0;
    if (!reply) return 0;
    memset(&msg, 0, sizeof msg);
    msg.mn_ReplyPort = reply;
    msg.mn_Length = sizeof msg;
    Forbid();
    if ((p = FindPort((STRPTR)FRONT_PORT))) { PutMsg(p, &msg); sent = 1; }
    Permit();
    if (sent) { WaitPort(reply); GetMsg(reply); }
    DeleteMsgPort(reply);
    return sent;
}

static struct MsgPort *claim_front(int *other)
{
    struct MsgPort *p = CreateMsgPort();
    *other = 0;
    if (!p) return NULL;
    p->mp_Node.ln_Name = (char *)FRONT_PORT;
    p->mp_Node.ln_Pri = 0;
    Forbid();
    if (FindPort((STRPTR)FRONT_PORT)) *other = 1; else AddPort(p);
    Permit();
    if (*other) { DeleteMsgPort(p); send_to_front(); return NULL; }
    return p;
}

static void release_front(struct MsgPort *p)
{
    struct Message *m;
    if (!p) return;
    Forbid();
    RemPort(p);
    Permit();
    while ((m = GetMsg(p))) ReplyMsg(m);
    DeleteMsgPort(p);
}

/* From a Shell: OpenMonitor starts again as a Shell process of its own (NIL: in and out), with WAIT, and this
 * command ends; a resident copy by its name, a file by its full path. 0 when it couldn't: it then runs here. */
static int detach(const char *view, const char *task)
{
    struct CommandLineInterface *cli = Cli();
    struct Segment *seg;
    char name[108], cmd[400], extra[160];
    const char *base;
    BPTR in, out, dir;
    LONG rc;
    int resident;
    if (!cli || !GetProgramName((STRPTR)name, sizeof name)) return 0;
    base = (const char *)FilePart((STRPTR)name);
    extra[0] = 0;
    if (view && strlen(view) < 32) snprintf(extra, sizeof extra, " VIEW \"%s\"", view);
    if (task && strlen(task) < 40) snprintf(extra + strlen(extra), sizeof extra - strlen(extra), " TASK \"%s\"", task);
    if (pubscreen[0]) snprintf(extra + strlen(extra), sizeof extra - strlen(extra), " PUBSCREEN \"%s\"", pubscreen);
    Forbid();
    seg = FindSegment((STRPTR)base, NULL, FALSE);
    if (!seg || seg->seg_Seg != cli->cli_Module) seg = FindSegment((STRPTR)base, NULL, TRUE);
    resident = seg && (seg->seg_Seg == cli->cli_Module || !GetProgramDir());
    Permit();
    if (resident) snprintf(cmd, sizeof cmd, "\"%s\" WAIT%s", base, extra);
    else {
        char path[256];
        if (!(dir = GetProgramDir()) || !NameFromLock(dir, (STRPTR)path, sizeof path) ||
            !AddPart((STRPTR)path, (STRPTR)base, sizeof path))
            return 0;
        snprintf(cmd, sizeof cmd, "\"%s\" WAIT%s", path, extra);
    }
    in = Open((STRPTR)"NIL:", MODE_OLDFILE);
    out = Open((STRPTR)"NIL:", MODE_NEWFILE);
    if (!in || !out) { if (in) Close(in); if (out) Close(out); return 0; }
    rc = SystemTags((STRPTR)cmd, SYS_Input, (ULONG)in, SYS_Output, (ULONG)out, SYS_Asynch, TRUE,
                    NP_StackSize, (ULONG)__stack, NP_ConsoleTask, (ULONG)NULL, NP_WindowPtr, (ULONG)NULL, TAG_DONE);
    if (rc == -1) { Close(in); Close(out); return 0; }
    return 1;
}

int main(void)
{
    struct MsgPort *fp;
    int other, rc;
    char view[40];
    view[0] = 0;
    om_first_task[0] = 0;
    if (Cli()) {
        /* WAIT/S: stay in this Shell until the window closes; VIEW/K: the view to open on (Tasks, Memory...);
         * TASK/K: the task to choose in the Tasks view, by its name; PUBSCREEN/K: the public screen to open on */
        LONG arg[4] = { 0, 0, 0, 0 };
        struct RDArgs *ra = ReadArgs((STRPTR)"WAIT/S,VIEW/K,TASK/K,PUBSCREEN/K", arg, NULL);
        if (!ra) { PrintFault(IoErr(), (STRPTR)"OpenMonitor"); return RETURN_ERROR; }
        if (arg[1]) { strncpy(view, (const char *)arg[1], sizeof view - 1); view[sizeof view - 1] = 0; }
        if (arg[2]) { strncpy(om_first_task, (const char *)arg[2], sizeof om_first_task - 1); om_first_task[sizeof om_first_task - 1] = 0; }
        if (arg[3]) { strncpy(pubscreen, (const char *)arg[3], sizeof pubscreen - 1); pubscreen[sizeof pubscreen - 1] = 0; }
        FreeArgs(ra);
        if (!arg[0]) {
            if (send_to_front()) return RETURN_OK;
            if (detach(view[0] ? view : NULL, om_first_task[0] ? om_first_task : NULL)) return RETURN_OK;
        }
    }
    fp = claim_front(&other);
    if (other) return RETURN_OK;
    front_port = fp;
    rc = gui(view[0] ? (STRPTR)view : NULL);
    front_port = NULL;
    release_front(fp);
    return rc;
}
