/* OpenClock: the clock on the desktop, in the Open family's look. It takes over SYS:Utilities/Clock
 * (OpenUp's RECONCILE retires that one).
 *
 *   OpenClock [DIGITAL | ANALOGUE | BOTH] [SECONDS | NOSECONDS] [AMPM | H24]
 *
 * An analogue face drawn from the theme, a digital line under it, and the date. The window can be
 * resized and the face follows it; the view, seconds and 12- or 24-hour are on the menu and are kept
 * in ENVARC:OpenClock/Clock, so the window comes back as it was left.
 *
 * No maths library and no floating point: the hands come from a table of sines for 60 positions,
 * scaled by 1024, so this runs with no FPU, as every Open app must.
 *
 * MIT, Copyright (c) 2026 AmigaChrome Team. */
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <devices/timer.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <utility/date.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#include <string.h>
#include <stdio.h>

#include "ogt_theme.h"
#include "ogt_draw.h"
#include "ogt_look.h"
#include "ogt_font.h"

static const char version[] __attribute__((used)) = "$VER: OpenClock 0.1 (10.10.2026) OpenUp, MIT, Copyright (c) 2026 AmigaChrome Team";
#define PREFS_ENV "ENV:OpenClock/Clock"
#define PREFS_ENVARC "ENVARC:OpenClock/Clock"

enum { V_ANALOGUE, V_DIGITAL, V_BOTH };
enum { M_ANALOGUE = 1, M_DIGITAL, M_BOTH, M_SECONDS, M_H24, M_DATE, M_SAVE, M_QUIT };

struct Library *UtilityBase;              /* proto/utility.h declares it: not static */
static struct Window *win;
static struct Screen *scr;
static APTR vi;
static struct Menu *menu;
static ogt_theme theme;
static ogt_ctx ctx;
static struct TextFont *font;     /* the theme's font, as every Open app draws in */
static struct TextAttr font_ta;
static int theme_mode;

static int view = V_BOTH, show_secs = 1, h24 = 1, show_date = 1;
static int last_sec = -1, last_min = -1;

/* sin(n * 6 degrees) * 1024, for n of 0 to 15; the rest by symmetry (sin_q below). */
static const short sine_q[16] = {
    0, 107, 213, 316, 416, 512, 603, 688, 765, 836, 898, 950, 992, 1014, 1023, 1024
};

/* sin and cos of n sixtieths of a turn, scaled by 1024. */
static int sin60(int n)
{
    n = ((n % 60) + 60) % 60;
    if (n <= 15) return sine_q[n];
    if (n <= 30) return sine_q[30 - n];
    if (n <= 45) return -sine_q[n - 30];
    return -sine_q[60 - n];
}
static int cos60(int n) { return sin60(n + 15); }

static void settings_read(void)
{
    BPTR fh = Open((STRPTR)PREFS_ENV, MODE_OLDFILE);
    char buf[160], *p;
    LONG n;
    if (!fh) return;
    n = Read(fh, buf, sizeof buf - 1);
    Close(fh);
    if (n <= 0) return;
    buf[n] = 0;
    for (p = buf; *p; p++) if (*p == '\n') *p = ' ';
    if (strstr(buf, "view analogue")) view = V_ANALOGUE;
    else if (strstr(buf, "view digital")) view = V_DIGITAL;
    else if (strstr(buf, "view both")) view = V_BOTH;
    if (strstr(buf, "seconds off")) show_secs = 0;
    if (strstr(buf, "hours 12")) h24 = 0;
    if (strstr(buf, "date off")) show_date = 0;
}

static void settings_write(const char *path)
{
    BPTR fh;
    char buf[200];
    int n;
    if (!path) return;
    fh = Open((STRPTR)path, MODE_NEWFILE);
    if (!fh) return;
    n = snprintf(buf, sizeof buf, "; OpenClock's settings, written by the Settings menu\nview %s\nseconds %s\nhours %d\ndate %s\n",
                 view == V_ANALOGUE ? "analogue" : view == V_DIGITAL ? "digital" : "both",
                 show_secs ? "on" : "off", h24 ? 24 : 12, show_date ? "on" : "off");
    Write(fh, buf, n);
    Close(fh);
}

static void settings_save(void)
{
    BPTR lock = CreateDir((STRPTR)"ENV:OpenClock");
    if (lock) UnLock(lock);
    lock = CreateDir((STRPTR)"ENVARC:OpenClock");
    if (lock) UnLock(lock);
    settings_write(PREFS_ENV);
    settings_write(PREFS_ENVARC);
}

