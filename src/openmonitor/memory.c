/* OpenMonitor: the Memory view (the design, section 4.3).
 *
 * The last minute of chip and fast memory in use, three boxes (chip, fast, the graphics card's video
 * memory), and exec's memory regions: name, CHIP or FAST, attributes, priority, the range, the size, what is
 * free, the largest free piece and how many free pieces there are (exec's MemChunk list, read under Forbid).
 *
 * Free unused memory: an AllocMem of a size no Amiga has, so exec runs the low-memory handlers and the
 * libraries, devices and fonts nobody has open are flushed; the bytes it gained are shown. Check the memory
 * list: every region's free pieces walked and checked (inside the region, in order, apart, a multiple of 8,
 * adding up to what the region says is free).
 *
 * The video memory: openrtg.library doesn't yet say how much of the card is in use, so its total comes from
 * the card's AutoConfig board (ACRTG, AmigaChrome Team $DA15 product 9, 15 or 16, less its ROM and register
 * pages), as OpenMeters reads it.
 *
 * MIT, Copyright (c) 2026 AmigaChrome Team. */

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <libraries/configvars.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <string.h>
#include <stdio.h>

#include "om.h"

#define HIST 60
#define MAXREG 16

struct mrow {
    ULONG addr;
    char name[32];
    UWORD attr;
    BYTE pri;
    ULONG lower, upper, free, largest, pieces;
    UBYTE video;
};
static struct mrow regs[MAXREG];
static int nregs, order[MAXREG];
static UWORD hist_chip[HIST], hist_fast[HIST];
static ULONG chip_total, chip_free, chip_largest, fast_total, fast_free, fast_largest, vram_total;
static int vram_known;

enum { C_NAME, C_KIND, C_ATTR, C_PRI, C_FROM, C_TO, C_SIZE, C_FREE, C_LARGEST, C_PIECES, C_N };
static struct om_col cols[C_N] = {
    { "Region", 20, OM_LEFT, 0, 1, 0 },
    { "Kind", 5, OM_LEFT, 0, 0, 0 },
    { "Attributes", 10, OM_LEFT, 0, 0, 0 },
    { "Pri", 4, OM_RIGHT, 0, 0, 1 },
    { "From", 9, OM_LEFT, 0, 0, 0 },
    { "To", 9, OM_LEFT, 0, 0, 0 },
    { "Size", 10, OM_RIGHT, 0, 0, 1 },
    { "Free", 13, OM_RIGHT, 0, 0, 1 },
    { "Largest", 13, OM_RIGHT, 0, 0, 1 },
    { "Pieces", 6, OM_RIGHT, 0, 0, 1 },
};
static struct om_table T;

enum { G_FREE, G_CHECK, G_COPY, G_COUNT };
static struct Gadget *gad[G_COUNT];
static int gx[G_COUNT], gy, gw[G_COUNT], gh;
static int graph_x, graph_y, graph_w, graph_h, box_y, box_h, note_y, note_h, head_y = -1;

static int pushed;

/* the minute so far: the first reading fills it, so the lines start level rather than at 0 */
static void push(UWORD *h, ULONG v)
{
    int i;
    if (v > 1000) v = 1000;
    if (!pushed) for (i = 0; i < HIST; i++) h[i] = (UWORD)v;
    memmove(&h[0], &h[1], (HIST - 1) * sizeof h[0]);
    h[HIST - 1] = (UWORD)v;
}

#define commas om_commas

static void size_text(ULONG bytes, char *buf, int size)
{
    if (bytes >= 1048576UL) snprintf(buf, size, "%lu.%lu MB", (unsigned long)(bytes >> 20), (unsigned long)((bytes & 0xfffff) * 10 >> 20));
    else snprintf(buf, size, "%lu KB", (unsigned long)(bytes >> 10));
}

/* the graphics card's video memory: ACRTG's boards (16, 64 or 256 MiB), less the ROM and register pages */
static void take_vram(void)
{
    struct Library *eb = OpenLibrary((STRPTR)"expansion.library", 36);
    struct ConfigDev *cd = NULL;
    vram_total = 0;
    vram_known = 1;
    if (!eb) return;
    for (;;) {
        register struct ConfigDev *d0 __asm("d0");
        register struct Library *a6 __asm("a6") = eb;
        register APTR a0 __asm("a0") = cd;
        register LONG d0in __asm("d0") = 0xDA15;
        register LONG d1 __asm("d1") = -1;
        __asm volatile ("jsr -72(%%a6)" : "=r" (d0) : "r" (a6), "r" (a0), "0" (d0in), "r" (d1) : "a1", "cc", "memory");
        if (!(cd = d0)) break;
        if ((cd->cd_Rom.er_Product == 9 || cd->cd_Rom.er_Product == 15 || cd->cd_Rom.er_Product == 16) && cd->cd_BoardSize > 0x20000UL)
            vram_total += cd->cd_BoardSize - 0x20000UL;
    }
    CloseLibrary(eb);
}

