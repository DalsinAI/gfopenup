/* GFOpenMonitor: the Ports view (the design, section 4.5).
 *
 * Exec's public message ports: name, owner (the task its signal goes to), signal bit, messages waiting (amber
 * when any wait), action (signal, soft interrupt, ignore) and address. Under them its public semaphores: name,
 * held by, nesting, waiting and address. Show the owner chooses that task in the Tasks view.
 *
 * Read every second while shown, under Forbid, into a copy that is then drawn.
 *
 * MIT, Copyright (c) 2026 AmigaChrome Team. */

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <exec/semaphores.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <string.h>
#include <stdio.h>

#include "om.h"

#define MAXP 96
#define MAXS 48

struct prow {
    APTR addr;
    struct Task *owner;
    char name[36], who[36];
    UBYTE sigbit, action, shared;
    UWORD waiting, nest;
};
static struct prow *ports, *sems;
static int nports, nsems, *porder, *sorder;

enum { P_NAME, P_OWNER, P_SIG, P_WAIT, P_ACTION, P_ADDR, P_N };
static struct om_col pcols[P_N] = {
    { "Port", 22, OM_LEFT, 0, 1, 0 },
    { "Owner (signal task)", 20, OM_LEFT, 0, 0, 0 },
    { "Signal", 6, OM_RIGHT, 0, 0, 1 },
    { "Waiting", 7, OM_RIGHT, 0, 0, 1 },
    { "Action", 14, OM_LEFT, 0, 0, 0 },
    { "Address", 8, OM_LEFT, 0, 0, 0 },
};
enum { S_NAME, S_HELD, S_NEST, S_WAIT, S_ADDR, S_N };
static struct om_col scols[S_N] = {
    { "Semaphore", 22, OM_LEFT, 0, 1, 0 },
    { "Held by", 20, OM_LEFT, 0, 0, 0 },
    { "Nesting", 7, OM_RIGHT, 0, 0, 1 },
    { "Waiting", 7, OM_RIGHT, 0, 0, 1 },
    { "Address", 8, OM_LEFT, 0, 0, 0 },
};
static struct om_table PT, ST;
static int active;                      /* 0: the ports' table was last chosen in, 1: the semaphores' */

enum { G_OWNER, G_COPY, G_COUNT };
static struct Gadget *gad[G_COUNT];
static int gx[G_COUNT], gw[G_COUNT], gy, gh, ph_y, sh_y;

/* ---- reading ------------------------------------------------------------------------------------------ */

static void copy_name(char *to, const char *from, int size)
{
    int i;
    if (!from || !from[0]) { strncpy(to, "(no name)", size - 1); to[size - 1] = 0; return; }
    for (i = 0; i < size - 1 && from[i] && from[i] != '\n'; i++) to[i] = from[i];
    to[i] = 0;
}

static int count(struct List *l, int most)
{
    struct Node *n;
    int k = 0;
    for (n = l->lh_Head; n && n->ln_Succ && k < most; n = n->ln_Succ) k++;
    return k;
}

static void raw_port(int row, int col, char *buf, int size, LONG *pen);
static void raw_sem(int row, int col, char *buf, int size, LONG *pen);

