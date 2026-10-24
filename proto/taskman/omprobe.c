/* OMProbe: the lab prototype behind the OpenMonitor task-manager design (Team 4, 24 October 2026).
 * Not a product: it gathers what each of the new OpenMonitor's views would show, as text, and measures
 * the two numbers the design depends on:
 *
 *   OMProbe SNAP           every view's data: CPU and chips, memory, tasks, libraries, devices,
 *                          resources, ports, semaphores, interrupts, residents, boards, DOS list
 *   OMProbe CPU n          CPU share per task over n seconds, two ways at once: today's VBlank sampler
 *                          (50 looks a second) and exact accounting (each task's switch and launch hooks,
 *                          timed by the E clock); with the cost of the exact way
 *   OMProbe SNOOP n        a SnoopDos-style call monitor for n seconds (SetFunction wedges on exec and dos,
 *                          a ring of 512 entries, safe removal), then the log
 *   OMProbe COST           what one wedged call costs: Lock/UnLock and OpenLibrary/CloseLibrary timed
 *                          2,000 times bare and wedged
 *
 * Clean room: written from the NDK 3.2 headers and the autodocs' published behaviour.
 * MIT, Copyright (c) 2026 AmigaChrome Team. */
#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/interrupts.h>
#include <exec/tasks.h>
#include <exec/resident.h>
#include <exec/semaphores.h>
#include <exec/devices.h>
#include <hardware/intbits.h>
#include <devices/timer.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <graphics/gfxbase.h>
#include <libraries/configvars.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/timer.h>
#include <stdio.h>
#include <string.h>

#define REG(r, decl) register decl __asm(#r)

const char version[] __attribute__((used)) = "$VER: OMProbe 0.1 (24.10.2026) MIT, Copyright (c) 2026 AmigaChrome Team";

extern struct ExecBase *SysBase;
extern struct DosLibrary *DOSBase;
struct Device *TimerBase;
static struct timerequest tio;
static ULONG efreq;

static ULONG elo(void)
{
    struct EClockVal ev;
    ReadEClock(&ev);
    return ev.ev_lo;
}

static void cpy(char *d, const char *s, int n)
{
    int i = 0;
    if (s) for (; s[i] && i < n - 1; i++) d[i] = s[i] >= 32 && s[i] < 127 ? s[i] : (s[i] == 0 ? 0 : '.');
    d[i] = 0;
}

/* a task's shown name: a Shell's process by its command's file part, else its own */
static void tname(struct Task *t, char *out, int n)
{
    out[0] = 0;
    if (t->tc_Node.ln_Type == NT_PROCESS) {
        struct Process *p = (struct Process *)t;
        struct CommandLineInterface *cli = p->pr_CLI ? (struct CommandLineInterface *)BADDR(p->pr_CLI) : NULL;
        if (cli && cli->cli_Module && cli->cli_CommandName) {
            UBYTE *b = (UBYTE *)BADDR(cli->cli_CommandName);
            int len = b[0], s = 0, i, k;
            for (i = 0; i < len; i++) if (b[1 + i] == '/' || b[1 + i] == ':') s = i + 1;
            k = len - s < n - 1 ? len - s : n - 1;
            if (k > 0) { memcpy(out, b + 1 + s, k); out[k] = 0; return; }
        }
    }
    cpy(out, t->tc_Node.ln_Name ? t->tc_Node.ln_Name : "(no name)", n);
}

/* ================================================================== SNAP */

#define MAXT 160
struct trow {
    struct Task *t;
    char name[40], tcname[40];
    BYTE pri; UBYTE state, type, flags;
    ULONG stack, used, sigalloc, sigwait, sigrecvd;
    LONG cli, held;
};
static struct trow trows[MAXT];
static int ntrows;

static void add_trow(struct Task *t)
{
    struct trow *r;
    ULONG *rec;
    if (!t || ntrows >= MAXT) return;
    r = &trows[ntrows++];
    r->t = t;
    tname(t, r->name, sizeof r->name);
    cpy(r->tcname, t->tc_Node.ln_Name, sizeof r->tcname);
    r->pri = t->tc_Node.ln_Pri; r->state = t->tc_State; r->type = t->tc_Node.ln_Type; r->flags = t->tc_Flags;
    r->stack = (ULONG)t->tc_SPUpper - (ULONG)t->tc_SPLower;
    r->used = t != SysBase->ThisTask && (ULONG)t->tc_SPReg <= (ULONG)t->tc_SPUpper && (ULONG)t->tc_SPReg >= (ULONG)t->tc_SPLower ?
              (ULONG)t->tc_SPUpper - (ULONG)t->tc_SPReg : 0;
    r->sigalloc = t->tc_SigAlloc; r->sigwait = t->tc_SigWait; r->sigrecvd = t->tc_SigRecvd;
    r->cli = t->tc_Node.ln_Type == NT_PROCESS && ((struct Process *)t)->pr_CLI ? ((struct Process *)t)->pr_TaskNum : 0;
    /* the task guard's record (amigachrome docs/architecture/TASK_GUARD.md): 'TGRD' 56 bytes below tc_SPUpper */
    rec = (ULONG *)(((ULONG)t->tc_SPUpper - 56) & ~3UL);
    r->held = t->tc_State == TS_WAIT && t->tc_SigWait == 0 && (ULONG)rec >= (ULONG)t->tc_SPLower && rec[0] == 0x54475244UL ? (LONG)rec[1] : 0;
}

