/* OpenMonitor 0.3.2: what the machine is doing, as Task Manager on Windows or Resources on Ubuntu show it
 * (the user, 10 October 2026: "a cpu / task / process app similar to task manager in windows or resources
 * in ubuntu ... have it show gpu too").
 *
 * The window (800 x 600, the Open apps' rule):
 *   - three graphs across the top, the last minute of each: CPU (and the board's cores, when there are
 *     some), memory (chip and fast together), and the GPU: how much of the drawing OpenGPU did, with the
 *     pixels a second (openrtg.library's ORTG_DrawStats, 0.11 on);
 *   - every task and process, busiest first: name, CPU, priority, state, stack, owner;
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
 * Keys on the chosen row: Ctrl-C Break, Del End program, Shift-Del Force quit, Return Details.
 *
 * The right-click menu (0.3, the user's yes to the mockup, 11 October 2026): on any row, Break, Send
 * signal (Ctrl-C to Ctrl-F), Priority (High to Background, or Set...), Show owner, Details..., Copy
 * name, End program... and Force quit.... The list has an Owner column: the Shell, the system, a file
 * system, or a program started by Workbench or another program.
 *
 * How busy a task is: AmigaOS keeps no time per task, so OpenMonitor looks 50 times a second (a vertical
 * blank interrupt server) at which task is running, and counts. A task waiting, the machine idle, counts
 * as idle. Over a second that is 50 looks: the numbers are good to a couple of per cent, like any sampling
 * monitor's.
 *
 * Started from a Shell (0.3.2), it starts itself again as a Shell process of its own and the prompt comes
 * straight back; `OpenMonitor WAIT` stays in the Shell as before. A second start, from a Shell or its icon,
 * brings the open window to the front. It is built resident (libnix's -resident: every run gets its own
 * copy of the variables), so `Resident OpenMonitor PURE` and a ROM: copy share one seglist.
 *
 * MIT, Copyright (c) 2026 AmigaChrome Team. */

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/interrupts.h>
#include <exec/tasks.h>
#include <devices/timer.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <hardware/intbits.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <devices/inputevent.h>
#include <devices/clipboard.h>
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

static const char version[] __attribute__((used)) = "$VER: OpenMonitor 0.3.2 (16.10.2026) MIT, Copyright (c) 2026 AmigaChrome Team";
/* The window runs on 32 KB, however OpenMonitor is started (0.3.1). From a Shell (Run, a script) it got
 * the Shell's 4 KB, and closing the window rebooted the machine every time; with 32 KB it closes cleanly
 * (OpenUp 0.7.5's labs, 12 October 2026; OpenTypes 0.2.3 had the same). libnix's start code swaps to a
 * stack of this size when the one it was given is smaller. The icon's 32 KB is unchanged. */
unsigned long __stack = 32768;

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
    /* exec's base from address 4, not the SysBase variable: built resident (0.3.2), the variables are
     * reached through a4, which holds nothing of ours in an interrupt */
    struct ExecBase *eb;
    struct Task *t;
    __asm volatile ("move.l 4.w,%0" : "=a" (eb));
    t = eb->ThisTask;
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
    UBYTE state, process, kind;    /* kind: OWN_* below, who started it */
    LONG shell;                    /* OWN_SHELL: the Shell's number */
    ULONG tenths;                  /* CPU, in tenths of a per cent */
};

/* Who a task belongs to, as the Owner column and Show owner say it. AmigaOS keeps no parent, so: a Shell's
 * process by its number; a DOS device's handler is a file system; a plain task or one of the OS's own
 * processes is the system; any other process was started by Workbench or by another program. */
enum { OWN_PROGRAM, OWN_SHELL, OWN_FS, OWN_SYSTEM };
static const char *const own_names[] = { "Workbench", "ramlib", "IPrefs", "ConClip", "input.device", NULL };

#define MAXFS 128
static struct MsgPort *fs_port[MAXFS];          /* the DOS devices' handlers' ports, from the last look */
static int nfs;

/* The file systems' ports. Not under Disable: LockDosList may wait. */
static void take_handlers(void)
{
    struct DosList *dl = LockDosList(LDF_DEVICES | LDF_READ);
    nfs = 0;
    while ((dl = NextDosEntry(dl, LDF_DEVICES)) && nfs < MAXFS) if (dl->dol_Task) fs_port[nfs++] = dl->dol_Task;
    UnLockDosList(LDF_DEVICES | LDF_READ);
}

/* Under Disable or Forbid. */
static int task_kind(struct Task *t, LONG *shell)
{
    int i;
    *shell = 0;
    if (t->tc_Node.ln_Type != NT_PROCESS) return OWN_SYSTEM;
    if (((struct Process *)t)->pr_CLI) { *shell = ((struct Process *)t)->pr_TaskNum; return OWN_SHELL; }
    for (i = 0; i < nfs; i++) if (fs_port[i] == &((struct Process *)t)->pr_MsgPort) return OWN_FS;
    for (i = 0; own_names[i]; i++) if (t->tc_Node.ln_Name && !strcmp(t->tc_Node.ln_Name, own_names[i])) return OWN_SYSTEM;
    return OWN_PROGRAM;
}

/* The Owner column's words: "Shell 3", "system", "file system", "program". */
static void owner_text(int kind, LONG shell, char *out, int size)
{
    if (kind == OWN_SHELL) snprintf(out, size, "Shell %ld", (long)shell);
    else snprintf(out, size, "%s", kind == OWN_FS ? "file system" : kind == OWN_SYSTEM ? "system" : "program");
}

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
    r->kind = task_kind(t, &r->shell);
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
    take_handlers();
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
 * should read the amount of cores presented via autoconfig"): an AmigaChrome Team ($DA15) product 7 board carrying
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
static struct MsgPort *front_port;              /* a second start's message: the window to the front (0.3.2) */
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
static int col_pri, col_state, col_stack, col_owner; /* each column's left edge, from the list's */
static const char *const state_names[] = { "invalid", "added", "running", "ready", "waiting", "exception", "removed" };

static void right_text(struct RastPort *rp, int right, int y, const char *s)
{
    int w = TextLength(rp, (STRPTR)s, strlen(s));
    Move(rp, right - w, y);
    Text(rp, (STRPTR)s, strlen(s));
}

