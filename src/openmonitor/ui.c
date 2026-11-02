/* GFOpenMonitor: the table every view draws, and the pop-up menu.
 *
 * The table: a header in the theme's font (a click sorts, a right-click chooses the columns), the rows in
 * the table font (DejaVu Sans Mono 11), drawn by gfopengadtools' ogt_list. The menu: 0.3's right-click menu,
 * made general (ticks, sub-menus, a heading, the dangerous entry in red).
 *
 * MIT, Copyright (c) 2026 AmiCradle Team. */

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <devices/inputevent.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "om.h"

/* ---- the table ------------------------------------------------------------------------------------------ */

#define GAP (U.compact ? 8 : 12)

static void row_draw(void *user, int index, struct RastPort *rp, int x, int y, int w, int h, int selected);
static int row_height(void *user, int index) { return ((struct om_table *)user)->row_h; }

static int char_w(void)
{
    return U.tfont->tf_Flags & FPF_PROPORTIONAL ? om_text_width(U.tfont, "0") : U.tfont->tf_XSize;
}

static void cols_from_prefs(struct om_table *t)
{
    char key[40];
    const char *v;
    int i;
    snprintf(key, sizeof key, "%s.Columns", t->prefs_key);
    if ((v = om_pref(key)) && (int)strlen(v) == t->ncols)
        for (i = 0; i < t->ncols; i++) t->cols[i].hidden = t->cols[i].fixed ? 0 : v[i] == '0';
    snprintf(key, sizeof key, "%s.Sort", t->prefs_key);
    if ((v = om_pref(key))) {
        int s = 0;
        char d = 0;
        if (sscanf(v, "%d %c", &s, &d) >= 1 && s >= 0 && s < t->ncols) { t->sort = s; t->desc = d == 'd'; }
    }
}

static void cols_to_prefs(struct om_table *t)
{
    char key[40], v[40];
    int i;
    for (i = 0; i < t->ncols && i < (int)sizeof v - 1; i++) v[i] = t->cols[i].hidden ? '0' : '1';
    v[i] = 0;
    snprintf(key, sizeof key, "%s.Columns", t->prefs_key);
    om_set_pref(key, v);
    snprintf(key, sizeof key, "%s.Sort", t->prefs_key);
    snprintf(v, sizeof v, "%d %c", t->sort, t->desc ? 'd' : 'a');
    om_set_pref(key, v);
}

void om_table_init(struct om_table *t, struct om_col *cols, int ncols, om_cell_fn cell, int gid, const char *prefs_key)
{
    t->cols = cols;
    t->ncols = ncols;
    t->cell = cell;
    t->gid = gid;
    t->prefs_key = prefs_key;
    t->list = NULL;
    t->count = 0;
    cols_from_prefs(t);
}

void om_table_columns(struct om_table *t, int *cx, int *cw)
{
    int i, x = t->x + 6, cwid = char_w(), last = -1, right = t->x + t->w - 16;
    for (i = 0; i < t->ncols; i++) {
        int w;
        if (t->cols[i].hidden) { cx[i] = 0; cw[i] = 0; continue; }
        w = t->cols[i].chars * cwid;
        {
            int tw = om_text_width(U.font, t->cols[i].title) + (i == t->sort ? 12 : 0);
            if (tw > w) w = tw;
        }
        cx[i] = x;
        cw[i] = w;
        x += w + GAP;
        last = i;
    }
    /* the last column takes what is left, so a long name or ID string shows more */
    if (last >= 0 && cx[last] + cw[last] < right - 4 && t->cols[last].align == OM_LEFT) cw[last] = right - 4 - cx[last];
}

void om_table_layout(struct om_table *t, int x, int y, int w, int h)
{
    t->x = x; t->y = y; t->w = w; t->h = h;
    t->head_h = U.fh + (U.compact ? 4 : 8);
    t->row_h = U.tfh + (U.compact ? 1 : 3);
    if (!t->list) t->list = ogt_list_new(U.win, &U.ctx, t->gid, row_height, row_draw, NULL, t, "list");
    if (t->list) {
        ogt_list_layout(t->list, x, y + t->head_h, w, h - t->head_h);
        ogt_list_set_count(t->list, t->count);
    }
}

