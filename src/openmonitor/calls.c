/* GFOpenMonitor: the Calls view (the design, section 6): SnoopDos's part, the call monitor.
 *
 * Start monitoring wedges 22 calls (SetFunction, under Forbid): exec's OpenLibrary, OpenDevice, OpenResource,
 * FindPort, FindTask (by name only), FindSemaphore and FindResident; dos's Open, Lock, DeleteFile, CreateDir,
 * Rename, MakeLink, SetProtection, LoadSeg, NewLoadSeg, Execute, SystemTagList, GetVar, SetVar and DeleteVar;
 * graphics' OpenFont. Each call goes on as before, then is logged: the time, the task, the call, its name, its
 * second argument, the result and IoErr() when it failed, into a ring of 2,048 entries.
 *
 * Each wedge is a stub of 72 bytes in memory of its own, not GFOpenMonitor's code: it counts the call in, loads
 * GFOpenMonitor's a4 (built resident, its variables are reached through a4, which another task's call doesn't
 * have), calls the C wedge (the old vector with the caller's registers, then the log), counts it out. Its
 * handler at 0, it passes the call straight to the old vector.
 *
 * Stop monitoring puts each vector back if it is still our stub (if another program patched over one, ours
 * stays and passes calls through), sets every handler to 0, and waits until no call is inside a wedge before
 * the ring is freed. The stubs stay until the next reset (about 1.6 KB; the next Start uses them again,
 * their code unchanged, unless one was patched over), so a call already on its way through one always finds
 * code there.
 *
 * The log keeps everything; the view filters (the groups, failures only, a task's or a name's text, GFOpenMonitor
 * itself) when it shows it, once a second. Pause stops the view, not the log.
 *
 * MIT, Copyright (c) 2026 AmiCradle Team. */

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <devices/timer.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/var.h>
#include <graphics/text.h>
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

/* ---- the calls ---------------------------------------------------------------------------------------- */

enum { F_OPENLIB, F_OPENDEV, F_OPENRES, F_FINDPORT, F_FINDTASK, F_FINDSEM, F_FINDRES,
       F_OPEN, F_LOCK, F_DELETE, F_CREATEDIR, F_RENAME, F_MAKELINK, F_SETPROT, F_LOADSEG, F_NEWLOADSEG,
       F_EXECUTE, F_SYSTEM, F_GETVAR, F_SETVAR, F_DELVAR, F_OPENFONT, F_N };
enum { G_LIBS, G_FILES, G_PROGS, G_VARS, G_PORTS, G_N };      /* the groups, as the view's checkboxes */
static const char *const fn_name[F_N] = { "OpenLibrary", "OpenDevice", "OpenResource", "FindPort", "FindTask",
    "FindSemaphore", "FindResident", "Open", "Lock", "DeleteFile", "CreateDir", "Rename", "MakeLink",
    "SetProtection", "LoadSeg", "NewLoadSeg", "Execute", "SystemTagList", "GetVar", "SetVar", "DeleteVar", "OpenFont" };
static const UBYTE fn_group[F_N] = { G_LIBS, G_LIBS, G_LIBS, G_PORTS, G_PORTS, G_PORTS, G_PORTS,
    G_FILES, G_FILES, G_FILES, G_FILES, G_FILES, G_FILES, G_FILES, G_PROGS, G_PROGS, G_PROGS, G_PROGS,
    G_VARS, G_VARS, G_VARS, G_LIBS };
/* which library (0 exec, 1 dos, 2 graphics) and its LVO */
static const UBYTE fn_lib[F_N] = { 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2 };
static const WORD fn_lvo[F_N] = { -552, -444, -498, -390, -294, -594, -96, -30, -84, -72, -120, -78, -444, -186,
    -150, -768, -222, -606, -906, -900, -912, -72 };

/* ---- the log ------------------------------------------------------------------------------------------ */

#define LOGN 2048
struct lent {
    ULONG hi, lo;                       /* the E clock */
    struct Task *task;
    LONG a2, err;
    UBYTE fn, ok, valid, pad;
    char who[24];
    char name[60];
};
static struct lent *ring;
static volatile ULONG head;             /* entries taken so far (the ring holds the last LOGN) */
static struct Device *tbase;            /* timer.device, for ReadEClock */
static struct timerequest treq;
static ULONG efreq, t0hi, t0lo, t0secs;  /* the E clock when monitoring started, and the clock's seconds then */
static APTR old[F_N];
static struct Library *libs[3];

