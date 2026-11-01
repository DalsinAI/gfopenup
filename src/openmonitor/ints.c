/* GFOpenMonitor: the Interrupts view (the design, section 4.6).
 *
 * Exec's 16 interrupt vectors by level: a handler's name, priority, code and data, and for each of the five
 * that share (PORTS, COPER, VERTB, EXTER, NMI) every server on its chain. Where: the code is in the ROM (its
 * image, or its unpacked copy) or in RAM (a program's). Per second: only the vertical blank's rate is known
 * until exec counts each level (the design's P4).
 *
 * Read every 5 seconds while shown, under Disable (exec changes the chains under Disable), into a copy.
 *
 * MIT, Copyright (c) 2026 AmigaChrome Team. */

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/interrupts.h>
#include <hardware/intbits.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <string.h>
#include <stdio.h>

#include "om.h"

#define MAXI 64

struct irow {
    UBYTE vec, server, empty;
    BYTE pri;
    APTR code, data;
    char name[36];
    const char *where;
};
static struct irow *rows;               /* allocated, as the other views' */
static int nrows, *order, ticks;

static const char *const vec_name[16] = { "TBE", "DSKBLK", "SOFTINT", "PORTS", "COPER", "VERTB", "BLIT", "AUD0",
                                          "AUD1", "AUD2", "AUD3", "RBF", "DSKSYNC", "EXTER", "INTEN", "NMI" };
static const UBYTE vec_level[16] = { 1, 1, 1, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 6, 6, 7 };

static int shares(int v)
{
    return v == INTB_PORTS || v == INTB_COPER || v == INTB_VERTB || v == INTB_EXTER || v == INTB_NMI;
}

enum { C_LEVEL, C_INT, C_KIND, C_NAME, C_PRI, C_CODE, C_DATA, C_WHERE, C_RATE, C_N };
static struct om_col cols[C_N] = {
    { "Level", 5, OM_RIGHT, 0, 0, 0 },
    { "Interrupt", 8, OM_LEFT, 0, 0, 0 },
    { "Kind", 7, OM_LEFT, 0, 0, 0 },
    { "Name", 24, OM_LEFT, 0, 1, 0 },
    { "Pri", 4, OM_RIGHT, 0, 0, 1 },
    { "Code", 9, OM_LEFT, 0, 0, 0 },
    { "Data", 9, OM_LEFT, 0, 0, 0 },
    { "Where", 5, OM_LEFT, 0, 0, 0 },
    { "Per second", 10, OM_RIGHT, 0, 0, 1 },
};
static struct om_table T;

enum { G_COPY, G_COUNT };
static struct Gadget *gad[G_COUNT];
static int gx, gw, gy, gh, head_y = -1, note_y, note_h;

static void copy_name(char *to, const char *from, int size)
{
    int i;
    if (!from) { strcpy(to, "(no name)"); return; }
    for (i = 0; i < size - 1 && from[i] && from[i] != '\n'; i++) to[i] = from[i];
    to[i] = 0;
}

static void raw_cell(int row, int col, char *buf, int size, LONG *pen);

static void take(void)
{
    int v, i;
    nrows = 0;
    if (!rows || !order) return;
    Disable();
    for (v = 0; v < 16 && nrows < MAXI; v++) {
        struct IntVector *iv = &SysBase->IntVects[v];
        if (shares(v)) {
            struct List *l = (struct List *)iv->iv_Data;
            struct Node *n;
            int any = 0;
            for (n = l ? l->lh_Head : NULL; n && n->ln_Succ && nrows < MAXI; n = n->ln_Succ) {
                struct Interrupt *is = (struct Interrupt *)n;
                struct irow *r = &rows[nrows++];
                r->vec = v; r->server = 1; r->empty = 0;
                r->pri = n->ln_Pri;
                r->code = (APTR)is->is_Code;
                r->data = is->is_Data;
                copy_name(r->name, n->ln_Name, sizeof r->name);
                any = 1;
            }
            if (!any && nrows < MAXI) {
                struct irow *r = &rows[nrows++];
                memset(r, 0, sizeof *r);
                r->vec = v; r->server = 1; r->empty = 1;
                strcpy(r->name, "-");
            }
        } else if (iv->iv_Code || iv->iv_Node) {
            struct irow *r = &rows[nrows++];
            r->vec = v; r->server = 0; r->empty = 0;
            r->pri = iv->iv_Node ? iv->iv_Node->ln_Pri : 0;
            r->code = (APTR)iv->iv_Code;
            r->data = iv->iv_Data;
            copy_name(r->name, iv->iv_Node ? iv->iv_Node->ln_Name : v == INTB_SOFTINT ? "exec's soft interrupts" : "(no node)", sizeof r->name);
        }
    }
    Enable();
    for (i = 0; i < nrows; i++) {
        rows[i].where = rows[i].empty ? "-" : om_where(rows[i].code);
        if (!strcmp(rows[i].where, "ROM group")) rows[i].where = "ROM";
        order[i] = i;
    }
    om_table_sort(&T, raw_cell, order, nrows);
}