void om_table_free(struct om_table *t)
{
    if (t->list) ogt_list_free(t->list);
    t->list = NULL;
}

void om_table_set_count(struct om_table *t, int count)
{
    int sel = t->list ? t->list->selected : -1;
    t->count = count;
    if (t->list) {
        ogt_list_set_count(t->list, count);
        if (sel >= count) t->list->selected = -1;
    }
}

int om_table_selected(struct om_table *t) { return t->list ? t->list->selected : -1; }

void om_table_select(struct om_table *t, int row)
{
    if (t->list) ogt_list_select(t->list, row);
}

static void draw_header(struct om_table *t)
{
    struct RastPort *rp = U.win->RPort;
    int cx[32], cw[32], i, ty;
    om_table_columns(t, cx, cw);
    ogt_box(rp, U.pen_head, t->x + 1, t->y + 1, t->w - 2, t->head_h - 1);
    ogt_bevel(rp, U.pen_shine, U.pen_shadow, t->x, t->y, t->w, t->head_h + 1);
    SetFont(rp, U.font);
    ty = t->y + (t->head_h - U.fh) / 2 + 1;
    for (i = 0; i < t->ncols; i++) {
        int tw, tx;
        if (t->cols[i].hidden) continue;
        tw = om_text_width(U.font, t->cols[i].title);
        tx = t->cols[i].align == OM_LEFT ? cx[i] : cx[i] + cw[i] - tw;
        ogt_text(rp, U.pen_text, tx, ty, t->cols[i].title, cw[i]);
        if (i == t->sort) {
            /* the sort's arrow: down for biggest first */
            int ax = t->cols[i].align == OM_LEFT ? tx + tw + 4 : tx - 10, ay = t->y + t->head_h / 2 - 2, xy[6];
            if (t->desc) { xy[0] = ax; xy[1] = ay; xy[2] = ax + 6; xy[3] = ay; xy[4] = ax + 3; xy[5] = ay + 4; }
            else { xy[0] = ax; xy[1] = ay + 4; xy[2] = ax + 6; xy[3] = ay + 4; xy[4] = ax + 3; xy[5] = ay; }
            ogt_fill_poly(rp, U.pen_muted, xy, 3);
        }
        if (cx[i] > t->x + 6) ogt_vline(rp, U.pen_shadow, cx[i] - GAP / 2, t->y + 4, t->head_h - 6);
    }
}

void om_table_draw(struct om_table *t)
{
    draw_header(t);
    if (t->list) ogt_list_draw(t->list);
}

void om_table_draw_rows(struct om_table *t)
{
    if (t->list) ogt_list_draw(t->list);            /* one clip region for every row, the rest cleared */
}

static void row_draw(void *user, int index, struct RastPort *rp, int x, int y, int w, int h, int selected)
{
    struct om_table *t = user;
    int cx[32], cw[32], i, ty;
    char buf[160];
    LONG base = selected ? U.pen_filltext : U.pen_text;
    if (selected) ogt_fill(&U.ctx, rp, "fill", x, y, w, h);
    else if (index & 1) ogt_box(rp, U.pen_alt, x, y, w, h);
    om_table_columns(t, cx, cw);
    if (rp->Font != U.tfont) SetFont(rp, U.tfont);
    ty = y + (h - U.tfh) / 2;
    for (i = 0; i < t->ncols; i++) {
        LONG pen = base;
        int tw;
        if (t->cols[i].hidden) continue;
        buf[0] = 0;
        t->cell(index, i, buf, sizeof buf, &pen);
        if (selected) pen = base;
        if (!buf[0]) continue;
        if (t->cols[i].align == OM_LEFT) ogt_text(rp, pen, cx[i], ty, buf, cw[i]);
        else {
            tw = TextLength(rp, (STRPTR)buf, strlen(buf));
            ogt_text(rp, pen, tw > cw[i] ? cx[i] : cx[i] + cw[i] - tw, ty, buf, cw[i]);
        }
    }
}