/* the stubs: in memory of their own (kept), see the top */
#define STUB 72
#define S_HAND 62                        /* the stub's handler (0: straight through) */
#define S_OLD 66                         /* the old vector */
static UBYTE *stubs;                     /* F_N stubs, then the shared words: the in-use count and OM's a4 */
#define ST_INUSE(b) ((volatile ULONG *)((b) + F_N * STUB))
#define ST_DATA(b) ((ULONG *)((b) + F_N * STUB + 4))
static int monitoring, patched_over;
static ULONG cleared_at;                 /* Clear: the entries before it aren't shown */

static ULONG read_eclock(struct EClockVal *v)
{
    register ULONG d0 __asm("d0");
    register struct Device *a6 __asm("a6") = tbase;
    register struct EClockVal *a0 __asm("a0") = v;
    __asm volatile ("jsr -60(%%a6)" : "=r" (d0), "+r" (a0) : "r" (a6) : "d1", "a1", "cc", "memory");
    return d0;
}

/* Runs in the caller's task, GFOpenMonitor's a4 loaded by the stub: no call here goes through a wedge. */
static void log_call(int fn, const char *name, LONG a2, int ok)
{
    struct Task *t = SysBase->ThisTask;
    struct lent *e;
    struct EClockVal ev;
    const char *n;
    int i;
    if (!ring) return;
    Disable();
    e = &ring[head++ % LOGN];
    Enable();
    e->valid = 0;
    read_eclock(&ev);
    e->hi = ev.ev_hi; e->lo = ev.ev_lo;
    e->task = t;
    e->fn = fn; e->ok = ok; e->a2 = a2;
    e->err = ok || t->tc_Node.ln_Type != NT_PROCESS ? 0 : ((struct Process *)t)->pr_Result2;
    om_task_name(t, e->who, sizeof e->who);
    for (i = 0, n = name ? name : ""; i < (int)sizeof e->name - 1 && n[i]; i++) e->name[i] = n[i];
    e->name[i] = 0;
    e->valid = 1;
}

/* The C wedges: the old vector with the registers the call came with, then the log. */
static APTR w_OpenLibrary(REG(a1, STRPTR name), REG(d0, ULONG ver), REG(a6, struct Library *base))
{
    register APTR d0r __asm("d0") = (APTR)ver;
    register STRPTR a1r __asm("a1") = name;
    register struct Library *a6r __asm("a6") = base;
    register APTR a0r __asm("a0") = old[F_OPENLIB];
    APTR r;
    __asm volatile ("jsr (%%a0)" : "+r" (d0r), "+r" (a1r), "+r" (a0r) : "r" (a6r) : "d1", "cc", "memory");
    r = d0r;
    log_call(F_OPENLIB, (const char *)name, ver, r != NULL);
    return r;
}

static LONG w_OpenDevice(REG(a0, STRPTR name), REG(d0, ULONG unit), REG(a1, APTR io), REG(d1, ULONG flags),
                         REG(a6, struct Library *base))
{
    register LONG d0r __asm("d0") = unit;
    register ULONG d1r __asm("d1") = flags;
    register STRPTR a0r __asm("a0") = name;
    register APTR a1r __asm("a1") = io;
    register struct Library *a6r __asm("a6") = base;
    register APTR a2r __asm("a2") = old[F_OPENDEV];
    LONG r;
    __asm volatile ("jsr (%%a2)" : "+r" (d0r), "+r" (d1r), "+r" (a0r), "+r" (a1r) : "r" (a6r), "r" (a2r) : "cc", "memory");
    r = (BYTE)d0r;
    log_call(F_OPENDEV, (const char *)name, unit, r == 0);
    return r;
}

