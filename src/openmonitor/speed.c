/* GFOpenMonitor: the Speed view (the design, section 4.8): SysInfo's part.
 *
 * ACInfo SPEED's test, built in (sysprobe's sp_speed, which ACInfo runs too): copy speed in chip and fast
 * memory, then a Dhrystone-style loop for a second under Forbid(), timed by the E clock, in SysInfo's units,
 * against six Amigas on a log scale. Each run of this session is kept, so a slow one stands out: on
 * AmiCradle the score follows the PC's load. On AmiCradle a run can be made at a chosen pace
 * (accontrol.device), which is put back afterwards.
 *
 * Nothing runs until asked: the machine stops for about 1.3 seconds while it measures, and the view says so.
 *
 * MIT, Copyright (c) 2026 AmigaChrome Team. */

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/datetime.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <string.h>
#include <stdio.h>

#include "om.h"
#include "sysprobe.h"

#define MAXRUN 24
struct run { ULONG dhry, chip, fast, pace; UBYTE pace_known; char when[10]; };
static struct run runs[MAXRUN];
static int nruns, order[MAXRUN];

/* the paces offered on AmiCradle (Hz; 0: hyper); the first entry leaves it as it is */
static const ULONG paces[] = { 0, 7093790, 14187580, 28375160, 50000000, 0 };
static const char *const pace_labels[] = { "Pace: as it is", "7 MHz (an A500)", "14 MHz (an A1200)", "28 MHz", "50 MHz", "hyper", NULL };
static int pace_choice, ac_present;

enum { C_WHEN, C_MIPS, C_DHRY, C_CHIP, C_FAST, C_PACE, C_N };
static struct om_col cols[C_N] = {
    { "When", 8, OM_LEFT, 0, 1, 0 },
    { "MIPS", 7, OM_RIGHT, 0, 0, 1 },
    { "Dhrystones", 12, OM_RIGHT, 0, 0, 1 },
    { "Chip KB/s", 11, OM_RIGHT, 0, 0, 1 },
    { "Fast KB/s", 11, OM_RIGHT, 0, 0, 1 },
    { "Pace", 10, OM_LEFT, 0, 0, 0 },
};
static struct om_table T;

enum { G_RUN, G_RUN3, G_PACE, G_COPY, G_COUNT };
static struct Gadget *gad[G_COUNT];
static int gx[G_COUNT], gw[G_COUNT], gy, gh, top_y, top_h, bars_y, bars_h, head_y;

static ULONG mips_of(ULONG dhry) { return dhry / SP_SYSINFO_MIPS; }

static void pace_text(const struct run *r, char *buf, int size)
{
    if (!r->pace_known) snprintf(buf, size, "-");
    else if (!r->pace) snprintf(buf, size, "hyper");
    else snprintf(buf, size, "%lu MHz", (unsigned long)((r->pace + 500000) / 1000000));
}

static void raw_cell(int row, int col, char *buf, int size, LONG *pen)
{
    struct run *r;
    char a[16];
    if (row < 0 || row >= nruns) return;
    r = &runs[row];
    switch (col) {
    case C_WHEN: snprintf(buf, size, "%s", r->when); break;
    case C_MIPS: snprintf(buf, size, "%s", om_commas(mips_of(r->dhry), a)); break;
    case C_DHRY: snprintf(buf, size, "%s", om_commas(r->dhry, a)); break;
    case C_CHIP: snprintf(buf, size, "%s", r->chip ? om_commas(r->chip, a) : "no memory"); break;
    case C_FAST: snprintf(buf, size, "%s", r->fast ? om_commas(r->fast, a) : "no memory"); break;
    case C_PACE: pace_text(r, buf, size); break;
    }
}

static void cell(int row, int col, char *buf, int size, LONG *pen)
{
    if (row >= 0 && row < nruns) raw_cell(order[row], col, buf, size, pen);
}

static void make_order(void)
{
    int i;
    for (i = 0; i < nruns; i++) order[i] = nruns - 1 - i;      /* the newest first, until a column is clicked */
    om_table_sort(&T, raw_cell, order, nruns);
}