/* The list's hook is entered through lv_entry, which puts this run's a4 (kept in h_Data) in place around
 * lv_render. Built resident (0.3.2), each run has its own variables and a4 points at them; a hook gets the
 * caller's registers, so a4 is this run's only when the caller is OpenMonitor's own code. */
__asm__(
    "    .text\n"
    "    .even\n"
    "_lv_entry:\n"
    "    move.l  %a4,-(%sp)\n"
    "    move.l  16(%a0),%a4\n"          /* h_Data */
    "    pea     1f(%pc)\n"              /* lv_render comes back to 1 */
    "    move.l  12(%a0),-(%sp)\n"       /* h_SubEntry: lv_render, with a0-a2 as they came */
    "    rts\n"
    "1:  move.l  (%sp)+,%a4\n"
    "    rts\n");
extern void lv_entry(void) __asm("_lv_entry");   /* code: its address is not a4-relative */

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
    owner_text(r->kind, r->shell, s, sizeof s);
    if (!sel && r->kind != OWN_SHELL && r->kind != OWN_PROGRAM) SetAPen(rp, pens[SHADOWPEN]);   /* the system's: quieter */
    Move(rp, x + col_owner, base);
    Text(rp, (STRPTR)s, strlen(s));
    return LVCB_OK;
}

/* ---- the window --------------------------------------------------------------------------- */

enum { G_LIST, G_UP, G_DOWN, G_BREAK, G_END, G_STATUS, G_COUNT };
static struct Gadget *gad[G_COUNT];
static char status_text[120];
static struct Task *chosen;
static char chosen_name[40];
static struct MsgPort *tport;                   /* the one-second timer */
static struct timerequest *tr;
static int timer;
static int in_menu;                             /* the right-click menu is open: no requesters from tick() */
static int list_x, list_y, list_w, list_h;
static struct Screen *pscr;                     /* the screen OpenMonitor is on */
static int font_h;                              /* the theme's font's height */
static LONG danger_pen = -1;                    /* Force quit's red, in the menu */

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
    Move(rp, list_x + col_owner, y); Text(rp, (STRPTR)"Owner", 5);
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
    if (!in_menu) end_check();
    refresh_list();
    take_memory();
    take_gpu();
    push(GR_CPU, cpu_tenths);
    push(GR_MEM, mem_used_tenths);
    push(GR_GPU, gpu_share_tenths);
    draw_graphs();
}

/* The second is up: the next look, and the timer again. */
static void timer_due(void)
{
    if (!timer || !CheckIO((struct IORequest *)tr)) return;
    WaitIO((struct IORequest *)tr);
    tick();
    tr->tr_node.io_Command = TR_ADDREQUEST; tr->tr_time.tv_secs = 1; tr->tr_time.tv_micro = 0;
    SendIO((struct IORequest *)tr);
}

