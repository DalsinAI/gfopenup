/* OpenMonitor: the Tasks view (the design, section 4.2).
 *
 * Every task and process, the held ones first (red), then by the column chosen (CPU by default): name, CPU in
 * the last second, CPU time since OpenMonitor started, priority, state, core, stack used and size, the signals
 * it waits for, owner and address. Address, Waits for and Core start hidden (the column chooser, a right-click
 * on the header, shows them; the user, 24 October 2026, 15:05).
 *
 * How busy a task is: AmigaOS keeps no time per task, so OpenMonitor looks 50 (or 60) times a second, in a
 * vertical blank interrupt server, at which task is running, and counts. A task waiting, the machine idle,
 * counts as idle. Exact time comes with OpenExec's accounting (the design's P4).
 *
 * The actions, each with its confirmation: Priority... (a window with the number, a scale and what it means),
 * Break (Ctrl-C), Signal... (Ctrl-C to Ctrl-F), Freeze and Resume, End program... (Ctrl-C, up to 3 seconds,
 * then Force quit behind one or two warnings: 0.3's), Details....
 *
 * Freeze leaves a waiting program where exec has it, in TaskWait, and makes it wait on nothing: its
 * tc_SigWait and tc_SigExcept are kept and set to 0, so no signal wakes it (they are still received, in
 * tc_SigRecvd). Resume gives them back and signals it with what it received meanwhile, so it wakes as it would
 * have. No list of exec's is changed (taking the task out of TaskWait into a list of our own hung the lab at
 * the resume, 24 October 2026). A program that is running or ready just then isn't frozen: it is tried again a
 * few times, then the user is told. Never for a plain task, a file system, a system process or OpenMonitor
 * itself; every frozen task is resumed when OpenMonitor quits.
 *
 * A task the task guard holds (amigachrome docs/architecture/TASK_GUARD.md): it waits on no signals and its
 * stack carries 'TGRD' 56 bytes below tc_SPUpper: the vector, the PC, the address. Shown as "held", in red.
 *
 * MIT, Copyright (c) 2026 AmigaChrome Team. */

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/interrupts.h>
#include <exec/tasks.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <hardware/intbits.h>
#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <devices/inputevent.h>
#include <libraries/gadtools.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "om.h"

/* ---- the sampler -------------------------------------------------------------------------------------- */

#define MAXT 224
struct samp {
    struct Task *task[MAXT];
    ULONG hits[MAXT];
    ULONG n, idle, total;
};
static struct samp S;
static struct Interrupt vbi;
static int vbi_on;

/* 50 times a second, in the interrupt: which task is running (0 in d0: the next server runs too). exec's
 * base from address 4: built resident, the variables are reached through a4, which holds nothing of ours
 * here. */
static ULONG vb_server(REG(a1, struct samp *s))
{
    struct ExecBase *eb;
    struct Task *t;
    ULONG i;
    __asm volatile ("move.l 4.w,%0" : "=a" (eb));
    t = eb->ThisTask;
    s->total++;
    if (!t || t->tc_State != TS_RUN) { s->idle++; return 0; }
    for (i = 0; i < s->n; i++)
        if (s->task[i] == t) { s->hits[i]++; return 0; }
    if (s->n < MAXT) { s->task[s->n] = t; s->hits[s->n] = 1; s->n++; }
    return 0;
}

/* ---- the tasks ---------------------------------------------------------------------------------------- */

enum { OWN_PROGRAM, OWN_SHELL, OWN_FS, OWN_SYSTEM };
static const char *const own_names[] = { "Workbench", "ramlib", "IPrefs", "ConClip", "input.device", "OpenBar",
                                         "OpenInput", "OpenRTG cards", "OpenUSB bus", "exec boot", NULL };
enum { ST_ADDED = 1, ST_RUN, ST_READY, ST_WAIT, ST_EXCEPT, ST_REMOVED, ST_HELD = 10, ST_FROZEN };
static const char *const state_names[] = { "invalid", "added", "running", "ready", "waiting", "exception", "removed",
                                           "?", "?", "?", "held", "frozen" };

struct trow {
    struct Task *task;
    STRPTR lnname;
    APTR splower;
    char name[40];
    char cmd[80];                       /* a Shell's command as it was typed; a file system's device */
    LONG pri, shell;
    UBYTE state, process, kind, guard;  /* guard: the task guard's vector (3, 4, 10), 0 when not held */
    ULONG guard_pc, guard_addr;
    ULONG stack, used, sigalloc, sigwait, sigrecvd;
    ULONG tenths;                       /* CPU in the last second, tenths of a per cent */
    ULONG ms;                           /* CPU time since OpenMonitor started, in milliseconds */
};

/* allocated at the start: built resident, the variables (a4's 64 KB) can't hold them */
static struct trow *rows;
static int nrows;
static int *order, norder;              /* the rows shown: filtered and sorted */
static ULONG cpu_tenths, switches, disp_before;

/* CPU time since the start, kept by task (its address, its name's pointer and its stack: exec reuses a freed
 * task's memory, so the address alone could be a newer task) */
#define MAXACC 256
struct acc { struct Task *t; STRPTR lnname; APTR spl; ULONG hits; UBYTE seen; };
static struct acc *accs;
static int naccs;

/* the frozen ones: in TaskWait, waiting on nothing; what they waited for is kept here */
#define MAXFRZ 16
struct frz { struct Task *t; STRPTR lnname; APTR spl; ULONG sigwait, sigexcept; };
static struct frz frzs[MAXFRZ];
static int nfrz;

#define MAXFS 64
static struct MsgPort *fs_port[MAXFS];
static char fs_name[MAXFS][24];
static int nfs;

static int find_frz(struct Task *t);

/* The file systems' ports, with their devices' names. Not under Disable: LockDosList may wait. */
static void take_handlers(void)
{
    struct DosList *dl = LockDosList(LDF_DEVICES | LDF_READ);
    nfs = 0;
    while ((dl = NextDosEntry(dl, LDF_DEVICES)) && nfs < MAXFS)
        if (dl->dol_Task) {
            UBYTE *b = (UBYTE *)BADDR(dl->dol_Name);
            int len = b ? b[0] : 0;
            if (len > 22) len = 22;
            fs_port[nfs] = dl->dol_Task;
            if (b) memcpy(fs_name[nfs], b + 1, len);
            fs_name[nfs][len] = 0;
            nfs++;
        }
    UnLockDosList(LDF_DEVICES | LDF_READ);
}

/* Under Disable or Forbid. */
static int task_kind(struct Task *t, LONG *shell, const char **fs)
{
    int i;
    *shell = 0;
    if (fs) *fs = NULL;
    if (t->tc_Node.ln_Type != NT_PROCESS) return OWN_SYSTEM;
    if (((struct Process *)t)->pr_CLI) { *shell = ((struct Process *)t)->pr_TaskNum; return OWN_SHELL; }
    for (i = 0; i < nfs; i++) if (fs_port[i] == &((struct Process *)t)->pr_MsgPort) { if (fs) *fs = fs_name[i]; return OWN_FS; }
    /* a console window's handler (each CON: or RAW: window has one): a handler like the file systems */
    if (t->tc_Node.ln_Name && (!strcmp(t->tc_Node.ln_Name, "CON") || !strcmp(t->tc_Node.ln_Name, "RAW"))) {
        if (fs) *fs = t->tc_Node.ln_Name;
        return OWN_FS;
    }
    for (i = 0; own_names[i]; i++) if (t->tc_Node.ln_Name && !strcmp(t->tc_Node.ln_Name, own_names[i])) return OWN_SYSTEM;
    return OWN_PROGRAM;
}

static void owner_text(int kind, LONG shell, char *out, int size)
{
    if (kind == OWN_SHELL) snprintf(out, size, "Shell %ld", (long)shell);
    else snprintf(out, size, "%s", kind == OWN_FS ? "file system" : kind == OWN_SYSTEM ? "system" : "program");
}