/* every second, shown or not: the graph's two lines */
void om_memory_take(void)
{
    ULONG ct = AvailMem(MEMF_CHIP | MEMF_TOTAL), cf = AvailMem(MEMF_CHIP);
    ULONG ft = AvailMem(MEMF_FAST | MEMF_TOTAL), ff = AvailMem(MEMF_FAST);
    chip_total = ct; chip_free = cf; fast_total = ft; fast_free = ff;
    push(hist_chip, ct ? (ct - cf) / (ct / 1000 ? ct / 1000 : 1) : 0);
    push(hist_fast, ft ? (ft - ff) / (ft / 1000 ? ft / 1000 : 1) : 0);
    pushed = 1;
}

static void make_order(void);

static void take_regions(void)
{
    struct Node *n;
    nregs = 0;
    chip_largest = AvailMem(MEMF_CHIP | MEMF_LARGEST);
    fast_largest = AvailMem(MEMF_FAST | MEMF_LARGEST);
    Forbid();
    for (n = SysBase->MemList.lh_Head; n->ln_Succ && nregs < MAXREG - 1; n = n->ln_Succ) {
        struct MemHeader *mh = (struct MemHeader *)n;
        struct MemChunk *c;
        struct mrow *r = &regs[nregs++];
        ULONG k = 0, big = 0;
        memset(r, 0, sizeof *r);
        r->addr = (ULONG)mh;
        strncpy(r->name, n->ln_Name ? n->ln_Name : "(no name)", sizeof r->name - 1);
        r->attr = mh->mh_Attributes;
        r->pri = n->ln_Pri;
        r->lower = (ULONG)mh->mh_Lower;
        r->upper = (ULONG)mh->mh_Upper;
        r->free = mh->mh_Free;
        for (c = mh->mh_First; c && k < 1000000; c = c->mc_Next) { k++; if (c->mc_Bytes > big) big = c->mc_Bytes; }
        r->pieces = k;
        r->largest = big;
    }
    Permit();
    if (!vram_known) take_vram();
    if (vram_total && nregs < MAXREG) {
        struct mrow *r = &regs[nregs++];
        memset(r, 0, sizeof *r);
        strcpy(r->name, "the graphics card");
        r->video = 1;
        r->upper = vram_total;
    }
    make_order();
}

static void raw_cell(int row, int col, char *buf, int size, LONG *pen)
{
    struct mrow *r;
    char a[24];
    if (row < 0 || row >= nregs) return;
    r = &regs[row];
    if (r->video) {
        switch (col) {
        case C_NAME: snprintf(buf, size, "%s", r->name); break;
        case C_KIND: snprintf(buf, size, "VIDEO"); break;
        case C_ATTR: snprintf(buf, size, "card"); *pen = U.pen_muted; break;
        case C_SIZE: size_text(r->upper, buf, size); break;
        case C_FREE: snprintf(buf, size, "not told"); *pen = U.pen_muted; break;
        }
        return;
    }
    switch (col) {
    case C_NAME: snprintf(buf, size, "%s", r->name); break;
    case C_KIND: snprintf(buf, size, "%s", r->attr & MEMF_CHIP ? "CHIP" : "FAST"); break;
    case C_ATTR: snprintf(buf, size, "$%04x", r->attr); break;
    case C_PRI: snprintf(buf, size, "%d", r->pri); break;
    case C_FROM: snprintf(buf, size, "$%08lx", (unsigned long)r->lower); *pen = U.pen_muted; break;
    case C_TO: snprintf(buf, size, "$%08lx", (unsigned long)(r->upper - 1)); *pen = U.pen_muted; break;
    case C_SIZE: size_text(r->upper - r->lower, buf, size); break;
    case C_FREE: snprintf(buf, size, "%s", commas(r->free, a)); break;
    case C_LARGEST: snprintf(buf, size, "%s", commas(r->largest, a)); break;
    case C_PIECES: snprintf(buf, size, "%lu", (unsigned long)r->pieces); if (r->pieces > 500) *pen = U.pen_amber; break;
    }
}

static void cell(int row, int col, char *buf, int size, LONG *pen)
{
    if (row >= 0 && row < nregs) raw_cell(order[row], col, buf, size, pen);
}