/* the name in a1, the result in d0: OpenResource, FindPort, FindTask (logged by name only), FindSemaphore, FindResident */
#define WEDGE_A1(wname, F)                                                                                   \
static APTR wname(REG(a1, STRPTR name), REG(a6, struct Library *base))                                      \
{                                                                                                            \
    register APTR d0r __asm("d0");                                                                           \
    register STRPTR a1r __asm("a1") = name;                                                                  \
    register struct Library *a6r __asm("a6") = base;                                                         \
    register APTR a0r __asm("a0") = old[F];                                                                  \
    APTR r;                                                                                                  \
    __asm volatile ("jsr (%%a0)" : "=r" (d0r), "+r" (a1r), "+r" (a0r) : "r" (a6r) : "d1", "cc", "memory");  \
    r = d0r;                                                                                                 \
    if (name) log_call(F, (const char *)name, 0, r != NULL);                                                 \
    return r;                                                                                                \
}
WEDGE_A1(w_OpenResource, F_OPENRES)
WEDGE_A1(w_FindPort, F_FINDPORT)
WEDGE_A1(w_FindTask, F_FINDTASK)
WEDGE_A1(w_FindSemaphore, F_FINDSEM)
WEDGE_A1(w_FindResident, F_FINDRES)

/* dos: d1 to d4 in, the result in d0; OK as the call reads it */
#define WEDGE_D(wname, F, OKEXPR)                                                                            \
static LONG wname(REG(d1, LONG a1), REG(d2, LONG a2), REG(d3, LONG a3), REG(d4, LONG a4_), REG(a6, struct Library *base)) \
{                                                                                                            \
    register LONG d0r __asm("d0");                                                                           \
    register LONG d1r __asm("d1") = a1;                                                                      \
    register LONG d2r __asm("d2") = a2;                                                                      \
    register LONG d3r __asm("d3") = a3;                                                                      \
    register LONG d4r __asm("d4") = a4_;                                                                     \
    register struct Library *a6r __asm("a6") = base;                                                         \
    register APTR a0r __asm("a0") = old[F];                                                                  \
    LONG r;                                                                                                  \
    __asm volatile ("jsr (%%a0)" : "=r" (d0r), "+r" (d1r), "+r" (a0r) : "r" (d2r), "r" (d3r), "r" (d4r), "r" (a6r) \
                    : "a1", "cc", "memory");                                                                 \
    r = d0r;                                                                                                 \
    log_call(F, (const char *)a1, F == F_GETVAR || F == F_SETVAR ? a4_ : a2, OKEXPR);                       \
    return r;                                                                                                \
}
WEDGE_D(w_Open, F_OPEN, r != 0)
WEDGE_D(w_Lock, F_LOCK, r != 0)
WEDGE_D(w_DeleteFile, F_DELETE, r != 0)
WEDGE_D(w_CreateDir, F_CREATEDIR, r != 0)
WEDGE_D(w_Rename, F_RENAME, r != 0)
WEDGE_D(w_MakeLink, F_MAKELINK, r != 0)
WEDGE_D(w_SetProtection, F_SETPROT, r != 0)
WEDGE_D(w_LoadSeg, F_LOADSEG, r != 0)
WEDGE_D(w_NewLoadSeg, F_NEWLOADSEG, r != 0)
WEDGE_D(w_Execute, F_EXECUTE, r != 0)
WEDGE_D(w_System, F_SYSTEM, r != -1)
WEDGE_D(w_GetVar, F_GETVAR, r != -1)
WEDGE_D(w_SetVar, F_SETVAR, r != 0)
WEDGE_D(w_DeleteVar, F_DELVAR, r != 0)

static APTR w_OpenFont(REG(a0, struct TextAttr *ta), REG(a6, struct Library *base))
{
    register APTR d0r __asm("d0");
    register struct TextAttr *a0r __asm("a0") = ta;
    register struct Library *a6r __asm("a6") = base;
    register APTR a1r __asm("a1") = old[F_OPENFONT];
    APTR r;
    __asm volatile ("jsr (%%a1)" : "=r" (d0r), "+r" (a0r), "+r" (a1r) : "r" (a6r) : "d1", "cc", "memory");
    r = d0r;
    log_call(F_OPENFONT, ta ? (const char *)ta->ta_Name : "", ta ? ta->ta_YSize : 0, r != NULL);
    return r;
}

static const APTR handler[F_N] = { (APTR)w_OpenLibrary, (APTR)w_OpenDevice, (APTR)w_OpenResource, (APTR)w_FindPort,
    (APTR)w_FindTask, (APTR)w_FindSemaphore, (APTR)w_FindResident, (APTR)w_Open, (APTR)w_Lock, (APTR)w_DeleteFile,
    (APTR)w_CreateDir, (APTR)w_Rename, (APTR)w_MakeLink, (APTR)w_SetProtection, (APTR)w_LoadSeg, (APTR)w_NewLoadSeg,
    (APTR)w_Execute, (APTR)w_System, (APTR)w_GetVar, (APTR)w_SetVar, (APTR)w_DeleteVar, (APTR)w_OpenFont };