/* Under Disable: the name (a Shell's command by its file part) and the whole command. */
static void task_name(struct Task *t, char *out, int size, char *cmd, int csize)
{
    const char *n = t->tc_Node.ln_Name ? t->tc_Node.ln_Name : "(no name)";
    out[0] = 0;
    cmd[0] = 0;
    if (t->tc_Node.ln_Type == NT_PROCESS) {
        struct Process *p = (struct Process *)t;
        struct CommandLineInterface *cli = p->pr_CLI ? (struct CommandLineInterface *)BADDR(p->pr_CLI) : NULL;
        if (cli && cli->cli_Module && cli->cli_CommandName) {
            UBYTE *b = (UBYTE *)BADDR(cli->cli_CommandName);
            int len = b ? b[0] : 0, s = 0, i, k;
            for (i = 0; i < len; i++) if (b[1 + i] == '/' || b[1 + i] == ':') s = i + 1;
            k = len < csize - 1 ? len : csize - 1;
            if (k > 0) { memcpy(cmd, b + 1, k); cmd[k] = 0; }
            if (len - s > 0) {
                k = len - s < size - 1 ? len - s : size - 1;
                memcpy(out, b + 1 + s, k);
                out[k] = 0;
                return;
            }
        }
    }
    strncpy(out, n, size - 1);
    out[size - 1] = 0;
}

/* Under Disable. */
static void add_task(struct Task *t)
{
    struct trow *r;
    ULONG *rec;
    const char *fs;
    int frozen;
    if (!t || nrows >= MAXT) return;
    frozen = find_frz(t) >= 0;
    r = &rows[nrows++];
    memset(r, 0, sizeof *r);
    r->task = t;
    r->lnname = (STRPTR)t->tc_Node.ln_Name;
    r->splower = t->tc_SPLower;
    task_name(t, r->name, sizeof r->name, r->cmd, sizeof r->cmd);
    r->pri = t->tc_Node.ln_Pri;
    r->stack = (ULONG)t->tc_SPUpper - (ULONG)t->tc_SPLower;
    if (t == SysBase->ThisTask) {
        /* running: its stack pointer is in the CPU, not in tc_SPReg (it is OpenMonitor itself, here) */
        ULONG sp;
        __asm volatile ("move.l %%sp,%0" : "=r" (sp));
        r->used = sp <= (ULONG)t->tc_SPUpper && sp >= (ULONG)t->tc_SPLower ? (ULONG)t->tc_SPUpper - sp : 0;
    } else
        r->used = (ULONG)t->tc_SPReg <= (ULONG)t->tc_SPUpper && (ULONG)t->tc_SPReg >= (ULONG)t->tc_SPLower ?
                  (ULONG)t->tc_SPUpper - (ULONG)t->tc_SPReg : 0;
    r->sigalloc = t->tc_SigAlloc; r->sigwait = t->tc_SigWait; r->sigrecvd = t->tc_SigRecvd;
    r->process = t->tc_Node.ln_Type == NT_PROCESS;
    r->kind = task_kind(t, &r->shell, &fs);
    if (r->kind == OWN_FS && fs && !r->cmd[0]) snprintf(r->cmd, sizeof r->cmd, "%s:", fs);
    r->state = frozen ? ST_FROZEN : t->tc_State;
    /* the task guard's record: waiting on nothing, 'TGRD' inside its own stack */
    rec = (ULONG *)(((ULONG)t->tc_SPUpper - 56) & ~3UL);
    if (!frozen && t->tc_State == TS_WAIT && t->tc_SigWait == 0 && (ULONG)rec >= (ULONG)t->tc_SPLower &&
        (ULONG)rec + 16 <= (ULONG)t->tc_SPUpper && rec[0] == 0x54475244UL && rec[1] && rec[1] < 64) {
        r->state = ST_HELD;
        r->guard = rec[1];
        r->guard_pc = rec[2];
        r->guard_addr = rec[3];
    }
}

static struct acc *acc_find(struct Task *t, STRPTR lnname, APTR spl, int make)
{
    int i;
    for (i = 0; i < naccs; i++) if (accs[i].t == t && accs[i].lnname == lnname && accs[i].spl == spl) return &accs[i];
    if (!make) return NULL;
    if (naccs >= MAXACC) {                      /* full: the ones not seen this time go */
        int j = 0;
        for (i = 0; i < naccs; i++) if (accs[i].seen) accs[j++] = accs[i];
        naccs = j;
        if (naccs >= MAXACC) return NULL;
    }
    memset(&accs[naccs], 0, sizeof accs[0]);
    accs[naccs].t = t; accs[naccs].lnname = lnname; accs[naccs].spl = spl;
    return &accs[naccs++];
}

/* ---- the view's state --------------------------------------------------------------------------------- */

enum { C_NAME, C_CPU, C_TIME, C_PRI, C_STATE, C_CORE, C_STACK, C_WAITS, C_OWNER, C_ADDR, C_N };
static struct om_col cols[C_N] = {
    { "Task or program", 24, OM_LEFT, 0, 1, 0 },
    { "CPU", 6, OM_RIGHT, 0, 0, 1 },
    { "CPU time", 8, OM_RIGHT, 0, 0, 1 },
    { "Pri", 3, OM_RIGHT, 0, 0, 1 },
    { "State", 9, OM_LEFT, 0, 0, 0 },
    { "Core", 4, OM_RIGHT, 1, 0, 0 },
    { "Stack used / size", 15, OM_RIGHT, 0, 0, 1 },
    { "Waits for", 8, OM_LEFT, 1, 0, 0 },
    { "Owner", 11, OM_LEFT, 0, 0, 0 },
    { "Address", 8, OM_LEFT, 1, 0, 0 },
};
static struct om_table T;
static int show_mode;                   /* 0 every task, 1 programs, 2 the system */
static char find[32];
static struct Task *chosen;
static STRPTR chosen_lnname;
static APTR chosen_spl;
static char chosen_name[40];

enum { G_SHOW, G_FIND, G_PRI, G_BREAK, G_SIGNAL, G_FREEZE, G_END, G_DETAILS, G_COUNT };
static struct Gadget *gad[G_COUNT];
static int gx[G_COUNT], gy[G_COUNT], gw[G_COUNT], gh_;
static int graph_x, graph_y, graph_w, graph_h, filter_y, det_x, det_y, det_w, det_h;
static const char *show_labels[] = { "Every task", "Programs", "The system", NULL };

/* ---- sorting and filtering ---------------------------------------------------------------------------- */

static int ci_contains(const char *hay, const char *needle)
{
    int n = strlen(needle), i, j;
    if (!n) return 1;
    for (i = 0; hay[i]; i++) {
        for (j = 0; j < n && hay[i + j]; j++) {
            char a = hay[i + j], b = needle[j];
            if (a >= 'A' && a <= 'Z') a += 32;
            if (b >= 'A' && b <= 'Z') b += 32;
            if (a != b) break;
        }
        if (j == n) return 1;
    }
    return 0;
}

static int ci_cmp(const char *a, const char *b)
{
    for (; *a && *b; a++, b++) {
        char x = *a, y = *b;
        if (x >= 'A' && x <= 'Z') x += 32;
        if (y >= 'A' && y <= 'Z') y += 32;
        if (x != y) return x < y ? -1 : 1;
    }
    return *a ? 1 : *b ? -1 : 0;
}

static int state_rank(int s)
{
    switch (s) {
    case ST_RUN: return 0;
    case ST_READY: return 1;
    case ST_WAIT: return 2;
    case ST_FROZEN: return 3;
    default: return 4;
    }
}

static int by_column(const void *pa, const void *pb)
{
    const struct trow *a = &rows[*(const int *)pa], *b = &rows[*(const int *)pb];
    int c = 0;
    char oa[24], ob[24];
    if ((a->state == ST_HELD) != (b->state == ST_HELD)) return a->state == ST_HELD ? -1 : 1;
    switch (T.sort) {
    case C_CPU: c = a->tenths < b->tenths ? -1 : a->tenths > b->tenths; break;
    case C_TIME: c = a->ms < b->ms ? -1 : a->ms > b->ms; break;
    case C_PRI: c = a->pri < b->pri ? -1 : a->pri > b->pri; break;
    case C_STATE: c = state_rank(a->state) - state_rank(b->state); break;
    case C_STACK: c = a->used < b->used ? -1 : a->used > b->used; break;
    case C_WAITS: c = a->sigwait < b->sigwait ? -1 : a->sigwait > b->sigwait; break;
    case C_OWNER: owner_text(a->kind, a->shell, oa, sizeof oa); owner_text(b->kind, b->shell, ob, sizeof ob); c = ci_cmp(oa, ob); break;
    case C_ADDR: c = (ULONG)a->task < (ULONG)b->task ? -1 : (ULONG)a->task > (ULONG)b->task; break;
    default: break;
    }
    if (T.desc) c = -c;
    if (!c) c = ci_cmp(a->name, b->name);
    if (!c && a->task != b->task) c = (ULONG)a->task < (ULONG)b->task ? -1 : 1;   /* 0 for itself: qsort compares a row with itself */
    return c;
}

