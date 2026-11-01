/* GFOpenMonitor: the Libraries view (the design, section 4.4).
 *
 * Four lists behind four buttons: exec's libraries, devices and resources, and its ROM modules (ResModules).
 * Name, version, open count, base, where it came from and the ID string; for a module its priority, type and
 * address instead of the open count and base.
 *
 * Where: the code behind the library's Open vector (the JMP at -6) is in the ROM (Kickstart's places, or the
 * open ROM's image that romunpack unpacked at boot), in a group of the ROM's unpacked on demand, or elsewhere.
 * Elsewhere and the ROM has a RomTag of the same name and version (a stub of a file on demand), it came from
 * the ROM's groups; else from the disk.
 *
 * A resource that isn't a Library (FileSystem.resource, keymap.resource, rom.volume, romunpack) shows "-" for
 * its version, as ShowConfig does; its fields are never read as a library's.
 *
 * Flush unused: RemLibrary and RemDevice on those open 0 times (each may refuse). Show the file: the copy on the
 * disk (LIBS: or DEVS:) and its $VER line, against the one in memory (the newer version wins).
 *
 * Read every 5 seconds while shown, under Forbid, into a copy that is then drawn.
 *
 * MIT, Copyright (c) 2026 AmigaChrome Team. */

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <dos/dos.h>
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

#define MAXL 192

enum { K_LIBS, K_DEVS, K_RES, K_MODS, K_N };
static const char *const kind_name[K_N] = { "Libraries", "Devices", "Resources", "ROM modules" };

enum { W_ROM, W_GROUP, W_DISK, W_RAM, W_NONE };
static const char *const where_name[] = { "ROM", "ROM group", "disk", "RAM", "-" };

struct lrow {
    APTR addr;
    char name[40];
    char id[88];
    UWORD ver, rev, open;
    UBYTE plain, where, type, flags;
    BYTE pri;
    UWORD negsize, possize;
};
static struct lrow *rows;               /* allocated: the view's a4 data can't hold them */
static int nrows, counts[K_N];
static int *order, norder;
static int kind;
static char find[40];

/* the libraries' columns, and the modules' */
enum { C_NAME, C_VER, C_OPEN, C_BASE, C_WHERE, C_ID, C_N };
static struct om_col cols[C_N] = {
    { "Name", 26, OM_LEFT, 0, 1, 0 },
    { "Version", 7, OM_RIGHT, 0, 0, 1 },
    { "Open", 4, OM_RIGHT, 0, 0, 1 },
    { "Base", 8, OM_LEFT, 0, 0, 0 },
    { "Where", 9, OM_LEFT, 0, 0, 0 },
    { "ID string", 40, OM_LEFT, 0, 0, 0 },
};
enum { M_NAME, M_VER, M_PRI, M_TYPE, M_ADDR, M_WHERE, M_ID, M_N };
static struct om_col mcols[M_N] = {
    { "Name", 26, OM_LEFT, 0, 1, 0 },
    { "Version", 7, OM_RIGHT, 0, 0, 1 },
    { "Pri", 4, OM_RIGHT, 0, 0, 1 },
    { "Type", 9, OM_LEFT, 0, 0, 0 },
    { "Address", 8, OM_LEFT, 0, 0, 0 },
    { "Where", 9, OM_LEFT, 0, 0, 0 },
    { "ID string", 40, OM_LEFT, 0, 0, 0 },
};
static struct om_table T;

enum { G_FLUSH, G_FILE, G_COPY, G_FIND, G_COUNT };
static struct Gadget *gad[G_COUNT];
static int gx[G_COUNT], gw[G_COUNT], gy, gh, seg_x[K_N], seg_w[K_N], seg_y, seg_h, det_y, det_h, ticks;

/* the details strip's second line: the disk's copy, read when a row is chosen (not each time it is drawn) */
static char disk_line[120];
static APTR disk_for;

/* ---- reading ------------------------------------------------------------------------------------------ */