/* ---- the stubs ---------------------------------------------------------------------------------------- */

static void put_w(UBYTE **p, UWORD w) { (*p)[0] = w >> 8; (*p)[1] = w; *p += 2; }
static void put_l(UBYTE **p, ULONG l) { put_w(p, l >> 16); put_w(p, l); }

/* One call's stub (70 bytes, in 72):
 *      move.l  a4,-(sp)            the caller's a4
 *      addq.l  #1,INUSE.l          counted in
 *      tst.l   HANDLER.l
 *      beq.s   pass                0: straight through
 *      movea.l DATA.l,a4           GFOpenMonitor's a4
 *      pea     back.l
 *      move.l  HANDLER.l,-(sp)
 *      rts                         into the C wedge, which returns to back
 * back:subq.l  #1,INUSE.l          counted out
 *      movea.l (sp)+,a4
 *      rts                         d0 as the call left it
 * pass:subq.l  #1,INUSE.l
 *      movea.l (sp)+,a4
 *      move.l  OLD.l,-(sp)
 *      rts                         into the old vector, the registers as they came
 * HANDLER: dc.l, OLD: dc.l */
static void make_stub(UBYTE *s, ULONG inuse, ULONG data, APTR h, APTR o)
{
    UBYTE *p = s;
    ULONG hand = (ULONG)s + S_HAND, oldv = (ULONG)s + S_OLD;
    put_w(&p, 0x2F0C);
    put_w(&p, 0x52B9); put_l(&p, inuse);
    put_w(&p, 0x4AB9); put_l(&p, hand);
    put_w(&p, 0x671E);                          /* beq.s pass: from 16 to 46 */
    put_w(&p, 0x2879); put_l(&p, data);
    put_w(&p, 0x4879); put_l(&p, (ULONG)s + 36);
    put_w(&p, 0x2F39); put_l(&p, hand);
    put_w(&p, 0x4E75);
    put_w(&p, 0x53B9); put_l(&p, inuse);        /* back: 36 */
    put_w(&p, 0x285F);
    put_w(&p, 0x4E75);
    put_w(&p, 0x53B9); put_l(&p, inuse);        /* pass: 46 */
    put_w(&p, 0x285F);
    put_w(&p, 0x2F39); put_l(&p, oldv);
    put_w(&p, 0x4E75);
    put_l(&p, (ULONG)h);                         /* 62: S_HAND */
    put_l(&p, (ULONG)o);                         /* 66: S_OLD */
}

static int start(void)
{
    ULONG a4;
    int i;
    struct EClockVal ev;
    struct DateStamp ds;
    int fresh = 0;
    if (monitoring) return 1;
    /* a block of stubs another program patched over stays in use: never written again */
    if (patched_over) { stubs = NULL; patched_over = 0; }
    if (!stubs) {
        if (!(stubs = AllocMem(F_N * STUB + 8, MEMF_PUBLIC | MEMF_CLEAR))) return 0;
        fresh = 1;
    }
    if (!(ring = AllocVec(LOGN * sizeof *ring, MEMF_PUBLIC | MEMF_CLEAR))) return 0;
    memset(&treq, 0, sizeof treq);
    if (OpenDevice((STRPTR)TIMERNAME, UNIT_ECLOCK, (struct IORequest *)&treq, 0)) { FreeVec(ring); ring = NULL; return 0; }
    tbase = treq.tr_node.io_Device;
    efreq = read_eclock(&ev);
    t0hi = ev.ev_hi; t0lo = ev.ev_lo;
    DateStamp(&ds);
    t0secs = ds.ds_Minute * 60 + ds.ds_Tick / TICKS_PER_SECOND;
    head = 0;
    libs[0] = (struct Library *)SysBase;
    libs[1] = (struct Library *)DOSBase;
    libs[2] = (struct Library *)GfxBase;
    __asm volatile ("move.l %%a4,%0" : "=r" (a4));
    *ST_DATA(stubs) = a4;
    /* the code once, when the block is new; a block used before keeps its code as it was (a call may still be
     * on its way through it) */
    if (fresh) for (i = 0; i < F_N; i++) make_stub(stubs + i * STUB, (ULONG)ST_INUSE(stubs), (ULONG)ST_DATA(stubs), NULL, NULL);
    CacheClearU();
    Forbid();
    for (i = 0; i < F_N; i++) {
        UBYTE *s = stubs + i * STUB;
        old[i] = SetFunction(libs[fn_lib[i]], fn_lvo[i], (ULONG (*)())s);
        *(APTR *)(s + S_OLD) = old[i];
        *(APTR *)(s + S_HAND) = handler[i];      /* on: the old vector is there first */
    }
    Permit();
    monitoring = 1;
    cleared_at = 0;
    return 1;
}