static int header_col(struct om_table *t, int mx)
{
    int cx[32], cw[32], i;
    om_table_columns(t, cx, cw);
    for (i = 0; i < t->ncols; i++) if (cw[i] && mx >= cx[i] - GAP / 2 && mx < cx[i] + cw[i] + GAP / 2) return i;
    return -1;
}

int om_table_event(struct om_table *t, ULONG cls, UWORD code, APTR ia, int mx, int my, ULONG secs, ULONG micros,
                   int *picked, int *row, int *sorted)
{
    int r, idx = -1;
    *picked = 0; *sorted = 0;
    if (!t->list) return 0;
    if (cls == IDCMP_MOUSEBUTTONS && mx >= t->x && mx < t->x + t->w && my >= t->y && my < t->y + t->head_h) {
        int c = header_col(t, mx);
        if (code == SELECTDOWN && c >= 0) {
            if (c == t->sort) t->desc = !t->desc;
            else { t->sort = c; t->desc = t->cols[c].sort_desc; }
            cols_to_prefs(t);
            draw_header(t);
            *sorted = 1;
            return 1;
        }
        if (code == MENUDOWN) {
            if (om_table_choose_columns(t)) om_table_draw(t);
            return 1;
        }
        return 1;
    }
    r = ogt_list_event(t->list, cls, code, ia, mx, my, secs, micros, &idx);
    if (r == OGT_LIST_PICKED || r == OGT_LIST_OPENED) {
        *picked = r == OGT_LIST_OPENED ? 2 : 1;
        *row = idx;
        return 1;
    }
    return r != OGT_LIST_NONE;
}

int om_table_choose_columns(struct om_table *t)
{
    struct om_mitem items[34];
    int i, n = 0, a, value = 0;
    memset(items, 0, sizeof items);
    items[n].kind = MI_HEAD; items[n].label = "Columns"; items[n++].key = "a tick for each one shown";
    for (i = 0; i < t->ncols && n < 33; i++) {
        items[n].kind = MI_CHECK; items[n].action = 1; items[n].value = i;
        items[n].ticked = !t->cols[i].hidden; items[n++].label = t->cols[i].title;
    }
    a = om_menu(items, n, &value);
    if (a != 1 || value < 0 || value >= t->ncols || t->cols[value].fixed) return 0;
    t->cols[value].hidden = !t->cols[value].hidden;
    if (t->cols[value].hidden && t->sort == value) { t->sort = 0; t->desc = t->cols[0].sort_desc; }
    cols_to_prefs(t);
    return 1;
}

/* The table as text: each column padded to its width, the header first (r -1). A row's room: each shown
 * column at its width or a whole cell (159), whichever is more, and its gap. */
static ULONG row_room(struct om_table *t)
{
    int i;
    ULONG total = 2;
    for (i = 0; i < t->ncols; i++) if (!t->cols[i].hidden) total += (t->cols[i].chars > 159 ? t->cols[i].chars : 159) + 2;
    return total;
}

static int table_line(struct om_table *t, int r, char *buf)
{
    char *p = buf, cell[160];
    int i;
    for (i = 0; i < t->ncols; i++) {
        LONG pen = 0;
        int w = t->cols[i].chars;
        if (t->cols[i].hidden) continue;
        cell[0] = 0;
        if (r < 0) snprintf(cell, sizeof cell, "%s", t->cols[i].title);
        else t->cell(r, i, cell, sizeof cell, &pen);
        if ((int)strlen(cell) > w) w = strlen(cell);
        p += sprintf(p, t->cols[i].align == OM_LEFT ? "%-*s  " : "%*s  ", w, cell);
    }
    while (p > buf && p[-1] == ' ') p--;
    *p++ = '\n';
    *p = 0;
    return p - buf;
}