static void copy_str(char *to, const char *from, int size)
{
    int i;
    if (!from) { to[0] = 0; return; }
    for (i = 0; i < size - 1 && from[i] && from[i] != '\r' && from[i] != '\n'; i++) to[i] = from[i];
    to[i] = 0;
}

/* resources that are not Library structures (as ShowConfig keeps them) */
static int plain_resource(const char *name)
{
    static const char *const plain[] = { "FileSystem.resource", "keymap.resource", "rom.volume", "romunpack" };
    unsigned i;
    for (i = 0; i < sizeof plain / sizeof plain[0]; i++) if (name && !strcmp(name, plain[i])) return 1;
    return 0;
}

static int where_of(struct Library *l)
{
    APTR code = NULL;
    const char *w;
    struct Resident *rt;
    UWORD *jmp = (UWORD *)((UBYTE *)l - 6);
    if (l->lib_NegSize >= 6 && jmp[0] == 0x4ef9) code = *(APTR *)(jmp + 1);
    w = om_where(code ? code : (APTR)l);
    if (!strcmp(w, "ROM")) return W_ROM;
    if (!strcmp(w, "ROM group")) return W_GROUP;
    /* elsewhere: a ROM RomTag of the same name and version is a stub of a file on demand, loaded from the ROM */
    if (l->lib_Node.ln_Name && (rt = FindResident((STRPTR)l->lib_Node.ln_Name)) && rt->rt_Version == l->lib_Version &&
        strcmp(om_where(rt), "RAM")) return W_GROUP;
    return W_DISK;
}

static void take_list(struct List *list, int k)
{
    struct Node *n;
    for (n = list->lh_Head; n->ln_Succ && nrows < MAXL; n = n->ln_Succ) {
        struct lrow *r = &rows[nrows];
        struct Library *l = (struct Library *)n;
        counts[k]++;
        if (kind != k) continue;
        memset(r, 0, sizeof *r);
        r->addr = n;
        copy_str(r->name, n->ln_Name ? n->ln_Name : "(no name)", sizeof r->name);
        r->plain = k == K_RES && plain_resource(n->ln_Name);
        if (!r->plain) {
            r->ver = l->lib_Version; r->rev = l->lib_Revision; r->open = l->lib_OpenCnt;
            r->flags = l->lib_Flags; r->negsize = l->lib_NegSize; r->possize = l->lib_PosSize;
            copy_str(r->id, (const char *)l->lib_IdString, sizeof r->id);
            r->where = where_of(l);
        } else
            r->where = om_where(n) && !strcmp(om_where(n), "ROM") ? W_ROM : W_RAM;
        nrows++;
    }
}

static void take_modules(void)
{
    ULONG *p = (ULONG *)SysBase->ResModules;
    int guard = 0;
    while (p && *p && guard++ < 1024) {
        struct Resident *rt;
        if (*p & 0x80000000UL) { p = (ULONG *)(*p & 0x7fffffffUL); continue; }
        rt = (struct Resident *)*p++;
        counts[K_MODS]++;
        if (kind != K_MODS || nrows >= MAXL) continue;
        {
            struct lrow *r = &rows[nrows++];
            const char *w = om_where(rt);
            memset(r, 0, sizeof *r);
            r->addr = rt;
            copy_str(r->name, (const char *)rt->rt_Name, sizeof r->name);
            copy_str(r->id, (const char *)rt->rt_IdString, sizeof r->id);
            r->ver = rt->rt_Version;
            r->pri = rt->rt_Pri;
            r->type = rt->rt_Type;
            r->flags = rt->rt_Flags;
            r->where = !strcmp(w, "ROM") ? W_ROM : !strcmp(w, "ROM group") ? W_GROUP : W_RAM;
        }
    }
}

static void raw_cell(int row, int col, char *buf, int size, LONG *pen);