static void take(void)
{
    struct Node *n;
    int i;
    nports = nsems = 0;
    if (!ports || !sems) return;
    Forbid();
    for (n = SysBase->PortList.lh_Head; n->ln_Succ && nports < MAXP; n = n->ln_Succ) {
        struct MsgPort *mp = (struct MsgPort *)n;
        struct prow *r = &ports[nports++];
        memset(r, 0, sizeof *r);
        r->addr = mp;
        copy_name(r->name, n->ln_Name, sizeof r->name);
        r->action = mp->mp_Flags & PF_ACTION;
        r->sigbit = mp->mp_SigBit;
        r->waiting = count(&mp->mp_MsgList, 999);
        if (r->action == PA_SIGNAL && mp->mp_SigTask) {
            r->owner = mp->mp_SigTask;
            om_task_name(r->owner, r->who, sizeof r->who);
        } else if (r->action == PA_SOFTINT && mp->mp_SigTask)
            copy_name(r->who, ((struct Node *)mp->mp_SigTask)->ln_Name, sizeof r->who);
        else
            strcpy(r->who, "-");
    }
    for (n = SysBase->SemaphoreList.lh_Head; n->ln_Succ && nsems < MAXS; n = n->ln_Succ) {
        struct SignalSemaphore *ss = (struct SignalSemaphore *)n;
        struct prow *r = &sems[nsems++];
        memset(r, 0, sizeof *r);
        r->addr = ss;
        copy_name(r->name, n->ln_Name, sizeof r->name);
        r->nest = ss->ss_NestCount;
        r->waiting = count((struct List *)&ss->ss_WaitQueue, 999);
        if (ss->ss_Owner) {
            r->owner = ss->ss_Owner;
            om_task_name(r->owner, r->who, sizeof r->who);
        } else if (ss->ss_NestCount > 0) {
            r->shared = 1;
            strcpy(r->who, "shared");
        } else
            strcpy(r->who, "free");
    }
    Permit();
    for (i = 0; i < nports; i++) porder[i] = i;
    for (i = 0; i < nsems; i++) sorder[i] = i;
    om_table_sort(&PT, raw_port, porder, nports);
    om_table_sort(&ST, raw_sem, sorder, nsems);
}

/* ---- the cells ---------------------------------------------------------------------------------------- */

static void raw_port(int row, int col, char *buf, int size, LONG *pen)
{
    static const char *const act[4] = { "signal", "soft interrupt", "ignore", "none" };
    struct prow *r;
    if (row < 0 || row >= nports) return;
    r = &ports[row];
    switch (col) {
    case P_NAME: snprintf(buf, size, "%s", r->name); if (!strcmp(r->name, "(no name)")) *pen = U.pen_muted; break;
    case P_OWNER: snprintf(buf, size, "%s", r->who); if (!r->owner) *pen = U.pen_muted; break;
    case P_SIG: if (r->action == PA_SIGNAL) snprintf(buf, size, "%u", r->sigbit); else snprintf(buf, size, "-"); break;
    case P_WAIT: snprintf(buf, size, "%u", r->waiting); if (r->waiting) *pen = U.pen_amber; break;
    case P_ACTION: snprintf(buf, size, "%s", act[r->action & 3]); if (r->action == PA_IGNORE) *pen = U.pen_muted; break;
    case P_ADDR: snprintf(buf, size, "%08lx", (unsigned long)r->addr); *pen = U.pen_muted; break;
    }
}

static void raw_sem(int row, int col, char *buf, int size, LONG *pen)
{
    struct prow *r;
    if (row < 0 || row >= nsems) return;
    r = &sems[row];
    switch (col) {
    case S_NAME: snprintf(buf, size, "%s", r->name); if (!strcmp(r->name, "(no name)")) *pen = U.pen_muted; break;
    case S_HELD: snprintf(buf, size, "%s", r->who); if (!r->owner && !r->shared) *pen = U.pen_muted; break;
    case S_NEST: snprintf(buf, size, "%u", r->nest); break;
    case S_WAIT: snprintf(buf, size, "%u", r->waiting); if (r->waiting) *pen = U.pen_amber; break;
    case S_ADDR: snprintf(buf, size, "%08lx", (unsigned long)r->addr); *pen = U.pen_muted; break;
    }
}

static void port_cell(int row, int col, char *buf, int size, LONG *pen)
{
    if (row >= 0 && row < nports) raw_port(porder[row], col, buf, size, pen);
}

static void sem_cell(int row, int col, char *buf, int size, LONG *pen)
{
    if (row >= 0 && row < nsems) raw_sem(sorder[row], col, buf, size, pen);
}

/* ---- the actions -------------------------------------------------------------------------------------- */