int om_table_copy(struct om_table *t)
{
    int r, ok;
    char *buf, *p;
    if (!(buf = p = AllocVec((ULONG)(t->count + 2) * row_room(t) + 64, MEMF_ANY))) return 0;
    for (r = -1; r < t->count; r++) p += table_line(t, r, p);
    ok = om_clip_text(buf);
    FreeVec(buf);
    return ok;
}

int om_table_save(struct om_table *t, const char *path)
{
    BPTR fh;
    char *line = AllocVec(row_room(t) + 8, MEMF_ANY);
    int r, ok = 1;
    if (!line) return 0;
    if (!(fh = Open((STRPTR)path, MODE_NEWFILE))) { FreeVec(line); return 0; }
    for (r = -1; r < t->count && ok; r++) {
        LONG n = table_line(t, r, line);
        ok = Write(fh, line, n) == n;
    }
    Close(fh);
    FreeVec(line);
    return ok;
}

/* ---- sorting and finding --------------------------------------------------------------------------- */

static struct om_table *sort_t;
static om_cell_fn sort_raw;

/* a cell's number: "$" then hex, else digits with commas and a fraction, then KB or MB; none: 0, and *num 0 */
static unsigned long long cell_number(const char *s, int *num)
{
    unsigned long long v = 0, frac = 0, div = 1;
    int any = 0, neg = 0;
    while (*s == ' ') s++;
    if (*s == '-') { neg = 1; s++; }
    if (*s == '$') {
        for (s++; *s; s++) {
            int d = *s >= '0' && *s <= '9' ? *s - '0' : *s >= 'a' && *s <= 'f' ? *s - 'a' + 10 : *s >= 'A' && *s <= 'F' ? *s - 'A' + 10 : -1;
            if (d < 0) break;
            v = v * 16 + d; any = 1;
        }
    } else {
        for (; *s; s++) {
            if (*s >= '0' && *s <= '9') { v = v * 10 + (*s - '0'); any = 1; }
            else if (*s != ',') break;
        }
        if (*s == '.') for (s++; *s >= '0' && *s <= '9'; s++) if (div < 1000) { frac = frac * 10 + (*s - '0'); div *= 10; }
        while (*s == ' ') s++;
        if (s[0] == 'K' && s[1] == 'B') { v = v * 1024 + frac * 1024 / div; }
        else if (s[0] == 'M' && s[1] == 'B') { v = v * 1048576 + frac * 1048576 / div; }
        else if (s[0] == 'G' && s[1] == 'B') { v = v * 1073741824ULL + frac * 1073741824ULL / div; }
    }
    *num = any && !neg;
    return any && !neg ? v : 0;
}

static int lower(int c) { return c >= 'A' && c <= 'Z' ? c + 32 : c; }

static int text_cmp(const char *a, const char *b)
{
    for (; *a && lower(*a) == lower(*b); a++, b++) {}
    return lower(*a) - lower(*b);
}

static int sort_cmp(const void *pa, const void *pb)
{
    int a = *(const int *)pa, b = *(const int *)pb, c, col = sort_t->sort;
    char ta[160], tb[160];
    LONG pen = 0;
    if (a == b) return 0;
    ta[0] = tb[0] = 0;
    sort_raw(a, col, ta, sizeof ta, &pen);
    sort_raw(b, col, tb, sizeof tb, &pen);
    if (sort_t->cols[col].align == OM_RIGHT) {
        int na, nb;
        unsigned long long va = cell_number(ta, &na), vb = cell_number(tb, &nb);
        c = na != nb ? na - nb : va < vb ? -1 : va > vb ? 1 : 0;
    } else
        c = text_cmp(ta, tb);
    if (sort_t->desc) c = -c;
    return c ? c : a - b;
}

void om_table_sort(struct om_table *t, om_cell_fn raw, int *order, int n)
{
    if (t->sort < 0 || t->sort >= t->ncols || n < 2) return;
    sort_t = t;
    sort_raw = raw;
    qsort(order, n, sizeof *order, sort_cmp);
}