static void make_order(void)
{
    int i;
    norder = 0;
    for (i = 0; i < nrows; i++) if (om_contains(rows[i].name, find)) order[norder++] = i;
    om_table_sort(&T, raw_cell, order, norder);
}

static void take(void)
{
    int k;
    for (k = 0; k < K_N; k++) counts[k] = 0;
    nrows = norder = 0;
    if (!rows || !order) return;
    Forbid();
    take_list(&SysBase->LibList, K_LIBS);
    take_list(&SysBase->DeviceList, K_DEVS);
    take_list(&SysBase->ResourceList, K_RES);
    take_modules();
    Permit();
    make_order();
}

/* ---- the cells ---------------------------------------------------------------------------------------- */

static const char *type_name(UBYTE t)
{
    switch (t) {
    case NT_LIBRARY: return "library";
    case NT_DEVICE: return "device";
    case NT_RESOURCE: return "resource";
    case NT_TASK: return "task";
    case NT_PROCESS: return "process";
    case 0: return "-";
    default: return "other";
    }
}

static void raw_cell(int row, int col, char *buf, int size, LONG *pen)
{
    struct lrow *r;
    if (row < 0 || row >= nrows) return;
    r = &rows[row];
    if (kind == K_MODS) {
        switch (col) {
        case M_NAME: snprintf(buf, size, "%s", r->name); break;
        case M_VER: snprintf(buf, size, "%u", r->ver); break;
        case M_PRI: snprintf(buf, size, "%d", r->pri); break;
        case M_TYPE: snprintf(buf, size, "%s", type_name(r->type)); break;
        case M_ADDR: snprintf(buf, size, "%08lx", (unsigned long)r->addr); *pen = U.pen_muted; break;
        case M_WHERE: snprintf(buf, size, "%s", where_name[r->where]); if (r->where == W_RAM) *pen = U.pen_amber; break;
        case M_ID: snprintf(buf, size, "%s", r->id); break;
        }
        return;
    }
    switch (col) {
    case C_NAME: snprintf(buf, size, "%s", r->name); break;
    case C_VER: if (r->plain) snprintf(buf, size, "-"); else snprintf(buf, size, "%u.%u", r->ver, r->rev); break;
    case C_OPEN: if (r->plain) snprintf(buf, size, "-"); else snprintf(buf, size, "%u", r->open); break;
    case C_BASE: snprintf(buf, size, "%08lx", (unsigned long)r->addr); *pen = U.pen_muted; break;
    case C_WHERE: snprintf(buf, size, "%s", where_name[r->where]); if (r->where == W_DISK) *pen = U.pen_muted; break;
    case C_ID: snprintf(buf, size, "%s", r->plain ? "(not a library: no version)" : r->id); if (r->plain) *pen = U.pen_muted; break;
    }
}

static void cell(int row, int col, char *buf, int size, LONG *pen)
{
    if (row >= 0 && row < norder) raw_cell(order[row], col, buf, size, pen);
}

static struct lrow *chosen(void)
{
    int s = om_table_selected(&T);
    return s >= 0 && s < norder ? &rows[order[s]] : NULL;
}

/* ---- the disk's copy ---------------------------------------------------------------------------------- */

/* The file's $VER line's version and revision: 1 when found. */
static int file_version(const char *path, int *ver, int *rev)
{
    BPTR fh = Open((STRPTR)path, MODE_OLDFILE);
    UBYTE *buf;
    LONG n, keep = 0, total = 0;
    int found = 0;
    if (!fh) return -1;
    if (!(buf = AllocVec(4096 + 64, MEMF_ANY))) { Close(fh); return 0; }
    while (!found && total < 4 * 1024 * 1024 && (n = Read(fh, buf + keep, 4096)) > 0) {
        LONG i, len = keep + n;
        total += n;
        for (i = 0; i + 6 < len && !found; i++)
            if (buf[i] == '$' && !memcmp(buf + i, "$VER: ", 6)) {
                LONG j = i + 6;
                char line[64];
                int k = 0;
                if (len - j < 48) {             /* the line may go on in the next piece: read it whole first */
                    LONG more = Read(fh, buf + len, 48);
                    if (more > 0) len += more;
                }
                while (j < len && k < (int)sizeof line - 1 && buf[j] && buf[j] != '\n' && buf[j] != '\r') line[k++] = buf[j++];
                line[k] = 0;
                {
                    char *p = strchr(line, ' ');
                    if (p && sscanf(p + 1, "%d.%d", ver, rev) >= 1) found = 1;
                }
            }
        keep = len > 48 ? 48 : len;             /* the last bytes again: a "$VER: " across two pieces */
        memmove(buf, buf + len - keep, keep);
    }
    FreeVec(buf);
    Close(fh);
    return found;
}