static void take_tasks(void)
{
    struct Node *n;
    ntrows = 0;
    Disable();
    add_trow(SysBase->ThisTask);
    for (n = SysBase->TaskReady.lh_Head; n->ln_Succ; n = n->ln_Succ) add_trow((struct Task *)n);
    for (n = SysBase->TaskWait.lh_Head; n->ln_Succ; n = n->ln_Succ) add_trow((struct Task *)n);
    Enable();
}

static const char *const states[] = { "invalid", "added", "running", "ready", "waiting", "except", "removed" };

static void snap_tasks(void)
{
    int i;
    take_tasks();
    printf("== TASKS %d\n", ntrows);
    printf("#addr\ttype\tname\ttc_name\tpri\tstate\tstack\tused\tsigalloc\tsigwait\tsigrecvd\tflags\tcli\theld\n");
    for (i = 0; i < ntrows; i++) {
        struct trow *r = &trows[i];
        printf("%08lx\t%s\t%s\t%s\t%d\t%s\t%lu\t%lu\t%08lx\t%08lx\t%08lx\t%02x\t%ld\t%ld\n", (ULONG)r->t,
               r->type == NT_PROCESS ? "process" : "task", r->name, r->tcname, r->pri, r->state <= 6 ? states[r->state] : "?",
               r->stack, r->used, r->sigalloc, r->sigwait, r->sigrecvd, r->flags, r->cli, r->held);
    }
}

#define BUFN 200
struct lrow { ULONG addr; char name[40]; char id[64]; UWORD ver, rev, open; UBYTE flags, type; BYTE pri; ULONG neg, pos; };
static struct lrow lrows[BUFN];

static int take_libs(struct List *l)
{
    struct Node *n;
    int k = 0;
    Forbid();
    for (n = l->lh_Head; n->ln_Succ && k < BUFN; n = n->ln_Succ, k++) {
        struct Library *b = (struct Library *)n;
        struct lrow *r = &lrows[k];
        r->addr = (ULONG)b; cpy(r->name, n->ln_Name, sizeof r->name); r->type = n->ln_Type; r->pri = n->ln_Pri;
        r->ver = b->lib_Version; r->rev = b->lib_Revision; r->open = b->lib_OpenCnt; r->flags = b->lib_Flags;
        r->neg = b->lib_NegSize; r->pos = b->lib_PosSize;
        cpy(r->id, (const char *)b->lib_IdString, sizeof r->id);
        { int j = strlen(r->id); while (j && (r->id[j - 1] == '.' || r->id[j - 1] == ' ')) r->id[--j] = 0; }
    }
    Permit();
    return k;
}

static void snap_libs(const char *what, struct List *l)
{
    int k = take_libs(l), i;
    printf("== %s %d\n#addr\tname\tver\trev\topen\tflags\tpri\tneg\tpos\tid\n", what, k);
    for (i = 0; i < k; i++)
        printf("%08lx\t%s\t%u\t%u\t%u\t%02x\t%d\t%lu\t%lu\t%s\n", lrows[i].addr, lrows[i].name, lrows[i].ver, lrows[i].rev,
               lrows[i].open, lrows[i].flags, lrows[i].pri, lrows[i].neg, lrows[i].pos, lrows[i].id);
}

static void snap_ports(void)
{
    struct prow { ULONG addr; char name[40], task[40]; UBYTE flags, bit; UWORD msgs; } *p;
    static struct prow pr[BUFN];
    struct Node *n, *m;
    int k = 0, i;
    Forbid();
    for (n = SysBase->PortList.lh_Head; n->ln_Succ && k < BUFN; n = n->ln_Succ, k++) {
        struct MsgPort *mp = (struct MsgPort *)n;
        p = &pr[k];
        p->addr = (ULONG)mp; cpy(p->name, n->ln_Name, sizeof p->name);
        p->task[0] = 0;
        if ((mp->mp_Flags & PF_ACTION) == PA_SIGNAL && mp->mp_SigTask) tname((struct Task *)mp->mp_SigTask, p->task, sizeof p->task);
        p->flags = mp->mp_Flags; p->bit = mp->mp_SigBit; p->msgs = 0;
        for (m = mp->mp_MsgList.lh_Head; m->ln_Succ && p->msgs < 999; m = m->ln_Succ) p->msgs++;
    }
    Permit();
    printf("== PORTS %d\n#addr\tname\ttask\tflags\tsigbit\tqueued\n", k);
    for (i = 0; i < k; i++) printf("%08lx\t%s\t%s\t%u\t%u\t%u\n", pr[i].addr, pr[i].name, pr[i].task, pr[i].flags, pr[i].bit, pr[i].msgs);
}