static void raw_cell(int row, int col, char *buf, int size, LONG *pen)
{
    struct irow *r;
    if (row < 0 || row >= nrows) return;
    r = &rows[row];
    switch (col) {
    case C_LEVEL: snprintf(buf, size, "%u", vec_level[r->vec]); break;
    case C_INT: snprintf(buf, size, "%s", vec_name[r->vec]); break;
    case C_KIND: snprintf(buf, size, "%s", r->server ? "server" : "handler"); break;
    case C_NAME: snprintf(buf, size, "%s", r->name); if (r->empty || !strcmp(r->name, "(no name)")) *pen = U.pen_muted; break;
    case C_PRI: if (!r->empty) snprintf(buf, size, "%d", r->pri); break;
    case C_CODE: if (!r->empty) snprintf(buf, size, "$%08lx", (unsigned long)r->code); else snprintf(buf, size, "-"); *pen = U.pen_muted; break;
    case C_DATA: if (!r->empty) snprintf(buf, size, "$%08lx", (unsigned long)r->data); else snprintf(buf, size, "-"); *pen = U.pen_muted; break;
    case C_WHERE: snprintf(buf, size, "%s", r->where); if (!strcmp(r->where, "RAM")) *pen = U.pen_amber; break;
    case C_RATE:                        /* exec's counts (GFOpenExec's task clock), else the VBlank's rate only */
        if (r->empty) break;
        if (om_irq_per_second(r->vec) >= 0) snprintf(buf, size, "%d", om_irq_per_second(r->vec));
        else if (r->vec == INTB_VERTB) snprintf(buf, size, "%u", SysBase->VBlankFrequency ? SysBase->VBlankFrequency : 50);
        break;
    }
}

static void cell(int row, int col, char *buf, int size, LONG *pen)
{
    if (row >= 0 && row < nrows) raw_cell(order[row], col, buf, size, pen);
}

/* ---- the view ----------------------------------------------------------------------------------------- */

static void draw_note(void)
{
    struct RastPort *rp = U.win->RPort;
    char s[200];
    int i, len, ram = 0;
    if (note_h <= 0) return;
    om_sunk(U.px, note_y, U.pw, note_h);
    len = snprintf(s, sizeof s, "Where: ROM is code in the ROM image; RAM is a program's");
    for (i = 0; i < nrows && len < (int)sizeof s - 40; i++)
        if (!strcmp(rows[i].where, "RAM"))
            len += snprintf(s + len, sizeof s - len, "%s%s", ram++ ? ", " : ": ",
                            strcmp(rows[i].name, "(no name)") ? rows[i].name : "one with no name");
    if (ram) snprintf(s + len, sizeof s - len, " here.");
    else snprintf(s + len, sizeof s - len, ": none here.");
    SetFont(rp, U.font);
    ogt_text(rp, U.pen_muted, U.px + 8, note_y + 4, s, U.pw - 16);
    ogt_text(rp, U.pen_muted, U.px + 8, note_y + 6 + U.fh, om_irq_per_second(INTB_VERTB) >= 0 ?
             "Per second: exec's own counts, each vector's (GFOpenExec's task clock); a server's is its vector's." :
             "Per second: the vertical blank's rate only; the others need GFOpenExec's task clock (the open ROM).", U.pw - 16);
}