/* The vectors back, every handler off, then no call left inside a wedge (waited for as long as it takes: a
 * call may be a long Execute). The ring then goes; the stubs stay. */
static void stop(void)
{
    int i, waited = 0;
    if (!monitoring) return;
    Forbid();
    for (i = 0; i < F_N; i++) {
        UBYTE *s = stubs + i * STUB;
        APTR cur = SetFunction(libs[fn_lib[i]], fn_lvo[i], (ULONG (*)())old[i]);
        if (cur != (APTR)s) { SetFunction(libs[fn_lib[i]], fn_lvo[i], (ULONG (*)())cur); patched_over++; }
        *(APTR *)(s + S_HAND) = NULL;
    }
    Permit();
    while (*ST_INUSE(stubs)) {
        if (!(waited++ % 50) && U.win) om_status(0, "Waiting for %lu calls to come back through the wedges...", *ST_INUSE(stubs));
        Delay(2);
    }
    Delay(2);
    monitoring = 0;
    CloseDevice((struct IORequest *)&treq);
    tbase = NULL;
    FreeVec(ring);
    ring = NULL;
}

/* ---- the view ----------------------------------------------------------------------------------------- */

static ULONG *rows;                     /* the entries shown, by their number (head counts) */
static int nrows, paused, failures, hide_om = 1;
static ULONG groups_on = (1 << G_N) - 1;    /* a bit a group (an array indexed by a gadget's id made GCC fold an offset
                                             * a4's 16 bits can't reach) */
static char ftask[32], fname[40];
static ULONG shown_head;
static struct Task *me;

enum { C_TIME, C_TASK, C_CALL, C_NAME, C_MODE, C_RESULT, C_N };
static struct om_col cols[C_N] = {
    { "Time", 12, OM_LEFT, 0, 0, 0 },
    { "Task", 16, OM_LEFT, 0, 0, 0 },
    { "Call", 13, OM_LEFT, 0, 1, 0 },
    { "Name", 40, OM_LEFT, 0, 0, 0 },
    { "Mode", 10, OM_LEFT, 0, 0, 0 },
    { "Result", 16, OM_LEFT, 0, 0, 0 },
};
static struct om_table T;

enum { B_START, B_PAUSE, B_CLEAR, B_SAVE, CB_0, CB_FAIL = CB_0 + G_N, CB_HIDE, S_TASK, S_NAME, G_COUNT };
static struct Gadget *gad[G_COUNT];
static int gx[G_COUNT], gy[G_COUNT], gw[G_COUNT], gh;
static const char *const group_label[G_N] = { "Libraries and devices", "Files and locks", "Programs", "Variables", "Ports and tasks" };

static const char *mode_text(const struct lent *e, char *buf, int size)
{
    switch (e->fn) {
    case F_OPENLIB: snprintf(buf, size, "v%ld", (long)e->a2); break;
    case F_OPENDEV: snprintf(buf, size, "unit %ld", (long)e->a2); break;
    case F_OPEN: snprintf(buf, size, "%s", e->a2 == MODE_OLDFILE ? "old" : e->a2 == MODE_NEWFILE ? "new" : e->a2 == MODE_READWRITE ? "read/write" : "?"); break;
    case F_LOCK: snprintf(buf, size, "%s", e->a2 == ACCESS_WRITE ? "exclusive" : "shared"); break;
    case F_GETVAR: case F_SETVAR: case F_DELVAR: snprintf(buf, size, "%s", e->a2 & GVF_GLOBAL_ONLY ? "global" : e->a2 & GVF_LOCAL_ONLY ? "local" : "any"); break;
    case F_OPENFONT: snprintf(buf, size, "size %ld", (long)e->a2); break;
    default: snprintf(buf, size, "-");
    }
    return buf;
}