static void snap_sems(void)
{
    struct srow { ULONG addr; char name[40], owner[40]; WORD nest; UWORD queue; };
    static struct srow sr[BUFN];
    struct Node *n, *m;
    int k = 0, i;
    Forbid();
    for (n = SysBase->SemaphoreList.lh_Head; n->ln_Succ && k < BUFN; n = n->ln_Succ, k++) {
        struct SignalSemaphore *s = (struct SignalSemaphore *)n;
        sr[k].addr = (ULONG)s; cpy(sr[k].name, n->ln_Name, sizeof sr[k].name);
        sr[k].owner[0] = 0;
        if (s->ss_Owner) tname(s->ss_Owner, sr[k].owner, sizeof sr[k].owner);
        sr[k].nest = s->ss_NestCount; sr[k].queue = 0;
        for (m = (struct Node *)s->ss_WaitQueue.mlh_Head; m->ln_Succ && sr[k].queue < 999; m = m->ln_Succ) sr[k].queue++;
    }
    Permit();
    printf("== SEMAPHORES %d\n#addr\tname\towner\tnest\twaiting\n", k);
    for (i = 0; i < k; i++) printf("%08lx\t%s\t%s\t%d\t%u\n", sr[i].addr, sr[i].name, sr[i].owner, sr[i].nest, sr[i].queue);
}

static const char *const int_names[16] = { "TBE", "DSKBLK", "SOFTINT", "PORTS", "COPER", "VERTB", "BLIT", "AUD0",
                                             "AUD1", "AUD2", "AUD3", "RBF", "DSKSYNC", "EXTER", "INTEN", "NMI" };

static void snap_ints(void)
{
    int i;
    printf("== INTERRUPTS\n#vector\tkind\tname\tpri\tcode\tdata\n");
    for (i = 0; i < 16; i++) {
        struct IntVector *v = &SysBase->IntVects[i];
        int server = i == INTB_PORTS || i == INTB_COPER || i == INTB_VERTB || i == INTB_EXTER || i == INTB_NMI;
        Forbid();
        if (server && v->iv_Data) {
            struct Node *n;
            int any = 0;
            for (n = ((struct List *)v->iv_Data)->lh_Head; n->ln_Succ; n = n->ln_Succ) {
                struct Interrupt *in = (struct Interrupt *)n;
                char nm[40];
                cpy(nm, n->ln_Name, sizeof nm);
                printf("%s\tserver\t%s\t%d\t%08lx\t%08lx\n", int_names[i], nm, n->ln_Pri, (ULONG)in->is_Code, (ULONG)in->is_Data);
                any = 1;
            }
            if (!any) printf("%s\tserver\t-\t0\t0\t0\n", int_names[i]);
        } else {
            char nm[40];
            cpy(nm, v->iv_Node ? v->iv_Node->ln_Name : "-", sizeof nm);
            printf("%s\thandler\t%s\t%d\t%08lx\t%08lx\n", int_names[i], nm, v->iv_Node ? v->iv_Node->ln_Pri : 0, (ULONG)v->iv_Code, (ULONG)v->iv_Data);
        }
        Permit();
    }
}

static void snap_residents(void)
{
    ULONG *p = (ULONG *)SysBase->ResModules;
    int k = 0;
    printf("== RESIDENTS\n#addr\tname\tver\tpri\tflags\ttype\tid\n");
    while (p && *p && k < 200) {
        if (*p & 0x80000000UL) { p = (ULONG *)(*p & 0x7fffffffUL); continue; }
        {
            struct Resident *r = (struct Resident *)*p;
            char nm[40], id[64];
            cpy(nm, r->rt_Name, sizeof nm);
            cpy(id, r->rt_IdString, sizeof id);
            { int j = strlen(id); while (j && (id[j - 1] == '.' || id[j - 1] == ' ')) id[--j] = 0; }
            printf("%08lx\t%s\t%u\t%d\t%02x\t%u\t%s\n", (ULONG)r, nm, r->rt_Version, r->rt_Pri, r->rt_Flags, r->rt_Type, id);
        }
        p++; k++;
    }
}

static void snap_mem(void)
{
    struct Node *n;
    printf("== MEMORY\n#addr\tname\tattr\tpri\tlower\tupper\tfree\tchunks\tlargest\n");
    Forbid();
    for (n = SysBase->MemList.lh_Head; n->ln_Succ; n = n->ln_Succ) {
        struct MemHeader *mh = (struct MemHeader *)n;
        struct MemChunk *c;
        ULONG chunks = 0, largest = 0;
        char nm[40];
        for (c = mh->mh_First; c && chunks < 100000; c = c->mc_Next) { chunks++; if (c->mc_Bytes > largest) largest = c->mc_Bytes; }
        cpy(nm, n->ln_Name, sizeof nm);
        printf("%08lx\t%s\t%04x\t%d\t%08lx\t%08lx\t%lu\t%lu\t%lu\n", (ULONG)mh, nm, mh->mh_Attributes, n->ln_Pri, (ULONG)mh->mh_Lower,
               (ULONG)mh->mh_Upper, mh->mh_Free, chunks, largest);
    }
    Permit();
    printf("avail\tchip\t%lu\tfast\t%lu\tlargest_chip\t%lu\tlargest_fast\t%lu\ttotal_chip\t%lu\ttotal_fast\t%lu\n",
           AvailMem(MEMF_CHIP), AvailMem(MEMF_FAST), AvailMem(MEMF_CHIP | MEMF_LARGEST), AvailMem(MEMF_FAST | MEMF_LARGEST),
           AvailMem(MEMF_CHIP | MEMF_TOTAL), AvailMem(MEMF_FAST | MEMF_TOTAL));
}