static void make_order(void)
{
    int i;
    norder = 0;
    for (i = 0; i < nrows; i++) {
        struct trow *r = &rows[i];
        if (show_mode == 1 && r->kind != OWN_PROGRAM && r->kind != OWN_SHELL) continue;
        if (show_mode == 2 && r->kind != OWN_SYSTEM && r->kind != OWN_FS) continue;
        if (find[0] && !ci_contains(r->name, find) && !ci_contains(r->cmd, find)) continue;
        order[norder++] = i;
    }
    qsort(order, norder, sizeof order[0], by_column);
}

static struct trow *chosen_row(void)
{
    int i;
    if (!chosen) return NULL;
    for (i = 0; i < nrows; i++)
        if (rows[i].task == chosen && rows[i].lnname == chosen_lnname && rows[i].splower == chosen_spl) return &rows[i];
    return NULL;
}

static void choose_row(int shown)
{
    struct trow *r;
    if (shown < 0 || shown >= norder) { chosen = NULL; return; }
    r = &rows[order[shown]];
    chosen = r->task; chosen_lnname = r->lnname; chosen_spl = r->splower;
    strcpy(chosen_name, r->name);
}

/* ---- the second's look -------------------------------------------------------------------------------- */

struct prev { struct Task *t; STRPTR lnname; APTR spl; ULONG tenths; };
static struct prev *prev;

/* full: the second is up (the sampler's counts are taken and the CPU graph moves on); else only the list is
 * read again after an action, each task keeping the CPU it had */
static void take(int full)
{
    static struct samp got;
    struct Node *n;
    int i, j, nprev = 0;
    ULONG disp, hz = SysBase->VBlankFrequency ? SysBase->VBlankFrequency : 50;
    if (!full)
        for (i = 0; i < nrows; i++) {
            prev[nprev].t = rows[i].task; prev[nprev].lnname = rows[i].lnname; prev[nprev].spl = rows[i].splower;
            prev[nprev++].tenths = rows[i].tenths;
        }
    nrows = 0;
    take_handlers();
    Disable();
    if (full) {
        got = S;
        S.n = 0; S.idle = 0; S.total = 0;
    }
    add_task(SysBase->ThisTask);
    for (n = SysBase->TaskReady.lh_Head; n->ln_Succ; n = n->ln_Succ) add_task((struct Task *)n);
    for (n = SysBase->TaskWait.lh_Head; n->ln_Succ; n = n->ln_Succ) add_task((struct Task *)n);
    disp = SysBase->DispCount;
    Enable();
    if (full) {
        switches = disp - disp_before;
        disp_before = disp;
        cpu_tenths = got.total ? (got.total - got.idle) * 1000 / got.total : 0;
        for (i = 0; i < naccs; i++) accs[i].seen = 0;
    }
    for (i = 0; i < nrows; i++) {
        struct trow *r = &rows[i];
        struct acc *a = acc_find(r->task, r->lnname, r->splower, full);
        if (full) {
            for (j = 0; j < (int)got.n; j++)
                if (got.task[j] == r->task) {
                    r->tenths = got.total ? got.hits[j] * 1000 / got.total : 0;
                    if (a) a->hits += got.hits[j];
                    break;
                }
            if (a) a->seen = 1;
        } else {
            for (j = 0; j < nprev; j++)
                if (prev[j].t == r->task && prev[j].lnname == r->lnname && prev[j].spl == r->splower) { r->tenths = prev[j].tenths; break; }
        }
        if (a) r->ms = a->hits * 1000 / hz;
    }
    if (full) om_graphs_push_cpu(cpu_tenths, switches, 0);
    make_order();
    if (om_first_task[0]) {                     /* TASK/K: chosen at the start */
        for (i = 0; i < norder; i++)
            if (!ci_cmp(rows[order[i]].name, om_first_task)) { choose_row(i); break; }
        om_first_task[0] = 0;
    }
}

void om_tasks_take(void) { take(1); }

/* ---- the table's cells -------------------------------------------------------------------------------- */

static void time_text(ULONG ms, char *buf, int size)
{
    snprintf(buf, size, "%lu:%02lu.%02lu", (unsigned long)(ms / 60000), (unsigned long)(ms / 1000 % 60), (unsigned long)(ms % 1000 / 10));
}

static void cell(int row, int col, char *buf, int size, LONG *pen)
{
    struct trow *r;
    if (row < 0 || row >= norder) return;
    r = &rows[order[row]];
    switch (col) {
    case C_NAME: snprintf(buf, size, "%s", r->name); if (r->state == ST_HELD) *pen = U.pen_danger; break;
    case C_CPU: snprintf(buf, size, "%lu.%lu%%", (unsigned long)r->tenths / 10, (unsigned long)r->tenths % 10); break;
    case C_TIME: time_text(r->ms, buf, size); break;
    case C_PRI: snprintf(buf, size, "%ld", (long)r->pri); break;
    case C_STATE:
        snprintf(buf, size, "%s", r->state < sizeof state_names / sizeof state_names[0] ? state_names[r->state] : "?");
        if (r->state == ST_HELD) *pen = U.pen_danger;
        else if (r->state == ST_FROZEN) *pen = U.pen_amber;
        else if (r->state == ST_RUN) *pen = U.pen_accent;
        break;
    case C_CORE: snprintf(buf, size, "0"); break;
    case C_STACK:
        if (r->stack % 1024 == 0 || r->stack > 9999) snprintf(buf, size, "%lu / %luK", (unsigned long)r->used, (unsigned long)r->stack / 1024);
        else snprintf(buf, size, "%lu / %lu", (unsigned long)r->used, (unsigned long)r->stack);
        break;
    case C_WAITS: snprintf(buf, size, "%08lx", (unsigned long)r->sigwait); break;
    case C_OWNER: owner_text(r->kind, r->shell, buf, size); if (r->kind == OWN_SYSTEM || r->kind == OWN_FS) *pen = U.pen_muted; break;
    case C_ADDR: snprintf(buf, size, "%08lx", (unsigned long)r->task); *pen = U.pen_muted; break;
    }
}

/* ---- the details strip -------------------------------------------------------------------------------- */

/* 1048576 as "1,048,576" */
static const char *commas(ULONG v, char *buf)
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

static const char *guard_why(int v)
{
    return v == 3 ? "an address error (a jump to an odd address)" : v == 4 ? "an illegal instruction" : v == 10 ? "an unused line-A word" : "a crash";
}

static void describe(struct trow *r, char *out, int size)
{
    char who[112];
    if (r->kind == OWN_SHELL) snprintf(who, sizeof who, "started from Shell %ld", (long)r->shell);
    else if (r->kind == OWN_FS && (!strcmp(r->cmd, "CON:") || !strcmp(r->cmd, "RAW:"))) snprintf(who, sizeof who, "the handler of a %s window", r->cmd);
    else if (r->kind == OWN_FS) snprintf(who, sizeof who, "the file system of %s", r->cmd[0] ? r->cmd : "a DOS device");
    else if (r->kind == OWN_SYSTEM) snprintf(who, sizeof who, "part of the system");
    else snprintf(who, sizeof who, "started by Workbench or another program");
    snprintf(out, size, "a %s %s, priority %ld%s", r->process ? "process" : "task", who, (long)r->pri,
             r->state == ST_FROZEN ? ", frozen by OpenMonitor" : "");
}

static void draw_details(void)
{
    struct RastPort *rp = U.win->RPort;
    struct trow *r = chosen_row();
    char s[200], d[120];
    if (det_h <= 0) return;
    om_sunk(det_x, det_y, det_w, det_h);
    if (!r) {
        SetFont(rp, U.font);
        ogt_text(rp, U.pen_muted, det_x + 8, det_y + 4, "Choose a task: its details show here. Right-click it for more.", det_w - 16);
        return;
    }
    describe(r, d, sizeof d);
    if (U.compact) {
        snprintf(s, sizeof s, "%s: %s", r->name, d);
        SetFont(rp, U.tfont);
        ogt_text(rp, U.pen_text, det_x + 4, det_y + 2, s, det_w - 8);
        return;
    }
    SetFont(rp, U.font);
    ogt_bold(rp, 1);
    {
        int nw = ogt_text(rp, r->state == ST_HELD ? U.pen_danger : U.pen_text, det_x + 8, det_y + 4, r->name, det_w / 2);
        ogt_bold(rp, 0);
        ogt_text(rp, U.pen_muted, det_x + 8 + nw + 10, det_y + 4, d, det_w - nw - 26);
    }
    if (r->state == ST_HELD)
        snprintf(s, sizeof s, "held by the task guard: %s at PC $%08lx, $%08lx", guard_why(r->guard),
                 (unsigned long)r->guard_pc, (unsigned long)r->guard_addr);
    else {
        char t[16], a[16], b[16];
        time_text(r->ms, t, sizeof t);
        snprintf(s, sizeof s, "signals: allocated %08lx  waits for %08lx  received %08lx    stack %s of %s bytes    CPU time %s%s%s",
                 (unsigned long)r->sigalloc, (unsigned long)r->sigwait, (unsigned long)r->sigrecvd, commas(r->used, a),
                 commas(r->stack, b), t, r->cmd[0] && r->kind == OWN_SHELL ? "    " : "", r->kind == OWN_SHELL ? r->cmd : "");
    }
    SetFont(rp, U.tfont);
    ogt_text(rp, r->state == ST_HELD ? U.pen_danger : U.pen_text, det_x + 8, det_y + 4 + U.fh + 4, s, det_w - 16);
}