static const char *result_text(const struct lent *e, char *buf, int size)
{
    if (e->ok) return "OK";
    switch (e->err) {
    case 0: return "failed";
    case ERROR_OBJECT_NOT_FOUND: case ERROR_DIR_NOT_FOUND: return "not found";
    case ERROR_OBJECT_IN_USE: return "in use";
    case ERROR_OBJECT_EXISTS: return "exists already";
    case ERROR_NO_DISK: return "no disk";
    case ERROR_DEVICE_NOT_MOUNTED: return "not mounted";
    case ERROR_WRITE_PROTECTED: case ERROR_DISK_WRITE_PROTECTED: return "write-protected";
    case ERROR_DELETE_PROTECTED: return "protected";
    case ERROR_OBJECT_WRONG_TYPE: return "wrong type";
    }
    snprintf(buf, size, "failed (%ld)", (long)e->err);
    return buf;
}

static void raw_cell(int row, int col, char *buf, int size, LONG *pen)
{
    struct lent *e;
    char t[24];
    if (!ring || row < 0 || row >= nrows) return;
    e = &ring[rows[row] % LOGN];
    switch (col) {
    case C_TIME: {
        unsigned long long d = (((unsigned long long)e->hi << 32) | e->lo) - (((unsigned long long)t0hi << 32) | t0lo);
        ULONG ms = (ULONG)(d * 1000 / efreq), s = t0secs + ms / 1000;
        snprintf(buf, size, "%02lu:%02lu:%02lu.%03lu", (unsigned long)(s / 3600 % 24), (unsigned long)(s / 60 % 60),
                 (unsigned long)(s % 60), (unsigned long)(ms % 1000));
    } break;
    case C_TASK: snprintf(buf, size, "%s", e->who); break;
    case C_CALL: snprintf(buf, size, "%s", fn_name[e->fn]); break;
    case C_NAME: snprintf(buf, size, "%s", e->name); break;
    case C_MODE: snprintf(buf, size, "%s", mode_text(e, t, sizeof t)); break;
    case C_RESULT: snprintf(buf, size, "%s", result_text(e, t, sizeof t)); if (!e->ok) *pen = U.pen_danger; break;
    }
}

/* the entries to show: the last LOGN, filtered */
static void make_rows(void)
{
    ULONG h = head, first = h > LOGN ? h - LOGN : 0, i;
    if (first < cleared_at) first = cleared_at;
    nrows = 0;
    if (!ring || !rows) return;
    for (i = first; i < h; i++) {
        struct lent *e = &ring[i % LOGN];
        if (!e->valid || !(groups_on >> fn_group[e->fn] & 1) || (failures && e->ok) || (hide_om && e->task == me) ||
            !om_contains(e->who, ftask) || !om_contains(e->name, fname))
            continue;
        rows[nrows++] = i;
    }
    shown_head = h;
}

static void cell(int row, int col, char *buf, int size, LONG *pen) { raw_cell(row, col, buf, size, pen); }

static void redraw(int follow)
{
    om_table_set_count(&T, nrows);
    if (follow && T.list && nrows) ogt_list_show(T.list, nrows - 1);
    om_table_draw_rows(&T);
}

static void layout(int x, int y, int w, int h)
{
    int bh = U.fh + 8, cbw = U.fh + 6, i, cx = x, ty;
    gh = bh;
    for (i = 0; i < G_COUNT; i++) { gw[i] = 0; gad[i] = NULL; }
    if (!U.compact) {
        for (i = 0; i < G_N; i++) {               /* the groups, then Failures only and Hide GFOpenMonitor */
            gx[CB_0 + i] = cx; gy[CB_0 + i] = y; gw[CB_0 + i] = cbw;
            cx += cbw + 6 + om_text_width(U.font, group_label[i]) + 18;
        }
        gx[CB_FAIL] = x; gy[CB_FAIL] = y + cbw + 8; gw[CB_FAIL] = cbw;
        cx = x + cbw + 6 + om_text_width(U.font, "Failures only") + 24 + om_text_width(U.font, "Task") + 8;
        gx[S_TASK] = cx; gy[S_TASK] = gy[CB_FAIL]; gw[S_TASK] = 160;
        cx += 160 + 16 + om_text_width(U.font, "Name") + 8;
        gx[S_NAME] = cx; gy[S_NAME] = gy[CB_FAIL]; gw[S_NAME] = 180;
        cx += 180 + 16;
        gx[CB_HIDE] = cx; gy[CB_HIDE] = gy[CB_FAIL]; gw[CB_HIDE] = cbw;
        ty = gy[CB_FAIL] + cbw + 10;
    } else
        ty = y;
    for (i = B_START; i <= B_SAVE; i++) {
        static const char *const lab[4] = { "Stop monitoring", "Pause", "Clear", "Save..." };
        gw[i] = om_text_width(U.font, lab[i]) + 30;
        gy[i] = y + h - bh;
    }
    gx[B_PAUSE] = x;
    gx[B_CLEAR] = x + gw[B_PAUSE] + 8;
    gx[B_SAVE] = gx[B_CLEAR] + gw[B_CLEAR] + 8;
    gx[B_START] = x + w - gw[B_START];
    om_table_layout(&T, x, ty, w, y + h - bh - (U.compact ? 4 : 10) - ty);
}