static void snap_boards(void)
{
    struct Library *ExpansionBase = OpenLibrary((STRPTR)"expansion.library", 37);
    struct ConfigDev *cd = NULL;
    printf("== BOARDS\n#configdev\tmanuf\tprod\ttype\tflags\taddr\tsize\tcdflags\tdriver\n");
    if (!ExpansionBase) return;
    for (;;) {
        register struct ConfigDev *d0 __asm("d0");
        register struct Library *a6 __asm("a6") = ExpansionBase;
        register APTR a0 __asm("a0") = cd;
        register LONG d0in __asm("d0") = -1;
        register LONG d1 __asm("d1") = -1;
        __asm volatile ("jsr -72(%%a6)" : "=r" (d0) : "r" (a6), "r" (a0), "0" (d0in), "r" (d1) : "a1", "cc", "memory");
        cd = d0;
        if (!cd) break;
        {
            char drv[40] = "";
            if (cd->cd_Driver) cpy(drv, ((struct Node *)cd->cd_Driver)->ln_Name, sizeof drv);
            printf("%08lx\t%u\t%u\t%02x\t%02x\t%08lx\t%lu\t%02x\t%s\n", (ULONG)cd, cd->cd_Rom.er_Manufacturer, cd->cd_Rom.er_Product,
                   cd->cd_Rom.er_Type, cd->cd_Rom.er_Flags, (ULONG)cd->cd_BoardAddr, cd->cd_BoardSize, cd->cd_Flags, drv);
        }
    }
    CloseLibrary(ExpansionBase);
}

static void snap_sys(void)
{
    struct GfxBase *gb = (struct GfxBase *)OpenLibrary((STRPTR)"graphics.library", 0);
    struct Library *v = OpenLibrary((STRPTR)"version.library", 0);
    printf("== SYSTEM\n");
    printf("exec\t%u.%u\tsoftver\t%u\tattn\t%04x\tvblank\t%u\tpower\t%u\teclock\t%lu\tquantum\t%u\n", SysBase->LibNode.lib_Version,
           SysBase->LibNode.lib_Revision, SysBase->SoftVer, SysBase->AttnFlags, SysBase->VBlankFrequency, SysBase->PowerSupplyFrequency,
           SysBase->ex_EClockFrequency, SysBase->Quantum);
    printf("dispcount\t%lu\tidlecount\t%lu\tmaxlocmem\t%08lx\tmaxextmem\t%08lx\n", SysBase->DispCount, SysBase->IdleCount,
           (ULONG)SysBase->MaxLocMem, (ULONG)SysBase->MaxExtMem);
    if (gb) {
        printf("graphics\t%u.%u\tchiprev\t%02x\tdisplayflags\t%04x\n", gb->LibNode.lib_Version, gb->LibNode.lib_Revision, gb->ChipRevBits0,
               gb->DisplayFlags);
        CloseLibrary((struct Library *)gb);
    }
    if (v) { printf("version\t%u.%u\n", v->lib_Version, v->lib_Revision); CloseLibrary(v); }
    {
        volatile UWORD *deniseid = (volatile UWORD *)0xdff07c, *vposr = (volatile UWORD *)0xdff004;
        printf("deniseid\t%04x\tvposr\t%04x\n", *deniseid, *vposr);
    }
}

static void snap_dos(void)
{
    struct DosList *dl;
    struct drow { char name[32], task[32]; UBYTE type; } ;
    static struct drow dr[BUFN];
    int k = 0, i;
    dl = LockDosList(LDF_ALL | LDF_READ);
    while ((dl = NextDosEntry(dl, LDF_ALL)) && k < BUFN) {
        UBYTE *b = (UBYTE *)BADDR(dl->dol_Name);
        int len = b ? b[0] : 0;
        if (len > 31) len = 31;
        memcpy(dr[k].name, b + 1, len); dr[k].name[len] = 0;
        dr[k].type = dl->dol_Type;
        dr[k].task[0] = 0;
        if (dl->dol_Type != DLT_DIRECTORY && dl->dol_Task) {
            struct Task *t = ((struct MsgPort *)dl->dol_Task)->mp_SigTask;
            if (t) tname(t, dr[k].task, sizeof dr[k].task);
        }
        k++;
    }
    UnLockDosList(LDF_ALL | LDF_READ);
    printf("== DOSLIST %d\n#name\ttype\thandler\n", k);
    for (i = 0; i < k; i++) printf("%s\t%s\t%s\n", dr[i].name, dr[i].type == DLT_DEVICE ? "device" : dr[i].type == DLT_VOLUME ? "volume" :
                                   dr[i].type == DLT_DIRECTORY ? "assign" : "other", dr[i].task);
}

/* ================================================================== CPU */

struct samp { struct Task *task[MAXT]; ULONG hits[MAXT]; ULONG n, idle, total; };
static struct samp S;
static struct Interrupt vbi;

