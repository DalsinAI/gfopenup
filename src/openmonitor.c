/* OpenMonitor 0.2: what the machine is doing, as Task Manager on Windows or Resources on Ubuntu show it
 * (the user, 10 October 2026: "a cpu / task / process app similar to task manager in windows or resources
 * in ubuntu ... have it show gpu too").
 *
 * The window (800 x 600, the Open apps' rule):
 *   - three graphs across the top, the last minute of each: CPU (and the board's cores, when there are
 *     some), memory (chip and fast together), and the GPU: how much of the drawing OpenGPU did, with the
 *     pixels a second (openrtg.library's ORTG_DrawStats, 0.11 on);
 *   - every task and process, busiest first: name, CPU, priority, state, stack;
 *   - Priority up and down, Break (Ctrl-C) and End program, for the one chosen.
 *
 * Its text is in the theme's font, as every Open app's (OpenGadTools' ogt_look and ogt_font, as OpenFiles):
 * the screen's own font only when the theme's and the nearest proportional ones are missing.
 *
 * Ending a program (the user, 11 October 2026: "a proper kill process"). AmigaOS keeps no list of what a
 * task owns, so removing one (RemTask) leaves its memory, files, windows and locks behind, and a reply
 * to a request it made lands in freed memory. So End program asks first: a Ctrl-C, then up to 3 seconds
 * for the program to tidy up and go by itself. Only if it is still there is Force quit offered, behind a
 * warning; a system task or a file system gets a second, stronger one; OpenMonitor never removes itself.
 * Keys on the chosen row: Ctrl-C Break, Del End program, Shift-Del Force quit.
 *
 * How busy a task is: AmigaOS keeps no time per task, so OpenMonitor looks 50 times a second (a vertical
 * blank interrupt server) at which task is running, and counts. A task waiting, the machine idle, counts
 * as idle. Over a second that is 50 looks: the numbers are good to a couple of per cent, like any sampling
 * monitor's.
 *
 * MIT, Copyright (c) 2026 Dalsin Limited. */

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/interrupts.h>
#include <exec/tasks.h>
#include <devices/timer.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <hardware/intbits.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <devices/inputevent.h>
#include <libraries/gadtools.h>
#include <utility/hooks.h>
#include <graphics/gfxmacros.h>
#include <proto/exec.h>
#include <libraries/configvars.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "ogt_look.h"
#include "ogt_font.h"

static const char version[] __attribute__((used)) = "$VER: OpenMonitor 0.2 (11.10.2026) MIT, Copyright (c) 2026 Dalsin Limited";

#define REG(r, decl) register decl __asm(#r)

/* ---- the sampler -------------------------------------------------------------------------- */

#define MAXT 192
struct samp {
    struct Task *task[MAXT];
    ULONG hits[MAXT];
    ULONG n, idle, total;
};
static struct samp S;
static struct Interrupt vbi;

/* 50 times a second, in the interrupt: which task is running (0 in d0: the next server runs too) */
static ULONG vb_server(REG(a1, struct samp *s))
{
    struct Task *t = SysBase->ThisTask;
    ULONG i;
    s->total++;
    if (!t || t->tc_State != TS_RUN) { s->idle++; return 0; }
    for (i = 0; i < s->n; i++)
        if (s->task[i] == t) { s->hits[i]++; return 0; }
    if (s->n < MAXT) { s->task[s->n] = t; s->hits[s->n] = 1; s->n++; }
    return 0;
}

/* ---- the tasks ---------------------------------------------------------------------------- */

struct row {
    struct Node node;              /* ln_Name: the shown line (unused: the hook draws the columns) */
    struct Task *task;
    char name[40];
    LONG pri;
    ULONG stack;
    UBYTE state, process;
    ULONG tenths;                  /* CPU, in tenths of a per cent */
};

#define MAXROWS 192
static struct row rows[MAXROWS];
static int nrows;
static struct List rowlist;

static void list_init(struct List *l)
{
    l->lh_Head = (struct Node *)&l->lh_Tail;
    l->lh_Tail = NULL;
    l->lh_TailPred = (struct Node *)&l->lh_Head;
}

/* The task's name: a Shell's process by its command (the file name only), else its own name. Under Disable. */
static void task_name(struct Task *t, char *out, int size)
{
    const char *n = t->tc_Node.ln_Name ? t->tc_Node.ln_Name : "(no name)";
    out[0] = 0;
    if (t->tc_Node.ln_Type == NT_PROCESS) {
        struct Process *p = (struct Process *)t;
        struct CommandLineInterface *cli = p->pr_CLI ? (struct CommandLineInterface *)BADDR(p->pr_CLI) : NULL;
        if (cli && cli->cli_Module && cli->cli_CommandName) {
            UBYTE *b = (UBYTE *)BADDR(cli->cli_CommandName);
            int len = b ? b[0] : 0, s = 0, i;
            for (i = 0; i < len; i++) if (b[1 + i] == '/' || b[1 + i] == ':') s = i + 1;
            if (len - s > 0) {
                int k = len - s < size - 1 ? len - s : size - 1;
                memcpy(out, b + 1 + s, k);
                out[k] = 0;
                return;
            }
        }
    }
    strncpy(out, n, size - 1);
    out[size - 1] = 0;
}