int om_contains(const char *s, const char *find)
{
    int n = strlen(find), i;
    if (!n) return 1;
    for (; *s; s++) {
        for (i = 0; i < n && s[i] && lower(s[i]) == lower(find[i]); i++) {}
        if (i == n) return 1;
    }
    return 0;
}

/* ---- the pop-up menu: borderless windows at the pointer (Intuition's menus open only in the screen's title
 * bar). Held: release on an entry. Clicked: it stays open and the next click chooses; a click anywhere else,
 * or Esc, closes it. ----------------------------------------------------------------------------------------- */

#define MAXMI 34
#define PANEL_IDCMP (IDCMP_MOUSEMOVE | IDCMP_MOUSEBUTTONS | IDCMP_INACTIVEWINDOW | IDCMP_VANILLAKEY | IDCMP_RAWKEY)

struct panel {
    struct Window *w;
    int shared;
    const struct om_mitem *it;
    int n, hl, width, height;
    int top[MAXMI], h[MAXMI];
};

static int item_h(const struct om_mitem *m)
{
    return m->kind == MI_HEAD ? 2 * U.fh + 10 : m->kind == MI_SEP ? 7 : U.fh + 6;
}

static int selectable(const struct om_mitem *m)
{
    return m->kind == MI_ITEM || m->kind == MI_SUB || m->kind == MI_DANGER || m->kind == MI_CHECK;
}

static void tick_mark(struct RastPort *rp, int y, int h)
{
    int cy = y + h / 2;
    Move(rp, 7, cy); Draw(rp, 10, cy + 3); Draw(rp, 16, cy - 3);
    Move(rp, 7, cy + 1); Draw(rp, 10, cy + 4); Draw(rp, 16, cy - 2);
}

static void panel_draw(struct panel *p)
{
    struct RastPort *rp = p->w->RPort;
    int i, w = p->width, h = p->height;
    SetFont(rp, U.font);
    SetDrMd(rp, JAM1);
    ogt_box(rp, U.pen_back, 0, 0, w, h);
    ogt_bevel(rp, U.pen_shine, U.pen_shadow, 0, 0, w, h);
    for (i = 0; i < p->n; i++) {
        const struct om_mitem *m = &p->it[i];
        int y = p->top[i], on = i == p->hl, ty = y + 3;
        LONG pen;
        if (m->kind == MI_SEP) { ogt_hline(rp, U.pen_shadow, 6, y + 3, w - 12); continue; }
        if (m->kind == MI_HEAD) {
            ogt_bold(rp, 1);
            ogt_text(rp, U.pen_text, 10, ty + 1, m->label, w - 20);
            ogt_bold(rp, 0);
            ogt_text(rp, U.pen_muted, 10, ty + 3 + U.fh, m->key, w - 20);
            ogt_hline(rp, U.pen_shadow, 4, y + p->h[i] - 2, w - 8);
            continue;
        }
        if (on) ogt_fill(&U.ctx, rp, "fill", 3, y, w - 6, p->h[i]);
        pen = on ? U.pen_filltext : m->kind == MI_DANGER ? U.pen_danger : U.pen_text;
        if (m->ticked) { ogt_set_apen(rp, pen); tick_mark(rp, y, p->h[i]); }
        ogt_text(rp, pen, 22, ty, m->label, w - 40);
        if (m->kind == MI_SUB) {
            int cy = y + p->h[i] / 2, ax = w - 12;
            ogt_set_apen(rp, pen);
            Move(rp, ax, cy - 4); Draw(rp, ax + 4, cy); Draw(rp, ax, cy + 4);
            Move(rp, ax + 1, cy - 4); Draw(rp, ax + 5, cy); Draw(rp, ax + 1, cy + 4);
        }
        if (m->key) {
            int kw = om_text_width(U.font, m->key);
            ogt_text(rp, on ? pen : U.pen_muted, w - (m->kind == MI_SUB ? 22 : 10) - kw, ty, m->key, 0);
        }
    }
}