static ULONG vb_server(REG(a1, struct samp *s))
{
    struct Task *t = SysBase->ThisTask;
    ULONG i;
    s->total++;
    if (!t || t->tc_State != TS_RUN) { s->idle++; return 0; }
    for (i = 0; i < s->n; i++) if (s->task[i] == t) { s->hits[i]++; return 0; }
    if (s->n < MAXT) { s->task[s->n] = t; s->hits[s->n] = 1; s->n++; }
    return 0;
}

/* exact: each hooked task's launch and switch, E clock ticks between */
struct ex { struct Task *t; ULONG start, ticks, runs; UBYTE oflags; APTR osw, ola; };
static struct ex X[MAXT];
static int nx;
static volatile ULONG hook_calls;

static struct ex *xfind(struct Task *t)
{
    int i;
    for (i = 0; i < nx; i++) if (X[i].t == t) return &X[i];
    return NULL;
}

/* called by exec in supervisor mode, a6 = ExecBase, ThisTask = the task */
static void sw_hook(void)
{
    struct ex *e = xfind(SysBase->ThisTask);
    if (e && e->start) { e->ticks += elo() - e->start; e->start = 0; }
    hook_calls++;
}

static void la_hook(void)
{
    struct ex *e = xfind(SysBase->ThisTask);
    if (e) { e->start = elo(); e->runs++; }
    hook_calls++;
}

static void cpu_run(LONG secs, int exact)
{
    int i, j, skipped = 0;
    ULONG d0, d1, i0, i1, e0, e1, tot;
    static struct samp got;
    struct Task *me = FindTask(NULL);
    memset(&S, 0, sizeof S);
    vbi.is_Node.ln_Type = NT_INTERRUPT; vbi.is_Node.ln_Pri = -60; vbi.is_Node.ln_Name = (char *)"OMProbe";
    vbi.is_Data = &S; vbi.is_Code = (void (*)())vb_server;
    nx = 0;
    if (exact) {
        struct Node *n;
        Disable();
        for (n = SysBase->TaskReady.lh_Head; n->ln_Succ; n = n->ln_Succ) {
            struct Task *t = (struct Task *)n;
            if (t->tc_Flags & (TF_SWITCH | TF_LAUNCH)) { skipped++; continue; }
            if (nx < MAXT) { X[nx].t = t; X[nx].start = 0; X[nx].ticks = 0; X[nx].runs = 0; X[nx].oflags = t->tc_Flags; nx++; }
        }
        for (n = SysBase->TaskWait.lh_Head; n->ln_Succ; n = n->ln_Succ) {
            struct Task *t = (struct Task *)n;
            if (t->tc_Flags & (TF_SWITCH | TF_LAUNCH)) { skipped++; continue; }
            if (nx < MAXT) { X[nx].t = t; X[nx].start = 0; X[nx].ticks = 0; X[nx].runs = 0; X[nx].oflags = t->tc_Flags; nx++; }
        }
        if (nx < MAXT) { X[nx].t = me; X[nx].start = elo(); X[nx].ticks = 0; X[nx].runs = 1; X[nx].oflags = me->tc_Flags; nx++; }
        for (i = 0; i < nx; i++) {
            struct Task *t = X[i].t;
            X[i].osw = (APTR)t->tc_Switch; X[i].ola = (APTR)t->tc_Launch;
            t->tc_Switch = (void (*)())sw_hook; t->tc_Launch = (void (*)())la_hook;
            t->tc_Flags |= TF_SWITCH | TF_LAUNCH;
        }
        Enable();
    }
    AddIntServer(INTB_VERTB, &vbi);
    Disable(); d0 = SysBase->DispCount; i0 = SysBase->IdleCount; Enable();
    e0 = elo();
    Delay(secs * 50);
    e1 = elo();
    Disable(); d1 = SysBase->DispCount; i1 = SysBase->IdleCount; got = S; Enable();
    RemIntServer(INTB_VERTB, &vbi);
    if (exact) {
        Disable();
        { struct ex *e = xfind(me); if (e && e->start) { e->ticks += elo() - e->start; e->start = 0; } }
        for (i = 0; i < nx; i++) {
            struct Task *t = X[i].t;
            struct Node *n;
            int alive = t == me;
            for (n = SysBase->TaskReady.lh_Head; !alive && n->ln_Succ; n = n->ln_Succ) alive = (struct Task *)n == t;
            for (n = SysBase->TaskWait.lh_Head; !alive && n->ln_Succ; n = n->ln_Succ) alive = (struct Task *)n == t;
            if (alive && (APTR)t->tc_Switch == (APTR)sw_hook) {
                t->tc_Flags = (t->tc_Flags & ~(TF_SWITCH | TF_LAUNCH)) | (X[i].oflags & (TF_SWITCH | TF_LAUNCH));
                t->tc_Switch = (void (*)())X[i].osw; t->tc_Launch = (void (*)())X[i].ola;
            }
        }
        Enable();
    }
    tot = e1 - e0;
    take_tasks();
    printf("== CPU %ld\n", secs);
    printf("window_eticks\t%lu\teclock\t%lu\tvblank_looks\t%lu\tvblank_idle\t%lu\tdispatches\t%lu\tidle_entries\t%lu\thook_calls\t%lu\thooked\t%d\tskipped\t%d\n",
           tot, efreq, got.total, got.idle, d1 - d0, i1 - i0, hook_calls, nx, skipped);
    printf("#addr\tname\tpri\tstate\tvblank_hits\tvblank_pct10\texact_eticks\texact_pct100\tlaunches\n");
    for (i = 0; i < ntrows; i++) {
        ULONG hits = 0, ticks = 0, runs = 0;
        struct ex *e = exact ? xfind(trows[i].t) : NULL;
        for (j = 0; j < (int)got.n; j++) if (got.task[j] == trows[i].t) hits = got.hits[j];
        if (e) { ticks = e->ticks; runs = e->runs; }
        printf("%08lx\t%s\t%d\t%s\t%lu\t%lu\t%lu\t%lu\t%lu\n", (ULONG)trows[i].t, trows[i].name, trows[i].pri,
               trows[i].state <= 6 ? states[trows[i].state] : "?", hits, got.total ? hits * 1000 / got.total : 0,
               ticks, tot ? (ULONG)(((unsigned long long)ticks * 10000ULL) / tot) : 0, runs);
    }
    if (exact) {
        /* the cost of the exact way: one switch's two hook calls, timed bare */
        ULONG a, b, k;
        struct ex *e = xfind(me);
        Disable();
        a = elo();
        for (k = 0; k < 2000; k++) { if (e) { e->start = 0; } la_hook(); sw_hook(); }
        b = elo();
        Enable();
        printf("hookpair_eticks_x2000\t%lu\n", b - a);
        a = elo();
        for (k = 0; k < 2000; k++) (void)elo();
        b = elo();
        printf("readeclock_eticks_x2000\t%lu\n", b - a);
    }
}