static void make_order(void)
{
    int i;
    for (i = 0; i < nregs; i++) order[i] = i;
    om_table_sort(&T, raw_cell, order, nregs);
}

/* ---- drawing ------------------------------------------------------------------------------------------ */

static void draw_graph(void)
{
    struct RastPort *rp = U.win->RPort;
    int ix = graph_x + 6, iy = graph_y + 4 + U.fh + 6, iw = graph_w - 12, ih = graph_h - (iy - graph_y) - 6, i, k;
    char t[120];
    om_sunk(graph_x, graph_y, graph_w, graph_h);
    SetFont(rp, U.font);
    snprintf(t, sizeof t, "In use, the last minute:   chip %lu.%lu%%   fast %lu.%lu%%",
             (unsigned long)hist_chip[HIST - 1] / 10, (unsigned long)hist_chip[HIST - 1] % 10,
             (unsigned long)hist_fast[HIST - 1] / 10, (unsigned long)hist_fast[HIST - 1] % 10);
    ogt_text(rp, U.pen_text, graph_x + 8, graph_y + 4, t, graph_w - 16);
    if (ih < 8 || iw < HIST) return;
    ogt_set_apen(rp, U.pen_shadow);
    for (i = 1; i < 4; i++) {
        int ly = iy + ih * i / 4, lx;
        for (lx = ix; lx < ix + iw; lx += 4) WritePixel(rp, lx, ly);
    }
    for (k = 0; k < 2; k++) {
        UWORD *h = k ? hist_fast : hist_chip;
        ogt_set_apen(rp, U.graph_pen[k ? 0 : 1]);
        for (i = 0; i < HIST; i++) {
            int px = ix + i * (iw - 1) / (HIST - 1), py = iy + ih - 1 - (int)((ULONG)h[i] * (ULONG)(ih - 1) / 1000);
            if (!i) Move(rp, px, py); else Draw(rp, px, py);
        }
        for (i = 0; i < HIST; i++) {
            int px = ix + i * (iw - 1) / (HIST - 1), py = iy + ih - (int)((ULONG)h[i] * (ULONG)(ih - 1) / 1000);
            if (py > iy + ih - 1) py = iy + ih - 1;
            if (!i) Move(rp, px, py); else Draw(rp, px, py);
        }
    }
}

static void draw_box(int x, int w, const char *name, ULONG total, ULONG freeb, ULONG largest, int known, LONG pen)
{
    struct RastPort *rp = U.win->RPort;
    char a[20], s[64];
    int y = box_y, bw = w - 20, lh = U.tfh + 2, ty;
    om_sunk(x, y, w, box_h);
    SetFont(rp, U.font);
    ogt_bold(rp, 1);
    ogt_text(rp, U.pen_text, x + 10, y + 5, name, w - 20);
    ogt_bold(rp, 0);
    ty = y + 5 + U.fh + 4;
    ogt_box(rp, U.pen_list, x + 10, ty, bw, 10);
    ogt_bevel(rp, U.pen_shadow, U.pen_shine, x + 10, ty, bw, 10);
    if (known && total) {
        ULONG used = total - freeb;
        int fw = (int)((ULONG)(bw - 2) * (used / 1024) / (total / 1024 ? total / 1024 : 1));
        ogt_box(rp, pen, x + 11, ty + 1, fw < 1 ? 1 : fw, 8);
    }
    ty += 14;
    SetFont(rp, U.tfont);
    if (known) {
        snprintf(s, sizeof s, "in use  %15s bytes", commas(total - freeb, a));
        ogt_text(rp, U.pen_text, x + 10, ty, s, w - 20);
        snprintf(s, sizeof s, "free    %15s bytes", commas(freeb, a));
        ogt_text(rp, U.pen_text, x + 10, ty + lh, s, w - 20);
        snprintf(s, sizeof s, "largest %15s bytes", commas(largest, a));
        ogt_text(rp, U.pen_text, x + 10, ty + 2 * lh, s, w - 20);
    } else {
        snprintf(s, sizeof s, "total   %15s bytes", commas(total, a));
        ogt_text(rp, U.pen_text, x + 10, ty, s, w - 20);
        ogt_text(rp, U.pen_muted, x + 10, ty + lh, total ? "in use: not told yet" : "no graphics card", w - 20);
        ogt_text(rp, U.pen_muted, x + 10, ty + 2 * lh, total ? "(openrtg.library)" : "", w - 20);
    }
}