static void read_disk_copy(struct lrow *r)
{
    char path[64];
    int v = 0, rv = 0, got;
    disk_for = r ? r->addr : NULL;
    disk_line[0] = 0;
    if (!r || r->plain || (kind != K_LIBS && kind != K_DEVS)) return;
    snprintf(path, sizeof path, "%s%s", kind == K_DEVS ? "DEVS:" : "LIBS:", r->name);
    got = file_version(path, &v, &rv);
    if (got < 0) {
        if (r->where == W_DISK) snprintf(disk_line, sizeof disk_line, "%s isn't on the disk now (it came from another drawer, or was moved)", path);
        else snprintf(disk_line, sizeof disk_line, "%s: none on the disk, so the ROM's runs", path);
    } else if (!got)
        snprintf(disk_line, sizeof disk_line, "%s on the disk: no version in it", path);
    else if (v > r->ver || (v == r->ver && rv > r->rev))
        snprintf(disk_line, sizeof disk_line, "%s on the disk: %d.%d, newer than the one running: it runs from the next start", path, v, rv);
    else if (v == r->ver && rv == r->rev)
        snprintf(disk_line, sizeof disk_line, "%s on the disk: %d.%d, the one running", path, v, rv);
    else
        snprintf(disk_line, sizeof disk_line, "%s on the disk: %d.%d, older: the %s runs (the newer version wins)", path, v, rv,
                 r->where == W_ROM || r->where == W_GROUP ? "ROM's" : "newer");
}

/* ---- drawing ------------------------------------------------------------------------------------------ */

static void draw_segments(void)
{
    struct RastPort *rp = U.win->RPort;
    int k;
    char t[32];
    SetFont(rp, U.font);
    for (k = 0; k < K_N; k++) {
        int on = k == kind, tw;
        if (U.compact) snprintf(t, sizeof t, "%s", k == K_MODS ? "Modules" : kind_name[k]);
        else snprintf(t, sizeof t, "%s %d", kind_name[k], counts[k]);
        if (on) {
            ogt_fill(&U.ctx, rp, "fill", seg_x[k] + 1, seg_y + 1, seg_w[k] - 2, seg_h - 2);
            ogt_bevel(rp, U.pen_shadow, U.pen_shine, seg_x[k], seg_y, seg_w[k], seg_h);
        } else {
            ogt_box(rp, U.pen_back, seg_x[k] + 1, seg_y + 1, seg_w[k] - 2, seg_h - 2);
            ogt_bevel(rp, U.pen_shine, U.pen_shadow, seg_x[k], seg_y, seg_w[k], seg_h);
        }
        ogt_bold(rp, on);
        tw = om_text_width(U.font, t) + (on ? 1 : 0);
        ogt_text(rp, on ? U.pen_filltext : U.pen_text, seg_x[k] + (seg_w[k] - tw) / 2, seg_y + (seg_h - U.fh) / 2, t, seg_w[k] - 4);
        ogt_bold(rp, 0);
    }
}

