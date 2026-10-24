/* OpenMonitor 1.0: the task manager (the design of 24 October 2026, "OpenMonitor-Task-Manager-Design").
 * What the parts share: the window, the fonts and pens, the table every view draws, the views.
 *
 * Built resident (libnix -resident): each run its own variables, reached through a4. Code that another task
 * or an interrupt runs (the VBlank sampler) takes what it needs from its own data, never from a global.
 *
 * MIT, Copyright (c) 2026 AmigaChrome Team. */
#ifndef OPENMONITOR_OM_H
#define OPENMONITOR_OM_H

#include <exec/types.h>
#include <exec/tasks.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <graphics/text.h>

#include "ogt_theme.h"
#include "ogt_draw.h"
#include "ogt_list.h"
#include "ogt_tabs.h"
#include "ogt_status.h"

#define OM_VERSION "1.0"
#define OM_DATE "24.10.2026"

#define REG(r, decl) register decl __asm(#r)

/* ---- the views: one tab each (P1 Tasks; P2 Memory, Libraries, Ports, Interrupts; P3 Hardware, Speed; P5 Calls) */

enum { V_TASKS, V_MEMORY, V_LIBS, V_PORTS, V_INTS, V_HW, V_SPEED, V_CALLS, V_N };

struct om_view {
    const char *name;
    void (*layout)(int x, int y, int w, int h);           /* its box inside the page; makes its gadgets */
    struct Gadget *(*gadgets)(struct Gadget *after);      /* its GadTools gadgets, after `after` */
    void (*draw)(void);                                   /* the whole view */
    void (*tick)(void);                                   /* each second while shown */
    int (*event)(ULONG cls, UWORD code, UWORD qual, APTR ia, int mx, int my, ULONG secs, ULONG micros);
    void (*shown)(int on);                                /* becomes the view, or stops being it */
    void (*status)(void);                                 /* sets the status bar's fields */
};

extern const struct om_view *om_views[V_N];
void om_open_view(int v);                         /* another view, as its tab would */

/* ---- the window and its look ------------------------------------------------------------------------- */

struct om_ui {
    struct Screen *scr;
    struct Window *win;
    struct DrawInfo *dri;
    APTR vi;
    ogt_theme *theme;               /* allocated: 4.5 KB the a4 data (64 KB, built resident) needn't hold */
    int mode;
    ogt_ctx ctx;
    int ctx_ok;
    struct TextFont *font;          /* labels: the theme's font (the screen's on a small screen) */
    struct TextAttr ta;
    struct TextFont *tfont;         /* tables: DejaVu Sans Mono 11, the system default text */
    struct TextAttr tta;
    int compact;                    /* a screen under 400 lines: the screen's font for all, no graphs */
    int fh, tfh;                    /* the fonts' heights */
    LONG pen_text, pen_muted, pen_accent, pen_danger, pen_amber, pen_list, pen_alt, pen_head, pen_fill,
         pen_filltext, pen_shine, pen_shadow, pen_back;
    LONG graph_pen[4];
    struct Gadget *glist;           /* the window's gadgets: GadTools', then the tabs' */
    ogt_tabs tabs;
    ogt_status status;
    int view;
    int px, py, pw, ph;             /* the open tab's page, inside its frame */
};

extern struct om_ui U;
extern char om_first_task[40];                    /* TASK/K: chosen in the Tasks view at the start */

/* ---- the table every view draws: a header in the theme's font, rows in the table font ---------------- */

enum { OM_LEFT, OM_RIGHT };

struct om_col {
    const char *title;
    UBYTE chars;                    /* its width in the table font's characters */
    UBYTE align;                    /* OM_LEFT or OM_RIGHT */
    UBYTE hidden;                   /* the column chooser's: not shown */
    UBYTE fixed;                    /* can't be hidden (the name) */
    UBYTE sort_desc;                /* sorts biggest first when chosen */
};

/* a cell's text; *pen (preset to the row's text pen) may be changed for its colour */
typedef void (*om_cell_fn)(int row, int col, char *buf, int size, LONG *pen);

struct om_table {
    struct om_col *cols;
    int ncols;
    int sort, desc;                 /* the column sorted on, and biggest first */
    int x, y, w, h;                 /* header and list together */
    int head_h, row_h;
    ogt_list *list;
    int count;
    om_cell_fn cell;
    int gid;                        /* the list's scroller's gadget id */
    const char *prefs_key;          /* "Tasks": its columns are kept as Tasks.Columns */
};

void om_table_init(struct om_table *t, struct om_col *cols, int ncols, om_cell_fn cell, int gid, const char *prefs_key);
void om_table_layout(struct om_table *t, int x, int y, int w, int h);
void om_table_free(struct om_table *t);
void om_table_set_count(struct om_table *t, int count);
void om_table_draw(struct om_table *t);           /* header and rows */
void om_table_draw_rows(struct om_table *t);      /* rows only: the second's update */
int om_table_selected(struct om_table *t);
void om_table_select(struct om_table *t, int row);
/* The table's share of an event: 1 when it took it. *picked: a row was clicked (2: double-clicked);
 * *sorted: the header was clicked and the sort changed (the view sorts again). */
int om_table_event(struct om_table *t, ULONG cls, UWORD code, APTR ia, int mx, int my, ULONG secs, ULONG micros,
                   int *picked, int *row, int *sorted);