static struct Gadget *gadgets(struct Gadget *g)
{
    struct NewGadget ng;
    int i;
    memset(&ng, 0, sizeof ng);
    ng.ng_TextAttr = &U.ta; ng.ng_VisualInfo = U.vi;
    for (i = 0; i < G_COUNT && g; i++) {
        if (!gw[i]) continue;
        ng.ng_LeftEdge = gx[i]; ng.ng_TopEdge = gy[i]; ng.ng_Width = gw[i]; ng.ng_GadgetID = GID_CALLS + i;
        if (i <= B_SAVE) {
            ng.ng_Height = gh; ng.ng_Flags = PLACETEXT_IN;
            ng.ng_GadgetText = (STRPTR)(i == B_START ? (monitoring ? "Stop monitoring" : "Start monitoring") :
                                        i == B_PAUSE ? (paused ? "Go on" : "Pause") : i == B_CLEAR ? "Clear" : "Save...");
            g = gad[i] = CreateGadget(BUTTON_KIND, g, &ng, GA_Disabled, (ULONG)(i != B_START && !monitoring), TAG_DONE);
        } else if (i == S_TASK || i == S_NAME) {
            ng.ng_Height = U.fh + 6; ng.ng_Flags = PLACETEXT_LEFT;
            ng.ng_GadgetText = (STRPTR)(i == S_TASK ? "Task" : "Name");
            g = gad[i] = CreateGadget(STRING_KIND, g, &ng, GTST_String, (ULONG)(i == S_TASK ? ftask : fname),
                                      GTST_MaxChars, (ULONG)(i == S_TASK ? sizeof ftask - 1 : sizeof fname - 1), TAG_DONE);
        } else {
            int on = i == CB_FAIL ? failures : i == CB_HIDE ? hide_om : (int)(groups_on >> (i - CB_0) & 1);
            ng.ng_Height = U.fh + 6; ng.ng_Flags = PLACETEXT_RIGHT;
            ng.ng_GadgetText = (STRPTR)(i == CB_FAIL ? "Failures only" : i == CB_HIDE ? "Hide GFOpenMonitor" : group_label[i - CB_0]);
            g = gad[i] = CreateGadget(CHECKBOX_KIND, g, &ng, GTCB_Checked, (ULONG)on, GTCB_Scaled, TRUE, TAG_DONE);
        }
    }
    return g;
}

static void draw(void)
{
    make_rows();
    om_table_set_count(&T, nrows);
    om_table_draw(&T);
    if (!monitoring && !U.compact && T.list) {
        struct RastPort *rp = U.win->RPort;
        SetFont(rp, U.font);
        ogt_text(rp, U.pen_muted, T.x + 12, T.y + T.head_h + 10,
                 "Start monitoring wedges 22 calls of exec, dos and graphics; every program's calls are then logged here.", T.w - 24);
        ogt_text(rp, U.pen_muted, T.x + 12, T.y + T.head_h + 16 + U.fh,
                 "Each logged call costs a few microseconds on AmiCradle; Stop monitoring takes the wedges out.", T.w - 24);
    }
}

static void tick(void)
{
    if (!monitoring || paused || head == shown_head) return;
    make_rows();
    redraw(1);
}