static void draw_details(void)
{
    struct RastPort *rp = U.win->RPort;
    struct lrow *r = chosen();
    char s[200];
    int y = det_y + 4;
    if (det_h <= 0) return;
    om_sunk(U.px, det_y, U.pw, det_h);
    if (!r) {
        SetFont(rp, U.font);
        ogt_text(rp, U.pen_muted, U.px + 8, y, "Choose a row: where it came from, the copy on the disk, its sizes.", U.pw - 16);
        return;
    }
    if (r->addr != disk_for) read_disk_copy(r);
    SetFont(rp, U.font);
    ogt_bold(rp, 1);
    if (kind == K_MODS) snprintf(s, sizeof s, "%s %u", r->name, r->ver);
    else if (r->plain) snprintf(s, sizeof s, "%s", r->name);
    else snprintf(s, sizeof s, "%s %u.%u", r->name, r->ver, r->rev);
    ogt_text(rp, U.pen_text, U.px + 8, y, s, U.pw - 16);
    {
        int tw = om_text_width(U.font, s) + 8;
        ogt_bold(rp, 0);
        if (kind == K_MODS)
            snprintf(s, sizeof s, "a %s RomTag in %s, priority %d, flags $%02x", type_name(r->type),
                     r->where == W_ROM ? "the ROM" : r->where == W_GROUP ? "a group of the ROM's" : "RAM (added after the ROM)",
                     r->pri, r->flags);
        else if (r->plain)
            snprintf(s, sizeof s, "a resource that isn't a library: no version or open count, %s", r->where == W_ROM ? "in the ROM" : "in RAM");
        else
            snprintf(s, sizeof s, "from %s, opened %u time%s", r->where == W_ROM ? "the ROM" : r->where == W_GROUP ? "the ROM's groups (on demand)" : "the disk",
                     r->open, r->open == 1 ? "" : "s");
        ogt_text(rp, U.pen_muted, U.px + 8 + tw, y, s, U.pw - 16 - tw);
    }
    if (U.compact) return;
    SetFont(rp, U.tfont);
    y += U.fh + 4;
    if (disk_line[0]) { ogt_text(rp, U.pen_text, U.px + 8, y, disk_line, U.pw - 16); y += U.tfh + 2; }
    if (kind != K_MODS && !r->plain) {
        snprintf(s, sizeof s, "base $%08lx   jump table %u bytes (%u calls)   data %u bytes   flags $%02x%s%s",
                 (unsigned long)r->addr, r->negsize, r->negsize / 6, r->possize, r->flags,
                 r->flags & LIBF_DELEXP ? " (expunge when closed)" : "", r->flags & LIBF_SUMUSED ? " (summed)" : "");
        ogt_text(rp, U.pen_muted, U.px + 8, y, s, U.pw - 16);
    }
}

/* ---- the actions -------------------------------------------------------------------------------------- */

static int still_listed(struct List *l, struct Node *x)
{
    struct Node *n;
    for (n = l->lh_Head; n->ln_Succ; n = n->ln_Succ) if (n == x) return 1;
    return 0;
}

static void flush_unused(void)
{
    struct Node *n, *next;
    struct Library *gone[64];
    int candidates = 0, removed = 0, i;
    Forbid();
    for (n = SysBase->LibList.lh_Head; n->ln_Succ; n = n->ln_Succ) if (!((struct Library *)n)->lib_OpenCnt) candidates++;
    for (n = SysBase->DeviceList.lh_Head; n->ln_Succ; n = n->ln_Succ) if (!((struct Library *)n)->lib_OpenCnt) candidates++;
    Permit();
    if (!candidates) { om_status(0, "Every library and device is open: nothing to flush."); return; }
    if (om_ask("GFOpenMonitor", "Flush|Cancel",
               "Flush the %d libraries and devices nobody has open?\n\n"
               "Each is taken out of memory (one may say not now).\n"
               "It loads again when a program asks for it.", candidates) != 1) {
        om_status(0, "Nothing flushed.");
        return;
    }
    Forbid();
    for (i = 0, n = SysBase->LibList.lh_Head; (next = n->ln_Succ); n = next)
        if (!((struct Library *)n)->lib_OpenCnt && i < 64) { gone[i++] = (struct Library *)n; RemLibrary((struct Library *)n); }
    for (n = SysBase->DeviceList.lh_Head; (next = n->ln_Succ); n = next)
        if (!((struct Library *)n)->lib_OpenCnt && i < 64) { gone[i++] = (struct Library *)n; RemDevice((struct Device *)n); }
    {
        int k;
        for (k = 0; k < i; k++)
            if (!still_listed(&SysBase->LibList, &gone[k]->lib_Node) && !still_listed(&SysBase->DeviceList, &gone[k]->lib_Node)) removed++;
    }
    Permit();
    om_status(0, "Flushed %d of %d; %s", removed, i, removed == i ? "they load again when asked." : "the others said not now.");
    disk_for = NULL;
    take();
}

