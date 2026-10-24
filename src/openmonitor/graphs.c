/* OpenMonitor: today's three graphs, the last minute of each: CPU (and the cores board's cores), memory
 * (chip and fast together), and the GPU (how much of the drawing OpenGPU did, with the pixels a second, from
 * openrtg.library's ORTG_DrawStats, 0.11 on). On a small screen they are one line of meters instead.
 *
 * MIT, Copyright (c) 2026 AmigaChrome Team. */

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <libraries/configvars.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <string.h>
#include <stdio.h>

#include "om.h"

#define HIST 60
enum { GR_CPU, GR_MEM, GR_GPU, GR_N };
static UWORD hist[GR_N][HIST];                  /* tenths of a per cent, the newest last */
static ULONG cpu_tenths, switches, ncores;
static ULONG mem_used_tenths, chip_free_k, fast_free_k;
static struct Library *OpenRTGBase;
static ULONG gpu_counts[20 * 4];
static ULONG gpu_share_tenths, gpu_kpx;
static int gpu_known;

static void push(int g, ULONG v)
{
    memmove(&hist[g][0], &hist[g][1], (HIST - 1) * sizeof hist[g][0]);
    hist[g][HIST - 1] = (UWORD)(v > 1000 ? 1000 : v);
}

/* openrtg.library's ORTG_DrawStats (-108): counts[kind * 4 + 0..3] = pieces OpenGPU drew, pieces the CPU
 * drew, the pixels of each. Called by its offset: OpenMonitor needs none of OpenRTG's headers. */
static ULONG draw_stats(ULONG *counts, ULONG kinds, ULONG flags)
{
    register ULONG d0 __asm("d0") = kinds;
    register ULONG d1 __asm("d1") = flags;
    register ULONG *a0 __asm("a0") = counts;
    register struct Library *a6 __asm("a6") = OpenRTGBase;
    __asm volatile ("jsr -108(%%a6)" : "+r" (d0), "+r" (d1), "+r" (a0) : "r" (a6) : "a1", "cc", "memory");
    return d0;
}

/* The cores the CPU cores board presents through AutoConfig: an AmigaChrome Team ($DA15) product 7 board
 * carrying the ACSV block, CLASS ($20) 2, UNITS ($24) its cores. 0 without the board. */
static ULONG board_cores(void)
{
    struct Library *eb = OpenLibrary((STRPTR)"expansion.library", 36);
    ULONG n = 0;
    if (eb) {
        register struct ConfigDev *d0 __asm("d0");
        register struct Library *a6 __asm("a6") = eb;
        register APTR a0 __asm("a0") = NULL;
        register ULONG d0in __asm("d0") = 0xDA15;
        register ULONG d1 __asm("d1") = 7;
        __asm volatile ("jsr -72(%%a6)" : "=r" (d0) : "r" (a6), "r" (a0), "0" (d0in), "r" (d1) : "a1", "cc", "memory");
        if (d0 && d0->cd_BoardAddr) {
            volatile ULONG *r = (volatile ULONG *)d0->cd_BoardAddr;
            if (r[0] == 0x41435356UL && r[0x20 / 4] == 2) n = r[0x24 / 4];
        }
        CloseLibrary(eb);
    }
    return n > 1024 ? 0 : n;
}

static ULONG omc_cores(void)
{
    struct Library *omc = OpenLibrary((STRPTR)"openmulticore.library", 0);
    ULONG n = 0;
    if (omc) {
        register ULONG d0 __asm("d0");
        register struct Library *a6 __asm("a6") = omc;
        __asm volatile ("jsr -30(%%a6)" : "=r" (d0) : "r" (a6) : "d1", "a0", "a1", "cc", "memory");
        n = d0;
        CloseLibrary(omc);
    }
    return n > 1024 ? 0 : n;
}

void om_graphs_init(void)
{
    memset(hist, 0, sizeof hist);
    OpenRTGBase = OpenLibrary((STRPTR)"openrtg.library", 0);
    if (OpenRTGBase && OpenRTGBase->lib_Version == 0 && OpenRTGBase->lib_Revision < 11) { CloseLibrary(OpenRTGBase); OpenRTGBase = NULL; }
    if (OpenRTGBase) draw_stats(NULL, 0, 1);    /* counting from now */
    ncores = board_cores();
    if (!ncores) ncores = omc_cores();
    gpu_known = 0;
}

void om_graphs_free(void)
{
    if (OpenRTGBase) CloseLibrary(OpenRTGBase);
    OpenRTGBase = NULL;
}

void om_graphs_push_cpu(ULONG tenths, ULONG sw, ULONG cores)
{
    cpu_tenths = tenths;
    switches = sw;
    (void)cores;
    push(GR_CPU, tenths);
}

void om_graphs_take(void)
{
    ULONG cf = AvailMem(MEMF_CHIP), ff = AvailMem(MEMF_FAST);
    ULONG ca = AvailMem(MEMF_CHIP | MEMF_TOTAL) >> 10, fa = AvailMem(MEMF_FAST | MEMF_TOTAL) >> 10;
    chip_free_k = cf >> 10; fast_free_k = ff >> 10;
    mem_used_tenths = ca + fa ? ((ca + fa) - (chip_free_k + fast_free_k)) * 1000 / (ca + fa) : 0;
    push(GR_MEM, mem_used_tenths);
    if (OpenRTGBase) {
        ULONG k, g = 0, c = 0;
        draw_stats(gpu_counts, 20, 1);          /* the counts since the last time, then back to 0 */
        for (k = 0; k < 20; k++) { g += gpu_counts[k * 4 + 2]; c += gpu_counts[k * 4 + 3]; }
        gpu_kpx = (g + c) / 1000;
        gpu_share_tenths = g + c ? (ULONG)(((g / 16) * 1000) / (((g + c) / 16) ? ((g + c) / 16) : 1)) : 0;
        gpu_known = 1;
    }
    push(GR_GPU, gpu_share_tenths);
}