/* ---- the actions -------------------------------------------------------------------------------------- */

/* The chosen task, if it is still the same one: 1 when found (call under Forbid, so it stays while used). */
static int task_alive(struct Task *t)
{
    struct Node *n;
    if (t == SysBase->ThisTask) return 1;
    for (n = SysBase->TaskReady.lh_Head; n->ln_Succ; n = n->ln_Succ) if ((struct Task *)n == t) return 1;
    for (n = SysBase->TaskWait.lh_Head; n->ln_Succ; n = n->ln_Succ) if ((struct Task *)n == t) return 1;
    return 0;
}

static int same_task(struct Task *t, STRPTR lnname, APTR spl)
{
    return task_alive(t) && (STRPTR)t->tc_Node.ln_Name == lnname && t->tc_SPLower == spl;
}

static int chosen_ok(int not_self)
{
    if (!chosen) { om_status(0, "Choose a task in the list first."); return 0; }
    if (not_self && chosen == FindTask(NULL)) { om_status(0, "That is OpenMonitor: close its window to end it."); return 0; }
    return 1;
}

/* The list as it is now: the count, the chosen row kept chosen (it may have moved), the rows, the details. */
static void redraw_rows(void)
{
    int i, sel = -1;
    if (U.view != V_TASKS || !T.list) return;
    om_table_set_count(&T, norder);
    for (i = 0; i < norder; i++)
        if (rows[order[i]].task == chosen && rows[order[i]].lnname == chosen_lnname && rows[order[i]].splower == chosen_spl) { sel = i; break; }
    T.list->selected = sel;
    if (sel < 0 && chosen && !chosen_row()) chosen = NULL;
    om_table_draw_rows(&T);
    draw_details();
}

/* After an action: the list read again at once. */
static void refresh(void)
{
    take(0);
    redraw_rows();
}

static void change_pri(LONG now)
{
    if (!chosen_ok(0)) return;
    if (now > 20) now = 20;
    if (now < -20) now = -20;
    Forbid();
    if (!same_task(chosen, chosen_lnname, chosen_spl)) { Permit(); chosen = NULL; om_status(0, "That task has ended."); return; }
    SetTaskPri(chosen, now);
    Permit();
    om_status(0, "%s: priority now %ld.", chosen_name, (long)now);
    refresh();
}

static void send_signal(ULONG sig, const char *what)
{
    if (!chosen_ok(0)) return;
    Forbid();
    if (!same_task(chosen, chosen_lnname, chosen_spl)) { Permit(); chosen = NULL; om_status(0, "That task has ended."); return; }
    Signal(chosen, sig);
    Permit();
    om_status(0, "Sent %s to %s.", what, chosen_name);
}

/* -- the priority window -- */

static void pri_texts(struct Window *w, struct trow *r, const char *d, const char *line, int sysk, int ix,
                      struct Gadget *num, struct Gadget *sld, int W)
{
    static const char *const scale[] = { "Background", "Normal", "High", "Highest" };
    static const int scale_at[] = { -20, 0, 5, 20 };
    struct RastPort *rp = w->RPort;
    int y = U.scr->WBorTop + U.scr->Font->ta_YSize + 1 + 12, nw, k;
    SetFont(rp, U.font);
    ogt_bold(rp, 1);
    nw = ogt_text(rp, sysk ? U.pen_danger : U.pen_text, ix, y, r->name, W / 2);
    ogt_bold(rp, 0);
    ogt_text(rp, U.pen_muted, ix + nw + 8, y, d, W - nw - 40);
    ogt_text(rp, U.pen_text, ix, num->TopEdge + (num->Height - U.fh) / 2, line, 0);
    y = sld->TopEdge + sld->Height + 6;
    for (k = 0; k < 4; k++) {
        int sx = sld->LeftEdge + (sld->Width - 1) * (scale_at[k] + 20) / 40, tw = om_text_width(U.font, scale[k]);
        int tx = sx - tw / 2;
        if (tx < ix) tx = ix;
        if (tx + tw > ix + W - 32) tx = ix + W - 32 - tw;
        ogt_text(rp, U.pen_muted, tx, y, scale[k], 0);
    }
    y += U.fh + 12;
    ogt_text(rp, U.pen_text, ix, y, sysk ? "This is part of the system: changing it can slow or stop the Amiga." :
             "Above 0 it runs before the programs you use; input.device (20)", W - 32);
    ogt_text(rp, U.pen_text, ix, y + U.fh + 3, sysk ? "Its makers chose its priority. Change it only to find a fault." :
             "and the file systems (5 to 10) keep theirs. A busy program above 0", W - 32);
    if (!sysk) ogt_text(rp, U.pen_text, ix, y + 2 * (U.fh + 3), "can slow everything else; below 0 it waits for them.", W - 32);
}