static void show_file(void)
{
    struct lrow *r = chosen();
    if (!r) { om_status(0, "Choose a row first."); return; }
    if (kind == K_RES || kind == K_MODS) {
        om_status(0, "%s has no file of its own: it is %s.", r->name, r->where == W_ROM ? "in the ROM" : "in memory");
        return;
    }
    read_disk_copy(r);
    om_ask("GFOpenMonitor", "OK", "%s %u.%u, running: from %s.\n\n%s", r->name, r->ver, r->rev,
           r->where == W_ROM ? "the ROM" : r->where == W_GROUP ? "the ROM's groups (on demand)" : "the disk",
           disk_line[0] ? disk_line : "No copy on the disk.");
}

/* ---- the view ----------------------------------------------------------------------------------------- */

static void layout(int x, int y, int w, int h)
{
    int bh = U.fh + 8, k, sx = x, th;
    seg_y = y;
    seg_h = U.fh + (U.compact ? 6 : 10);
    for (k = 0; k < K_N; k++) {
        char t[32];
        if (U.compact) snprintf(t, sizeof t, "%s", k == K_MODS ? "Modules" : kind_name[k]);
        else snprintf(t, sizeof t, "%s 000", kind_name[k]);
        seg_x[k] = sx;
        seg_w[k] = om_text_width(U.font, t) + (U.compact ? 12 : 24);
        sx += seg_w[k] + (U.compact ? 4 : 8);
    }
    gw[G_FIND] = U.compact ? 120 : 220;
    gx[G_FIND] = x + w - gw[G_FIND];
    if (gx[G_FIND] - (U.compact ? 0 : om_text_width(U.font, "Find") + 8) < sx + 8) gx[G_FIND] = -1;   /* no room */
    det_h = U.compact ? U.fh + 8 : U.fh + 2 * (U.tfh + 2) + 14;
    gy = y + h - bh;
    gh = bh;
    th = gy - (U.compact ? 4 : 10) - det_h - (U.compact ? 3 : 8) - (seg_y + seg_h + (U.compact ? 4 : 10));
    om_table_layout(&T, x, seg_y + seg_h + (U.compact ? 4 : 10), w, th);
    det_y = T.y + T.h + (U.compact ? 3 : 8);
    gw[G_FLUSH] = om_text_width(U.font, U.compact ? "Flush" : "Flush unused (open 0)") + 30;
    gw[G_FILE] = om_text_width(U.font, U.compact ? "File" : "Show the file") + 30;
    gw[G_COPY] = om_text_width(U.font, U.compact ? "Copy" : "Copy as text") + 30;
    gx[G_FLUSH] = x;
    gx[G_FILE] = x + gw[G_FLUSH] + 8;
    gx[G_COPY] = x + w - gw[G_COPY];
}