static void draw_graph(int g, int x, int y, int w, int h, const char *title)
{
    struct RastPort *rp = U.win->RPort;
    int i, ix = x + 6, iy = y + 4 + U.fh + 4, iw = w - 12, ih = h - (iy - y) - 6;
    om_sunk(x, y, w, h);
    SetFont(rp, U.font);
    ogt_text(rp, U.pen_text, x + 8, y + 4, title, w - 16);
    if (ih < 8 || iw < HIST) return;
    ogt_set_apen(rp, U.pen_shadow);
    for (i = 1; i < 4; i++) {
        int ly = iy + ih * i / 4, lx;
        for (lx = ix; lx < ix + iw; lx += 4) WritePixel(rp, lx, ly);
    }
    ogt_set_apen(rp, U.graph_pen[g]);
    for (i = 0; i < HIST; i++) {
        int px = ix + i * (iw - 1) / (HIST - 1), py = iy + ih - 1 - (int)((ULONG)hist[g][i] * (ULONG)(ih - 1) / 1000);
        if (!i) Move(rp, px, py); else Draw(rp, px, py);
    }
    for (i = 0; i < HIST; i++) {                /* twice, a pixel lower: a line seen on any theme */
        int px = ix + i * (iw - 1) / (HIST - 1), py = iy + ih - (int)((ULONG)hist[g][i] * (ULONG)(ih - 1) / 1000);
        if (py > iy + ih - 1) py = iy + ih - 1;
        if (!i) Move(rp, px, py); else Draw(rp, px, py);
    }
}

static void cpu_title(char *t, int size)
{
    if (ncores)
        snprintf(t, size, "CPU %lu%%   CPU0 + %lu core%s   %lu switches/s", (unsigned long)(cpu_tenths + 5) / 10,
                 (unsigned long)ncores, ncores == 1 ? "" : "s", (unsigned long)switches);
    else
        snprintf(t, size, "CPU %lu%%   %lu switches/s", (unsigned long)(cpu_tenths + 5) / 10, (unsigned long)switches);
}

void om_graphs_draw(int x, int y, int w, int h)
{
    char t[96];
    int gw = (w - 16) / 3;
    cpu_title(t, sizeof t);
    draw_graph(GR_CPU, x, y, gw, h, t);
    snprintf(t, sizeof t, "Memory %lu%%   chip %lu KB, fast %lu MB free", (unsigned long)(mem_used_tenths + 5) / 10,
             (unsigned long)chip_free_k, (unsigned long)(fast_free_k >> 10));
    draw_graph(GR_MEM, x + gw + 8, y, gw, h, t);
    if (!OpenRTGBase) snprintf(t, sizeof t, "GPU   (no openrtg.library)");
    else if (!gpu_known) snprintf(t, sizeof t, "GPU");
    else snprintf(t, sizeof t, "GPU %lu%% of the drawing   %lu.%lu Mpx/s", (unsigned long)(gpu_share_tenths + 5) / 10,
                  (unsigned long)(gpu_kpx / 1000), (unsigned long)(gpu_kpx % 1000 / 100));
    draw_graph(GR_GPU, x + 2 * (gw + 8), y, w - 2 * (gw + 8), h, t);
}

/* One line: "CPU [===   ] 8%   Memory [==      ] 21%   fast 495 MB free" */
static int meter(struct RastPort *rp, int x, int y, int h, const char *label, ULONG tenths, LONG pen)
{
    int lw = om_text_width(U.font, label), bw = 60;
    ogt_text(rp, U.pen_text, x, y + (h - U.fh) / 2, label, 0);
    x += lw + 4;
    ogt_box(rp, U.pen_list, x + 1, y + 2, bw - 2, h - 4);
    ogt_bevel(rp, U.pen_shadow, U.pen_shine, x, y + 1, bw, h - 2);
    ogt_box(rp, pen, x + 1, y + 2, (int)((bw - 2) * (tenths > 1000 ? 1000 : tenths) / 1000) + 1, h - 4);
    return lw + 4 + bw + 6;
}

void om_graphs_meter(int x, int y, int w, int h)
{
    struct RastPort *rp = U.win->RPort;
    char t[64];
    int at = x;
    SetFont(rp, U.font);
    EraseRect(rp, x, y, x + w - 1, y + h - 1);
    at += meter(rp, at, y, h, "CPU", cpu_tenths, U.graph_pen[0]);
    snprintf(t, sizeof t, "%lu%%", (unsigned long)(cpu_tenths + 5) / 10);
    at += ogt_text(rp, U.pen_text, at, y + (h - U.fh) / 2, t, 0) + 12;
    at += meter(rp, at, y, h, "Mem", mem_used_tenths, U.graph_pen[0]);
    snprintf(t, sizeof t, "%lu%%  chip %luK  fast %luM free", (unsigned long)(mem_used_tenths + 5) / 10,
             (unsigned long)chip_free_k, (unsigned long)(fast_free_k >> 10));
    ogt_text(rp, U.pen_text, at, y + (h - U.fh) / 2, t, x + w - at);
}