/* ================================================================== SNOOP */

#define LOGN 512
struct lent {
    ULONG eclk;
    struct Task *task;
    LONG res, err, a2;
    UBYTE fn, ok, valid, pad;
    char who[24];
    char name[60];
};
static struct lent *LOG;
static volatile ULONG log_head, active, log_lost;

enum { F_OPENLIB, F_OPENDEV, F_OPENRES, F_FINDPORT, F_OPEN, F_LOCK, F_LOADSEG, F_NEWLOADSEG, F_DELETE, F_CREATEDIR,
       F_RENAME, F_EXECUTE, F_SYSTEM, F_GETVAR, F_SETVAR, F_N };
static const char *const fn_names[F_N] = { "OpenLibrary", "OpenDevice", "OpenResource", "FindPort", "Open", "Lock",
    "LoadSeg", "NewLoadSeg", "DeleteFile", "CreateDir", "Rename", "Execute", "SystemTagList", "GetVar", "SetVar" };
static APTR old[F_N];

static void log_call(int fn, const char *name, LONG a2, LONG res, int ok)
{
    struct Task *t = SysBase->ThisTask;
    struct lent *e;
    ULONG i;
    if (!LOG) return;
    Disable();
    i = log_head++;
    Enable();
    e = &LOG[i % LOGN];
    e->valid = 0;
    e->eclk = elo();
    e->task = t;
    e->fn = fn; e->ok = ok; e->res = res; e->a2 = a2;
    e->err = ok || !t || t->tc_Node.ln_Type != NT_PROCESS ? 0 : ((struct Process *)t)->pr_Result2;
    if (t) tname(t, e->who, sizeof e->who); else e->who[0] = 0;
    cpy(e->name, name, sizeof e->name);
    e->valid = 1;
}

#define ENTER() __asm volatile ("addq.l #1,%0" : "+m" (active) : : "cc")
#define LEAVE() __asm volatile ("subq.l #1,%0" : "+m" (active) : : "cc")

/* each wedge: the old vector with the registers it came with, then the log */
static struct Library *w_OpenLibrary(REG(a1, STRPTR name), REG(d0, ULONG ver), REG(a6, struct Library *base))
{
    struct Library *r;
    ENTER();
    {
        register struct Library *d0r __asm("d0") = (struct Library *)ver;
        register STRPTR a1r __asm("a1") = name;
        register struct Library *a6r __asm("a6") = base;
        register APTR a0r __asm("a0") = old[F_OPENLIB];
        __asm volatile ("jsr (%%a0)" : "+r" (d0r), "+r" (a1r), "+r" (a0r) : "r" (a6r) : "d1", "cc", "memory");
        r = d0r;
    }
    log_call(F_OPENLIB, (const char *)name, ver, (LONG)r, r != NULL);
    LEAVE();
    return r;
}