int om_speed_last(ULONG *mips, char *when)
{
    if (!nruns) return 0;
    *mips = mips_of(runs[nruns - 1].dhry);
    strcpy(when, runs[nruns - 1].when);
    return 1;
}

/* ---- measuring ---------------------------------------------------------------------------------------- */

static void now_text(char *buf)
{
    struct DateTime dt;
    memset(&dt, 0, sizeof dt);
    DateStamp(&dt.dat_Stamp);
    dt.dat_Format = FORMAT_DOS;
    dt.dat_StrTime = (STRPTR)buf;
    if (!DateToStr(&dt)) strcpy(buf, "?");
}

static void measure(int times)
{
    struct sp_ac ac;
    ULONG was = 0;
    int set = 0, k;
    ac_present = sp_ac(&ac, NULL, 0);
    if (ac_present && pace_choice > 0) {
        was = ac.pace;
        if (sp_ac_setpace(paces[pace_choice]) < 0) { om_status(0, "The machine didn't take that pace."); return; }
        set = 1;
    }
    for (k = 0; k < times; k++) {
        struct sp_speed s;
        struct run *r;
        om_status(0, "Measuring (run %d of %d): the machine waits about 1.3 s...", k + 1, times);
        if (!sp_speed(&s)) { om_status(0, "The speed couldn't be measured (no timer.device, or no memory)."); break; }
        if (nruns == MAXRUN) { memmove(&runs[0], &runs[1], (MAXRUN - 1) * sizeof runs[0]); nruns--; }
        r = &runs[nruns++];
        r->dhry = s.dhry; r->chip = s.chip_kbs; r->fast = s.fast_kbs;
        r->pace_known = ac_present;
        r->pace = set ? paces[pace_choice] : ac.pace;
        now_text(r->when);
    }
    if (set) sp_ac_setpace(was);
    make_order();
    if (nruns) {
        char a[16];
        om_status(0, "%s MIPS%s.", om_commas(mips_of(runs[nruns - 1].dhry), a), set ? " (the pace is put back)" : "");
    }
}

/* ---- drawing ------------------------------------------------------------------------------------------ */

/* log2 in 1/1024ths, for the bars' log scale (no floating point) */
static ULONG lg(ULONG x)
{
    int msb = 31;
    if (!x) return 0;
    while (!(x & (1UL << msb))) msb--;
    return (ULONG)msb * 1024 + (msb >= 10 ? (x >> (msb - 10)) & 1023 : (x << (10 - msb)) & 1023);
}

static void draw_top(void)
{
    struct RastPort *rp = U.win->RPort;
    struct run *r = nruns ? &runs[nruns - 1] : NULL;
    char s[160], a[16], b[16];
    int y = top_y + 8;
    om_sunk(U.px, top_y, U.pw, top_h);
    SetFont(rp, U.font);
    if (!r) {
        ogt_bold(rp, 1);
        ogt_text(rp, U.pen_text, U.px + 10, y, "Not measured yet", U.pw - 20);
        ogt_bold(rp, 0);
        ogt_text(rp, U.pen_muted, U.px + 10, y + U.fh + 6,
                 "Run measures memory copies, then a second of Dhrystones under Forbid(): the machine waits about 1.3 s.", U.pw - 20);
        return;
    }
    snprintf(s, sizeof s, "%s MIPS", om_commas(mips_of(r->dhry), a));
    ogt_bold(rp, 1);
    ogt_text(rp, U.pen_text, U.px + 10, y, s, U.pw - 20);
    ogt_bold(rp, 0);
    snprintf(s, sizeof s, "%s Dhrystones a second (SysInfo's units: MIPS is Dhrystones / %d), at %s", om_commas(r->dhry, a),
             SP_SYSINFO_MIPS, r->when);
    ogt_text(rp, U.pen_muted, U.px + 10, y + U.fh + 4, s, U.pw - 20);
    SetFont(rp, U.tfont);
    snprintf(s, sizeof s, "copy in chip memory %s KB/s    copy in fast memory %s KB/s", om_commas(r->chip, a), om_commas(r->fast, b));
    ogt_text(rp, U.pen_text, U.px + 10, y + 2 * U.fh + 10, s, U.pw - 20);
    SetFont(rp, U.font);
    ogt_text(rp, U.pen_muted, U.px + 10, y + 2 * U.fh + 14 + U.tfh,
             ac_present ? "On AmiCradle the score follows the PC's load: run it again when the PC is quiet." :
                          "The loop runs for a second under Forbid(), timed by the E clock.", U.pw - 20);
}