static void draw_boxes(void)
{
    int w = (U.pw - 16) / 3;
    if (box_h <= 0) return;
    draw_box(U.px, w, "Chip RAM", chip_total, chip_free, chip_largest, 1, U.graph_pen[1]);
    draw_box(U.px + w + 8, w, "Fast RAM", fast_total, fast_free, fast_largest, 1, U.graph_pen[0]);
    draw_box(U.px + 2 * (w + 8), U.pw - 2 * (w + 8), "Video RAM", vram_total, 0, 0, 0, U.graph_pen[3]);
}

static void draw_note(void)
{
    struct RastPort *rp = U.win->RPort;
    if (note_h <= 0) return;
    om_sunk(U.px, note_y, U.pw, note_h);
    SetFont(rp, U.font);
    ogt_text(rp, U.pen_muted, U.px + 8, note_y + 4, "Which task uses how much isn't kept by AmigaOS: a program's memory has no owner.", U.pw - 16);
    ogt_text(rp, U.pen_muted, U.px + 8, note_y + 6 + U.fh, "Free unused memory flushes what nobody has open; the libraries load again when asked.", U.pw - 16);
}

/* ---- the actions -------------------------------------------------------------------------------------- */

static void free_unused(void)
{
    ULONG before = AvailMem(MEMF_ANY), after;
    APTR p = AllocMem(0x7ffffff0UL, MEMF_PUBLIC);   /* no Amiga has it: exec flushes, then fails */
    if (p) FreeMem(p, 0x7ffffff0UL);
    after = AvailMem(MEMF_ANY);
    {
        char a[20];
        if (after > before) om_status(0, "Freed %s bytes.", commas(after - before, a));
        else om_status(0, "Nothing more could be freed: what is loaded is in use.");
    }
}

static void check_list(void)
{
    struct Node *n;
    int regions = 0, bad = 0;
    ULONG pieces = 0;
    char where[160];
    where[0] = 0;
    Forbid();
    for (n = SysBase->MemList.lh_Head; n->ln_Succ; n = n->ln_Succ) {
        struct MemHeader *mh = (struct MemHeader *)n;
        struct MemChunk *c, *last = NULL;
        ULONG sum = 0, k = 0;
        regions++;
        for (c = mh->mh_First; c && k < 1000000; c = c->mc_Next, k++) {
            int wrong = (ULONG)c < (ULONG)mh->mh_Lower || (ULONG)c + c->mc_Bytes > (ULONG)mh->mh_Upper ||
                        (c->mc_Bytes & 7) || ((ULONG)c & 7) || !c->mc_Bytes ||
                        (last && (ULONG)last + last->mc_Bytes >= (ULONG)c);
            if (wrong) {
                if (!bad) snprintf(where, sizeof where, "%s: the free piece at $%08lx (%lu bytes)",
                                   n->ln_Name ? n->ln_Name : "a region", (unsigned long)c, (unsigned long)c->mc_Bytes);
                bad++;
                break;
            }
            sum += c->mc_Bytes;
            last = c;
        }
        pieces += k;
        if (!bad && sum != mh->mh_Free) {
            snprintf(where, sizeof where, "%s: its free pieces add up to %lu bytes, and it says %lu are free",
                     n->ln_Name ? n->ln_Name : "a region", (unsigned long)sum, (unsigned long)mh->mh_Free);
            bad++;
        }
    }
    Permit();
    if (!bad)
        om_ask("OpenMonitor", "OK", "The memory list is sound.\n\n%d regions, %lu free pieces: each inside its region,\n"
               "in order, apart from the next, and adding up to what is free.", regions, (unsigned long)pieces);
    else
        om_ask("OpenMonitor", "OK", "The memory list is damaged.\n\n%s.\n\n"
               "A program wrote where it shouldn't, or freed memory twice.\n"
               "Save your work and restart.", where);
}

/* ---- the view ----------------------------------------------------------------------------------------- */