static int panel_open(struct panel *p, int x, int y, int beside, int active, struct MsgPort *share)
{
    struct Screen *scr = U.scr;
    int i, w = 150, kw;
    p->height = 3;
    for (i = 0; i < p->n && i < MAXMI; i++) {
        const struct om_mitem *m = &p->it[i];
        p->top[i] = p->height;
        p->h[i] = item_h(m);
        p->height += p->h[i];
        if (m->label) {
            int lw = om_text_width(U.font, m->label);
            kw = m->key && m->kind != MI_HEAD ? om_text_width(U.font, m->key) + 24 : 0;
            if (m->kind == MI_SUB) kw += 20;
            if (lw + 40 + kw > w) w = lw + 40 + kw;
        }
        if (m->kind == MI_HEAD && m->key && om_text_width(U.font, m->key) + 24 > w) w = om_text_width(U.font, m->key) + 24;
    }
    p->n = i;
    p->height += 3;
    p->width = w;
    p->hl = -1;
    if (x + w > scr->Width) x = beside >= 0 ? beside - w + 4 : scr->Width - w;
    if (x < 0) x = 0;
    if (y + p->height > scr->Height) y = scr->Height - p->height;
    if (y < 0) y = 0;
    p->w = OpenWindowTags(NULL, WA_PubScreen, (ULONG)scr, WA_Left, x, WA_Top, y, WA_Width, w, WA_Height, p->height,
                          WA_Borderless, TRUE, WA_Activate, (ULONG)active, WA_RMBTrap, TRUE, WA_ReportMouse, TRUE,
                          WA_SmartRefresh, TRUE, WA_NoCareRefresh, TRUE, WA_IDCMP, share ? 0 : PANEL_IDCMP, TAG_DONE);
    p->shared = share != NULL;
    if (p->w && share) {
        p->w->UserPort = share;
        ModifyIDCMP(p->w, PANEL_IDCMP);
    }
    return p->w != NULL;
}