static BYTE w_OpenDevice(REG(a0, STRPTR name), REG(d0, ULONG unit), REG(a1, struct IORequest *io), REG(d1, ULONG flags),
                         REG(a6, struct Library *base))
{
    LONG r;
    ENTER();
    {
        register LONG d0r __asm("d0") = unit;
        register ULONG d1r __asm("d1") = flags;
        register STRPTR a0r __asm("a0") = name;
        register struct IORequest *a1r __asm("a1") = io;
        register struct Library *a6r __asm("a6") = base;
        register APTR a2r __asm("a2") = old[F_OPENDEV];
        __asm volatile ("jsr (%%a2)" : "+r" (d0r), "+r" (d1r), "+r" (a0r), "+r" (a1r) : "r" (a6r), "r" (a2r) : "cc", "memory");
        r = (BYTE)d0r;
    }
    log_call(F_OPENDEV, (const char *)name, unit, r, r == 0);
    LEAVE();
    return (BYTE)r;
}

/* one register in a1, the result in d0: OpenResource, FindPort */
#define WEDGE_A1(fname, F)                                                                                  \
static APTR fname(REG(a1, STRPTR name), REG(a6, struct Library *base))                                     \
{                                                                                                           \
    APTR r;                                                                                                 \
    ENTER();                                                                                                \
    {                                                                                                       \
        register APTR d0r __asm("d0");                                                                      \
        register STRPTR a1r __asm("a1") = name;                                                             \
        register struct Library *a6r __asm("a6") = base;                                                    \
        register APTR a0r __asm("a0") = old[F];                                                             \
        __asm volatile ("jsr (%%a0)" : "=r" (d0r), "+r" (a1r), "+r" (a0r) : "r" (a6r) : "d1", "cc", "memory"); \
        r = d0r;                                                                                            \
    }                                                                                                       \
    log_call(F, (const char *)name, 0, (LONG)r, r != NULL);                                                 \
    LEAVE();                                                                                                \
    return r;                                                                                               \
}
WEDGE_A1(w_OpenResource, F_OPENRES)
WEDGE_A1(w_FindPort, F_FINDPORT)

/* dos: d1 (and d2..d4), the result in d0 */
#define WEDGE_D(fname, F, OKEXPR)                                                                           \
static LONG fname(REG(d1, LONG a1), REG(d2, LONG a2), REG(d3, LONG a3), REG(d4, LONG a4), REG(a6, struct Library *base)) \
{                                                                                                           \
    LONG r;                                                                                                 \
    ENTER();                                                                                                \
    {                                                                                                       \
        register LONG d0r __asm("d0");                                                                      \
        register LONG d1r __asm("d1") = a1;                                                                 \
        register LONG d2r __asm("d2") = a2;                                                                 \
        register LONG d3r __asm("d3") = a3;                                                                 \
        register LONG d4r __asm("d4") = a4;                                                                 \
        register struct Library *a6r __asm("a6") = base;                                                    \
        register APTR a0r __asm("a0") = old[F];                                                             \
        __asm volatile ("jsr (%%a0)" : "=r" (d0r), "+r" (d1r), "+r" (a0r) : "r" (d2r), "r" (d3r), "r" (d4r), "r" (a6r) \
                        : "a1", "cc", "memory");                                                            \
        r = d0r;                                                                                            \
    }                                                                                                       \
    log_call(F, (const char *)a1, a2, r, OKEXPR);                                                           \
    LEAVE();                                                                                                \
    return r;                                                                                               \
}
WEDGE_D(w_Open, F_OPEN, r != 0)
WEDGE_D(w_Lock, F_LOCK, r != 0)
WEDGE_D(w_LoadSeg, F_LOADSEG, r != 0)
WEDGE_D(w_NewLoadSeg, F_NEWLOADSEG, r != 0)
WEDGE_D(w_DeleteFile, F_DELETE, r != 0)
WEDGE_D(w_CreateDir, F_CREATEDIR, r != 0)
WEDGE_D(w_Rename, F_RENAME, r != 0)
WEDGE_D(w_Execute, F_EXECUTE, r != 0)
WEDGE_D(w_System, F_SYSTEM, r != -1)
WEDGE_D(w_GetVar, F_GETVAR, r != -1)
WEDGE_D(w_SetVar, F_SETVAR, r != 0)

struct wedge { int fn; int exec; LONG lvo; APTR code; };
static const struct wedge W[F_N] = {
    { F_OPENLIB, 1, -552, (APTR)w_OpenLibrary }, { F_OPENDEV, 1, -444, (APTR)w_OpenDevice },
    { F_OPENRES, 1, -498, (APTR)w_OpenResource }, { F_FINDPORT, 1, -390, (APTR)w_FindPort },
    { F_OPEN, 0, -30, (APTR)w_Open }, { F_LOCK, 0, -84, (APTR)w_Lock }, { F_LOADSEG, 0, -150, (APTR)w_LoadSeg },
    { F_NEWLOADSEG, 0, -768, (APTR)w_NewLoadSeg }, { F_DELETE, 0, -72, (APTR)w_DeleteFile },
    { F_CREATEDIR, 0, -120, (APTR)w_CreateDir }, { F_RENAME, 0, -78, (APTR)w_Rename }, { F_EXECUTE, 0, -222, (APTR)w_Execute },
    { F_SYSTEM, 0, -606, (APTR)w_System }, { F_GETVAR, 0, -906, (APTR)w_GetVar }, { F_SETVAR, 0, -900, (APTR)w_SetVar },
};