static struct Gadget *gadgets(struct Gadget *g)
{
    static const char *const big[3] = { "Flush unused (open 0)", "Show the file", "Copy as text" };
    static const char *const small[3] = { "Flush", "File", "Copy" };
    struct NewGadget ng;
    int i;
    memset(gad, 0, sizeof gad);
    memset(&ng, 0, sizeof ng);
    ng.ng_TextAttr = &U.ta; ng.ng_VisualInfo = U.vi; ng.ng_Flags = PLACETEXT_IN;
    for (i = 0; i < 3 && g; i++) {
        ng.ng_LeftEdge = gx[i]; ng.ng_TopEdge = gy; ng.ng_Width = gw[i]; ng.ng_Height = gh;
        ng.ng_GadgetText = (STRPTR)(U.compact ? small[i] : big[i]); ng.ng_GadgetID = GID_LIBS + i;
        g = gad[i] = CreateGadget(BUTTON_KIND, g, &ng, GA_Disabled, (ULONG)(i == G_FLUSH && kind != K_LIBS && kind != K_DEVS), TAG_DONE);
    }
    if (g && gx[G_FIND] >= 0) {
        ng.ng_LeftEdge = gx[G_FIND]; ng.ng_TopEdge = seg_y + (seg_h - (U.fh + 6)) / 2; ng.ng_Width = gw[G_FIND]; ng.ng_Height = U.fh + 6;
        ng.ng_GadgetText = NULL; ng.ng_GadgetID = GID_LIBS + G_FIND;
        g = gad[G_FIND] = CreateGadget(STRING_KIND, g, &ng, GTST_String, (ULONG)find, GTST_MaxChars, sizeof find - 1, TAG_DONE);
    }
    return g;
}

static void draw(void)
{
    struct RastPort *rp = U.win->RPort;
    take();
    draw_segments();
    if (gad[G_FIND] && !find[0] && !U.compact) {
        SetFont(rp, U.font);
        ogt_text(rp, U.pen_muted, gx[G_FIND] - om_text_width(U.font, "Find") - 8, seg_y + (seg_h - U.fh) / 2, "Find", 0);
    }
    om_table_set_count(&T, norder);
    om_table_draw(&T);
    draw_details();
}

static void redraw(void)
{
    om_table_set_count(&T, norder);
    om_table_draw_rows(&T);
    draw_details();
}

static void tick(void)
{
    if (gad[G_FIND]) {
        char *s = NULL;
        GT_GetGadgetAttrs(gad[G_FIND], U.win, NULL, GTST_String, (ULONG)&s, TAG_DONE);
        if (s && strcmp(s, find)) { strncpy(find, s, sizeof find - 1); find[sizeof find - 1] = 0; make_order(); redraw(); return; }
    }
    if (++ticks < 5) return;                    /* the lists every 5 seconds */
    ticks = 0;
    {
        APTR sel = chosen() ? chosen()->addr : NULL;
        int i;
        take();
        for (i = 0; i < norder; i++) if (rows[order[i]].addr == sel) break;
        if (T.list) T.list->selected = i < norder ? i : -1;
    }
    draw_segments();
    redraw();
}

static void status(void)
{
    int i, w[5] = { 0, 0, 0, 0, 0 };
    for (i = 0; i < nrows; i++) w[rows[i].where]++;
    if (kind == K_MODS)
        om_status(1, "%d modules: %d in the ROM, %d in groups, %d in RAM", nrows, w[W_ROM], w[W_GROUP], w[W_RAM]);
    else if (kind == K_RES)
        om_status(1, "%d resources", nrows);
    else
        om_status(1, "%d %s: %d ROM, %d ROM groups, %d disk", nrows,
                  kind == K_LIBS ? "libraries" : "devices", w[W_ROM], w[W_GROUP], w[W_DISK]);
    om_status(2, "Every 5 seconds");
}

static void set_kind(int k)
{
    if (k == kind || k < 0 || k >= K_N) return;
    om_table_free(&T);
    kind = k;
    if (k == K_MODS) { T.sort = -1; om_table_init(&T, mcols, M_N, cell, GID_LIBS + 50, "Modules"); }
    else { T.sort = -1; om_table_init(&T, cols, C_N, cell, GID_LIBS + 50, "Libraries"); }
    om_set_pref("Libraries.List", kind_name[k]);
    disk_for = NULL;
    ticks = 0;
    om_rebuild_gadgets();                       /* the list again, Flush on or off, everything drawn */
    status();
}