static void panel_close(struct panel *p)
{
    if (!p->w) return;
    if (p->shared) {
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

/* the next entry that can be chosen from i, going d (1 or -1); -1 when none */
static int panel_step(const struct panel *p, int i, int d)
{
    int k;
    for (k = 0; k < p->n; k++) {
        i += d;
        if (i < 0) i = p->n - 1;
        if (i >= p->n) i = 0;
        if (selectable(&p->it[i])) return i;
    }
    return -1;
}

static int panel_hit(const struct panel *p, int sx, int sy)
{
    int i, y;
    if (!panel_inside(p, sx, sy)) return -1;
    y = sy - p->w->TopEdge;
    for (i = 0; i < p->n; i++) if (y >= p->top[i] && y < p->top[i] + p->h[i]) return selectable(&p->it[i]) ? i : -1;
    return -1;
}

int om_menu(const struct om_mitem *items, int n, int *value)
{
    struct panel m, sb;
    struct Screen *scr = U.scr;
    int ox = scr->MouseX, oy = scr->MouseY, sub_of = -1, result = 0, done = 0, sticky = 0, moved = 0;
    memset(&m, 0, sizeof m);
    memset(&sb, 0, sizeof sb);
    m.it = items; m.n = n;
    if (!panel_open(&m, ox + 2, oy + 2, -1, 1, NULL)) return 0;
    panel_draw(&m);
    while (!done) {
        struct IntuiMessage *im;
        ULONG tsig = om_timer_sig();
        if (Wait((1UL << m.w->UserPort->mp_SigBit) | tsig) & tsig) om_timer_due();
        while (!done && (im = (struct IntuiMessage *)GetMsg(m.w->UserPort))) {
            ULONG cls = im->Class;
            UWORD code = im->Code;
            int sx = scr->MouseX, sy = scr->MouseY, hm, hs;
            ReplyMsg((struct Message *)im);
            if (sx - ox > 4 || ox - sx > 4 || sy - oy > 4 || oy - sy > 4) moved = 1;
            if (cls == IDCMP_VANILLAKEY && code == 27) { done = 1; break; }
            /* the keys: up and down move, right opens a sub-menu, left goes back, Return chooses */
            if ((cls == IDCMP_RAWKEY && !(code & 0x80)) || (cls == IDCMP_VANILLAKEY && code == 13)) {
                struct panel *a = sb.w && sb.hl >= 0 ? &sb : &m;
                int k = cls == IDCMP_VANILLAKEY ? 0x44 : code;
                if (k == 0x4C || k == 0x4D) {
                    int to = panel_step(a, a->hl < 0 ? (k == 0x4D ? -1 : a->n) : a->hl, k == 0x4D ? 1 : -1);
                    if (to >= 0) {
                        a->hl = to;
                        panel_draw(a);
                        if (a == &m && sub_of >= 0 && sub_of != to) { panel_close(&sb); sub_of = -1; }
                    }
                } else if (k == 0x4E && a == &m && m.hl >= 0 && m.it[m.hl].kind == MI_SUB) {
                    if (sub_of != m.hl) {
                        if (sub_of >= 0) panel_close(&sb);
                        sb.it = m.it[m.hl].sub; sb.n = m.it[m.hl].nsub;
                        if (panel_open(&sb, m.w->LeftEdge + m.width - 4, m.w->TopEdge + m.top[m.hl] - 3, m.w->LeftEdge, 0, m.w->UserPort))
                            sub_of = m.hl;
                    }
                    if (sb.w) { sb.hl = panel_step(&sb, -1, 1); panel_draw(&sb); }
                } else if (k == 0x4F && a == &sb) {
                    sb.hl = -1;
                    panel_draw(&sb);
                } else if (k == 0x44 && a->hl >= 0 && a->it[a->hl].kind != MI_SUB) {
                    result = a->it[a->hl].action;
                    *value = a->it[a->hl].value;
                    done = 1;
                    break;
                }
                continue;
            }
            if (cls == IDCMP_INACTIVEWINDOW && !panel_inside(&m, sx, sy) && !panel_inside(&sb, sx, sy)) { done = 1; break; }
            hs = panel_hit(&sb, sx, sy);
            hm = panel_hit(&m, sx, sy);
            if (panel_inside(&sb, sx, sy)) {
                if (hs != sb.hl) { sb.hl = hs; panel_draw(&sb); }
            } else if (panel_inside(&m, sx, sy)) {
                if (sb.w && sb.hl >= 0) { sb.hl = -1; panel_draw(&sb); }
                if (hm != m.hl && hm >= 0) {
                    m.hl = hm;
                    panel_draw(&m);
                    if (sub_of >= 0 && sub_of != hm) { panel_close(&sb); sub_of = -1; }
                    if (m.it[hm].kind == MI_SUB && sub_of != hm) {
                        sb.it = m.it[hm].sub; sb.n = m.it[hm].nsub;
                        if (panel_open(&sb, m.w->LeftEdge + m.width - 4, m.w->TopEdge + m.top[hm] - 3, m.w->LeftEdge, 0, m.w->UserPort)) {
                            sub_of = hm;
                            panel_draw(&sb);
                        }
                    }
                }
            }
            if (cls == IDCMP_MOUSEBUTTONS) {
                int up = code == MENUUP || code == SELECTUP, down = code == MENUDOWN || code == SELECTDOWN;
                const struct om_mitem *pick = NULL;
                if (sb.w && sb.hl >= 0 && panel_inside(&sb, sx, sy)) pick = &sb.it[sb.hl];
                else if (m.hl >= 0 && hm == m.hl && m.it[m.hl].kind != MI_SUB) pick = &m.it[m.hl];
                if (up && !sticky && !moved) sticky = 1;
                else if ((up && !sticky) || (down && sticky)) {
                    if (pick) { result = pick->action; *value = pick->value; done = 1; }
                    else if (!panel_inside(&m, sx, sy) && !panel_inside(&sb, sx, sy)) done = 1;
                    else if (up) done = 1;
                }
            }
        }
    }
    panel_close(&sb);
    panel_close(&m);
    if (U.win) ActivateWindow(U.win);
    return result;
}