static int ask_priority(LONG want)
{
    struct trow *r = chosen_row();
    struct Gadget *glist = NULL, *g, *num, *sld, *chk = NULL;
    struct NewGadget ng;
    struct Window *w;
    LONG now, v = want;
    int done = 0, set = 0, lh = U.fh + 6, W = 520, H, sysk, ix, iy;
    char title[64], d[120], line[80];
    if (!r) return 0;
    now = r->pri;
    sysk = r->kind == OWN_SYSTEM || r->kind == OWN_FS;
    describe(r, d, sizeof d);
    snprintf(title, sizeof title, "Priority of %s", r->name);
    ix = U.scr->WBorLeft + 16;
    iy = U.scr->WBorTop + U.scr->Font->ta_YSize + 1 + 12;
    H = 12 + U.fh + 12 + lh + 10 + lh + U.fh + 14 + 3 * (U.fh + 3) + 10 + (sysk ? 0 : lh + 8) + lh + 14;
    g = CreateContext(&glist);
    memset(&ng, 0, sizeof ng);
    ng.ng_TextAttr = &U.ta; ng.ng_VisualInfo = U.vi;
    snprintf(line, sizeof line, "Priority now: %ld.   New priority:", (long)now);
    ng.ng_LeftEdge = ix + om_text_width(U.font, line) + 10; ng.ng_TopEdge = iy + U.fh + 12;
    ng.ng_Width = 60; ng.ng_Height = lh; ng.ng_GadgetID = 1;
    g = num = CreateGadget(INTEGER_KIND, g, &ng, GTIN_Number, (ULONG)v, GTIN_MaxChars, 4, TAG_DONE);
    ng.ng_LeftEdge = ix + 24; ng.ng_TopEdge += lh + 10; ng.ng_Width = W - 48 - 8; ng.ng_Height = lh - 4; ng.ng_GadgetID = 2;
    g = sld = CreateGadget(SLIDER_KIND, g, &ng, GTSL_Min, (ULONG)-20, GTSL_Max, 20, GTSL_Level, (ULONG)v,
                           GA_RelVerify, TRUE, PGA_Freedom, LORIENT_HORIZ, TAG_DONE);
    ng.ng_TopEdge += lh + U.fh + 14 + 3 * (U.fh + 3) + 10;
    if (!sysk) {
        ng.ng_LeftEdge = ix; ng.ng_Width = 26; ng.ng_Height = lh; ng.ng_GadgetID = 3;
        ng.ng_GadgetText = (STRPTR)"Don't ask again for priorities up to 5"; ng.ng_Flags = PLACETEXT_RIGHT;
        g = chk = CreateGadget(CHECKBOX_KIND, g, &ng, GTCB_Checked, (ULONG)(om_pref("Tasks.AskPriority") && !strcmp(om_pref("Tasks.AskPriority"), "no")),
                               GTCB_Scaled, TRUE, TAG_DONE);
        ng.ng_TopEdge += lh + 8;
    }
    ng.ng_LeftEdge = ix; ng.ng_Width = 150; ng.ng_Height = lh; ng.ng_GadgetID = 4; ng.ng_Flags = PLACETEXT_IN;
    ng.ng_GadgetText = (STRPTR)"Set priority";
    g = CreateGadget(BUTTON_KIND, g, &ng, TAG_DONE);
    ng.ng_LeftEdge = ix + W - 32 - 110; ng.ng_Width = 110; ng.ng_GadgetID = 5; ng.ng_GadgetText = (STRPTR)"Cancel";
    g = CreateGadget(BUTTON_KIND, g, &ng, TAG_DONE);
    w = g ? OpenWindowTags(NULL, WA_Title, (ULONG)title, WA_PubScreen, (ULONG)U.scr,
                           WA_Left, U.win->LeftEdge + (U.win->Width - W) / 2, WA_Top, U.win->TopEdge + U.win->Height / 4,
                           WA_InnerWidth, W, WA_InnerHeight, H, WA_Gadgets, (ULONG)glist,
                           WA_DragBar, TRUE, WA_DepthGadget, TRUE, WA_CloseGadget, TRUE, WA_Activate, TRUE, WA_SmartRefresh, TRUE,
                           WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW | IDCMP_GADGETUP | IDCMP_VANILLAKEY |
                                     INTEGERIDCMP | SLIDERIDCMP | CHECKBOXIDCMP | BUTTONIDCMP, TAG_DONE) : NULL;
    if (!w) { FreeGadgets(glist); om_status(0, "The priority window could not open."); return 0; }
    GT_RefreshWindow(w, NULL);
    pri_texts(w, r, d, line, sysk, ix, num, sld, W);
    ActivateGadget(num, w, NULL);
    while (!done) {
        struct IntuiMessage *m;
        ULONG tsig = om_timer_sig();
        if (Wait((1UL << w->UserPort->mp_SigBit) | tsig) & tsig) om_timer_due();
        while ((m = GT_GetIMsg(w->UserPort))) {
            ULONG cls = m->Class;
            UWORD code = m->Code;
            struct Gadget *gg = (struct Gadget *)m->IAddress;
            GT_ReplyIMsg(m);
            if (cls == IDCMP_CLOSEWINDOW || (cls == IDCMP_VANILLAKEY && code == 27)) done = 1;
            else if (cls == IDCMP_REFRESHWINDOW) {
                GT_BeginRefresh(w);
                pri_texts(w, r, d, line, sysk, ix, num, sld, W);
                GT_EndRefresh(w, TRUE);
            } else if (cls == IDCMP_MOUSEMOVE && gg == sld) {
                v = (WORD)code;
                GT_SetGadgetAttrs(num, w, NULL, GTIN_Number, (ULONG)v, TAG_DONE);
            } else if (cls == IDCMP_GADGETUP) {
                if (gg->GadgetID == 2) { v = (WORD)code; GT_SetGadgetAttrs(num, w, NULL, GTIN_Number, (ULONG)v, TAG_DONE); }
                else if (gg->GadgetID == 1) {
                    GT_GetGadgetAttrs(num, w, NULL, GTIN_Number, (ULONG)&v, TAG_DONE);
                    if (v > 20) v = 20;
                    if (v < -20) v = -20;
                    GT_SetGadgetAttrs(sld, w, NULL, GTSL_Level, (ULONG)v, TAG_DONE);
                    if (code != 9) done = set = 1;  /* Return in the number sets it; Tab only moves on */
                } else if (gg->GadgetID == 4) {
                    GT_GetGadgetAttrs(num, w, NULL, GTIN_Number, (ULONG)&v, TAG_DONE);
                    done = set = 1;
                } else if (gg->GadgetID == 5) done = 1;
            } else if (cls == IDCMP_VANILLAKEY && code == 13) {
                GT_GetGadgetAttrs(num, w, NULL, GTIN_Number, (ULONG)&v, TAG_DONE);
                done = set = 1;
            }
        }
    }
    if (chk) {
        ULONG on = 0;
        GT_GetGadgetAttrs(chk, w, NULL, GTCB_Checked, (ULONG)&on, TAG_DONE);
        om_set_pref("Tasks.AskPriority", on ? "no" : "yes");
    }
    CloseWindow(w);
    FreeGadgets(glist);
    ActivateWindow(U.win);
    if (set) change_pri(v);
    return set;
}

/* A priority from the menu: straight away when it's a program's, up to 5, and "don't ask" was chosen; else
 * the window, with the number in it. */
static void priority_to(LONG v)
{
    struct trow *r = chosen_row();
    const char *ask = om_pref("Tasks.AskPriority");
    if (!r) { chosen_ok(0); return; }
    if (v <= 5 && r->kind != OWN_SYSTEM && r->kind != OWN_FS && ask && !strcmp(ask, "no")) change_pri(v);
    else ask_priority(v);
}

/* -- freeze and resume -- */

static int find_frz(struct Task *t)
{
    int k;
    for (k = 0; k < nfrz; k++)
        if (frzs[k].t == t && (STRPTR)t->tc_Node.ln_Name == frzs[k].lnname && t->tc_SPLower == frzs[k].spl) return k;
    return -1;
}

/* What it waited for given back; then, if a signal it waits for came meanwhile, that signal again (a
 * Signal of bits it already has changes nothing but wakes it, as the signal would have). */
static void resume_k(int k)
{
    struct Task *t = frzs[k].t;
    ULONG pending = 0;
    int there;
    Forbid();
    there = task_alive(t) && (STRPTR)t->tc_Node.ln_Name == frzs[k].lnname && t->tc_SPLower == frzs[k].spl;
    if (there) {
        Disable();
        t->tc_SigWait = frzs[k].sigwait;
        t->tc_SigExcept = frzs[k].sigexcept;
        pending = t->tc_SigRecvd & (t->tc_SigWait | t->tc_SigExcept);
        Enable();
    }
    frzs[k] = frzs[--nfrz];
    if (there && pending) Signal(t, pending);
    Permit();
}

static void freeze_chosen(void)
{
    struct trow *r = chosen_row();
    int k;
    LONG yes;
    if (!chosen_ok(1) || !r) return;
    if ((k = find_frz(chosen)) >= 0) {
        resume_k(k);
        om_status(0, "%s runs again.", chosen_name);
        refresh();
        om_rebuild_gadgets();
        return;
    }
    if (r->state == ST_HELD) { om_status(0, "%s is held by the task guard already.", r->name); return; }
    if (!r->process || r->kind == OWN_SYSTEM || r->kind == OWN_FS) {
        om_status(0, "%s is part of the system: OpenMonitor doesn't freeze it.", r->name);
        return;
    }
    if (nfrz >= MAXFRZ) { om_status(0, "%d tasks are frozen already: resume one first.", MAXFRZ); return; }
    yes = om_ask("OpenMonitor", "Freeze|Cancel",
                 "Freeze %s?\n\n"
                 "It stops running until you resume it, or until OpenMonitor quits.\n"
                 "It keeps whatever it holds while it is frozen: a semaphore,\n"
                 "a lock or a window it has may then stop other programs.", r->name);
    if (yes != 1) { om_status(0, "Left running."); return; }
    {
        int tries, done = 0, gone = 0;
        /* only while it waits: a program mostly does, so a busy moment is waited out (up to half a second) */
        for (tries = 0; tries < 25 && !done && !gone; tries++) {
            if (tries) Delay(1);
            Disable();
            if (!same_task(chosen, chosen_lnname, chosen_spl) || chosen == SysBase->ThisTask) gone = 1;
            else if (chosen->tc_State == TS_WAIT) {
                frzs[nfrz].t = chosen; frzs[nfrz].lnname = chosen_lnname; frzs[nfrz].spl = chosen_spl;
                frzs[nfrz].sigwait = chosen->tc_SigWait;
                frzs[nfrz].sigexcept = chosen->tc_SigExcept;
                chosen->tc_SigWait = 0;         /* no signal wakes it now */
                chosen->tc_SigExcept = 0;
                nfrz++;
                done = 1;
            }
            Enable();
        }
        if (done) om_status(0, "%s is frozen. Resume it with the same button, or F.", chosen_name);
        else if (gone) om_status(0, "%s has ended.", chosen_name);
        else om_status(0, "%s kept running: it couldn't be frozen just now. Try again.", chosen_name);
    }
    refresh();
    om_rebuild_gadgets();
}

/* -- ending a program (0.3's: Ctrl-C, a moment, and only then Force quit, behind a warning) -- */