static void wedges_on(void)
{
    int i;
    Forbid();
    for (i = 0; i < F_N; i++)
        old[W[i].fn] = SetFunction(W[i].exec ? (struct Library *)SysBase : (struct Library *)DOSBase, W[i].lvo, (ULONG (*)())W[i].code);
    Permit();
}

/* 1 when every vector went back; 0: someone patched over a wedge, which then stays (and so must the code) */
static int wedges_off(void)
{
    int i, clean = 1, waited = 0;
    Forbid();
    for (i = 0; i < F_N; i++) {
        struct Library *lib = W[i].exec ? (struct Library *)SysBase : (struct Library *)DOSBase;
        APTR cur = SetFunction(lib, W[i].lvo, (ULONG (*)())old[W[i].fn]);
        if (cur != W[i].code) { SetFunction(lib, W[i].lvo, (ULONG (*)())cur); clean = 0; printf("wedge %s: patched over, left in\n", fn_names[i]); }
    }
    Permit();
    while (active && waited < 500) { Delay(1); waited++; }
    printf("removal\tclean\t%d\tactive_after\t%lu\twaited_ticks\t%d\n", clean, active, waited);
    return clean && !active;
}

static void snoop_print(void)
{
    ULONG i, first = log_head > LOGN ? log_head - LOGN : 0, t0 = 0;
    printf("== SNOOP %lu\n#ms\twho\tcall\tname\targ2\tresult\tok\tioerr\n", log_head);
    for (i = first; i < log_head; i++) {
        struct lent *e = &LOG[i % LOGN];
        if (!e->valid) continue;
        if (!t0) t0 = e->eclk;
        printf("%lu\t%s\t%s\t%s\t%ld\t%08lx\t%s\t%ld\n", (ULONG)(((unsigned long long)(e->eclk - t0) * 1000ULL) / efreq), e->who,
               fn_names[e->fn], e->name, e->a2, (ULONG)e->res, e->ok ? "OK" : "FAIL", e->err);
    }
}

static ULONG time_calls(void)
{
    ULONG a, b, k;
    a = elo();
    for (k = 0; k < 2000; k++) {
        BPTR l = Lock((STRPTR)"RAM:", ACCESS_READ);
        if (l) UnLock(l);
    }
    b = elo();
    return b - a;
}

static ULONG time_libs(void)
{
    ULONG a, b, k;
    a = elo();
    for (k = 0; k < 2000; k++) {
        struct Library *l = OpenLibrary((STRPTR)"dos.library", 0);
        if (l) CloseLibrary(l);
    }
    b = elo();
    return b - a;
}

/* ================================================================== main */

int main(void)
{
    LONG arg[5] = { 0, 0, 0, 0, 0 };
    struct RDArgs *ra = ReadArgs((STRPTR)"SNAP/S,CPU/N,EXACT/S,SNOOP/N,COST/S", arg, NULL);
    int rc = RETURN_OK;
    if (!ra) { PrintFault(IoErr(), (STRPTR)"OMProbe"); return RETURN_ERROR; }
    if (OpenDevice((STRPTR)"timer.device", UNIT_ECLOCK, (struct IORequest *)&tio, 0)) { printf("no timer.device\n"); FreeArgs(ra); return RETURN_FAIL; }
    TimerBase = tio.tr_node.io_Device;
    { struct EClockVal ev; efreq = ReadEClock(&ev); }
    if (arg[0]) {
        snap_sys(); snap_mem(); snap_tasks();
        snap_libs("LIBRARIES", &SysBase->LibList); snap_libs("DEVICES", &SysBase->DeviceList); snap_libs("RESOURCES", &SysBase->ResourceList);
        snap_ports(); snap_sems(); snap_ints(); snap_residents(); snap_boards(); snap_dos();
    }
    if (arg[1]) cpu_run(*(LONG *)arg[1], arg[2] != 0);
    if (arg[3] || arg[4]) {
        LOG = AllocMem(sizeof(struct lent) * LOGN, MEMF_PUBLIC | MEMF_CLEAR);
        if (!LOG) { printf("no memory for the log\n"); rc = RETURN_FAIL; }
        else {
            ULONG bare_l = 0, bare_o = 0, wl = 0, wo = 0;
            int clean;
            if (arg[4]) { bare_l = time_calls(); bare_o = time_libs(); }
            wedges_on();
            if (arg[4]) { wl = time_calls(); wo = time_libs(); log_head = 0; memset(LOG, 0, sizeof(struct lent) * LOGN); }
            if (arg[3]) Delay(*(LONG *)arg[3] * 50);
            clean = wedges_off();
            if (arg[4])
                printf("== COST\neclock\t%lu\tlock_unlock_bare_x2000\t%lu\tlock_unlock_wedged_x2000\t%lu\topenlib_bare_x2000\t%lu\topenlib_wedged_x2000\t%lu\n",
                       efreq, bare_l, wl, bare_o, wo);
            if (arg[3]) snoop_print();
            if (clean) FreeMem(LOG, sizeof(struct lent) * LOGN);
            else { printf("left resident: a wedge is still in use\n"); Wait(0); }
        }
    }
    CloseDevice((struct IORequest *)&tio);
    FreeArgs(ra);
    return rc;
}