/* The chosen task's priority: up or down by some, or (absolute) set to a value, -20 to 20. */
static void change_pri(int by, int absolute)
{
    char s[100];
    LONG now;
    if (!chosen) { status("Choose a task in the list first."); return; }
    Forbid();
    if (!task_alive(chosen)) { Permit(); chosen = NULL; status("That task has ended."); return; }
    now = absolute ? by : chosen->tc_Node.ln_Pri + by;
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

/* A part of the system: a plain task (devices, input, the graphics and timer tasks), a file system's
 * handler (DH0, RAM, CON) or one of the OS's own processes (task_kind). */
static int is_system(struct Task *t)
{
    LONG shell;
    int kind;
    take_handlers();
    Forbid();
    if (!task_alive(t)) { Permit(); return 0; }
    kind = task_kind(t, &shell);
    Permit();
    if (kind == OWN_PROGRAM) {                  /* past take_handlers()' MAXFS: the whole list, for safety */
        struct MsgPort *port = &((struct Process *)t)->pr_MsgPort;
        struct DosList *dl = LockDosList(LDF_DEVICES | LDF_READ);
        while ((dl = NextDosEntry(dl, LDF_DEVICES))) if (dl->dol_Task == port) { kind = OWN_FS; break; }
        UnLockDosList(LDF_DEVICES | LDF_READ);
    }
    return kind == OWN_SYSTEM || kind == OWN_FS;
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
    if (is_system(id->t)) {
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

/* ---- the chosen task, more about it: owner, details, its name on the clipboard ------------------- */

/* Show owner: who started it and from what program file. */
static void show_owner(void)
{
    char text[400], name[40], task[40], cmd[120], who[100];
    LONG shell;
    int kind;
    if (!chosen) { status("Choose a task in the list first."); return; }
    take_handlers();
    cmd[0] = 0;
    Forbid();
    if (!task_alive(chosen)) { Permit(); chosen = NULL; status("That task has ended."); return; }
    kind = task_kind(chosen, &shell);
    strcpy(name, chosen_name);
    strncpy(task, chosen->tc_Node.ln_Name ? chosen->tc_Node.ln_Name : "(no name)", sizeof task - 1);
    task[sizeof task - 1] = 0;
    if (kind == OWN_SHELL) {
        struct CommandLineInterface *cli = (struct CommandLineInterface *)BADDR(((struct Process *)chosen)->pr_CLI);
        UBYTE *b = cli->cli_CommandName ? (UBYTE *)BADDR(cli->cli_CommandName) : NULL;
        if (cli->cli_Module && b && b[0]) {
            int k = b[0] < sizeof cmd - 1 ? b[0] : sizeof cmd - 1;
            memcpy(cmd, b + 1, k);
            cmd[k] = 0;
        }
    }
    Permit();
    if (kind == OWN_SHELL)
        snprintf(who, sizeof who, "Started from Shell %ld%s", (long)shell, cmd[0] ? ", running a command" : ", waiting at its prompt");
    else if (kind == OWN_FS)
        snprintf(who, sizeof who, "Part of the system: the file system (handler)\nof a DOS device");
    else if (kind == OWN_SYSTEM)
        snprintf(who, sizeof who, "Part of the system: a device or one of\nthe OS's own tasks");
    else
        snprintf(who, sizeof who, "Started by Workbench or by another program\n(not from a Shell)");
    snprintf(text, sizeof text, "%s\n\n%s%s%s\nTask name: %s", name, who, cmd[0] ? "\nProgram file: " : "", cmd, task);
    ask(text, "OK");
}

/* Details: what exec knows of it. */
static void show_details(void)
{
    static const char *const states[] = { "invalid", "added", "running", "ready", "waiting", "exception", "removed" };
    char text[600], who[24], stack[48];
    struct Task *t;
    ULONG alloc, wait, recvd, size, used, tenths = 0;
    LONG pri, shell;
    UBYTE state, type;
    int i, kind;
    if (!chosen) { status("Choose a task in the list first."); return; }
    take_handlers();
    Forbid();
    if (!task_alive(chosen)) { Permit(); chosen = NULL; status("That task has ended."); return; }
    t = chosen;
    pri = t->tc_Node.ln_Pri;
    state = t->tc_State;
    type = t->tc_Node.ln_Type;
    alloc = t->tc_SigAlloc; wait = t->tc_SigWait; recvd = t->tc_SigRecvd;
    size = (ULONG)t->tc_SPUpper - (ULONG)t->tc_SPLower;
    /* a task not running keeps its stack pointer here; OpenMonitor itself is running */
    used = t != FindTask(NULL) && (ULONG)t->tc_SPReg <= (ULONG)t->tc_SPUpper && (ULONG)t->tc_SPReg >= (ULONG)t->tc_SPLower ?
           (ULONG)t->tc_SPUpper - (ULONG)t->tc_SPReg : 0;
    kind = task_kind(t, &shell);
    Permit();
    for (i = 0; i < nrows; i++) if (rows[i].task == t) { tenths = rows[i].tenths; break; }
    owner_text(kind, shell, who, sizeof who);
    if (used) snprintf(stack, sizeof stack, "%lu KB, in use now %lu KB", (unsigned long)(size + 1023) / 1024, (unsigned long)(used + 1023) / 1024);
    else snprintf(stack, sizeof stack, "%lu KB", (unsigned long)(size + 1023) / 1024);
    snprintf(text, sizeof text,
             "%s\n\n"
             "A %s at $%08lx, owner: %s\n"
             "State: %s    Priority: %ld    CPU: %lu.%lu%%\n"
             "Stack: %s\n"
             "Signals: allocated $%08lx\n"
             "         waited on $%08lx\n"
             "         received  $%08lx",
             chosen_name, type == NT_PROCESS ? "process" : "task", (unsigned long)t, who,
             state <= 6 ? states[state] : "?", (long)pri, (unsigned long)tenths / 10, (unsigned long)tenths % 10,
             stack, (unsigned long)alloc, (unsigned long)wait, (unsigned long)recvd);
    ask(text, "OK");
}

/* The name on the clipboard (unit 0), as text (IFF FTXT), for pasting into a Shell or an editor. */
static void copy_name(void)
{
    struct MsgPort *port;
    struct IOClipReq *io = NULL;
    UBYTE buf[64];
    ULONG len = strlen(chosen_name), pad = len & 1, form = 4 + 8 + len + pad;
    int ok = 0;
    if (!chosen) { status("Choose a task in the list first."); return; }
    if (!(port = CreateMsgPort())) { status("No memory for the clipboard."); return; }
    if ((io = (struct IOClipReq *)CreateIORequest(port, sizeof *io)) && !OpenDevice((STRPTR)"clipboard.device", 0, (struct IORequest *)io, 0)) {
        memcpy(buf, "FORM", 4);
        buf[4] = form >> 24; buf[5] = form >> 16; buf[6] = form >> 8; buf[7] = form;
        memcpy(buf + 8, "FTXTCHRS", 8);
        buf[16] = len >> 24; buf[17] = len >> 16; buf[18] = len >> 8; buf[19] = len;
        memcpy(buf + 20, chosen_name, len);
        if (pad) buf[20 + len] = 0;
        io->io_Command = CMD_WRITE;
        io->io_Data = (STRPTR)buf;
        io->io_Length = 8 + form;
        io->io_Offset = 0;
        io->io_ClipID = 0;
        if (!DoIO((struct IORequest *)io)) {
            io->io_Command = CMD_UPDATE;
            ok = !DoIO((struct IORequest *)io);
        }
        CloseDevice((struct IORequest *)io);
    }
    if (io) DeleteIORequest((struct IORequest *)io);
    DeleteMsgPort(port);
    status(ok ? "The name is on the clipboard." : "The clipboard could not be written.");
}

/* One of the break signals, Ctrl-C to Ctrl-F. */
static void send_signal(ULONG sig, const char *what)
{
    char s[100];
    if (!chosen) { status("Choose a task in the list first."); return; }
    Forbid();
    if (!task_alive(chosen)) { Permit(); chosen = NULL; status("That task has ended."); return; }
    Signal(chosen, sig);
    Permit();
    snprintf(s, sizeof s, "Sent %s to %s.", what, chosen_name);
    status(s);
}

/* Priority, Set...: a small GadTools window with the number, -20 to 20. */
static void ask_priority(void)
{
    struct Gadget *glist = NULL, *g, *num;
    struct NewGadget ng;
    struct Window *w;
    LONG now;
    int done = 0, set = 0, lh = font_h + 6, W, lw;
    char title[60];
    if (!chosen) { status("Choose a task in the list first."); return; }
    Forbid();
    if (!task_alive(chosen)) { Permit(); chosen = NULL; status("That task has ended."); return; }
    now = chosen->tc_Node.ln_Pri;
    Permit();
    lw = TextLength(win->RPort, (STRPTR)"Priority (-20 to 20)", 20) + 16;
    W = lw + 80 + 24;
    if (W < 2 * 100 + 30) W = 2 * 100 + 30;
    g = CreateContext(&glist);
    memset(&ng, 0, sizeof ng);
    ng.ng_TextAttr = &ta;
    ng.ng_VisualInfo = vi;
    ng.ng_LeftEdge = pscr->WBorLeft + 12 + lw; ng.ng_TopEdge = pscr->WBorTop + pscr->Font->ta_YSize + 1 + 10;
    ng.ng_Width = 80; ng.ng_Height = lh; ng.ng_GadgetText = (STRPTR)"Priority (-20 to 20)"; ng.ng_Flags = PLACETEXT_LEFT; ng.ng_GadgetID = 1;
    g = num = CreateGadget(INTEGER_KIND, g, &ng, GTIN_Number, (ULONG)now, GTIN_MaxChars, 4, TAG_DONE);
    ng.ng_TopEdge += lh + 12; ng.ng_LeftEdge = pscr->WBorLeft + 12; ng.ng_Width = 100; ng.ng_Flags = PLACETEXT_IN;
    ng.ng_GadgetText = (STRPTR)"Set"; ng.ng_GadgetID = 2;
    g = CreateGadget(BUTTON_KIND, g, &ng, TAG_DONE);
    ng.ng_LeftEdge = pscr->WBorLeft + 12 + W - 24 - 100; ng.ng_GadgetText = (STRPTR)"Cancel"; ng.ng_GadgetID = 3;
    g = CreateGadget(BUTTON_KIND, g, &ng, TAG_DONE);
    snprintf(title, sizeof title, "Priority of %s", chosen_name);
    if (g)
        w = OpenWindowTags(NULL, WA_Title, (ULONG)title, WA_PubScreen, (ULONG)pscr,
                           WA_Left, win->LeftEdge + 60, WA_Top, win->TopEdge + 60,
                           WA_InnerWidth, W, WA_InnerHeight, 10 + lh + 12 + lh + 10, WA_Gadgets, (ULONG)glist,
                           WA_DragBar, TRUE, WA_DepthGadget, TRUE, WA_CloseGadget, TRUE, WA_Activate, TRUE, WA_SmartRefresh, TRUE,
                           WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW | IDCMP_GADGETUP | IDCMP_VANILLAKEY | INTEGERIDCMP,
                           TAG_DONE);
    else
        w = NULL;
    if (!w) { FreeGadgets(glist); status("The priority window could not open."); return; }
    SetFont(w->RPort, font);
    GT_RefreshWindow(w, NULL);
    ActivateGadget(num, w, NULL);
    while (!done) {
        struct IntuiMessage *m;
        WaitPort(w->UserPort);
        while ((m = GT_GetIMsg(w->UserPort))) {
            ULONG cls = m->Class;
            UWORD code = m->Code;
            struct Gadget *gg = (struct Gadget *)m->IAddress;
            GT_ReplyIMsg(m);
            if (cls == IDCMP_CLOSEWINDOW || (cls == IDCMP_VANILLAKEY && code == 27)) done = 1;
            else if (cls == IDCMP_REFRESHWINDOW) { GT_BeginRefresh(w); GT_EndRefresh(w, TRUE); }
            else if (cls == IDCMP_GADGETUP && (gg->GadgetID == 1 || gg->GadgetID == 2)) { done = set = 1; }
            else if (cls == IDCMP_GADGETUP && gg->GadgetID == 3) done = 1;
        }
    }
    if (set) {
        LONG v = 0;
        GT_GetGadgetAttrs(num, w, NULL, GTIN_Number, (ULONG)&v, TAG_DONE);
        change_pri(v, 1);
    }
    CloseWindow(w);
    FreeGadgets(glist);
}

/* ---- the right-click menu (the user's yes to the mockup, 11 October 2026) -------------------------
 * Intuition's menus open only in the screen's title bar, so the menu is drawn here: a borderless window
 * at the pointer, the theme's font and the screen's pens, a second one beside it for Priority and Send
 * signal. Held: release on an entry. Clicked: it stays open and the next click chooses; a click
 * anywhere else, or Esc, closes it. */

enum { MI_ITEM, MI_SUB, MI_SEP, MI_DANGER, MI_HEAD };
enum { A_NONE, A_BREAK, A_SIG_C, A_SIG_D, A_SIG_E, A_SIG_F, A_PRI, A_PRI_SET, A_OWNER, A_DETAILS, A_COPY, A_END, A_FORCE };

struct mitem {
    UBYTE kind, action;
    BYTE pri;                                   /* A_PRI: the priority it sets */
    const char *label, *key;                    /* MI_HEAD: the name, then what it is */
    const struct mitem *sub;
    int nsub;
};

static const struct mitem sig_menu[] = {
    { MI_ITEM, A_SIG_C, 0, "Ctrl-C  (break: stop)", NULL, NULL, 0 },
    { MI_ITEM, A_SIG_D, 0, "Ctrl-D  (break a script)", NULL, NULL, 0 },
    { MI_ITEM, A_SIG_E, 0, "Ctrl-E", NULL, NULL, 0 },
    { MI_ITEM, A_SIG_F, 0, "Ctrl-F  (bring to front)", NULL, NULL, 0 },
};
static const struct mitem pri_menu[] = {
    { MI_ITEM, A_PRI, 5, "High  (+5)", NULL, NULL, 0 },
    { MI_ITEM, A_PRI, 1, "Above normal  (+1)", NULL, NULL, 0 },
    { MI_ITEM, A_PRI, 0, "Normal  (0)", NULL, NULL, 0 },
    { MI_ITEM, A_PRI, -1, "Below normal  (-1)", NULL, NULL, 0 },
    { MI_ITEM, A_PRI, -5, "Low  (-5)", NULL, NULL, 0 },
    { MI_ITEM, A_PRI, -20, "Background  (-20)", NULL, NULL, 0 },
    { MI_SEP, A_NONE, 0, NULL, NULL, NULL, 0 },
    { MI_ITEM, A_PRI_SET, 0, "Set...", NULL, NULL, 0 },
};

#define MAXMI 16
#define PANEL_IDCMP (IDCMP_MOUSEMOVE | IDCMP_MOUSEBUTTONS | IDCMP_INACTIVEWINDOW | IDCMP_VANILLAKEY)
struct panel {
    struct Window *w;
    int shared;                                 /* its UserPort is the menu's */
    const struct mitem *it;
    int n, hl, width, height;
    int top[MAXMI], h[MAXMI];
};

static int item_h(const struct mitem *m)
{
    return m->kind == MI_HEAD ? 2 * font_h + 10 : m->kind == MI_SEP ? 7 : font_h + 6;
}

static int selectable(const struct mitem *m)
{
    return m->kind == MI_ITEM || m->kind == MI_SUB || m->kind == MI_DANGER;
}

static void panel_draw(struct panel *p, LONG cur_pri)
{
    struct RastPort *rp = p->w->RPort;
    UWORD *pens = dri->dri_Pens;
    int i, w = p->width, h = p->height;
    SetFont(rp, font);
    SetDrMd(rp, JAM1);
    SetAPen(rp, pens[BACKGROUNDPEN]);
    RectFill(rp, 0, 0, w - 1, h - 1);
    SetAPen(rp, pens[SHINEPEN]);
    Move(rp, 0, h - 1); Draw(rp, 0, 0); Draw(rp, w - 1, 0);
    SetAPen(rp, pens[SHADOWPEN]);
    Draw(rp, w - 1, h - 1); Draw(rp, 0, h - 1);
    for (i = 0; i < p->n; i++) {
        const struct mitem *m = &p->it[i];
        int y = p->top[i], on = i == p->hl, base = y + 3 + rp->TxBaseline;
        if (m->kind == MI_SEP) {
            SetAPen(rp, pens[SHADOWPEN]);
            Move(rp, 6, y + 3); Draw(rp, w - 7, y + 3);
            continue;
        }
        if (m->kind == MI_HEAD) {
            ULONG en = AskSoftStyle(rp);
            SetAPen(rp, pens[TEXTPEN]);
            SetSoftStyle(rp, FSF_BOLD, en);
            Move(rp, 10, base + 1); Text(rp, (STRPTR)m->label, strlen(m->label));
            SetSoftStyle(rp, FS_NORMAL, en);
            SetAPen(rp, pens[SHADOWPEN]);
            Move(rp, 10, base + 1 + font_h + 2); Text(rp, (STRPTR)m->key, strlen(m->key));
            Move(rp, 4, y + p->h[i] - 2); Draw(rp, w - 5, y + p->h[i] - 2);
            continue;
        }
        if (on) {
            SetAPen(rp, pens[FILLPEN]);
            RectFill(rp, 3, y, w - 4, y + p->h[i] - 1);
        }
        SetAPen(rp, on ? pens[FILLTEXTPEN] : m->kind == MI_DANGER && danger_pen >= 0 ? (UWORD)danger_pen : pens[TEXTPEN]);
        if (m->action == A_PRI && m->pri == cur_pri) {          /* the task's priority now: a tick */
            int cy = y + p->h[i] / 2;
            Move(rp, 7, cy); Draw(rp, 10, cy + 3); Draw(rp, 16, cy - 3);
            Move(rp, 7, cy + 1); Draw(rp, 10, cy + 4); Draw(rp, 16, cy - 2);
        }
        Move(rp, 22, base); Text(rp, (STRPTR)m->label, strlen(m->label));
        if (m->kind == MI_SUB) {
            int cy = y + p->h[i] / 2, ax = w - 12;
            Move(rp, ax, cy - 4); Draw(rp, ax + 4, cy); Draw(rp, ax, cy + 4);
            Move(rp, ax + 1, cy - 4); Draw(rp, ax + 5, cy); Draw(rp, ax + 1, cy + 4);
        }
        if (m->key) {
            int kw = TextLength(rp, (STRPTR)m->key, strlen(m->key));
            if (!on) SetAPen(rp, pens[SHADOWPEN]);
            Move(rp, w - (m->kind == MI_SUB ? 22 : 10) - kw, base); Text(rp, (STRPTR)m->key, strlen(m->key));
        }
    }
}

/* Opens a panel with its top left at x, y, kept on the screen; beside: the parent's left edge, to open on
 * its left instead when there is no room on the right. */
static int panel_open(struct panel *p, int x, int y, int beside, int active, struct MsgPort *share)
{
    struct RastPort *rp = win->RPort;
    int i, w = 150, kw;
    p->height = 3;
    for (i = 0; i < p->n && i < MAXMI; i++) {
        const struct mitem *m = &p->it[i];
        p->top[i] = p->height;
        p->h[i] = item_h(m);
        p->height += p->h[i];
        if (m->label) {
            kw = m->key ? TextLength(rp, (STRPTR)m->key, strlen(m->key)) + 24 : 0;
            if (m->kind == MI_SUB) kw += 20;
            if (TextLength(rp, (STRPTR)m->label, strlen(m->label)) + 40 + kw > w) w = TextLength(rp, (STRPTR)m->label, strlen(m->label)) + 40 + kw;
        }
        if (m->kind == MI_HEAD && m->key && TextLength(rp, (STRPTR)m->key, strlen(m->key)) + 24 > w)
            w = TextLength(rp, (STRPTR)m->key, strlen(m->key)) + 24;
    }
    p->n = i;
    p->height += 3;
    p->width = w;
    p->hl = -1;
    if (x + w > pscr->Width) x = beside >= 0 ? beside - w + 4 : pscr->Width - w;
    if (x < 0) x = 0;
    if (y + p->height > pscr->Height) y = pscr->Height - p->height;
    if (y < 0) y = 0;
    p->w = OpenWindowTags(NULL, WA_PubScreen, (ULONG)pscr, WA_Left, x, WA_Top, y, WA_Width, w, WA_Height, p->height,
                          WA_Borderless, TRUE, WA_Activate, (ULONG)active, WA_RMBTrap, TRUE, WA_ReportMouse, TRUE,
                          WA_SmartRefresh, TRUE, WA_NoCareRefresh, TRUE,
                          WA_IDCMP, share ? 0 : PANEL_IDCMP, TAG_DONE);
    p->shared = share != NULL;
    if (p->w && share) {                        /* the submenu shares the menu's port: a click on it is seen */
        p->w->UserPort = share;
        ModifyIDCMP(p->w, PANEL_IDCMP);
    }
    return p->w != NULL;
}

static void panel_close(struct panel *p)
{
    if (!p->w) return;
    if (p->shared) {                            /* a shared port: its messages go back first (the RKM's way) */
        struct IntuiMessage *im, *next;
        Forbid();
        for (im = (struct IntuiMessage *)p->w->UserPort->mp_MsgList.lh_Head;
             (next = (struct IntuiMessage *)im->ExecMessage.mn_Node.ln_Succ); im = next)
            if (im->IDCMPWindow == p->w) { Remove((struct Node *)im); ReplyMsg((struct Message *)im); }
        p->w->UserPort = NULL;
        ModifyIDCMP(p->w, 0);
        Permit();
    }
    CloseWindow(p->w);
    p->w = NULL;
}

static int panel_inside(const struct panel *p, int sx, int sy)
{
    return p->w && sx >= p->w->LeftEdge && sx < p->w->LeftEdge + p->width && sy >= p->w->TopEdge && sy < p->w->TopEdge + p->height;
}

/* The entry under the screen's point, or -1. */
static int panel_hit(const struct panel *p, int sx, int sy)
{
    int i, y;
    if (!panel_inside(p, sx, sy)) return -1;
    y = sy - p->w->TopEdge;
    for (i = 0; i < p->n; i++) if (y >= p->top[i] && y < p->top[i] + p->h[i]) return selectable(&p->it[i]) ? i : -1;
    return -1;
}

/* Runs the menu until an entry is chosen or it is closed: the entry's action (A_PRI's priority in *pri). */
static int run_menu(const struct mitem *items, int n, LONG cur_pri, LONG *pri)
{
    struct panel m, sb;
    int ox = pscr->MouseX, oy = pscr->MouseY, sub_of = -1, result = A_NONE, done = 0, sticky = 0, moved = 0;
    memset(&m, 0, sizeof m);
    memset(&sb, 0, sizeof sb);
    m.it = items; m.n = n;
    if (!panel_open(&m, ox + 2, oy + 2, -1, 1, NULL)) return A_NONE;
    panel_draw(&m, cur_pri);
    in_menu = 1;
    while (!done) {
        struct IntuiMessage *im;
        ULONG tsig = timer ? 1UL << tport->mp_SigBit : 0;
        if (Wait((1UL << m.w->UserPort->mp_SigBit) | tsig) & tsig) timer_due();   /* the list and graphs go on */
        while (!done && (im = (struct IntuiMessage *)GetMsg(m.w->UserPort))) {
            ULONG cls = im->Class;
            UWORD code = im->Code;
            int sx = pscr->MouseX, sy = pscr->MouseY, hm, hs;
            ReplyMsg((struct Message *)im);
            if (sx - ox > 4 || ox - sx > 4 || sy - oy > 4 || oy - sy > 4) moved = 1;
            if (cls == IDCMP_VANILLAKEY && code == 27) { done = 1; break; }
            /* the other one of the two taking over (a click on the submenu, or back) keeps it open */
            if (cls == IDCMP_INACTIVEWINDOW && !panel_inside(&m, sx, sy) && !panel_inside(&sb, sx, sy)) { done = 1; break; }
            /* the highlight follows the pointer */
            hs = panel_hit(&sb, sx, sy);
            hm = panel_hit(&m, sx, sy);
            if (panel_inside(&sb, sx, sy)) {
                if (hs != sb.hl) { sb.hl = hs; panel_draw(&sb, cur_pri); }
            } else if (panel_inside(&m, sx, sy)) {
                if (sb.w && sb.hl >= 0) { sb.hl = -1; panel_draw(&sb, cur_pri); }
                if (hm != m.hl && hm >= 0) {
                    m.hl = hm;
                    panel_draw(&m, cur_pri);
                    if (sub_of >= 0 && sub_of != hm) { panel_close(&sb); sub_of = -1; }
                    if (m.it[hm].kind == MI_SUB && sub_of != hm) {
                        sb.it = m.it[hm].sub; sb.n = m.it[hm].nsub;
                        if (panel_open(&sb, m.w->LeftEdge + m.width - 4, m.w->TopEdge + m.top[hm] - 3, m.w->LeftEdge, 0, m.w->UserPort)) {
                            sub_of = hm;
                            panel_draw(&sb, cur_pri);
                        }
                    }
                }
            }
            if (cls == IDCMP_MOUSEBUTTONS) {
                int up = code == MENUUP || code == SELECTUP, down = code == MENUDOWN || code == SELECTDOWN;
                const struct mitem *pick = NULL;
                if (sb.w && sb.hl >= 0 && panel_inside(&sb, sx, sy)) pick = &sb.it[sb.hl];
                else if (m.hl >= 0 && hm == m.hl && m.it[m.hl].kind != MI_SUB) pick = &m.it[m.hl];
                if (up && !sticky && !moved) sticky = 1;                          /* a click: it stays open */
                else if ((up && !sticky) || (down && sticky)) {
                    if (pick) { result = pick->action; *pri = pick->pri; done = 1; }
                    else if (!panel_inside(&m, sx, sy) && !panel_inside(&sb, sx, sy)) done = 1;
                    else if (up) done = 1;                                       /* held and let go on no entry */
                }
            }
        }
    }
    panel_close(&sb);
    panel_close(&m);
    in_menu = 0;
    return result;
}

/* A right-click on the list: that row is chosen and the menu opens at the pointer. */
static void row_menu(int mx, int my)
{
    static char sub[48];
    char own[24];
    struct mitem items[13];
    struct ident id;
    ULONG top = 0;
    LONG pri = 0, cur;
    int r, n = 0, a, st;
    if (mx < list_x + 2 || mx >= list_x + list_w - 18 || my < list_y + 2 || my >= list_y + list_h - 2) return;
    GT_GetGadgetAttrs(gad[G_LIST], win, NULL, GTLV_Top, (ULONG)&top, TAG_DONE);
    r = (int)top + (my - list_y - 2) / font_h;
    if (r < 0 || r >= nrows) return;
    chosen = rows[r].task;
    strcpy(chosen_name, rows[r].name);
    GT_SetGadgetAttrs(gad[G_LIST], win, NULL, GTLV_Selected, (ULONG)r, TAG_DONE);
    cur = rows[r].pri;
    Forbid();                                   /* the same run of it after the menu, not a newer task there */
    if (!task_alive(chosen)) { Permit(); chosen = NULL; status("That task has ended."); return; }
    take_ident(&id, chosen, chosen_name);
    Permit();
    owner_text(rows[r].kind, rows[r].shell, own, sizeof own);
    snprintf(sub, sizeof sub, "%s, %s", rows[r].process ? "process" : "task", own);
    memset(items, 0, sizeof items);
    items[n].kind = MI_HEAD; items[n].label = chosen_name; items[n++].key = sub;
    items[n].kind = MI_ITEM; items[n].action = A_BREAK; items[n].label = "Break (Ctrl-C)"; items[n++].key = "Ctrl C";
    items[n].kind = MI_SUB; items[n].label = "Send signal"; items[n].sub = sig_menu; items[n++].nsub = 4;
    items[n++].kind = MI_SEP;
    items[n].kind = MI_SUB; items[n].label = "Priority"; items[n].sub = pri_menu; items[n++].nsub = 8;
    items[n++].kind = MI_SEP;
    items[n].kind = MI_ITEM; items[n].action = A_OWNER; items[n++].label = "Show owner";
    items[n].kind = MI_ITEM; items[n].action = A_DETAILS; items[n].label = "Details..."; items[n++].key = "Return";
    items[n].kind = MI_ITEM; items[n].action = A_COPY; items[n++].label = "Copy name";
    items[n++].kind = MI_SEP;
    items[n].kind = MI_ITEM; items[n].action = A_END; items[n].label = "End program..."; items[n++].key = "Del";
    items[n].kind = MI_DANGER; items[n].action = A_FORCE; items[n].label = "Force quit..."; items[n++].key = "Shift Del";
    a = run_menu(items, n, cur, &pri);
    if (a == A_NONE) return;
    Forbid();
    st = ident_state(&id);
    Permit();
    if (st == ID_GONE) { if (chosen == id.t) chosen = NULL; status("That task has ended."); return; }
    if (st == ID_COMMAND_ENDED && (a == A_END || a == A_FORCE)) {
        char s[100];
        snprintf(s, sizeof s, "%s has stopped; its Shell is still open.", id.name);
        status(s);
        return;
    }
    chosen = id.t;
    switch (a) {
    case A_BREAK: case A_SIG_C: send_break(); break;
    case A_SIG_D: send_signal(SIGBREAKF_CTRL_D, "Ctrl-D"); break;
    case A_SIG_E: send_signal(SIGBREAKF_CTRL_E, "Ctrl-E"); break;
    case A_SIG_F: send_signal(SIGBREAKF_CTRL_F, "Ctrl-F"); break;
    case A_PRI: change_pri(pri, 1); break;
    case A_PRI_SET: ask_priority(); break;
    case A_OWNER: show_owner(); break;
    case A_DETAILS: show_details(); break;
    case A_COPY: copy_name(); break;
    case A_END: end_program(); break;
    case A_FORCE: force_chosen(); break;
    }
}

static int gui(void)
{
    struct Screen *scr = LockPubScreen(NULL);
    struct Gadget *glist = NULL, *g;
    struct NewGadget ng;
    int quit = 0, rc = RETURN_OK, fh, lh, tb, W, H, top, left, row, bw, i, lvh, mode;
    if (!scr) return RETURN_FAIL;
    vi = GetVisualInfoA(scr, NULL);
    dri = GetScreenDrawInfo(scr);
    if (!vi || !dri) { rc = RETURN_FAIL; goto out; }
    ogt_look_load(&theme, &mode);
    if (!(font = ogt_open_font(&theme, scr, &ta))) { rc = RETURN_FAIL; goto out; }
    fh = font_h = font->tf_YSize;                /* the theme's font: the window's text */
    pscr = scr;
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
    col_pri = list_w * 48 / 100;
    col_state = list_w * 56 / 100;
    col_stack = list_w * 70 / 100;
    col_owner = list_w * 84 / 100;
    lvh = list_h = H - (list_y - tb) - 2 * (lh + 8) - 4;
    danger_pen = ObtainBestPen(scr->ViewPort.ColorMap, 0xb0b0b0b0UL, 0x28282828UL, 0x28282828UL, OBP_Precision, PRECISION_GUI, TAG_DONE);
    for (i = 0; i < GR_N; i++)
        graph_pen[i] = ObtainBestPen(scr->ViewPort.ColorMap, (graph_rgb[i] >> 16) * 0x01010101UL,
                                     ((graph_rgb[i] >> 8) & 255) * 0x01010101UL, (graph_rgb[i] & 255) * 0x01010101UL,
                                     OBP_Precision, PRECISION_GUI, TAG_DONE);
    {
        APTR a4v;
        __asm volatile ("move.l %%a4,%0" : "=r" (a4v));
        lv_hook.h_Entry = (__typeof__(lv_hook.h_Entry))lv_entry;     /* whatever type this NDK gives h_Entry */
        lv_hook.h_SubEntry = (__typeof__(lv_hook.h_SubEntry))lv_render;
        lv_hook.h_Data = a4v;
    }
    list_init(&rowlist);

    g = CreateContext(&glist);
    memset(&ng, 0, sizeof ng);
    ng.ng_TextAttr = &ta;
    ng.ng_VisualInfo = vi;
    ng.ng_LeftEdge = list_x; ng.ng_TopEdge = list_y; ng.ng_Width = list_w; ng.ng_Height = lvh; ng.ng_GadgetID = G_LIST;
    g = gad[G_LIST] = CreateGadget(LISTVIEW_KIND, g, &ng, GTLV_Labels, (ULONG)&rowlist, GTLV_ShowSelected, (ULONG)NULL,
                                   GTLV_CallBack, (ULONG)&lv_hook, GTLV_ItemHeight, (ULONG)fh, TAG_DONE);
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
    strcpy(status_text, "Busiest first, every second. Right-click a task for more.");
    ng.ng_LeftEdge = list_x; ng.ng_TopEdge = row; ng.ng_Width = list_w; ng.ng_Height = lh; ng.ng_GadgetText = NULL;
    ng.ng_GadgetID = G_STATUS; ng.ng_Flags = 0;
    g = gad[G_STATUS] = CreateGadget(TEXT_KIND, g, &ng, GTTX_Text, (ULONG)status_text, GTTX_Border, TRUE, TAG_DONE);
    if (!g) { rc = RETURN_FAIL; goto out; }

    win = OpenWindowTags(NULL, WA_Title, (ULONG)"OpenMonitor", WA_ScreenTitle, (ULONG)"OpenMonitor 0.3.2", WA_PubScreen, (ULONG)scr,
                         WA_Left, (scr->Width - W - scr->WBorLeft - scr->WBorRight) / 2,
                         WA_Top, scr->BarHeight + 1 + (scr->Height - scr->BarHeight - 1 - H - tb - scr->WBorBottom) / 2,
                         WA_InnerWidth, W, WA_InnerHeight, H, WA_Gadgets, (ULONG)glist,
                         WA_DragBar, TRUE, WA_DepthGadget, TRUE, WA_CloseGadget, TRUE, WA_Activate, TRUE, WA_SmartRefresh, TRUE,
                         WA_RMBTrap, TRUE,      /* no menus of its own: the right button opens the list's menu */
                         WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW | LISTVIEWIDCMP | BUTTONIDCMP | IDCMP_VANILLAKEY |
                                   IDCMP_MOUSEBUTTONS,
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
        ULONG fsig = front_port ? 1UL << front_port->mp_SigBit : 0;
        ULONG got = Wait((1UL << win->UserPort->mp_SigBit) | tsig | fsig | SIGBREAKF_CTRL_C);
        if (got & SIGBREAKF_CTRL_C) quit = 1;
        if (got & fsig) {
            struct Message *fm;
            while ((fm = GetMsg(front_port))) ReplyMsg(fm);
            ScreenToFront(win->WScreen);
            WindowToFront(win);
            ActivateWindow(win);
        }
        if (got & tsig) timer_due();
        while (!quit && (m = GT_GetIMsg(win->UserPort))) {
            ULONG cls = m->Class;
            UWORD code = m->Code, qual = m->Qualifier;
            WORD mx = m->MouseX, my = m->MouseY;
            struct Gadget *gg = (struct Gadget *)m->IAddress;
            GT_ReplyIMsg(m);
            if (cls == IDCMP_CLOSEWINDOW) quit = 1;
            else if (cls == IDCMP_VANILLAKEY && (code == 27 || code == 'q')) quit = 1;
            else if (cls == IDCMP_VANILLAKEY && code == 3) send_break();                 /* Ctrl-C */
            else if (cls == IDCMP_VANILLAKEY && code == 13) show_details();              /* Return */
            else if (cls == IDCMP_MOUSEBUTTONS && code == MENUDOWN) row_menu(mx, my);
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
                case G_UP: change_pri(1, 0); break;
                case G_DOWN: change_pri(-1, 0); break;
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
    if (tr) { DeleteIORequest((struct IORequest *)tr); tr = NULL; }
    if (tport) { DeleteMsgPort(tport); tport = NULL; }
    timer = 0;
    if (OpenRTGBase) { CloseLibrary(OpenRTGBase); OpenRTGBase = NULL; }
    if (win) { CloseWindow(win); win = NULL; }
    FreeGadgets(glist);
    for (i = 0; i < GR_N; i++) if (graph_pen[i] >= 0) ReleasePen(scr->ViewPort.ColorMap, graph_pen[i]);
    if (danger_pen >= 0) { ReleasePen(scr->ViewPort.ColorMap, danger_pen); danger_pen = -1; }
    if (dri) FreeScreenDrawInfo(scr, dri);
    if (vi) FreeVisualInfo(vi);
    if (font) { CloseFont(font); font = NULL; }
    if (theme.text) ogt_theme_free(&theme);
    UnlockPubScreen(NULL, scr);
    return rc;
}

/* ---- one window, and leaving the Shell (0.3.2) ---------------------------------------------- */

/* A second start brings the open window to the front: the running one keeps a public port by this name,
 * and a start that finds it sends it a message and ends. */
#define FRONT_PORT "OpenMonitor"

/* Under Forbid, so the port can't go between FindPort and PutMsg: 1 when a running OpenMonitor had it. */
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

/* Takes the name for this run, or, when a running one has it, sends it to the front: NULL then. */
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
    while ((m = GetMsg(p))) ReplyMsg(m);        /* a start that came while closing: answered, it ends */
    DeleteMsgPort(p);
}

/* From a Shell: OpenMonitor starts again as a Shell process of its own, as Run would, with NIL: for its
 * input and output, and this command ends at once. The new process is started with WAIT, so it opens
 * the window where it is. It runs the same copy this one came from:
 *   - a resident one (Resident, or ROM: commands): by its name, which the new Shell finds in the resident
 *     list. Its seglist is shared; this run doesn't take the Shell's cli_Module, unload or free anything;
 *   - a file (SYS:Utilities, C: or anywhere): by its full path, from PROGDIR: and its name. The new Shell
 *     loads and unloads its own copy, and this Shell unloads this one as for any command.
 * 0 when it couldn't (no memory, no name): OpenMonitor then runs in this Shell, as before 0.3.2. */
static int detach(void)
{
    struct CommandLineInterface *cli = Cli();
    struct Segment *seg;
    char name[108], cmd[320];
    const char *base;
    BPTR in, out, dir;
    LONG rc;
    int resident;
    if (!cli || !GetProgramName((STRPTR)name, sizeof name)) return 0;
    base = (const char *)FilePart((STRPTR)name);
    Forbid();
    seg = FindSegment((STRPTR)base, NULL, FALSE);
    if (!seg || seg->seg_Seg != cli->cli_Module) seg = FindSegment((STRPTR)base, NULL, TRUE);
    resident = seg && seg->seg_Seg == cli->cli_Module;
    Permit();
    if (resident) snprintf(cmd, sizeof cmd, "\"%s\" WAIT", base);
    else {
        char path[256];
        if (!(dir = GetProgramDir()) || !NameFromLock(dir, (STRPTR)path, sizeof path) ||
            !AddPart((STRPTR)path, (STRPTR)base, sizeof path))
            return 0;
        snprintf(cmd, sizeof cmd, "\"%s\" WAIT", path);
    }
    in = Open((STRPTR)"NIL:", MODE_OLDFILE);
    out = Open((STRPTR)"NIL:", MODE_NEWFILE);
    if (!in || !out) { if (in) Close(in); if (out) Close(out); return 0; }
    /* No console task and no window of this Shell's. Given this Shell's console task, a new process whose
     * Shell had already ended (EndCLI just after it) never opened its window and didn't answer Ctrl-C
     * (lab t5-monitor, 16 October 2026); with none, it opens as when the Shell stays. */
    rc = SystemTags((STRPTR)cmd, SYS_Input, (ULONG)in, SYS_Output, (ULONG)out, SYS_Asynch, TRUE,
                    NP_StackSize, (ULONG)__stack, NP_ConsoleTask, (ULONG)NULL, NP_WindowPtr, (ULONG)NULL, TAG_DONE);
    if (rc == -1) { Close(in); Close(out); return 0; }   /* started, the new Shell closes them */
    return 1;
}

int main(void)
{
    struct MsgPort *fp;
    int other, rc;
    if (Cli()) {
        /* WAIT/S: stay in this Shell until the window closes (as before 0.3.2; the detached start uses it) */
        LONG arg[1] = { 0 };
        struct RDArgs *ra = ReadArgs((STRPTR)"WAIT/S", arg, NULL);
        if (!ra) { PrintFault(IoErr(), (STRPTR)"OpenMonitor"); return RETURN_ERROR; }
        FreeArgs(ra);
        if (!arg[0]) {
            if (send_to_front()) return RETURN_OK;
            if (detach()) return RETURN_OK;
        }
    }
    fp = claim_front(&other);
    if (other) return RETURN_OK;
    front_port = fp;
    rc = gui();
    front_port = NULL;
    release_front(fp);
    return rc;
}