/* The column chooser: a menu at the pointer with a tick each; 1 when the columns changed. */
int om_table_choose_columns(struct om_table *t);
/* Each column's left edge and width as shown (hidden ones get 0). */
void om_table_columns(struct om_table *t, int *cx, int *cw);
/* The table as text, onto the clipboard ("Copy as text"). */
int om_table_copy(struct om_table *t);
int om_table_save(struct om_table *t, const char *path);   /* the same, into a file */
/* The rows in the order of t's sort column, read from the text each cell shows (raw: by the row's own
 * number): a right-aligned column as a number (commas, "$" hex, KB and MB understood), a left one as text,
 * case ignored. order[] holds the rows' numbers; ties keep their order. */
void om_table_sort(struct om_table *t, om_cell_fn raw, int *order, int n);
/* 1 when s holds find, case ignored (an empty find holds everything). */
int om_contains(const char *s, const char *find);

/* ---- the pop-up menu (0.3's right-click menu, made general) ------------------------------------------ */

enum { MI_ITEM, MI_SUB, MI_SEP, MI_DANGER, MI_HEAD, MI_CHECK };

struct om_mitem {
    UBYTE kind;
    UBYTE action;
    BYTE value;                     /* the action's number (a priority, a column) */
    UBYTE ticked;                   /* MI_CHECK, and the current priority's tick */
    const char *label, *key;        /* MI_HEAD: the name, then what it is */
    const struct om_mitem *sub;
    int nsub;
};

/* Runs a menu at the pointer until an entry is chosen or it closes: the entry's action (0 for none), its
 * value in *value. */
int om_menu(const struct om_mitem *items, int n, int *value);

/* ---- small helpers ----------------------------------------------------------------------------------- */

void om_status(int field, const char *fmt, ...);
LONG om_ask(const char *title, const char *buttons, const char *fmt, ...);
void om_text(struct RastPort *rp, struct TextFont *f, LONG pen, int x, int y, const char *s, int maxw);
int om_text_width(struct TextFont *f, const char *s);
void om_sunk(int x, int y, int w, int h);         /* a sunk box in the window's colour */
void om_heading(int x, int y, const char *s);     /* a section's title, bold, in the labels' font */
const char *om_commas(ULONG v, char *buf);        /* 1,234,567 (buf: 16 bytes) */
const char *om_where(APTR code);                  /* "ROM" or "RAM": the ROM image or its unpacked copy */
void om_redraw_page(void);                        /* the open view again, whole */
void om_rebuild_gadgets(void);                    /* after a view changes its gadgets */
int om_clip_text(const char *s);                  /* text onto the clipboard (unit 0, FTXT) */
const char *om_pref(const char *key);             /* ENV:OpenMonitor/Settings, read at the start */
void om_set_pref(const char *key, const char *value);
int om_timer_due(void);                           /* inside a modal loop: the second's tick when it's due */
ULONG om_timer_sig(void);                         /* the tick's signal, for a modal loop's Wait() */
void om_tick(void);

/* the graphs (graphs.c): today's three, at the top of the Tasks view */
void om_graphs_init(void);
void om_graphs_free(void);
void om_graphs_take(void);                        /* memory and the GPU, each second; CPU comes from tasks.c */
void om_graphs_draw(int x, int y, int w, int h);
void om_graphs_meter(int x, int y, int w, int h); /* the one-line form, on a small screen */
void om_graphs_push_cpu(ULONG tenths, ULONG switches, ULONG cores);

/* the tasks (tasks.c) */
extern const struct om_view om_tasks_view;
int om_tasks_init(void);                          /* 0: no memory */
void om_tasks_free(void);                         /* resumes every frozen task */
void om_tasks_take(void);                         /* the second's look, every second, shown or not (the graph) */
void om_tasks_choose(struct Task *t);             /* "Show the owner": that task chosen in the Tasks view */
void om_task_name(struct Task *t, char *out, int size);   /* as the Tasks view names it; under Forbid */
int om_irq_per_second(int bit);                   /* from OpenExec's task clock; -1 when not known */

/* the other views (memory.c, libs.c, ports.c, ints.c) */
extern const struct om_view om_memory_view, om_libs_view, om_ports_view, om_ints_view;
void om_memory_init(void);
void om_memory_take(void);                        /* every second, shown or not: the last minute's graph */
ULONG om_video_bytes(void);                       /* the graphics card's video memory (its AutoConfig boards) */
struct ConfigDev;
struct ConfigDev *om_find_board(struct Library *eb, struct ConfigDev *after, LONG maker, LONG product);
void om_libs_init(void);
void om_libs_free(void);
void om_ports_init(void);
void om_ports_free(void);
void om_ints_init(void);
void om_ints_free(void);
extern const struct om_view om_hw_view, om_speed_view;
void om_hw_init(void);
void om_hw_free(void);
void om_speed_init(void);
/* the last Speed run: 1 when there was one; its MIPS and its time ("14:43:31") */
int om_speed_last(ULONG *mips, char *when);
extern const struct om_view om_calls_view;
void om_calls_init(void);
void om_calls_free(void);                         /* the wedges out first, every call through them back */

/* gadget ids: each view's from its own hundred */
#define GID_TABS 10
#define GID_TASKS 100
#define GID_MEMORY 200
#define GID_LIBS 300
#define GID_PORTS 400
#define GID_INTS 500
#define GID_HW 600
#define GID_SPEED 700
#define GID_CALLS 800

#endif