/* The time now, as hours, minutes, seconds and the date, from DateStamp. */
typedef struct { int h, m, s, mday, month, year, wday; } when;
static const char *const months[12] = { "January", "February", "March", "April", "May", "June",
                                        "July", "August", "September", "October", "November", "December" };
static const char *const days[7] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

static void now(when *t)
{
    struct DateStamp ds;
    struct ClockData cd;
    DateStamp(&ds);
    t->h = ds.ds_Minute / 60;
    t->m = ds.ds_Minute % 60;
    t->s = ds.ds_Tick / TICKS_PER_SECOND;
    Amiga2Date((ULONG)ds.ds_Days * 86400UL + (ULONG)ds.ds_Minute * 60UL + (ULONG)t->s, &cd);
    t->mday = cd.mday; t->month = cd.month; t->year = cd.year; t->wday = cd.wday;
}

static void time_text(const when *t, char *out, size_t cap)
{
    int h = t->h;
    const char *ap = "";
    if (!h24) { ap = h < 12 ? " am" : " pm"; h = h % 12; if (!h) h = 12; }
    if (show_secs) snprintf(out, cap, "%d:%02d:%02d%s", h, t->m, t->s, ap);
    else snprintf(out, cap, "%d:%02d%s", h, t->m, ap);
}

/* A hand from the middle, n sixtieths round, len long, as a line w wide. */
static void hand(struct RastPort *rp, LONG pen, int cx, int cy, int n, int len, int w)
{
    int dx = sin60(n) * len / 1024, dy = -cos60(n) * len / 1024, i;
    SetAPen(rp, pen);
    for (i = -(w / 2); i <= w / 2; i++) {
        Move(rp, cx + i, cy);
        Draw(rp, cx + dx + i, cy + dy);
        if (w > 1) { Move(rp, cx, cy + i); Draw(rp, cx + dx, cy + dy + i); }
    }
}

static void draw_face(struct RastPort *rp, const when *t, int x, int y, int size)
{
    int cx = x + size / 2, cy = y + size / 2, r = size / 2 - 2, i;
    LONG face = ogt_pen(&ctx, "string"), line = ogt_pen(&ctx, "muted"), text = ogt_pen(&ctx, "label"),
         accent = ogt_pen(&ctx, "accent");
    if (r < 8) return;
    ogt_fill_circle(rp, face, cx, cy, r);
    SetAPen(rp, line);
    for (i = 0; i < 60; i += 5) {                        /* the hours, as marks */
        int out = r - 2, in = r - (i % 15 == 0 ? 8 : 5);
        int sx = sin60(i), sy = -cos60(i);
        Move(rp, cx + sx * in / 1024, cy + sy * in / 1024);
        Draw(rp, cx + sx * out / 1024, cy + sy * out / 1024);
    }
    hand(rp, text, cx, cy, (t->h % 12) * 5 + t->m / 12, r * 1 / 2, 3);
    hand(rp, text, cx, cy, t->m, r * 3 / 4, 2);
    if (show_secs) hand(rp, accent, cx, cy, t->s, r * 7 / 8, 1);
    ogt_fill_circle(rp, text, cx, cy, 2);
}

static void draw_all(void)
{
    struct RastPort *rp;
    when t;
    char buf[64];
    int x, y, w, h, size, dy;
    LONG text = ogt_pen(&ctx, "label"), muted = ogt_pen(&ctx, "muted");
    if (!win) return;
    rp = win->RPort;
    x = win->BorderLeft; y = win->BorderTop;
    w = win->Width - win->BorderLeft - win->BorderRight;
    h = win->Height - win->BorderTop - win->BorderBottom;
    if (w <= 0 || h <= 0) return;
    now(&t);
    last_sec = t.s; last_min = t.m;
    ogt_fill(&ctx, rp, "window", x, y, w, h);
    dy = y + 6;
    if (view != V_DIGITAL) {
        size = h - 12 - (view == V_BOTH ? rp->TxHeight + 6 : 0) - (show_date ? rp->TxHeight + 4 : 0);
        if (size > w - 12) size = w - 12;
        if (size > 24) {
            draw_face(rp, &t, x + (w - size) / 2, dy, size);
            dy += size + 6;
        }
    }
    if (view != V_ANALOGUE) {
        time_text(&t, buf, sizeof buf);
        ogt_bold(rp, 1);
        ogt_text(rp, text, x + (w - ogt_text_width(rp, buf)) / 2, dy, buf, w);
        ogt_bold(rp, 0);
        dy += rp->TxHeight + 4;
    }
    if (show_date) {
        snprintf(buf, sizeof buf, "%s %d %s %d", days[t.wday % 7], t.mday, months[(t.month - 1) % 12], t.year);
        ogt_text(rp, muted, x + (w - ogt_text_width(rp, buf)) / 2, dy, buf, w);
    }
}