static void show_owner(void)
{
    int s = active ? om_table_selected(&ST) : om_table_selected(&PT);
    struct prow *r = NULL;
    struct Task *t;
    if (active && s >= 0 && s < nsems) r = &sems[sorder[s]];
    if (!active && s >= 0 && s < nports) r = &ports[porder[s]];
    if (!r) { om_status(0, "Choose a port or a semaphore first."); return; }
    if (!(t = r->owner)) {
        om_status(0, "%s has no owner to show%s.", r->name, active ? (r->shared ? " (it is held shared)" : " (it is free)") : "");
        return;
    }
    om_tasks_choose(t);
    om_open_view(V_TASKS);
}

/* ---- the view ----------------------------------------------------------------------------------------- */

static void layout(int x, int y, int w, int h)
{
    int bh = U.fh + 8, hh = U.compact ? 0 : U.fh + 6, room, pth;
    gy = y + h - bh;
    gh = bh;
    room = gy - (U.compact ? 4 : 10) - y - 2 * hh - (U.compact ? 4 : 12);
    pth = room * 3 / 5;
    ph_y = y;
    om_table_layout(&PT, x, y + hh, w, pth);
    sh_y = PT.y + PT.h + (U.compact ? 4 : 12);
    om_table_layout(&ST, x, sh_y + hh, w, room - pth);
    gw[G_OWNER] = om_text_width(U.font, U.compact ? "Owner" : "Show the owner") + 30;
    gw[G_COPY] = om_text_width(U.font, U.compact ? "Copy" : "Copy as text") + 30;
    gx[G_OWNER] = x;
    gx[G_COPY] = x + w - gw[G_COPY];
}

static struct Gadget *gadgets(struct Gadget *g)
{
    static const char *const big[G_COUNT] = { "Show the owner", "Copy as text" };
    static const char *const small[G_COUNT] = { "Owner", "Copy" };
    struct NewGadget ng;
    int i;
    memset(&ng, 0, sizeof ng);
    ng.ng_TextAttr = &U.ta; ng.ng_VisualInfo = U.vi; ng.ng_Flags = PLACETEXT_IN;
    for (i = 0; i < G_COUNT && g; i++) {
        ng.ng_LeftEdge = gx[i]; ng.ng_TopEdge = gy; ng.ng_Width = gw[i]; ng.ng_Height = gh;
        ng.ng_GadgetText = (STRPTR)(U.compact ? small[i] : big[i]); ng.ng_GadgetID = GID_PORTS + i;
        g = gad[i] = CreateGadget(BUTTON_KIND, g, &ng, TAG_DONE);
    }
    return g;
}

static void redraw(void)
{
    om_table_set_count(&PT, nports);
    om_table_set_count(&ST, nsems);
    om_table_draw_rows(&PT);
    om_table_draw_rows(&ST);
}

static void draw(void)
{
    take();
    if (!U.compact) {
        om_heading(U.px, ph_y, "Public message ports");
        om_heading(U.px, sh_y, "Semaphores");
    }
    om_table_set_count(&PT, nports);
    om_table_set_count(&ST, nsems);
    om_table_draw(&PT);
    om_table_draw(&ST);
}

/* the chosen rows kept chosen through a new reading (the rows may have moved) */
static void tick(void)
{
    int ps = om_table_selected(&PT), ss = om_table_selected(&ST), i;
    APTR pa = ps >= 0 && ps < nports ? ports[porder[ps]].addr : NULL;
    APTR sa = ss >= 0 && ss < nsems ? sems[sorder[ss]].addr : NULL;
    take();
    if (PT.list) { for (i = 0; i < nports && ports[porder[i]].addr != pa; i++) {} PT.list->selected = pa && i < nports ? i : -1; }
    if (ST.list) { for (i = 0; i < nsems && sems[sorder[i]].addr != sa; i++) {} ST.list->selected = sa && i < nsems ? i : -1; }
    redraw();
}