static int event(ULONG cls, UWORD code, UWORD qual, APTR ia, int mx, int my, ULONG secs, ULONG micros)
{
    int picked = 0, row = -1, sorted = 0, k;
    (void)qual;
    if (cls == IDCMP_MOUSEBUTTONS && code == SELECTDOWN && my >= seg_y && my < seg_y + seg_h)
        for (k = 0; k < K_N; k++)
            if (mx >= seg_x[k] && mx < seg_x[k] + seg_w[k]) { set_kind(k); return 1; }
    if (om_table_event(&T, cls, code, ia, mx, my, secs, micros, &picked, &row, &sorted)) {
        if (sorted) { make_order(); redraw(); }
        if (picked) { draw_details(); if (picked == 2) show_file(); }
        return 1;
    }
    if (cls == IDCMP_GADGETUP && ia) {
        switch (((struct Gadget *)ia)->GadgetID - GID_LIBS) {
        case G_FLUSH: flush_unused(); redraw(); draw_segments(); status(); return 1;
        case G_FILE: show_file(); return 1;
        case G_COPY: om_status(0, om_table_copy(&T) ? "The list is on the clipboard." : "The clipboard could not be written."); return 1;
        case G_FIND: {
            char *s = NULL;
            GT_GetGadgetAttrs(gad[G_FIND], U.win, NULL, GTST_String, (ULONG)&s, TAG_DONE);
            if (s) { strncpy(find, s, sizeof find - 1); find[sizeof find - 1] = 0; }
            make_order(); redraw();
        } return 1;
        }
    }
    if (cls == IDCMP_RAWKEY && (code == 0x4C || code == 0x4D)) {
        if (ogt_list_step(T.list, code == 0x4C ? -1 : 1) >= 0) draw_details();
        return 1;
    }
    if (cls == IDCMP_RAWKEY && (code == 0x4E || code == 0x4F)) {           /* right and left: the next list */
        set_kind((kind + (code == 0x4E ? 1 : K_N - 1)) % K_N);
        return 1;
    }
    if (cls == IDCMP_VANILLAKEY) {
        switch (code) {
        case 'c': case 'C': om_status(0, om_table_copy(&T) ? "The list is on the clipboard." : "The clipboard could not be written."); return 1;
        case 13: show_file(); return 1;
        case 'f': case 'F': if (kind == K_LIBS || kind == K_DEVS) { flush_unused(); redraw(); draw_segments(); status(); } return 1;
        case 'k': case 'K': if (om_table_choose_columns(&T)) om_table_draw(&T); return 1;
        }
    }
    return 0;
}

static void shown(int on)
{
    if (!on) { om_table_free(&T); return; }
    ticks = 0;
    disk_for = NULL;
    take();
}

const struct om_view om_libs_view = { "Libraries", layout, gadgets, draw, tick, event, shown, status };

void om_libs_free(void)
{
    om_table_free(&T);
    if (rows) FreeVec(rows);
    if (order) FreeVec(order);
    rows = NULL; order = NULL;
    nrows = norder = 0;
}

void om_libs_init(void)
{
    const char *v = om_pref("Libraries.List");
    int k;
    rows = AllocVec(MAXL * sizeof *rows, MEMF_ANY | MEMF_CLEAR);
    order = AllocVec(MAXL * sizeof *order, MEMF_ANY | MEMF_CLEAR);
    kind = K_LIBS;
    if (v) for (k = 0; k < K_N; k++) if (!strcmp(v, kind_name[k])) kind = k;
    T.sort = -1;
    if (kind == K_MODS) om_table_init(&T, mcols, M_N, cell, GID_LIBS + 50, "Modules");
    else om_table_init(&T, cols, C_N, cell, GID_LIBS + 50, "Libraries");
    find[0] = 0;
}