/* Only the parts that change, each tick: the whole window is drawn again, which is cheap at this
 * size and always right. A second's tick with no seconds shown does nothing. */
static void tick(void)
{
    when t;
    now(&t);
    if (!show_secs && view == V_DIGITAL && t.m == last_min) return;
    if (t.s == last_sec && t.m == last_min) return;
    draw_all();
}

static struct NewMenu menus[] = {
    { NM_TITLE, (STRPTR)"Clock", 0, 0, 0, 0 },
    { NM_ITEM, (STRPTR)"Save settings", (STRPTR)"S", 0, 0, (APTR)M_SAVE },
    { NM_ITEM, NM_BARLABEL, 0, 0, 0, 0 },
    { NM_ITEM, (STRPTR)"Quit", (STRPTR)"Q", 0, 0, (APTR)M_QUIT },
    { NM_TITLE, (STRPTR)"View", 0, 0, 0, 0 },
    { NM_ITEM, (STRPTR)"Analogue", 0, CHECKIT | MENUTOGGLE, ~1, (APTR)M_ANALOGUE },
    { NM_ITEM, (STRPTR)"Digital", 0, CHECKIT | MENUTOGGLE, ~2, (APTR)M_DIGITAL },
    { NM_ITEM, (STRPTR)"Both", 0, CHECKIT | MENUTOGGLE, ~4, (APTR)M_BOTH },
    { NM_ITEM, NM_BARLABEL, 0, 0, 0, 0 },
    { NM_ITEM, (STRPTR)"Seconds", 0, CHECKIT | MENUTOGGLE, 0, (APTR)M_SECONDS },
    { NM_ITEM, (STRPTR)"24 hour", 0, CHECKIT | MENUTOGGLE, 0, (APTR)M_H24 },
    { NM_ITEM, (STRPTR)"Date", 0, CHECKIT | MENUTOGGLE, 0, (APTR)M_DATE },
    { NM_END, 0, 0, 0, 0, 0 }
};

static void mark_menus(void)
{
    struct MenuItem *it;
    int i;
    static const int items[6] = { M_ANALOGUE, M_DIGITAL, M_BOTH, M_SECONDS, M_H24, M_DATE };
    static const UWORD where[6] = { 0, 1, 2, 4, 5, 6 };
    if (!menu || !win) return;
    for (i = 0; i < 6; i++) {
        int on = items[i] == M_ANALOGUE ? view == V_ANALOGUE : items[i] == M_DIGITAL ? view == V_DIGITAL :
                 items[i] == M_BOTH ? view == V_BOTH : items[i] == M_SECONDS ? show_secs :
                 items[i] == M_H24 ? h24 : show_date;
        if ((it = ItemAddress(menu, FULLMENUNUM(1, where[i], 0)))) {
            if (on) it->Flags |= CHECKED; else it->Flags &= ~CHECKED;
        }
    }
}