static void draw_bars(void)
{
    struct RastPort *rp = U.win->RPort;
    ULONG dhry = nruns ? runs[nruns - 1].dhry : 0, top = lg(100000);
    int i, lw = om_text_width(U.tfont, "A4000  68040   25 MHz") + 16, nw = om_text_width(U.tfont, "000,000 x") + 8;
    int bx = U.px + lw, bw = U.pw - lw - nw - 8, rh = (bars_h - (U.fh + 8)) / SP_REFS;
    if (bars_h <= 0) return;
    om_heading(U.px, bars_y, "Times as fast as");
    if (rh > U.tfh + 10) rh = U.tfh + 10;
    for (i = 0; i < SP_REFS; i++) {
        int y = bars_y + U.fh + 8 + i * rh, fill;
        ULONG x100 = dhry ? (ULONG)((unsigned long long)dhry * 100 / sp_refs[i].dhry) : 0;
        char s[24], n[16];
        SetFont(rp, U.tfont);
        ogt_box(rp, U.pen_back, U.px, y, U.pw, rh);
        ogt_text(rp, U.pen_text, U.px, y + (rh - U.tfh) / 2, sp_refs[i].name, lw - 8);
        ogt_box(rp, U.pen_list, bx, y + 2, bw, rh - 4);
        ogt_bevel(rp, U.pen_shadow, U.pen_shine, bx, y + 2, bw, rh - 4);
        fill = x100 > 100 ? (int)((unsigned long long)(bw - 2) * lg(x100 / 100) / top) : 0;
        if (fill > bw - 2) fill = bw - 2;
        if (fill > 0) ogt_box(rp, U.graph_pen[0], bx + 1, y + 3, fill, rh - 6);
        if (x100 >= 10000) snprintf(s, sizeof s, "%s x", om_commas(x100 / 100, n));
        else if (x100) snprintf(s, sizeof s, "%lu.%02lu x", (unsigned long)(x100 / 100), (unsigned long)(x100 % 100));
        else s[0] = 0;
        ogt_text(rp, U.pen_text, U.px + U.pw - om_text_width(U.tfont, s), y + (rh - U.tfh) / 2, s, nw);
    }
    SetFont(rp, U.font);
}

/* ---- the view ----------------------------------------------------------------------------------------- */

static void layout(int x, int y, int w, int h)
{
    int bh = U.fh + 8, i;
    gy = y + h - bh;
    gh = bh;
    if (U.compact) {
        top_y = y; top_h = 2 * U.fh + 14;
        bars_y = 0; bars_h = 0;
        head_y = -1;
        om_table_layout(&T, x, top_y + top_h + 4, w, gy - 4 - (top_y + top_h + 4));
    } else {
        top_y = y; top_h = 3 * U.fh + U.tfh + 30;
        bars_y = top_y + top_h + 12;
        bars_h = U.fh + 8 + SP_REFS * (U.tfh + 10);
        head_y = bars_y + bars_h + 12;
        om_table_layout(&T, x, head_y + U.fh + 6, w, gy - 10 - (head_y + U.fh + 6));
    }
    gw[G_RUN] = om_text_width(U.font, nruns ? "Run again" : "Run") + 40;
    gw[G_RUN3] = om_text_width(U.font, "Run 3 times") + 30;
    gw[G_PACE] = ac_present ? om_text_width(U.font, "14 MHz (an A1200)") + 50 : 0;
    gw[G_COPY] = om_text_width(U.font, U.compact ? "Copy" : "Copy as text") + 30;
    gx[G_RUN] = x;
    gx[G_RUN3] = x + gw[G_RUN] + 8;
    gx[G_PACE] = gx[G_RUN3] + gw[G_RUN3] + 8;
    gx[G_COPY] = x + w - gw[G_COPY];
    for (i = 0; i < G_COUNT; i++) gad[i] = NULL;
}