static void layout(int x, int y, int w, int h)
{
    int bh = U.fh + 8, th;
    if (U.compact) {
        graph_x = x; graph_y = y; graph_w = w; graph_h = 0;
        box_y = y; box_h = 0;
        note_h = 0;
        head_y = -1;
        th = h - bh - 6;
        om_table_layout(&T, x, y, w, th);
    } else {
        /* the boxes, the regions (room for 5) and the note keep their size; the graph takes what is left */
        int rest;
        box_h = 5 + U.fh + 4 + 14 + 3 * (U.tfh + 2) + 6;
        th = (U.fh + 8) + 2 + (U.tfh + 3) * 5 + 4;
        note_h = 2 * U.fh + 12;
        rest = 10 + box_h + 10 + U.fh + 6 + th + 10 + note_h + 8 + bh;
        if (h - rest < 60) { note_h = 0; rest -= 2 * U.fh + 12 + 10; }
        graph_h = h - rest;
        if (graph_h > 320) graph_h = 320;
        if (graph_h < 40) graph_h = 0;
        graph_x = x; graph_y = y; graph_w = w;
        box_y = graph_h ? y + graph_h + 10 : y;
        head_y = box_y + box_h + 10;
        om_table_layout(&T, x, head_y + U.fh + 6, w, th);
        note_y = T.y + T.h + 10;
    }
    gy = y + h - bh;
    gh = bh;
    gw[G_FREE] = om_text_width(U.font, "Free unused memory") + 30;
    gw[G_CHECK] = om_text_width(U.font, "Check the memory list") + 30;
    gw[G_COPY] = om_text_width(U.font, "Copy as text") + 30;
    gx[G_FREE] = x;
    gx[G_CHECK] = x + gw[G_FREE] + 8;
    gx[G_COPY] = x + w - gw[G_COPY];
}

static struct Gadget *gadgets(struct Gadget *g)
{
    static const char *const labels[G_COUNT] = { "Free unused memory", "Check the memory list", "Copy as text" };
    struct NewGadget ng;
    int i;
    memset(&ng, 0, sizeof ng);
    ng.ng_TextAttr = &U.ta; ng.ng_VisualInfo = U.vi; ng.ng_Flags = PLACETEXT_IN;
    for (i = 0; i < G_COUNT && g; i++) {
        ng.ng_LeftEdge = gx[i]; ng.ng_TopEdge = gy; ng.ng_Width = gw[i]; ng.ng_Height = gh;
        ng.ng_GadgetText = (STRPTR)labels[i]; ng.ng_GadgetID = GID_MEMORY + i;
        g = gad[i] = CreateGadget(BUTTON_KIND, g, &ng, TAG_DONE);
    }
    return g;
}

static void draw(void)
{
    take_regions();
    if (graph_h > 0) draw_graph();
    draw_boxes();
    if (head_y >= 0) om_heading(U.px, head_y, "Memory regions (exec's memory list, and the graphics card's)");
    om_table_set_count(&T, nregs);
    om_table_draw(&T);
    draw_note();
}

static void tick(void)
{
    take_regions();
    if (graph_h > 0) draw_graph();
    draw_boxes();
    om_table_set_count(&T, nregs);
    om_table_draw_rows(&T);
}

static void status(void)
{
    ULONG p = 0;
    int i, n = 0;
    char a[20], b[20];
    for (i = 0; i < nregs; i++) if (!regs[i].video) { p += regs[i].pieces; n++; }
    om_status(1, "Chip %s KB, fast %s KB free; %d regions, %lu pieces", commas(chip_free >> 10, a),
              commas(fast_free >> 10, b), n, (unsigned long)p);
    om_status(2, "Every second");
}

static int event(ULONG cls, UWORD code, UWORD qual, APTR ia, int mx, int my, ULONG secs, ULONG micros)
{
    int picked, row, sorted;
    (void)qual;
    if (om_table_event(&T, cls, code, ia, mx, my, secs, micros, &picked, &row, &sorted)) {
        if (sorted) { make_order(); om_table_draw_rows(&T); }
        return 1;
    }
    if (cls == IDCMP_GADGETUP && ia) {
        switch (((struct Gadget *)ia)->GadgetID - GID_MEMORY) {
        case G_FREE: free_unused(); tick(); return 1;
        case G_CHECK: check_list(); return 1;
        case G_COPY: om_status(0, om_table_copy(&T) ? "The regions are on the clipboard." : "The clipboard could not be written."); return 1;
        }
    }
    if (cls == IDCMP_VANILLAKEY) {
        switch (code) {
        case 'c': case 'C': om_status(0, om_table_copy(&T) ? "The regions are on the clipboard." : "The clipboard could not be written."); return 1;
        case 'f': case 'F': free_unused(); tick(); return 1;
        case 'v': case 'V': check_list(); return 1;
        }
    }
    return 0;
}

static void shown(int on)
{
    if (!on) om_table_free(&T);
    else { vram_known = 0; take_regions(); }
}

const struct om_view om_memory_view = { "Memory", layout, gadgets, draw, tick, event, shown, status };

void om_memory_init(void)
{
    memset(hist_chip, 0, sizeof hist_chip);
    memset(hist_fast, 0, sizeof hist_fast);
    pushed = 0;
    vram_known = 0;
    T.sort = -1;                        /* exec's order (its priority) until a column is clicked */
    om_table_init(&T, cols, C_N, cell, GID_MEMORY + 50, "Memory");
}