static void add_task(struct Task *t)
{
    struct row *r;
    if (!t || nrows >= MAXROWS) return;
    r = &rows[nrows++];
    memset(r, 0, sizeof *r);
    r->task = t;
    task_name(t, r->name, sizeof r->name);
    r->pri = t->tc_Node.ln_Pri;
    r->stack = (ULONG)t->tc_SPUpper - (ULONG)t->tc_SPLower;
    r->state = t->tc_State;
    r->process = t->tc_Node.ln_Type == NT_PROCESS;
}

static ULONG cpu_tenths;                       /* the whole CPU, busy, in tenths of a per cent */
static ULONG switches;                          /* task switches in the last second (exec's DispCount) */
static ULONG disp_before;

static int by_cpu(const void *a, const void *b)
{
    const struct row *x = a, *y = b;
    if (x->tenths != y->tenths) return x->tenths < y->tenths ? 1 : -1;
    return strcmp(x->name, y->name);
}

/* Every task, with what the sampler counted since the last time. */
static void take_tasks(void)
{
    static struct samp got;
    struct Node *n;
    int i;
    ULONG j, disp;
    nrows = 0;
    Disable();
    got = S;
    S.n = 0; S.idle = 0; S.total = 0;
    add_task(SysBase->ThisTask);
    for (n = SysBase->TaskReady.lh_Head; n->ln_Succ; n = n->ln_Succ) add_task((struct Task *)n);
    for (n = SysBase->TaskWait.lh_Head; n->ln_Succ; n = n->ln_Succ) add_task((struct Task *)n);
    disp = SysBase->DispCount;
    Enable();
    switches = disp - disp_before;
    disp_before = disp;
    cpu_tenths = got.total ? (got.total - got.idle) * 1000 / got.total : 0;
    for (i = 0; i < nrows; i++)
        for (j = 0; j < got.n; j++)
            if (got.task[j] == rows[i].task) { rows[i].tenths = got.total ? got.hits[j] * 1000 / got.total : 0; break; }
    qsort(rows, nrows, sizeof rows[0], by_cpu);
    list_init(&rowlist);
    for (i = 0; i < nrows; i++) { rows[i].node.ln_Name = rows[i].name; AddTail(&rowlist, &rows[i].node); }
}

/* The chosen task, if it is still there: 1 when found (under Forbid, so it stays while used). */
static int task_alive(struct Task *t)
{
    struct Node *n;
    if (t == SysBase->ThisTask) return 1;
    for (n = SysBase->TaskReady.lh_Head; n->ln_Succ; n = n->ln_Succ) if ((struct Task *)n == t) return 1;
    for (n = SysBase->TaskWait.lh_Head; n->ln_Succ; n = n->ln_Succ) if ((struct Task *)n == t) return 1;
    return 0;
}

/* ---- memory and the GPU ------------------------------------------------------------------- */

static struct Library *OpenRTGBase;
static ULONG gpu_counts[20 * 4];

/* openrtg.library's ORTG_DrawStats (-108): counts[kind * 4 + 0..3] = pieces OpenGPU drew, pieces the CPU
 * drew, the pixels of each. Called here by its offset, so OpenMonitor needs none of OpenRTG's headers. */
static ULONG draw_stats(ULONG *counts, ULONG kinds, ULONG flags)
{
    register ULONG d0 __asm("d0") = kinds;
    register ULONG d1 __asm("d1") = flags;
    register ULONG *a0 __asm("a0") = counts;
    register struct Library *a6 __asm("a6") = OpenRTGBase;
    __asm volatile ("jsr -108(%%a6)" : "+r" (d0), "+r" (d1), "+r" (a0) : "r" (a6) : "a1", "cc", "memory");
    return d0;
}

static ULONG gpu_share_tenths;                  /* of the pixels drawn in the last second, OpenGPU's share */
static ULONG gpu_kpx;                           /* thousands of pixels drawn in the last second, all of them */
static int gpu_known;

static void take_gpu(void)
{
    ULONG k, g = 0, c = 0;
    if (!OpenRTGBase) return;
    draw_stats(gpu_counts, 20, 1);              /* the counts since the last time, then back to 0 */
    for (k = 0; k < 20; k++) { g += gpu_counts[k * 4 + 2]; c += gpu_counts[k * 4 + 3]; }
    gpu_kpx = (g + c) / 1000;
    gpu_share_tenths = g + c ? (ULONG)(((g / 16) * 1000) / (((g + c) / 16) ? ((g + c) / 16) : 1)) : 0;
    gpu_known = 1;
}

static ULONG mem_used_tenths, chip_free_k, fast_free_k, chip_all_k, fast_all_k;

static void take_memory(void)
{
    ULONG cf = AvailMem(MEMF_CHIP), ff = AvailMem(MEMF_FAST);
    ULONG ca = AvailMem(MEMF_CHIP | MEMF_TOTAL), fa = AvailMem(MEMF_FAST | MEMF_TOTAL);
    chip_free_k = cf >> 10; fast_free_k = ff >> 10; chip_all_k = ca >> 10; fast_all_k = fa >> 10;
    mem_used_tenths = (chip_all_k + fast_all_k) ? ((chip_all_k + fast_all_k) - (chip_free_k + fast_free_k)) * 1000 /
                                                  (chip_all_k + fast_all_k) : 0;
}