static void status(void)
{
    ULONG h = head;
    int i, failed = 0;
    if (!monitoring) {
        om_status(1, patched_over ? "Not monitoring (%d wedges stay: patched over)" : "Not monitoring", patched_over);
        om_status(2, "22 calls can be wedged");
        return;
    }
    for (i = 0; i < nrows; i++) if (!ring[rows[i] % LOGN].ok) failed++;
    om_status(1, "%lu calls; %d shown, %d of them failed%s", (unsigned long)h, nrows, failed, paused ? "; paused" : "");
    om_status(2, "22 calls wedged (exec 7, dos 14, graphics 1)");
}

static void start_stop(void)
{
    if (monitoring) {
        stop();
        om_status(0, patched_over ? "Stopped; %d wedges stay (another program patched over them)." : "Stopped; every vector is back.", patched_over);
    } else if (!start())
        om_status(0, "Monitoring couldn't start (no memory, or no timer.device).");
    else
        om_status(0, "Monitoring: 22 calls wedged.");
    paused = 0;
    om_rebuild_gadgets();
    status();
}

static void save(void)
{
    static const char *const path = "RAM:GFOpenMonitor-Calls.txt";
    if (om_ask("GFOpenMonitor", "Save|Cancel", "Save the %d calls shown as text, as %s?", nrows, path) != 1) return;
    om_status(0, om_table_save(&T, path) ? "Saved as %s." : "%s couldn't be written.", path);
}

static void get_string(int i, char *to, int size)
{
    char *s = NULL;
    if (!gad[i]) return;
    GT_GetGadgetAttrs(gad[i], U.win, NULL, GTST_String, (ULONG)&s, TAG_DONE);
    if (s) { strncpy(to, s, size - 1); to[size - 1] = 0; }
}

static int event(ULONG cls, UWORD code, UWORD qual, APTR ia, int mx, int my, ULONG secs, ULONG micros)
{
    int picked = 0, row = -1, sorted = 0;
    (void)qual;
    if (om_table_event(&T, cls, code, ia, mx, my, secs, micros, &picked, &row, &sorted)) return 1;
    if (cls == IDCMP_GADGETUP && ia) {
        int id = ((struct Gadget *)ia)->GadgetID - GID_CALLS;
        switch (id) {
        case B_START: start_stop(); return 1;
        case B_PAUSE: paused = !paused; om_rebuild_gadgets(); status(); return 1;
        case B_CLEAR: cleared_at = head; make_rows(); redraw(0); status(); return 1;
        case B_SAVE: save(); return 1;
        case S_TASK: get_string(S_TASK, ftask, sizeof ftask); break;
        case S_NAME: get_string(S_NAME, fname, sizeof fname); break;
        case CB_FAIL: failures = code != 0; break;
        case CB_HIDE: hide_om = code != 0; break;
        default:
            if (id >= CB_0 && id < CB_0 + G_N) {
                ULONG bit = 1UL << (id - CB_0);
                groups_on = code ? groups_on | bit : groups_on & ~bit;
                break;
            }
            return 0;
        }
        make_rows();
        redraw(!paused);
        status();
        return 1;
    }
    if (cls == IDCMP_RAWKEY && (code == 0x4C || code == 0x4D)) { ogt_list_step(T.list, code == 0x4C ? -1 : 1); return 1; }
    if (cls == IDCMP_VANILLAKEY) {
        switch (code) {
        case 'm': case 'M': start_stop(); return 1;                      /* monitoring on or off */
        case ' ': if (monitoring) { paused = !paused; om_rebuild_gadgets(); status(); } return 1;
        case 'f': case 'F': failures = !failures; om_rebuild_gadgets(); make_rows(); redraw(1); status(); return 1;
        case 's': case 'S': if (monitoring) save(); return 1;
        case 'c': case 'C': om_status(0, om_table_copy(&T) ? "The calls shown are on the clipboard." : "The clipboard could not be written."); return 1;
        }
    }
    return 0;
}

static void shown(int on)
{
    if (!on) om_table_free(&T);
}

const struct om_view om_calls_view = { "Calls", layout, gadgets, draw, tick, event, shown, status };

void om_calls_free(void)
{
    stop();
    om_table_free(&T);
    if (rows) FreeVec(rows);
    rows = NULL;
    nrows = 0;
}

void om_calls_init(void)
{
    me = FindTask(NULL);
    rows = AllocVec(LOGN * sizeof *rows, MEMF_ANY);
    T.sort = -1;                        /* the log's order: a sort would hide the newest */
    om_table_init(&T, cols, C_N, cell, GID_CALLS + 50, "Calls");
    T.sort = -1;
}