#define END_WAIT 3

struct ident {
    struct Task *t;
    STRPTR lnname;
    APTR splower;
    BPTR module;
    LONG shell;
    char name[40];
};
static struct ident ending;
static int ending_secs;

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

static int ident_state(const struct ident *id)
{
    struct Process *p = (struct Process *)id->t;
    if (!same_task(id->t, id->lnname, id->splower)) return ID_GONE;
    if (id->shell && (!p->pr_CLI || ((struct CommandLineInterface *)BADDR(p->pr_CLI))->cli_Module != id->module))
        return ID_COMMAND_ENDED;
    return ID_THERE;
}

static int is_system(struct Task *t)
{
    LONG shell;
    int kind;
    take_handlers();
    Forbid();
    if (!task_alive(t)) { Permit(); return 0; }
    kind = task_kind(t, &shell, NULL);
    Permit();
    return kind == OWN_SYSTEM || kind == OWN_FS;
}

static void force_quit(const struct ident *id)
{
    int st, k;
    Forbid();
    st = ident_state(id);
    if (st == ID_THERE && id->t != FindTask(NULL)) {
        if ((k = find_frz(id->t)) >= 0) frzs[k] = frzs[--nfrz];   /* still in TaskWait: RemTask finds it there */
        RemTask(id->t);
    }
    Permit();
    if (chosen == id->t) chosen = NULL;
    if (ending.t == id->t) ending.t = NULL;
    if (st == ID_GONE) om_status(0, "%s had already ended.", id->name);
    else if (st == ID_COMMAND_ENDED) om_status(0, "%s stopped by itself; its Shell is still open.", id->name);
    else om_status(0, "Removed %s. Restart when you can: what it had open stays open.", id->name);
    refresh();
}

static int confirm_force(const struct ident *id, int after_break)
{
    char shell[80];
    LONG r;
    if (id->shell) snprintf(shell, sizeof shell, "\n\nIt runs in Shell %ld: Force quit removes that Shell too.", (long)id->shell);
    else shell[0] = 0;
    r = om_ask("OpenMonitor", after_break ? "Force quit|Wait longer|Cancel" : "Force quit|Cancel",
               "%s%s\n\n"
               "Force quit removes it at once. It cannot tidy up:\n"
               "its memory, files and windows stay open until you\n"
               "restart, and the Amiga may become unstable.\n"
               "Save your work in other programs first.%s",
               id->name, after_break ? " did not stop after Ctrl-C." : ": force quit?", shell);
    if (after_break && r == 2) { ending_secs = 0; om_status(0, "Waiting a little longer..."); return 0; }
    if (r != 1) { if (ending.t == id->t) ending.t = NULL; om_status(0, "Left running."); return 0; }
    if (is_system(id->t)) {
        if (om_ask("OpenMonitor", "Remove it anyway|Cancel",
                   "%s is part of the system (a device, a file system\n"
                   "or one of the OS's own tasks).\n\n"
                   "Removing it will very likely stop the Amiga, and a\n"
                   "file system's disk may need checking afterwards.", id->name) != 1) {
            if (ending.t == id->t) ending.t = NULL;
            om_status(0, "Left running.");
            return 0;
        }
    }
    return 1;
}

/* A task the task guard holds: Ctrl-C can't reach it (it waits on nothing). */
static void end_held(struct trow *r)
{
    struct ident id;
    LONG a;
    char s[200];
    a = om_ask("OpenMonitor", "Leave it held|Copy the details|Remove it",
               "%s is held by the task guard: it stopped on\n"
               "%s at PC $%08lx ($%08lx),\n"
               "and the other tasks ran on.\n\n"
               "Removing it frees nothing it had open: its memory, its\n"
               "libraries and its ports stay until you restart.%s",
               r->name, guard_why(r->guard), (unsigned long)r->guard_pc, (unsigned long)r->guard_addr,
               r->kind == OWN_SYSTEM || r->kind == OWN_FS ? "\nIt is part of the system: the Amiga may stop working as it should." : "");
    if (a == 2) {
        snprintf(s, sizeof s, "%s at $%08lx: held by the task guard, %s at PC $%08lx ($%08lx)\n", r->name,
                 (unsigned long)r->task, guard_why(r->guard), (unsigned long)r->guard_pc, (unsigned long)r->guard_addr);
        om_status(0, om_clip_text(s) ? "The details are on the clipboard." : "The clipboard could not be written.");
        return;
    }
    if (a != 0) { om_status(0, "Left held."); return; }
    Forbid();
    if (!same_task(chosen, chosen_lnname, chosen_spl)) { Permit(); om_status(0, "That task has ended."); return; }
    take_ident(&id, chosen, r->name);
    Permit();
    if (is_system(id.t) && om_ask("OpenMonitor", "Remove it anyway|Cancel",
                                  "%s is part of the system.\n\nRemoving it will very likely stop the Amiga.", id.name) != 1) {
        om_status(0, "Left held.");
        return;
    }
    force_quit(&id);
}

static void end_program(void)
{
    struct trow *r = chosen_row();
    if (!chosen_ok(1) || !r) return;
    if (r->state == ST_HELD) { end_held(r); return; }
    Forbid();
    if (!same_task(chosen, chosen_lnname, chosen_spl)) { Permit(); chosen = NULL; om_status(0, "That task has ended."); return; }
    take_ident(&ending, chosen, chosen_name);
    if (ending.shell && !ending.module) {
        Permit();
        ending.t = NULL;
        om_status(0, "Shell %ld is at its prompt: type EndCLI in it, or close its window.", (long)ending.shell);
        return;
    }
    Signal(chosen, SIGBREAKF_CTRL_C);
    Permit();
    ending_secs = 0;
    om_status(0, "Stopping %s... (sent Ctrl-C, waiting up to %d seconds)", ending.name, END_WAIT);
}

static int in_modal;

static void end_check(void)
{
    int st;
    if (!ending.t || in_modal) return;
    Forbid();
    st = ident_state(&ending);
    Permit();
    if (st != ID_THERE) {
        om_status(0, st == ID_GONE ? "%s has stopped." : "%s has stopped; its Shell is still open.", ending.name);
        ending.t = NULL;
    } else if (++ending_secs > END_WAIT) {
        struct ident id = ending;
        in_modal = 1;
        if (confirm_force(&id, 1)) force_quit(&id);
        in_modal = 0;
    }
}

static void force_chosen(void)
{
    struct ident id;
    struct trow *r = chosen_row();
    if (!chosen_ok(1)) return;
    if (r && r->state == ST_HELD) { end_held(r); return; }
    Forbid();
    if (!same_task(chosen, chosen_lnname, chosen_spl)) { Permit(); chosen = NULL; om_status(0, "That task has ended."); return; }
    take_ident(&id, chosen, chosen_name);
    Permit();
    in_modal = 1;
    if (confirm_force(&id, 0)) force_quit(&id);
    in_modal = 0;
}

static void show_details(void)
{
    struct trow *r = chosen_row();
    char who[24], t[16], d[120], extra[160];
    if (!chosen_ok(0) || !r) return;
    owner_text(r->kind, r->shell, who, sizeof who);
    time_text(r->ms, t, sizeof t);
    describe(r, d, sizeof d);
    extra[0] = 0;
    if (r->state == ST_HELD)
        snprintf(extra, sizeof extra, "\n\nHeld by the task guard: %s\nat PC $%08lx ($%08lx).", guard_why(r->guard),
                 (unsigned long)r->guard_pc, (unsigned long)r->guard_addr);
    else if (r->cmd[0] && r->kind == OWN_SHELL)
        snprintf(extra, sizeof extra, "\n\nProgram file: %s", r->cmd);
    in_modal = 1;
    om_ask("OpenMonitor", "OK",
           "%s\n%s\n\n"
           "A %s at $%08lx, owner: %s\n"
           "State: %s    Priority: %ld    CPU: %lu.%lu%%    CPU time: %s\n"
           "Stack: %lu of %lu bytes in use\n"
           "Signals: allocated $%08lx\n"
           "         waited on $%08lx\n"
           "         received  $%08lx%s",
           r->name, d, r->process ? "process" : "task", (unsigned long)r->task, who,
           r->state < sizeof state_names / sizeof state_names[0] ? state_names[r->state] : "?", (long)r->pri,
           (unsigned long)r->tenths / 10, (unsigned long)r->tenths % 10, t, (unsigned long)r->used, (unsigned long)r->stack,
           (unsigned long)r->sigalloc, (unsigned long)r->sigwait, (unsigned long)r->sigrecvd, extra);
    in_modal = 0;
}