/* The cores the CPU cores board presents through AutoConfig (the user, 10 October 2026: "the cpu count
 * should read the amount of cores presented via autoconfig"): a Dalsin ($DA15) product 7 board carrying
 * the ACSV block, CLASS ($20) 2, UNITS ($24) its cores. 0 without the board. */
static ULONG board_cores(void)
{
    struct Library *eb = OpenLibrary((STRPTR)"expansion.library", 36);
    ULONG n = 0;
    if (eb) {
        /* FindConfigDev(NULL, $DA15, 7): -72(a6), a0 oldConfigDev, d0 manufacturer, d1 product (no proto header:
         * proto/expansion.h changed the hook types this file uses) */
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

/* ---- the graphs --------------------------------------------------------------------------- */

#define HIST 60
enum { GR_CPU, GR_MEM, GR_GPU, GR_N };
static UWORD hist[GR_N][HIST];                  /* tenths of a per cent, the newest last */
static LONG graph_pen[GR_N] = { -1, -1, -1 };
static const ULONG graph_rgb[GR_N] = { 0x3d7fd6, 0x4caf50, 0xf08c28 };

static void push(int g, ULONG v)
{
    memmove(&hist[g][0], &hist[g][1], (HIST - 1) * sizeof hist[g][0]);
    hist[g][HIST - 1] = (UWORD)(v > 1000 ? 1000 : v);
}

static struct Window *win;
static APTR vi;
static struct DrawInfo *dri;
static ogt_theme theme;                         /* the Open apps' look: its font is OpenMonitor's */
static struct TextFont *font;
static struct TextAttr ta;
static int gx, gy, gw, gh;                      /* the graphs' row */
static ULONG ncores;

static void draw_graph(int g, int x, int y, int w, int h, const char *title)
{
    struct RastPort *rp = win->RPort;
    int i, ix = x + 6, iy = y + 4 + rp->TxHeight + 4, iw = w - 12, ih = h - (iy - y) - 6;
    UWORD bg = dri->dri_Pens[BACKGROUNDPEN];
    SetAPen(rp, bg);
    RectFill(rp, x + 2, y + 1, x + w - 3, y + h - 2);
    DrawBevelBox(rp, x, y, w, h, GT_VisualInfo, vi, GTBB_Recessed, TRUE, TAG_DONE);
    SetAPen(rp, dri->dri_Pens[TEXTPEN]);
    SetDrMd(rp, JAM1);
    Move(rp, x + 8, y + 4 + rp->TxBaseline);
    Text(rp, (STRPTR)title, strlen(title));
    if (ih < 8 || iw < HIST) return;
    SetAPen(rp, dri->dri_Pens[SHADOWPEN]);                      /* quarter lines */
    for (i = 1; i < 4; i++) {
        int ly = iy + ih * i / 4, lx;
        for (lx = ix; lx < ix + iw; lx += 4) WritePixel(rp, lx, ly);
    }
    SetAPen(rp, graph_pen[g] >= 0 ? (UWORD)graph_pen[g] : dri->dri_Pens[FILLPEN]);
    for (i = 0; i < HIST; i++) {
        int px = ix + i * (iw - 1) / (HIST - 1), py = iy + ih - 1 - (int)((ULONG)hist[g][i] * (ULONG)(ih - 1) / 1000);
        if (!i) Move(rp, px, py); else Draw(rp, px, py);
    }
    /* drawn twice, a pixel lower: a line that can be seen on any theme */
    for (i = 0; i < HIST; i++) {
        int px = ix + i * (iw - 1) / (HIST - 1), py = iy + ih - (int)((ULONG)hist[g][i] * (ULONG)(ih - 1) / 1000);
        if (py > iy + ih - 1) py = iy + ih - 1;
        if (!i) Move(rp, px, py); else Draw(rp, px, py);
    }
}

static void draw_graphs(void)
{
    char t[96];
    int w = (gw - 2 * 8) / 3;
    if (ncores)
        snprintf(t, sizeof t, "CPU %lu%%  (1 CPU + %lu core%s)  %lu switches/s", (unsigned long)(cpu_tenths + 5) / 10,
                 (unsigned long)ncores, ncores == 1 ? "" : "s", (unsigned long)switches);
    else
        snprintf(t, sizeof t, "CPU %lu%%  %lu switches/s", (unsigned long)(cpu_tenths + 5) / 10, (unsigned long)switches);
    draw_graph(GR_CPU, gx, gy, w, gh, t);
    snprintf(t, sizeof t, "Memory %lu%%  chip %lu KB, fast %lu MB free", (unsigned long)(mem_used_tenths + 5) / 10,
             (unsigned long)chip_free_k, (unsigned long)(fast_free_k >> 10));
    draw_graph(GR_MEM, gx + w + 8, gy, w, gh, t);
    if (!OpenRTGBase)
        snprintf(t, sizeof t, "GPU  (no openrtg.library)");
    else if (!gpu_known)
        snprintf(t, sizeof t, "GPU");
    else
        snprintf(t, sizeof t, "GPU %lu%% of the drawing  %lu.%lu Mpx/s", (unsigned long)(gpu_share_tenths + 5) / 10,
                 (unsigned long)(gpu_kpx / 1000), (unsigned long)(gpu_kpx % 1000 / 100));
    draw_graph(GR_GPU, gx + 2 * (w + 8), gy, gw - 2 * (w + 8), gh, t);
}

/* ---- the list: columns, drawn by a hook --------------------------------------------------- */

static struct Hook lv_hook;
static int col_pri, col_state, col_stack;           /* each column's left edge, from the list's */
static const char *const state_names[] = { "invalid", "added", "running", "ready", "waiting", "exception", "removed" };

static void right_text(struct RastPort *rp, int right, int y, const char *s)
{
    int w = TextLength(rp, (STRPTR)s, strlen(s));
    Move(rp, right - w, y);
    Text(rp, (STRPTR)s, strlen(s));
}

static ULONG lv_render(REG(a0, struct Hook *h), REG(a2, struct Node *node), REG(a1, struct LVDrawMsg *msg))
{
    struct RastPort *rp = msg->lvdm_RastPort;
    struct row *r = (struct row *)node;
    UWORD *pens = msg->lvdm_DrawInfo->dri_Pens;
    int x = msg->lvdm_Bounds.MinX, y = msg->lvdm_Bounds.MinY, sel = msg->lvdm_State == LVR_SELECTED;
    int base = y + ((msg->lvdm_Bounds.MaxY - y + 1) - rp->TxHeight) / 2 + rp->TxBaseline;
    char s[40];
    (void)h;
    if (msg->lvdm_MethodID != LV_DRAW) return LVCB_UNKNOWN;
    if (font && rp->Font != font) SetFont(rp, font);
    SetAPen(rp, sel ? pens[FILLPEN] : pens[BACKGROUNDPEN]);
    SetDrMd(rp, JAM1);
    RectFill(rp, msg->lvdm_Bounds.MinX, y, msg->lvdm_Bounds.MaxX, msg->lvdm_Bounds.MaxY);
    SetAPen(rp, sel ? pens[FILLTEXTPEN] : pens[TEXTPEN]);
    Move(rp, x + 4, base);
    {
        /* a proportional font: the name stops before the CPU column, never under it */
        struct TextExtent te;
        int room = col_pri - 16 - TextLength(rp, (STRPTR)"100.0%", 6) - 12;
        ULONG fit = room > 0 ? TextFit(rp, (STRPTR)r->name, strlen(r->name), &te, NULL, 1, room, rp->TxHeight + 1) : 0;
        Text(rp, (STRPTR)r->name, fit);
    }
    snprintf(s, sizeof s, "%lu.%lu%%", (unsigned long)r->tenths / 10, (unsigned long)r->tenths % 10);
    right_text(rp, x + col_pri - 16, base, s);
    snprintf(s, sizeof s, "%ld", (long)r->pri);
    right_text(rp, x + col_state - 24, base, s);
    Move(rp, x + col_state, base);
    {
        const char *st = r->state <= 6 ? state_names[r->state] : "?";
        Text(rp, (STRPTR)st, strlen(st));
    }
    snprintf(s, sizeof s, "%lu KB%s", (unsigned long)(r->stack + 1023) / 1024, r->process ? "" : "  task");
    Move(rp, x + col_stack, base);
    Text(rp, (STRPTR)s, strlen(s));
    return LVCB_OK;
}

/* ---- the window --------------------------------------------------------------------------- */

enum { G_LIST, G_UP, G_DOWN, G_BREAK, G_END, G_STATUS, G_COUNT };
static struct Gadget *gad[G_COUNT];
static char status_text[120];
static struct Task *chosen;
static char chosen_name[40];
static int list_x, list_y, list_w;

static void status(const char *s)
{
    strncpy(status_text, s, sizeof status_text - 1);
    status_text[sizeof status_text - 1] = 0;
    if (win && gad[G_STATUS]) GT_SetGadgetAttrs(gad[G_STATUS], win, NULL, GTTX_Text, (ULONG)status_text, TAG_DONE);
}

static void draw_header(void)
{
    struct RastPort *rp = win->RPort;
    int y = list_y - 4 - rp->TxHeight + rp->TxBaseline;
    SetAPen(rp, dri->dri_Pens[BACKGROUNDPEN]);
    RectFill(rp, list_x, list_y - 4 - rp->TxHeight, list_x + list_w - 1, list_y - 3);
    SetAPen(rp, dri->dri_Pens[TEXTPEN]);
    SetDrMd(rp, JAM1);
    Move(rp, list_x + 4, y); Text(rp, (STRPTR)"Task or program", 15);
    right_text(rp, list_x + col_pri - 16, y, "CPU");
    right_text(rp, list_x + col_state - 24, y, "Priority");
    Move(rp, list_x + col_state, y); Text(rp, (STRPTR)"State", 5);
    Move(rp, list_x + col_stack, y); Text(rp, (STRPTR)"Stack", 5);
}

static void refresh_list(void)
{
    int i, sel = -1;
    GT_SetGadgetAttrs(gad[G_LIST], win, NULL, GTLV_Labels, ~0UL, TAG_DONE);
    take_tasks();
    for (i = 0; i < nrows; i++) if (rows[i].task == chosen) { sel = i; break; }
    if (sel < 0) chosen = NULL;
    GT_SetGadgetAttrs(gad[G_LIST], win, NULL, GTLV_Labels, (ULONG)&rowlist, GTLV_Selected, (ULONG)sel, TAG_DONE);
}

static void end_check(void);

static void tick(void)
{
    end_check();
    refresh_list();
    take_memory();
    take_gpu();
    push(GR_CPU, cpu_tenths);
    push(GR_MEM, mem_used_tenths);
    push(GR_GPU, gpu_share_tenths);
    draw_graphs();
}

static void change_pri(int by)
{
    char s[100];
    LONG now;
    if (!chosen) { status("Choose a task in the list first."); return; }
    Forbid();
    if (!task_alive(chosen)) { Permit(); chosen = NULL; status("That task has ended."); return; }
    now = chosen->tc_Node.ln_Pri + by;
    if (now > 20) now = 20;
    if (now < -20) now = -20;
    SetTaskPri(chosen, now);
    Permit();
    snprintf(s, sizeof s, "Priority now %ld.", (long)now);
    status(s);
}

static void send_break(void)
{
    char s[100];
    if (!chosen) { status("Choose a task in the list first."); return; }
    Forbid();
    if (!task_alive(chosen)) { Permit(); chosen = NULL; status("That task has ended."); return; }
    Signal(chosen, SIGBREAKF_CTRL_C);
    Permit();
    snprintf(s, sizeof s, "Sent Ctrl-C to %s: a program that listens for it stops.", chosen_name);
    status(s);
}

/* ---- ending a program: Ctrl-C, a moment, and only then Force quit, behind a warning ------------- */

#define END_WAIT 3                              /* seconds a program gets to tidy up after its Ctrl-C */

/* Which task, and which run of it: exec reuses a freed task's memory, so the address alone could name a
 * newer task by then. Its name's pointer and its stack's bottom go with it; for a Shell's process the
 * command it runs (cli_Module), since Ctrl-C ends the command and leaves the Shell open. */
struct ident {
    struct Task *t;
    STRPTR lnname;
    APTR splower;
    BPTR module;                                /* the Shell's command (0: at its prompt) */
    LONG shell;                                 /* the Shell's number, 0 when not a Shell's process */
    char name[40];
};
static struct ident ending;                     /* the program End program is waiting on; ending.t NULL for none */
static int ending_secs;

/* Under Forbid, with t alive. */
static void take_ident(struct ident *id, struct Task *t, const char *name)
{
    struct Process *p = (struct Process *)t;
    id->t = t;
    id->lnname = (STRPTR)t->tc_Node.ln_Name;
    id->splower = t->tc_SPLower;
    id->module = 0;
    id->shell = 0;
    if (t->tc_Node.ln_Type == NT_PROCESS && p->pr_CLI) {
        id->module = ((struct CommandLineInterface *)BADDR(p->pr_CLI))->cli_Module;
        id->shell = p->pr_TaskNum;
    }
    strcpy(id->name, name);
}

enum { ID_GONE, ID_THERE, ID_COMMAND_ENDED };

/* Under Forbid: gone, still there, or (a Shell's command) ended with its Shell still open. */
static int ident_state(const struct ident *id)
{
    struct Process *p = (struct Process *)id->t;
    if (!task_alive(id->t) || (STRPTR)id->t->tc_Node.ln_Name != id->lnname || id->t->tc_SPLower != id->splower) return ID_GONE;
    if (id->shell && (!p->pr_CLI || ((struct CommandLineInterface *)BADDR(p->pr_CLI))->cli_Module != id->module))
        return ID_COMMAND_ENDED;
    return ID_THERE;
}

/* A part of the system: a plain task (devices, input, the graphics and timer tasks), a process that is
 * a DOS device's handler (a file system: DH0, RAM, CON), or one of the OS's own processes. Not under
 * Forbid: LockDosList may wait. Only the task's address is compared, so a task that ends meanwhile is
 * harmless here. */
static int is_system(struct Task *t, const char *name)
{
    static const char *const own[] = { "Workbench", "ramlib", "IPrefs", "ConClip", "input.device", NULL };
    struct DosList *dl;
    int i, sys = 0;
    Forbid();
    if (!task_alive(t)) { Permit(); return 0; }
    sys = t->tc_Node.ln_Type != NT_PROCESS;
    Permit();
    for (i = 0; !sys && own[i]; i++) if (!strcmp(name, own[i])) sys = 1;
    if (!sys) {
        struct MsgPort *port = &((struct Process *)t)->pr_MsgPort;
        dl = LockDosList(LDF_DEVICES | LDF_READ);
        while ((dl = NextDosEntry(dl, LDF_DEVICES))) if (dl->dol_Task == port) { sys = 1; break; }
        UnLockDosList(LDF_DEVICES | LDF_READ);
    }
    return sys;
}

/* One requester in the theme's look as far as Intuition's go: its text as it is (a task's name may hold
 * a %), its buttons. The button's number, Intuition's way: the last one is 0. */
static LONG ask(const char *text, const char *buttons)
{
    struct EasyStruct es;
    APTR args[1];
    args[0] = (APTR)text;
    es.es_StructSize = sizeof es;
    es.es_Flags = 0;
    es.es_Title = (UBYTE *)"OpenMonitor";
    es.es_TextFormat = (UBYTE *)"%s";
    es.es_GadgetFormat = (UBYTE *)buttons;
    return EasyRequestArgs(win, &es, NULL, args);
}

/* Removes the task at once: RemTask, which frees only what it lists in tc_MemEntry (its stack, for most
 * programs). Under Forbid, with the same task (ident_state) found again first, so a task that ended
 * meanwhile, a newer one at its address, or a Shell whose command has ended is never touched. */
static void force_quit(const struct ident *id)
{
    char s[100];
    int st;
    Forbid();
    st = ident_state(id);
    if (st == ID_THERE && id->t != FindTask(NULL)) RemTask(id->t);
    Permit();
    if (chosen == id->t) chosen = NULL;
    if (ending.t == id->t) ending.t = NULL;
    if (st == ID_GONE) snprintf(s, sizeof s, "%s had already ended.", id->name);
    else if (st == ID_COMMAND_ENDED) snprintf(s, sizeof s, "%s stopped by itself; its Shell is still open.", id->name);
    else snprintf(s, sizeof s, "Removed %s. Restart when you can: what it had open stays open.", id->name);
    status(s);
    refresh_list();
}

/* The warnings before Force quit: one for any program, a second for a part of the system. 1 to go on. */
static int confirm_force(const struct ident *id, int after_break)
{
    char text[520], shell[80];
    LONG r;
    if (id->shell) snprintf(shell, sizeof shell, "\n\nIt runs in Shell %ld: Force quit removes that Shell too.", (long)id->shell);
    else shell[0] = 0;
    snprintf(text, sizeof text,
             "%s%s\n\n"
             "Force quit removes it at once. It cannot tidy up:\n"
             "its memory, files and windows stay open until you\n"
             "restart, and the Amiga may become unstable.\n"
             "Save your work in other programs first.%s",
             id->name, after_break ? " did not stop after Ctrl-C." : ": force quit?", shell);
    r = ask(text, after_break ? "Force quit|Wait longer|Cancel" : "Force quit|Cancel");
    if (after_break && r == 2) { ending_secs = 0; status("Waiting a little longer..."); return 0; }
    if (r != 1) { if (ending.t == id->t) ending.t = NULL; status("Left running."); return 0; }
    if (is_system(id->t, id->name)) {
        snprintf(text, sizeof text,
                 "%s is part of the system (a device, a file system\n"
                 "or one of the OS's own tasks).\n\n"
                 "Removing it will very likely stop the Amiga, and a\n"
                 "file system's disk may need checking afterwards.",
                 id->name);
        if (ask(text, "Remove it anyway|Cancel") != 1) { if (ending.t == id->t) ending.t = NULL; status("Left running."); return 0; }
    }
    return 1;
}

static int chosen_usable(void)
{
    if (!chosen) { status("Choose a task in the list first."); return 0; }
    if (chosen == FindTask(NULL)) { status("That is OpenMonitor: close its window to end it."); return 0; }
    return 1;
}

/* End program: a Ctrl-C now; tick() checks each second and offers Force quit after END_WAIT. */
static void end_program(void)
{
    char s[100];
    if (!chosen_usable()) return;
    Forbid();
    if (!task_alive(chosen)) { Permit(); chosen = NULL; status("That task has ended."); return; }
    take_ident(&ending, chosen, chosen_name);
    if (ending.shell && !ending.module) {       /* a Shell at its prompt: Ctrl-C ends nothing there */
        Permit();
        ending.t = NULL;
        snprintf(s, sizeof s, "Shell %ld is at its prompt: type EndCLI in it, or close its window.", (long)ending.shell);
        status(s);
        return;
    }
    Signal(chosen, SIGBREAKF_CTRL_C);
    Permit();
    ending_secs = 0;
    snprintf(s, sizeof s, "Stopping %s... (sent Ctrl-C, waiting up to %d seconds)", ending.name, END_WAIT);
    status(s);
}

static void end_check(void)
{
    char s[100];
    int st;
    if (!ending.t) return;
    Forbid();
    st = ident_state(&ending);
    Permit();
    if (st != ID_THERE) {
        snprintf(s, sizeof s, st == ID_GONE ? "%s has stopped." : "%s has stopped; its Shell is still open.", ending.name);
        status(s);
        ending.t = NULL;
    } else if (++ending_secs > END_WAIT) {      /* > : the first tick may come at once, so a full END_WAIT at least */
        struct ident id = ending;
        if (confirm_force(&id, 1)) force_quit(&id);
    }
}

/* Force quit straight away (Shift-Del): the same warnings, without the Ctrl-C first. */
static void force_chosen(void)
{
    struct ident id;
    if (!chosen_usable()) return;
    Forbid();
    if (!task_alive(chosen)) { Permit(); chosen = NULL; status("That task has ended."); return; }
    take_ident(&id, chosen, chosen_name);
    Permit();
    if (confirm_force(&id, 0)) force_quit(&id);
}

static int gui(void)
{
    struct Screen *scr = LockPubScreen(NULL);
    struct Gadget *glist = NULL, *g;
    struct NewGadget ng;
    struct MsgPort *tport = NULL;
    struct timerequest *tr = NULL;
    int timer = 0, quit = 0, rc = RETURN_OK, fh, lh, tb, W, H, top, left, row, bw, i, lvh, mode;
    if (!scr) return RETURN_FAIL;
    vi = GetVisualInfoA(scr, NULL);
    dri = GetScreenDrawInfo(scr);
    if (!vi || !dri) { rc = RETURN_FAIL; goto out; }
    ogt_look_load(&theme, &mode);
    if (!(font = ogt_open_font(&theme, scr, &ta))) { rc = RETURN_FAIL; goto out; }
    fh = font->tf_YSize;                         /* the theme's font: the window's text */
    tb = scr->WBorTop + scr->Font->ta_YSize + 1; /* the title bar is the screen's font, Intuition's */
    lh = fh + 6;
    left = scr->WBorLeft + 8;
    top = tb + 8;
    W = 800 - scr->WBorLeft - scr->WBorRight;
    H = 600 - tb - scr->WBorBottom;
    if (W + scr->WBorLeft + scr->WBorRight > scr->Width) W = scr->Width - scr->WBorLeft - scr->WBorRight;
    if (H + tb + scr->WBorBottom > scr->Height - scr->BarHeight - 1)
        H = scr->Height - scr->BarHeight - 1 - tb - scr->WBorBottom;
    gx = left; gy = top; gw = W - 16; gh = 120;
    list_x = left; list_w = W - 16;
    list_y = gy + gh + 10 + fh + 6;
    col_pri = list_w * 58 / 100;
    col_state = list_w * 66 / 100;
    col_stack = list_w * 82 / 100;
    lvh = H - (list_y - tb) - 2 * (lh + 8) - 4;
    for (i = 0; i < GR_N; i++)
        graph_pen[i] = ObtainBestPen(scr->ViewPort.ColorMap, (graph_rgb[i] >> 16) * 0x01010101UL,
                                     ((graph_rgb[i] >> 8) & 255) * 0x01010101UL, (graph_rgb[i] & 255) * 0x01010101UL,
                                     OBP_Precision, PRECISION_GUI, TAG_DONE);
    lv_hook.h_Entry = (__typeof__(lv_hook.h_Entry))lv_render;   /* whatever type this NDK gives h_Entry */
    list_init(&rowlist);

    g = CreateContext(&glist);
    memset(&ng, 0, sizeof ng);
    ng.ng_TextAttr = &ta;
    ng.ng_VisualInfo = vi;
    ng.ng_LeftEdge = list_x; ng.ng_TopEdge = list_y; ng.ng_Width = list_w; ng.ng_Height = lvh; ng.ng_GadgetID = G_LIST;
    g = gad[G_LIST] = CreateGadget(LISTVIEW_KIND, g, &ng, GTLV_Labels, (ULONG)&rowlist, GTLV_ShowSelected, (ULONG)NULL,
                                   GTLV_CallBack, (ULONG)&lv_hook, TAG_DONE);
    row = list_y + lvh + 8;
    bw = (list_w - 3 * 10) / 4;
    {
        static const char *const names[4] = { "Priority up", "Priority down", "Break (Ctrl-C)", "End program" };
        for (i = 0; i < 4; i++) {
            ng.ng_LeftEdge = list_x + i * (bw + 10); ng.ng_TopEdge = row; ng.ng_Width = bw; ng.ng_Height = lh;
            ng.ng_GadgetText = (STRPTR)names[i]; ng.ng_GadgetID = G_UP + i; ng.ng_Flags = PLACETEXT_IN;
            g = gad[G_UP + i] = CreateGadget(BUTTON_KIND, g, &ng, TAG_DONE);
        }
    }
    row += lh + 8;
    strcpy(status_text, "Busiest first, every second. Choose a task for its priority, a Ctrl-C or End program.");
    ng.ng_LeftEdge = list_x; ng.ng_TopEdge = row; ng.ng_Width = list_w; ng.ng_Height = lh; ng.ng_GadgetText = NULL;
    ng.ng_GadgetID = G_STATUS; ng.ng_Flags = 0;
    g = gad[G_STATUS] = CreateGadget(TEXT_KIND, g, &ng, GTTX_Text, (ULONG)status_text, GTTX_Border, TRUE, TAG_DONE);
    if (!g) { rc = RETURN_FAIL; goto out; }

    win = OpenWindowTags(NULL, WA_Title, (ULONG)"OpenMonitor", WA_ScreenTitle, (ULONG)"OpenMonitor 0.2", WA_PubScreen, (ULONG)scr,
                         WA_Left, (scr->Width - W - scr->WBorLeft - scr->WBorRight) / 2,
                         WA_Top, scr->BarHeight + 1 + (scr->Height - scr->BarHeight - 1 - H - tb - scr->WBorBottom) / 2,
                         WA_InnerWidth, W, WA_InnerHeight, H, WA_Gadgets, (ULONG)glist,
                         WA_DragBar, TRUE, WA_DepthGadget, TRUE, WA_CloseGadget, TRUE, WA_Activate, TRUE, WA_SmartRefresh, TRUE,
                         WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW | LISTVIEWIDCMP | BUTTONIDCMP | IDCMP_VANILLAKEY,
                         TAG_DONE);
    if (!win) { rc = RETURN_FAIL; goto out; }
    SetFont(win->RPort, font);
    GT_RefreshWindow(win, NULL);

    vbi.is_Node.ln_Type = NT_INTERRUPT;
    vbi.is_Node.ln_Pri = -60;
    vbi.is_Node.ln_Name = (char *)"OpenMonitor";
    vbi.is_Data = &S;
    vbi.is_Code = (void (*)())vb_server;
    AddIntServer(INTB_VERTB, &vbi);
    OpenRTGBase = OpenLibrary((STRPTR)"openrtg.library", 0);
    if (OpenRTGBase && OpenRTGBase->lib_Version == 0 && OpenRTGBase->lib_Revision < 11) { CloseLibrary(OpenRTGBase); OpenRTGBase = NULL; }
    if (OpenRTGBase) draw_stats(NULL, 0, 1);    /* counting from now */
    ncores = (board_cores() ? board_cores() : omc_cores());
    disp_before = SysBase->DispCount;
    take_memory();
    draw_header();
    draw_graphs();

    if ((tport = CreateMsgPort()) && (tr = (struct timerequest *)CreateIORequest(tport, sizeof *tr)))
        timer = !OpenDevice((STRPTR)TIMERNAME, UNIT_VBLANK, (struct IORequest *)tr, 0);
    if (timer) {
        tr->tr_node.io_Command = TR_ADDREQUEST; tr->tr_time.tv_secs = 1; tr->tr_time.tv_micro = 0;
        SendIO((struct IORequest *)tr);
    }

    while (!quit) {
        struct IntuiMessage *m;
        ULONG tsig = timer ? 1UL << tport->mp_SigBit : 0;
        ULONG got = Wait((1UL << win->UserPort->mp_SigBit) | tsig | SIGBREAKF_CTRL_C);
        if (got & SIGBREAKF_CTRL_C) quit = 1;
        if (timer && (got & tsig) && CheckIO((struct IORequest *)tr)) {
            WaitIO((struct IORequest *)tr);
            tick();
            tr->tr_node.io_Command = TR_ADDREQUEST; tr->tr_time.tv_secs = 1; tr->tr_time.tv_micro = 0;
            SendIO((struct IORequest *)tr);
        }
        while (!quit && (m = GT_GetIMsg(win->UserPort))) {
            ULONG cls = m->Class;
            UWORD code = m->Code, qual = m->Qualifier;
            struct Gadget *gg = (struct Gadget *)m->IAddress;
            GT_ReplyIMsg(m);
            if (cls == IDCMP_CLOSEWINDOW) quit = 1;
            else if (cls == IDCMP_VANILLAKEY && (code == 27 || code == 'q')) quit = 1;
            else if (cls == IDCMP_VANILLAKEY && code == 3) send_break();                 /* Ctrl-C */
            else if (cls == IDCMP_VANILLAKEY && code == 127) {                          /* Del, Shift-Del */
                if (qual & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)) force_chosen(); else end_program();
            }
            else if (cls == IDCMP_REFRESHWINDOW) {
                GT_BeginRefresh(win);
                draw_header();
                draw_graphs();
                GT_EndRefresh(win, TRUE);
            } else if (cls == IDCMP_GADGETUP) {
                switch (gg->GadgetID) {
                case G_LIST:
                    if (code < (UWORD)nrows) {
                        char s[100];
                        chosen = rows[code].task;
                        strcpy(chosen_name, rows[code].name);
                        snprintf(s, sizeof s, "%s: %s, priority %ld.", rows[code].name, rows[code].process ? "a process" : "a task",
                                 (long)rows[code].pri);
                        status(s);
                    }
                    break;
                case G_UP: change_pri(1); break;
                case G_DOWN: change_pri(-1); break;
                case G_BREAK: send_break(); break;
                case G_END: end_program(); break;
                }
            }
        }
    }
    RemIntServer(INTB_VERTB, &vbi);
out:
    if (timer) {
        if (!CheckIO((struct IORequest *)tr)) AbortIO((struct IORequest *)tr);
        WaitIO((struct IORequest *)tr);
        CloseDevice((struct IORequest *)tr);
    }
    if (tr) DeleteIORequest((struct IORequest *)tr);
    if (tport) DeleteMsgPort(tport);
    if (OpenRTGBase) { CloseLibrary(OpenRTGBase); OpenRTGBase = NULL; }
    if (win) { CloseWindow(win); win = NULL; }
    FreeGadgets(glist);
    for (i = 0; i < GR_N; i++) if (graph_pen[i] >= 0) ReleasePen(scr->ViewPort.ColorMap, graph_pen[i]);
    if (dri) FreeScreenDrawInfo(scr, dri);
    if (vi) FreeVisualInfo(vi);
    if (font) { CloseFont(font); font = NULL; }
    if (theme.text) ogt_theme_free(&theme);
    UnlockPubScreen(NULL, scr);
    return rc;
}

int main(void)
{
    return gui();
}