static struct Gadget *gadgets(struct Gadget *g)
{
    struct NewGadget ng;
    int i;
    memset(&ng, 0, sizeof ng);
    ng.ng_TextAttr = &U.ta; ng.ng_VisualInfo = U.vi; ng.ng_Flags = PLACETEXT_IN;
    for (i = 0; i < G_COUNT && g; i++) {
        if (!gw[i]) continue;
        ng.ng_LeftEdge = gx[i]; ng.ng_TopEdge = gy; ng.ng_Width = gw[i]; ng.ng_Height = gh; ng.ng_GadgetID = GID_SPEED + i;
        if (i == G_PACE) {
            ng.ng_GadgetText = NULL;
            g = gad[i] = CreateGadget(CYCLE_KIND, g, &ng, GTCY_Labels, (ULONG)pace_labels, GTCY_Active, (ULONG)pace_choice, TAG_DONE);
        } else {
            static const char *const labels[G_COUNT] = { "Run", "Run 3 times", "", "Copy as text" };
            ng.ng_GadgetText = (STRPTR)(i == G_RUN && nruns ? "Run again" : i == G_COPY && U.compact ? "Copy" : labels[i]);
            g = gad[i] = CreateGadget(BUTTON_KIND, g, &ng, TAG_DONE);
        }
    }
    return g;
}

static void draw(void)
{
    draw_top();
    draw_bars();
    if (head_y >= 0) om_heading(U.px, head_y, "Runs since GFOpenMonitor started");
    om_table_set_count(&T, nruns);
    om_table_draw(&T);
}

static void status(void)
{
    om_status(1, "ACInfo SPEED's test, built in");
    om_status(2, "The machine waits about 1.3 s while it runs");
}

static void run(int times)
{
    int first = !nruns;
    measure(times);
    if (first) om_rebuild_gadgets();            /* "Run" becomes "Run again" */
    else { draw_top(); draw_bars(); om_table_set_count(&T, nruns); om_table_draw_rows(&T); }
}

static int event(ULONG cls, UWORD code, UWORD qual, APTR ia, int mx, int my, ULONG secs, ULONG micros)
{
    int picked = 0, row = -1, sorted = 0;
    (void)qual;
    if (om_table_event(&T, cls, code, ia, mx, my, secs, micros, &picked, &row, &sorted)) {
        if (sorted) { make_order(); om_table_draw_rows(&T); }
        return 1;
    }
    if (cls == IDCMP_GADGETUP && ia) {
        switch (((struct Gadget *)ia)->GadgetID - GID_SPEED) {
        case G_RUN: run(1); return 1;
        case G_RUN3: run(3); return 1;
        case G_PACE: pace_choice = code; return 1;
        case G_COPY: om_status(0, om_table_copy(&T) ? "The runs are on the clipboard." : "The clipboard could not be written."); return 1;
        }
    }
    if (cls == IDCMP_VANILLAKEY) {
        switch (code) {
        case 'r': case 'R': case 13: run(1); return 1;
        case 't': case 'T': run(3); return 1;
        case 'p': case 'P':                     /* the next pace (on AmiCradle) */
            if (ac_present && gad[G_PACE]) {
                pace_choice = (pace_choice + 1) % (int)(sizeof paces / sizeof paces[0]);
                GT_SetGadgetAttrs(gad[G_PACE], U.win, NULL, GTCY_Active, (ULONG)pace_choice, TAG_DONE);
            }
            return 1;
        case 'c': case 'C': om_status(0, om_table_copy(&T) ? "The runs are on the clipboard." : "The clipboard could not be written."); return 1;
        }
    }
    return 0;
}

static void shown(int on)
{
    if (!on) { om_table_free(&T); return; }
    {
        struct sp_ac ac;
        ac_present = sp_ac(&ac, NULL, 0);
    }
}

const struct om_view om_speed_view = { "Speed", layout, gadgets, draw, NULL, event, shown, status };

void om_speed_init(void)
{
    nruns = 0;
    pace_choice = 0;
    T.sort = -1;
    om_table_init(&T, cols, C_N, cell, GID_SPEED + 50, "Speed");
}