/* -- the menus -- */

enum { A_NONE, A_BREAK, A_SIG_C, A_SIG_D, A_SIG_E, A_SIG_F, A_PRI, A_PRI_SET, A_DETAILS, A_COPY, A_COPYALL, A_FREEZE,
       A_END, A_FORCE };

static const struct om_mitem sig_menu[] = {
    { MI_ITEM, A_SIG_C, 0, 0, "Ctrl-C  (break: stop)", NULL, NULL, 0 },
    { MI_ITEM, A_SIG_D, 0, 0, "Ctrl-D  (break a script)", NULL, NULL, 0 },
    { MI_ITEM, A_SIG_E, 0, 0, "Ctrl-E", NULL, NULL, 0 },
    { MI_ITEM, A_SIG_F, 0, 0, "Ctrl-F  (bring to front)", NULL, NULL, 0 },
};

static void do_action(int a, int value)
{
    switch (a) {
    case A_BREAK: case A_SIG_C: send_signal(SIGBREAKF_CTRL_C, "Ctrl-C"); break;
    case A_SIG_D: send_signal(SIGBREAKF_CTRL_D, "Ctrl-D"); break;
    case A_SIG_E: send_signal(SIGBREAKF_CTRL_E, "Ctrl-E"); break;
    case A_SIG_F: send_signal(SIGBREAKF_CTRL_F, "Ctrl-F"); break;
    case A_PRI: priority_to(value); break;
    case A_PRI_SET: { struct trow *r = chosen_row(); if (r) { in_modal = 1; ask_priority(r->pri); in_modal = 0; } else chosen_ok(0); } break;
    case A_DETAILS: show_details(); break;
    case A_COPY: om_status(0, chosen && om_clip_text(chosen_name) ? "The name is on the clipboard." : "The clipboard could not be written."); break;
    case A_COPYALL: om_status(0, om_table_copy(&T) ? "The list is on the clipboard." : "The clipboard could not be written."); break;
    case A_FREEZE: freeze_chosen(); break;
    case A_END: end_program(); break;
    case A_FORCE: force_chosen(); break;
    }
}

static void signal_menu(void)
{
    int value = 0, a;
    if (!chosen_ok(0)) return;
    a = om_menu(sig_menu, 4, &value);
    do_action(a, value);
}

static void row_menu(void)
{
    static struct om_mitem pri_menu[8];
    static char sub[64];
    static const char *const pri_label[6] = { "High  (+5)", "Above normal  (+1)", "Normal  (0)", "Below normal  (-1)", "Low  (-5)", "Background  (-20)" };
    static const BYTE pri_val[6] = { 5, 1, 0, -1, -5, -20 };
    struct om_mitem items[16];
    struct trow *r = chosen_row();
    char own[24];
    int n = 0, i, value = 0, a;
    if (!r) return;
    for (i = 0; i < 6; i++) {
        memset(&pri_menu[i], 0, sizeof pri_menu[i]);
        pri_menu[i].kind = MI_ITEM; pri_menu[i].action = A_PRI; pri_menu[i].value = pri_val[i];
        pri_menu[i].label = pri_label[i]; pri_menu[i].ticked = r->pri == pri_val[i];
    }
    memset(&pri_menu[6], 0, sizeof pri_menu[6]);
    pri_menu[6].kind = MI_SEP;
    memset(&pri_menu[7], 0, sizeof pri_menu[7]);
    pri_menu[7].kind = MI_ITEM; pri_menu[7].action = A_PRI_SET; pri_menu[7].label = "Set...";
    owner_text(r->kind, r->shell, own, sizeof own);
    snprintf(sub, sizeof sub, "%s, %s%s", r->process ? "process" : "task", own,
             r->state == ST_HELD ? ", held by the task guard" : r->state == ST_FROZEN ? ", frozen" : "");
    memset(items, 0, sizeof items);
    items[n].kind = MI_HEAD; items[n].label = r->name; items[n++].key = sub;
    items[n].kind = MI_ITEM; items[n].action = A_BREAK; items[n].label = "Break (Ctrl-C)"; items[n++].key = "Ctrl C";
    items[n].kind = MI_SUB; items[n].label = "Send signal"; items[n].sub = sig_menu; items[n++].nsub = 4;
    items[n++].kind = MI_SEP;
    items[n].kind = MI_SUB; items[n].label = "Priority"; items[n].sub = pri_menu; items[n++].nsub = 8;
    items[n].kind = MI_ITEM; items[n].action = A_FREEZE; items[n].label = r->state == ST_FROZEN ? "Resume" : "Freeze..."; items[n++].key = "F";
    items[n++].kind = MI_SEP;
    items[n].kind = MI_ITEM; items[n].action = A_DETAILS; items[n].label = "Details..."; items[n++].key = "Return";
    items[n].kind = MI_ITEM; items[n].action = A_COPY; items[n++].label = "Copy name";
    items[n].kind = MI_ITEM; items[n].action = A_COPYALL; items[n].label = "Copy the list as text"; items[n++].key = "C";
    items[n++].kind = MI_SEP;
    items[n].kind = MI_ITEM; items[n].action = A_END; items[n].label = "End program..."; items[n++].key = "Del";
    items[n].kind = MI_DANGER; items[n].action = A_FORCE; items[n].label = "Force quit..."; items[n++].key = "Shift Del";
    in_modal = 1;
    a = om_menu(items, n, &value);
    in_modal = 0;
    if (a == A_NONE) return;
    {
        struct trow *again = chosen_row();
        if (!again && a != A_COPYALL) { om_status(0, "That task has ended."); return; }
    }
    do_action(a, value);
}

/* ---- the view ----------------------------------------------------------------------------------------- */

static void layout(int x, int y, int w, int h)
{
    int lh = U.fh + 6, bh = lh + 2, ty, th, i, bw, nb = 6;
    gh_ = bh;
    if (U.compact) {
        graph_x = x; graph_y = y; graph_w = w; graph_h = U.fh + 2;
        filter_y = -1;
        ty = y + graph_h + 3;
        det_h = U.tfh + 4;
    } else {
        graph_x = x; graph_y = y; graph_w = w; graph_h = h > 560 ? 100 : 70;
        filter_y = y + graph_h + 10;
        ty = filter_y + lh + 10;
        det_h = U.fh + U.tfh + 14;
    }
    th = y + h - ty - det_h - bh - 16;
    if (U.compact) th = y + h - ty - det_h - bh - 6;
    om_table_layout(&T, x, ty, w, th);
    det_x = x; det_y = ty + th + (U.compact ? 2 : 8); det_w = w;
    {
        int by = det_y + det_h + (U.compact ? 3 : 8);
        bw = (w - (nb - 1) * 8) / nb;
        if (!U.compact && bw > 150) bw = 150;
        for (i = 0; i < nb; i++) {
            int b = G_PRI + i;
            gx[b] = x + i * (bw + 8); gy[b] = by; gw[b] = bw;
        }
        if (!U.compact) gx[G_DETAILS] = x + w - bw;
    }
    if (filter_y >= 0) {
        int sx = x + om_text_width(U.font, "Show") + 8;
        gx[G_SHOW] = sx; gy[G_SHOW] = filter_y; gw[G_SHOW] = om_text_width(U.font, "The system") + 44;
        gx[G_FIND] = sx + gw[G_SHOW] + 12 + om_text_width(U.font, "Find") + 8; gy[G_FIND] = filter_y; gw[G_FIND] = 200;
    }
}