int main(int argc, char **argv)
{
    struct MsgPort *tport = NULL;
    struct timerequest *treq = NULL;
    LONG args[3] = { 0, 0, 0 };
    struct RDArgs *rd = NULL;
    int quit = 0, W = 170, H = 210;
    ULONG wsig, tsig;
    (void)version;
    (void)argv;
    if (!(UtilityBase = OpenLibrary((STRPTR)"utility.library", 39))) return RETURN_FAIL;
    settings_read();
    if (argc > 0) {
        rd = ReadArgs((STRPTR)"DIGITAL/S,ANALOGUE/S,SECONDS/S", args, NULL);
        if (rd) {
            if (args[0]) view = V_DIGITAL;
            if (args[1]) view = V_ANALOGUE;
            if (args[2]) show_secs = 1;
            FreeArgs(rd);
        }
    }
    ogt_look_load(&theme, &theme_mode);
    if (!(scr = LockPubScreen(NULL))) { CloseLibrary(UtilityBase); return RETURN_FAIL; }
    vi = GetVisualInfoA(scr, NULL);
    ogt_ctx_init(&ctx, scr, &theme, theme_mode);
    if (view == V_DIGITAL) { W = 160; H = 60; }
    win = OpenWindowTags(NULL, WA_Title, (ULONG)"Clock", WA_ScreenTitle, (ULONG)"OpenClock 0.1",
                         WA_PubScreen, (ULONG)scr,
                         WA_InnerWidth, W, WA_InnerHeight, H,
                         WA_Left, scr->Width - W - 60 > 0 ? scr->Width - W - 60 : 0, WA_Top, scr->BarHeight + 24,
                         WA_MinWidth, 90, WA_MinHeight, 44, WA_MaxWidth, ~0, WA_MaxHeight, ~0,
                         WA_DragBar, TRUE, WA_DepthGadget, TRUE, WA_CloseGadget, TRUE, WA_SizeGadget, TRUE,
                         WA_Activate, TRUE, WA_SmartRefresh, TRUE, WA_NewLookMenus, TRUE,
                         WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW | IDCMP_NEWSIZE | IDCMP_MENUPICK,
                         TAG_DONE);
    if (!win) goto out;
    if (vi && (menu = CreateMenus(menus, TAG_DONE)) && LayoutMenus(menu, vi, GTMN_NewLookMenus, TRUE, TAG_DONE)) {
        mark_menus();
        SetMenuStrip(win, menu);
    }
    font = ogt_open_font(&theme, scr, &font_ta);
    SetFont(win->RPort, font ? font : scr->RastPort.Font);
    draw_all();

    /* a tick a second, from timer.device */
    if ((tport = CreateMsgPort())) {
        if ((treq = (struct timerequest *)CreateIORequest(tport, sizeof *treq))) {
            if (OpenDevice((STRPTR)TIMERNAME, UNIT_VBLANK, (struct IORequest *)treq, 0)) {
                DeleteIORequest((struct IORequest *)treq); treq = NULL;
            }
        }
    }
    wsig = 1UL << win->UserPort->mp_SigBit;
    tsig = tport ? 1UL << tport->mp_SigBit : 0;
    if (treq) {
        treq->tr_node.io_Command = TR_ADDREQUEST;
        treq->tr_time.tv_secs = 0; treq->tr_time.tv_micro = 250000;
        SendIO((struct IORequest *)treq);
    }
    while (!quit) {
        ULONG got = Wait(wsig | tsig);
        struct IntuiMessage *m;
        if (treq && (got & tsig) && GetMsg(tport)) {
            tick();
            treq->tr_node.io_Command = TR_ADDREQUEST;
            treq->tr_time.tv_secs = 0; treq->tr_time.tv_micro = 250000;
            SendIO((struct IORequest *)treq);
        }
        while ((m = (struct IntuiMessage *)GetMsg(win->UserPort))) {
            ULONG cls = m->Class;
            UWORD code = m->Code;
            ReplyMsg((struct Message *)m);
            if (cls == IDCMP_CLOSEWINDOW) quit = 1;
            else if (cls == IDCMP_NEWSIZE || cls == IDCMP_REFRESHWINDOW) {
                if (cls == IDCMP_REFRESHWINDOW) { BeginRefresh(win); EndRefresh(win, TRUE); }
                draw_all();
            } else if (cls == IDCMP_MENUPICK) {
                while (code != MENUNULL && !quit) {
                    struct MenuItem *it = ItemAddress(menu, code);
                    if (!it) break;
                    switch ((ULONG)GTMENUITEM_USERDATA(it)) {
                    case M_ANALOGUE: view = V_ANALOGUE; break;
                    case M_DIGITAL: view = V_DIGITAL; break;
                    case M_BOTH: view = V_BOTH; break;
                    case M_SECONDS: show_secs = !show_secs; break;
                    case M_H24: h24 = !h24; break;
                    case M_DATE: show_date = !show_date; break;
                    case M_SAVE: settings_save(); break;
                    case M_QUIT: quit = 1; break;
                    }
                    code = it->NextSelect;
                }
                mark_menus();
                draw_all();
            }
        }
    }
    if (treq) {
        if (!CheckIO((struct IORequest *)treq)) AbortIO((struct IORequest *)treq);
        WaitIO((struct IORequest *)treq);
        CloseDevice((struct IORequest *)treq);
        DeleteIORequest((struct IORequest *)treq);
    }
    if (tport) DeleteMsgPort(tport);
out:
    if (win) { if (menu) ClearMenuStrip(win); CloseWindow(win); }
    if (font) CloseFont(font);
    if (menu) FreeMenus(menu);
    if (vi) FreeVisualInfo(vi);
    if (scr) UnlockPubScreen(NULL, scr);
    CloseLibrary(UtilityBase);
    return RETURN_OK;
}