static void layout(int x, int y, int w, int h)
{
    int bh = U.fh + 8, ty;
    gy = y + h - bh;
    gh = bh;
    gw = om_text_width(U.font, U.compact ? "Copy" : "Copy as text") + 30;
    gx = x + w - gw;
    if (U.compact) { head_y = -1; note_h = 0; ty = y; }
    else { head_y = y; note_h = 2 * U.fh + 12; ty = y + U.fh + 6; }
    om_table_layout(&T, x, ty, w, gy - (U.compact ? 4 : 10) - (note_h ? note_h + 8 : 0) - ty);
    note_y = T.y + T.h + 8;
}

static struct Gadget *gadgets(struct Gadget *g)
{
    struct NewGadget ng;
    memset(&ng, 0, sizeof ng);
    ng.ng_TextAttr = &U.ta; ng.ng_VisualInfo = U.vi; ng.ng_Flags = PLACETEXT_IN;
    ng.ng_LeftEdge = gx; ng.ng_TopEdge = gy; ng.ng_Width = gw; ng.ng_Height = gh;
    ng.ng_GadgetText = (STRPTR)(U.compact ? "Copy" : "Copy as text"); ng.ng_GadgetID = GID_INTS + G_COPY;
    return gad[G_COPY] = CreateGadget(BUTTON_KIND, g, &ng, TAG_DONE);
}

static void draw(void)
{
    take();
    if (head_y >= 0) om_heading(U.px, head_y, "Exec's 16 interrupts, and the servers on the five that share");
    om_table_set_count(&T, nrows);
    om_table_draw(&T);
    draw_note();
}

static void tick(void)
{
    if (++ticks < 5) return;
    ticks = 0;
    take();
    om_table_set_count(&T, nrows);
    om_table_draw_rows(&T);
}

static void status(void)
{
    int i, chains = 0, servers = 0, handlers = 0, last = -1;
    for (i = 0; i < nrows; i++) {
        if (rows[i].server) { if (rows[i].vec != last) chains++; if (!rows[i].empty) servers++; last = rows[i].vec; }
        else handlers++;
    }
    om_status(1, "16 vectors: %d server chains with %d servers, %d handlers", chains, servers, handlers);
    om_status(2, "Every 5 seconds");
}

static int event(ULONG cls, UWORD code, UWORD qual, APTR ia, int mx, int my, ULONG secs, ULONG micros)
{
    int picked = 0, row = -1, sorted = 0;
    (void)qual;
    if (om_table_event(&T, cls, code, ia, mx, my, secs, micros, &picked, &row, &sorted)) {
        if (sorted) { take(); om_table_draw_rows(&T); }
        return 1;
    }
    if ((cls == IDCMP_GADGETUP && ia && ((struct Gadget *)ia)->GadgetID == GID_INTS + G_COPY) ||
        (cls == IDCMP_VANILLAKEY && (code == 'c' || code == 'C'))) {
        om_status(0, om_table_copy(&T) ? "The interrupts are on the clipboard." : "The clipboard could not be written.");
        return 1;
    }
    if (cls == IDCMP_RAWKEY && (code == 0x4C || code == 0x4D)) { ogt_list_step(T.list, code == 0x4C ? -1 : 1); return 1; }
    return 0;
}

static void shown(int on)
{
    om_tasks_exact(OM_EXACT_INTS, on);  /* exec's counts a second come with its task clock */
    if (!on) om_table_free(&T);
    else { ticks = 0; take(); }
}

const struct om_view om_ints_view = { "Interrupts", layout, gadgets, draw, tick, event, shown, status };

void om_ints_free(void)
{
    om_table_free(&T);
    if (rows) FreeVec(rows);
    if (order) FreeVec(order);
    rows = NULL; order = NULL;
    nrows = 0;
}

void om_ints_init(void)
{
    rows = AllocVec(MAXI * sizeof *rows, MEMF_ANY | MEMF_CLEAR);
    order = AllocVec(MAXI * sizeof *order, MEMF_ANY | MEMF_CLEAR);
    T.sort = -1;                        /* exec's order: by vector, so by level */
    om_table_init(&T, cols, C_N, cell, GID_INTS + 50, "Interrupts");
}