static struct Gadget *gadgets(struct Gadget *g)
{
    struct NewGadget ng;
    static const char *const big[6] = { "Priority...", "Break (Ctrl-C)", "Signal...", "Freeze...", "End program...", "Details..." };
    static const char *const small[6] = { "Priority", "Break", "Signal", "Freeze", "End...", "Details" };
    struct trow *r = chosen_row();
    int i;
    memset(gad, 0, sizeof gad);
    memset(&ng, 0, sizeof ng);
    ng.ng_TextAttr = &U.ta; ng.ng_VisualInfo = U.vi;
    if (filter_y >= 0) {
        ng.ng_LeftEdge = gx[G_SHOW]; ng.ng_TopEdge = gy[G_SHOW]; ng.ng_Width = gw[G_SHOW]; ng.ng_Height = U.fh + 6;
        ng.ng_GadgetText = (STRPTR)"Show"; ng.ng_Flags = PLACETEXT_LEFT; ng.ng_GadgetID = GID_TASKS + G_SHOW;
        g = gad[G_SHOW] = CreateGadget(CYCLE_KIND, g, &ng, GTCY_Labels, (ULONG)show_labels, GTCY_Active, (ULONG)show_mode, TAG_DONE);
        ng.ng_LeftEdge = gx[G_FIND]; ng.ng_TopEdge = gy[G_FIND]; ng.ng_Width = gw[G_FIND];
        ng.ng_GadgetText = (STRPTR)"Find"; ng.ng_GadgetID = GID_TASKS + G_FIND;
        g = gad[G_FIND] = CreateGadget(STRING_KIND, g, &ng, GTST_String, (ULONG)find, GTST_MaxChars, sizeof find - 1, TAG_DONE);
    }
    for (i = 0; i < 6 && g; i++) {
        int b = G_PRI + i;
        const char *label = U.compact ? small[i] : big[i];
        if (b == G_FREEZE && r && r->state == ST_FROZEN) label = "Resume";
        ng.ng_LeftEdge = gx[b]; ng.ng_TopEdge = gy[b]; ng.ng_Width = gw[b]; ng.ng_Height = gh_;
        ng.ng_GadgetText = (STRPTR)label; ng.ng_Flags = PLACETEXT_IN; ng.ng_GadgetID = GID_TASKS + b;
        g = gad[b] = CreateGadget(BUTTON_KIND, g, &ng, TAG_DONE);
    }
    return g;
}

static void draw(void)
{
    struct RastPort *rp = U.win->RPort;
    if (U.compact) om_graphs_meter(graph_x, graph_y, graph_w, graph_h);
    else om_graphs_draw(graph_x, graph_y, graph_w, graph_h);
    if (filter_y >= 0) {
        SetFont(rp, U.font);
        ogt_text(rp, U.pen_muted, U.px + U.pw - om_text_width(U.font, "Every second"), filter_y + 3, "Every second", 0);
    }
    om_table_draw(&T);
    redraw_rows();
}

static void tick(void)
{
    if (gad[G_FIND]) {
        char *s = NULL;
        GT_GetGadgetAttrs(gad[G_FIND], U.win, NULL, GTST_String, (ULONG)&s, TAG_DONE);
        if (s && strcmp(s, find)) { strncpy(find, s, sizeof find - 1); find[sizeof find - 1] = 0; make_order(); }
    }
    end_check();
    if (U.compact) om_graphs_meter(graph_x, graph_y, graph_w, graph_h);
    else om_graphs_draw(graph_x, graph_y, graph_w, graph_h);
    redraw_rows();
}

static void status(void)
{
    int held = 0, i;
    for (i = 0; i < nrows; i++) if (rows[i].state == ST_HELD) held++;
    if (held && nfrz) om_status(1, "%d tasks, %d held by the task guard, %d frozen", nrows, held, nfrz);
    else if (held) om_status(1, "%d tasks, %d held by the task guard", nrows, held);
    else if (nfrz) om_status(1, "%d tasks, %d frozen", nrows, nfrz);
    else om_status(1, "%d tasks", nrows);
    om_status(2, "CPU: sampled %u times a second", SysBase->VBlankFrequency ? SysBase->VBlankFrequency : 50);
}

static int event(ULONG cls, UWORD code, UWORD qual, APTR ia, int mx, int my, ULONG secs, ULONG micros)
{
    int picked = 0, row = -1, sorted = 0;
    if (cls == IDCMP_MOUSEBUTTONS && code == MENUDOWN && T.list && mx > T.x && mx < T.x + T.w - T.list->sw &&
        my > T.list->y && my < T.list->y + T.list->h - 1) {
        int r = (my - (T.list->y + 1) + T.list->top) / T.row_h;
        if (r >= 0 && r < norder) {
            choose_row(r);
            ogt_list_select(T.list, r);
            draw_details();
            row_menu();
            return 1;
        }
    }
    if (om_table_event(&T, cls, code, ia, mx, my, secs, micros, &picked, &row, &sorted)) {
        if (sorted) { make_order(); redraw_rows(); }
        if (picked) {
            int was_frozen = chosen_row() && chosen_row()->state == ST_FROZEN, now;
            choose_row(row);
            now = chosen_row() && chosen_row()->state == ST_FROZEN;
            draw_details();
            if (was_frozen != now) om_rebuild_gadgets();
            if (picked == 2) show_details();
        }
        return 1;
    }
    if (cls == IDCMP_GADGETUP && ia) {
        int id = ((struct Gadget *)ia)->GadgetID - GID_TASKS;
        switch (id) {
        case G_SHOW: show_mode = code; make_order(); redraw_rows(); break;
        case G_FIND: {
            char *s = NULL;
            GT_GetGadgetAttrs(gad[G_FIND], U.win, NULL, GTST_String, (ULONG)&s, TAG_DONE);
            if (s) { strncpy(find, s, sizeof find - 1); find[sizeof find - 1] = 0; }
            make_order(); redraw_rows();
        } break;
        case G_PRI: { struct trow *r = chosen_row(); if (r) { in_modal = 1; ask_priority(r->pri); in_modal = 0; } else chosen_ok(0); } break;
        case G_BREAK: send_signal(SIGBREAKF_CTRL_C, "Ctrl-C"); break;
        case G_SIGNAL: signal_menu(); break;
        case G_FREEZE: freeze_chosen(); break;
        case G_END: end_program(); break;
        case G_DETAILS: show_details(); break;
        default: return 0;
        }
        return 1;
    }
    if (cls == IDCMP_RAWKEY && (code == 0x4C || code == 0x4D)) {
        int r = ogt_list_step(T.list, code == 0x4C ? -1 : 1);
        if (r >= 0) { choose_row(r); draw_details(); }
        return 1;
    }
    if (cls == IDCMP_VANILLAKEY) {
        switch (code) {
        case 3: send_signal(SIGBREAKF_CTRL_C, "Ctrl-C"); return 1;
        case 13: show_details(); return 1;
        case 127: if (qual & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)) force_chosen(); else end_program(); return 1;
        case 'p': case 'P': { struct trow *r = chosen_row(); if (r) { in_modal = 1; ask_priority(r->pri); in_modal = 0; } else chosen_ok(0); } return 1;
        case 'f': case 'F': freeze_chosen(); return 1;
        case 's': case 'S': signal_menu(); return 1;
        case 'c': case 'C': do_action(A_COPYALL, 0); return 1;
        case 'm': case 'M': if (chosen_row()) row_menu(); else chosen_ok(0); return 1;    /* the right-click menu, from the keys */
        case 'k': case 'K': if (om_table_choose_columns(&T)) { om_table_draw(&T); redraw_rows(); } return 1;
        }
    }
    return 0;
}

static void shown(int on)
{
    if (!on) om_table_free(&T);
}

const struct om_view om_tasks_view = { "Tasks", layout, gadgets, draw, tick, event, shown, status };

int om_tasks_init(void)
{
    rows = AllocVec(MAXT * sizeof *rows, MEMF_ANY | MEMF_CLEAR);
    order = AllocVec(MAXT * sizeof *order, MEMF_ANY | MEMF_CLEAR);
    accs = AllocVec(MAXACC * sizeof *accs, MEMF_ANY | MEMF_CLEAR);
    prev = AllocVec(MAXT * sizeof *prev, MEMF_ANY | MEMF_CLEAR);
    if (!rows || !order || !accs || !prev) { om_tasks_free(); return 0; }
    nfrz = 0;
    naccs = 0;
    chosen = NULL;
    show_mode = 0;
    find[0] = 0;
    T.sort = C_CPU;
    T.desc = 1;
    om_table_init(&T, cols, C_N, cell, GID_TASKS + 50, "Tasks");     /* the columns and sort kept from the last run */
    memset(&S, 0, sizeof S);
    vbi.is_Node.ln_Type = NT_INTERRUPT;
    vbi.is_Node.ln_Pri = -60;
    vbi.is_Node.ln_Name = (char *)"OpenMonitor";
    vbi.is_Data = &S;
    vbi.is_Code = (void (*)())vb_server;
    AddIntServer(INTB_VERTB, &vbi);
    vbi_on = 1;
    disp_before = SysBase->DispCount;
    return 1;
}

void om_tasks_free(void)
{
    while (nfrz > 0) resume_k(nfrz - 1);
    if (vbi_on) RemIntServer(INTB_VERTB, &vbi);
    vbi_on = 0;
    if (rows) FreeVec(rows);
    if (order) FreeVec(order);
    if (accs) FreeVec(accs);
    if (prev) FreeVec(prev);
    rows = NULL; order = NULL; accs = NULL; prev = NULL;
    nrows = norder = naccs = 0;
}