static void status(void)
{
    int i, waiting = 0, held = 0;
    for (i = 0; i < nports; i++) waiting += ports[i].waiting;
    for (i = 0; i < nsems; i++) if (sems[i].owner || sems[i].shared) held++;
    om_status(1, "%d ports, %d message%s waiting; %d semaphores, %d held", nports, waiting, waiting == 1 ? "" : "s", nsems, held);
    om_status(2, "Every second");
}

static int event(ULONG cls, UWORD code, UWORD qual, APTR ia, int mx, int my, ULONG secs, ULONG micros)
{
    int picked = 0, row = -1, sorted = 0;
    (void)qual;
    if (om_table_event(&PT, cls, code, ia, mx, my, secs, micros, &picked, &row, &sorted)) {
        if (picked) {
            active = 0;
            if (ST.list && ST.list->selected >= 0) { ST.list->selected = -1; om_table_draw_rows(&ST); }
            if (picked == 2) show_owner();
        }
        if (sorted) { take(); redraw(); }
        return 1;
    }
    if (om_table_event(&ST, cls, code, ia, mx, my, secs, micros, &picked, &row, &sorted)) {
        if (picked) {
            active = 1;
            if (PT.list && PT.list->selected >= 0) { PT.list->selected = -1; om_table_draw_rows(&PT); }
            if (picked == 2) show_owner();
        }
        if (sorted) { take(); redraw(); }
        return 1;
    }
    if (cls == IDCMP_GADGETUP && ia) {
        switch (((struct Gadget *)ia)->GadgetID - GID_PORTS) {
        case G_OWNER: show_owner(); return 1;
        case G_COPY: om_status(0, om_table_copy(active ? &ST : &PT) ? "The table is on the clipboard." : "The clipboard could not be written."); return 1;
        }
    }
    if (cls == IDCMP_RAWKEY && (code == 0x4C || code == 0x4D)) {
        ogt_list_step(active ? ST.list : PT.list, code == 0x4C ? -1 : 1);
        return 1;
    }
    if (cls == IDCMP_VANILLAKEY) {
        switch (code) {
        case 13: show_owner(); return 1;
        case 'c': case 'C': om_status(0, om_table_copy(active ? &ST : &PT) ? "The table is on the clipboard." : "The clipboard could not be written."); return 1;
        case 'p': case 'P': active = 0; om_status(0, "The ports' table: the cursor keys move in it."); return 1;
        case 's': case 'S': active = 1; om_status(0, "The semaphores' table: the cursor keys move in it."); return 1;
        }
    }
    return 0;
}

static void shown(int on)
{
    if (!on) { om_table_free(&PT); om_table_free(&ST); }
    else take();
}

const struct om_view om_ports_view = { "Ports", layout, gadgets, draw, tick, event, shown, status };

void om_ports_free(void)
{
    om_table_free(&PT);
    om_table_free(&ST);
    if (ports) FreeVec(ports);
    if (sems) FreeVec(sems);
    if (porder) FreeVec(porder);
    if (sorder) FreeVec(sorder);
    ports = sems = NULL; porder = sorder = NULL;
    nports = nsems = 0;
}

void om_ports_init(void)
{
    ports = AllocVec(MAXP * sizeof *ports, MEMF_ANY | MEMF_CLEAR);
    sems = AllocVec(MAXS * sizeof *sems, MEMF_ANY | MEMF_CLEAR);
    porder = AllocVec(MAXP * sizeof *porder, MEMF_ANY | MEMF_CLEAR);
    sorder = AllocVec(MAXS * sizeof *sorder, MEMF_ANY | MEMF_CLEAR);
    if (!porder || !sorder) { if (ports) FreeVec(ports); if (sems) FreeVec(sems); ports = sems = NULL; }
    PT.sort = -1;
    ST.sort = -1;
    om_table_init(&PT, pcols, P_N, port_cell, GID_PORTS + 50, "Ports");
    om_table_init(&ST, scols, S_N, sem_cell, GID_PORTS + 60, "Semaphores");
    active = 0;
}
