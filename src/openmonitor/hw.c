/* GFOpenMonitor: the Hardware view (the design, section 4.7): ShowConfig's part.
 *
 * Processor, Chips, Software, the AmiCradle machine and Memory in boxes, then the expansion boards (name,
 * maker and product, bus, address, size, driver, ROM). What the machine is made of comes from sysprobe
 * (amigachrome-guest common/sysprobe), which ShowConfig and ACInfo read too, so the three say the same thing.
 * Read once when the view is first shown; F5 reads it again.
 *
 * Board details... shows a board's AutoConfig record; Drives... the mounted devices and their sizes; Save a
 * report runs ShowConfig REPORT (ShowConfig's own layout, by ShowConfig itself).
 *
 * MIT, Copyright (c) 2026 AmigaChrome Team. */

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <libraries/configvars.h>
#include <libraries/configregs.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>
#include <dos/dostags.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "om.h"
#include "sysprobe.h"

/* ---- the boxes: lines of "what" and "value" ----------------------------------------------------------- */

enum { B_CPU, B_CHIPS, B_SOFT, B_AC, B_MEM, B_N };
static const char *const box_title[B_N] = { "Processor", "Chips", "Software", "AmiCradle", "Memory" };
#define MAXLN 28
struct line { UBYTE box; char what[12]; char value[64]; };
static struct line *lines;
static int nlines, read_done;

static void add(int box, const char *what, const char *fmt, ...) __attribute__((format(printf, 3, 4)));
static void add(int box, const char *what, const char *fmt, ...)
{
    va_list ap;
    struct line *l;
    if (!lines || nlines >= MAXLN) return;
    l = &lines[nlines++];
    l->box = box;
    strncpy(l->what, what, sizeof l->what - 1);
    l->what[sizeof l->what - 1] = 0;
    va_start(ap, fmt);
    vsnprintf(l->value, sizeof l->value, fmt, ap);
    va_end(ap);
}

/* ---- the boards --------------------------------------------------------------------------------------- */

#define MAXB 24
struct board {
    struct ConfigDev *cd;
    UWORD maker;
    UBYTE product, type, flags, cdflags, z3, rom;
    ULONG addr, size, space;
    char name[36], driver[24];
};
static struct board *boards;
static int nboards, order[MAXB];

enum { C_NAME, C_MAKER, C_BUS, C_ADDR, C_SIZE, C_DRIVER, C_ROM, C_N };
static struct om_col cols[C_N] = {
    { "Board", 28, OM_LEFT, 0, 1, 0 },
    { "Maker / product", 14, OM_LEFT, 0, 0, 0 },
    { "Bus", 9, OM_LEFT, 0, 0, 0 },
    { "Address", 9, OM_LEFT, 0, 0, 0 },
    { "Size", 7, OM_RIGHT, 0, 0, 1 },
    { "Driver", 18, OM_LEFT, 0, 0, 0 },
    { "ROM", 4, OM_LEFT, 0, 0, 0 },
};
static struct om_table T;

enum { G_DETAILS, G_DRIVES, G_REPORT, G_SUMMARY, G_COUNT };
static struct Gadget *gad[G_COUNT];
static int gx[G_COUNT], gw[G_COUNT], gy, gh, box_y, head_y = -1;

static void size_text(ULONG n, char *buf, int size)
{
    if (n >= 0x100000UL && !(n & 0xfffff)) snprintf(buf, size, "%lu MB", (unsigned long)(n >> 20));
    else snprintf(buf, size, "%lu KB", (unsigned long)(n >> 10));
}

static void raw_cell(int row, int col, char *buf, int size, LONG *pen)
{
    struct board *b;
    if (row < 0 || row >= nboards) return;
    b = &boards[row];
    switch (col) {
    case C_NAME: snprintf(buf, size, "%s", b->name); if (b->name[0] == '(') *pen = U.pen_muted; break;
    case C_MAKER: snprintf(buf, size, "$%04X / %u", b->maker, b->product); break;
    case C_BUS: snprintf(buf, size, "%s", b->z3 ? "Zorro III" : "Zorro II"); break;
    case C_ADDR: snprintf(buf, size, "$%08lx", (unsigned long)b->addr); *pen = U.pen_muted; break;
    case C_SIZE: size_text(b->size, buf, size); break;
    case C_DRIVER: snprintf(buf, size, "%s", b->driver[0] ? b->driver : "-"); if (!b->driver[0]) *pen = U.pen_muted; break;
    case C_ROM: snprintf(buf, size, "%s", b->rom ? "yes" : "-"); break;
    }
}

static void cell(int row, int col, char *buf, int size, LONG *pen)
{
    if (row >= 0 && row < nboards) raw_cell(order[row], col, buf, size, pen);
}

/* ---- reading ------------------------------------------------------------------------------------------ */

static void take_boards(void)
{
    struct Library *eb = OpenLibrary((STRPTR)"expansion.library", 36);
    struct ConfigDev *cd = NULL;
    int i;
    nboards = 0;
    if (!eb || !boards) { if (eb) CloseLibrary(eb); return; }
    while ((cd = om_find_board(eb, cd, -1, -1)) && nboards < MAXB) {
        struct board *b = &boards[nboards++];
        const char *drv = "", *name;
        memset(b, 0, sizeof *b);
        b->cd = cd;
        b->maker = cd->cd_Rom.er_Manufacturer;
        b->product = cd->cd_Rom.er_Product;
        b->type = cd->cd_Rom.er_Type;
        b->flags = cd->cd_Rom.er_Flags;
        b->cdflags = cd->cd_Flags;
        b->z3 = sp_board_z3(cd);
        b->rom = (b->type & ERTF_DIAGVALID) != 0;
        b->addr = (ULONG)cd->cd_BoardAddr;
        b->size = cd->cd_BoardSize;
        b->space = sp_board_space(cd);
        if ((name = sp_board_name(b->maker, b->product, &drv))) snprintf(b->name, sizeof b->name, "%s", name);
        else if ((name = sp_maker_name(b->maker))) snprintf(b->name, sizeof b->name, "a %s board", name);
        else snprintf(b->name, sizeof b->name, "(unidentified)");
        /* the driver that took it, when one did; else the one it is meant for */
        if (cd->cd_Driver && ((struct Node *)cd->cd_Driver)->ln_Name)
            snprintf(b->driver, sizeof b->driver, "%s", ((struct Node *)cd->cd_Driver)->ln_Name);
        else if (b->type & ERTF_MEMLIST)
            snprintf(b->driver, sizeof b->driver, "memory");
        else
            snprintf(b->driver, sizeof b->driver, "%s", drv);
    }
    CloseLibrary(eb);
    for (i = 0; i < nboards; i++) order[i] = i;
    om_table_sort(&T, raw_cell, order, nboards);
}

static void mb_text(ULONG n, char *buf, int size)
{
    snprintf(buf, size, "%lu.%lu MB", (unsigned long)(n >> 20), (unsigned long)((n & 0xfffff) * 10 >> 20));
}

static const char *after_words(const char *s, int n)
{
    while (n-- > 0 && (s = strchr(s, ' '))) s++;
    return s ? s : "";
}

static void id_string(char *out, int size, struct Library *l)
{
    int i;
    const char *s = l && l->lib_IdString ? (const char *)l->lib_IdString : "";
    for (i = 0; i < size - 1 && s[i] && s[i] != '\r' && s[i] != '\n'; i++) out[i] = s[i];
    out[i] = 0;
}

/* the last Speed run, which may have come since the rest was read */
static void speed_value(char *out, int size)
{
    ULONG mips;
    char a[16], b[16];
    if (om_speed_last(&mips, a)) snprintf(out, size, "%s MIPS (the Speed view, %s)", om_commas(mips, b), a);
    else snprintf(out, size, "not measured yet (the Speed view)");
}

static void take(void)
{
    UWORD f = SysBase->AttnFlags;
    struct sp_chips c;
    struct sp_ac ac;
    char a[24], b[24], *st;
    nlines = 0;
    /* Processor */
    {
        ULONG cacr = SysBase->LibNode.lib_Version >= 37 ? CacheControl(0, 0) : 0;
        add(B_CPU, "CPU", "%s, caches %s%s%s%s", sp_cpu(f), cacr & (CACRF_EnableI | CACRF_EnableD) ? "on (" : "off",
            cacr & CACRF_EnableI ? "instruction" : "", cacr & CACRF_EnableD ? ", data" : "",
            cacr & CACRF_CopyBack ? ", copyback)" : cacr & (CACRF_EnableI | CACRF_EnableD) ? ")" : "");
        add(B_CPU, "FPU", "%s", f & AFF_FPU40 ? "on the CPU" : sp_fpu(f));
        add(B_CPU, "MMU", "%s", sp_mmu(f));
        char sv[64];
        speed_value(sv, sizeof sv);
        add(B_CPU, "Speed", "%s", sv);
        add(B_CPU, "AttnFlags", "$%04x", f);
    }
    /* Chips */
    sp_chips(&c);
    add(B_CHIPS, "Chipset", "%s, %s", c.aga ? "AGA" : c.agnus_id & 0x20 ? "ECS" : "OCS", c.pal ? "PAL" : "NTSC");
    add(B_CHIPS, c.aga ? "Alice" : "Agnus", "%s (ID $%02lX)", after_words(c.agnus, 2), (unsigned long)c.agnus_id);
    add(B_CHIPS, c.denise_id == 0xF8 ? "Lisa" : "Denise", "%s (ID $%02lX)", after_words(c.denise, 1), (unsigned long)c.denise_id);
    add(B_CHIPS, "E clock", "%s Hz", om_commas(SysBase->ex_EClockFrequency, a));
    add(B_CHIPS, "Vertical", "%u Hz, power %u Hz", SysBase->VBlankFrequency, SysBase->PowerSupplyFrequency);
    /* Software */
    {
        struct Library *v = OpenLibrary((STRPTR)"version.library", 0);
        char id[64];
        add(B_SOFT, "Kickstart", "%u.%u", SysBase->LibNode.lib_Version, SysBase->SoftVer);
        id_string(id, sizeof id, &SysBase->LibNode);
        add(B_SOFT, "exec", "%s", id);
        id_string(id, sizeof id, (struct Library *)GfxBase);
        add(B_SOFT, "graphics", "%s", id);
        if (v) { add(B_SOFT, "Workbench", "%u.%u (version.library)", v->lib_Version, v->lib_Revision); CloseLibrary(v); }
        else add(B_SOFT, "Workbench", "no version.library");
    }
    /* AmiCradle: the host's status, in JSON (sysprobe reads its fields) */
    st = AllocVec(8192, MEMF_ANY);
    if (sp_ac(&ac, st, st ? 8192 : 0)) {
        char v1[48], v2[48];
        add(B_AC, "Version", "%s", sp_json(st, "\"status\"", "version", v1, sizeof v1) ? v1 : "?");
        if (sp_json(st, "\"motherboard\"", "name", v1, sizeof v1)) add(B_AC, "Machine", "%s", v1);
        /* the CPU's own name comes after its engine (its provider's name comes first) */
        if (sp_json(st, "\"engine\"", "name", v1, sizeof v1))
            add(B_AC, "CPU", "%s%s%s", v1, sp_json(st, "\"engine\"", "backendResolved", v2, sizeof v2) ? ", " : "",
                sp_json(st, "\"engine\"", "backendResolved", v2, sizeof v2) ? v2 : "");
        if (ac.pace_known) {
            if (ac.pace) add(B_AC, "Pace", "%lu.%02lu MHz", (unsigned long)(ac.pace / 1000000), (unsigned long)(ac.pace / 10000 % 100));
            else add(B_AC, "Pace", "hyper (as fast as the PC)");
        }
        if (sp_json(st, "\"host\"", "machine", v1, sizeof v1))
            add(B_AC, "PC", "%s, %s logical cores", v1, sp_json(st, "\"host\"", "logicalCores", v2, sizeof v2) ? v2 : "?");
        if (sp_json(st, "\"instance\"", "name", v1, sizeof v1))
            add(B_AC, "Instance", "%s%s%s", v1, sp_json(st, "\"instance\"", "friendlyName", v2, sizeof v2) ? ", " : "",
                sp_json(st, "\"instance\"", "friendlyName", v2, sizeof v2) ? v2 : "");
        add(B_AC, "Bridge", "accontrol.device %u.%u", ac.ver, ac.rev);
    } else
        add(B_AC, "", "%s", ac.error == 40 ? "Not an AmiCradle machine (no ACBridge)." : "No accontrol.device in DEVS:.");
    if (st) FreeVec(st);
    /* Memory */
    mb_text(AvailMem(MEMF_CHIP | MEMF_TOTAL), a, sizeof a);
    add(B_MEM, "Chip", "%s, %s KB free", a, om_commas(AvailMem(MEMF_CHIP) >> 10, b));
    mb_text(AvailMem(MEMF_FAST | MEMF_TOTAL), a, sizeof a);
    add(B_MEM, "Fast", "%s, %s KB free", a, om_commas(AvailMem(MEMF_FAST) >> 10, b));
    if (om_video_bytes()) { mb_text(om_video_bytes(), a, sizeof a); add(B_MEM, "Video", "%s on the graphics card", a); }
    take_boards();
    read_done = 1;
}

/* ---- drawing ------------------------------------------------------------------------------------------ */

static int box_lines(int box)
{
    int i, n = 0;
    for (i = 0; i < nlines; i++) if (lines[i].box == box) n++;
    return n;
}

static int box_height(int box) { return 6 + U.fh + 6 + box_lines(box) * (U.tfh + 2) + 6; }

static int draw_box(int box, int x, int y, int w)
{
    struct RastPort *rp = U.win->RPort;
    int i, h = box_height(box), ty = y + 6 + U.fh + 6, lw = om_text_width(U.tfont, "AttnFlags ") + 8;
    om_sunk(x, y, w, h);
    SetFont(rp, U.font);
    ogt_bold(rp, 1);
    ogt_text(rp, U.pen_text, x + 8, y + 6, box_title[box], w - 16);
    ogt_bold(rp, 0);
    SetFont(rp, U.tfont);
    for (i = 0; i < nlines; i++) {
        if (lines[i].box != box) continue;
        ogt_text(rp, U.pen_muted, x + 8, ty, lines[i].what, lw);
        ogt_text(rp, U.pen_text, x + 8 + (lines[i].what[0] ? lw : 0), ty, lines[i].value, w - 16 - lw);
        ty += U.tfh + 2;
    }
    return h;
}

static void draw_boxes(void)
{
    int w = (U.pw - 12) / 2, y;
    if (box_y < 0) return;
    y = box_y;
    y += draw_box(B_CPU, U.px, y, w) + 8;
    y += draw_box(B_CHIPS, U.px, y, w) + 8;
    draw_box(B_SOFT, U.px, y, w);
    y = box_y;
    y += draw_box(B_AC, U.px + w + 12, y, U.pw - w - 12) + 8;
    draw_box(B_MEM, U.px + w + 12, y, U.pw - w - 12);
}

/* ---- the actions -------------------------------------------------------------------------------------- */

static struct board *chosen(void)
{
    int s = om_table_selected(&T);
    return s >= 0 && s < nboards ? &boards[order[s]] : NULL;
}

static void board_details(void)
{
    struct board *b = chosen();
    char s[16], t[16];
    if (!b) { om_status(0, "Choose a board first."); return; }
    size_text(b->size, s, sizeof s);
    size_text(b->space, t, sizeof t);
    om_ask("GFOpenMonitor", "OK",
           "%s\n\nMaker $%04X (%u), product %u: %s\n%s at $%08lx, %s (it asks for %s)\n"
           "AutoConfig type $%02x, flags $%02x: %s%s%s\nConfigDev at $%08lx, %s; driver %s",
           b->name, b->maker, b->maker, b->product, sp_maker_name(b->maker) ? sp_maker_name(b->maker) : "a maker not in the list",
           b->z3 ? "Zorro III" : "Zorro II", (unsigned long)b->addr, s, t, b->type, b->flags,
           b->type & ERTF_MEMLIST ? "memory for the free list" : "a board", b->rom ? ", with a ROM (its diag vector)" : "",
           b->type & ERTF_CHAINEDCONFIG ? ", chained" : "", (unsigned long)b->cd,
           b->cdflags & CDF_CONFIGME ? "not yet configured (CONFIGME)" : "configured", b->driver[0] ? b->driver : "none");
}

static void drives(void)
{
    struct DosList *dl;
    char *text = AllocVec(2400, MEMF_ANY | MEMF_CLEAR);
    int len = 0, n = 0;
    if (!text) return;
    dl = LockDosList(LDF_DEVICES | LDF_READ);
    while ((dl = NextDosEntry(dl, LDF_DEVICES)) && len < 2300 && n < 24) {
        struct FileSysStartupMsg *fssm = (struct FileSysStartupMsg *)BADDR(dl->dol_misc.dol_handler.dol_Startup);
        UBYTE *nm = (UBYTE *)BADDR(dl->dol_Name);
        struct DosEnvec *de;
        char dev[32];
        if (!nm || (ULONG)fssm < 1024 || !(de = (struct DosEnvec *)BADDR(fssm->fssm_Environ)) || de->de_TableSize < 10 ||
            !fssm->fssm_Device)
            continue;                       /* not a disk (CON:, a handler of its own) */
        {
            UBYTE *d = (UBYTE *)BADDR(fssm->fssm_Device);
            int k = d[0] < sizeof dev - 1 ? d[0] : sizeof dev - 1;
            memcpy(dev, d + 1, k);
            dev[k] = 0;
        }
        {
            unsigned long long bytes = (unsigned long long)(de->de_HighCyl - de->de_LowCyl + 1) * de->de_Surfaces *
                                       de->de_BlocksPerTrack * de->de_SizeBlock * 4;
            len += snprintf(text + len, 2400 - len, "%.*s:  %lu MB, %s unit %lu\n", nm[0], nm + 1,
                            (unsigned long)(bytes >> 20), dev, (unsigned long)fssm->fssm_Unit);
            n++;
        }
    }
    UnLockDosList(LDF_DEVICES | LDF_READ);
    om_ask("GFOpenMonitor", "OK", "The drives (DOS's devices) and their sizes:\n\n%s", n ? text : "none\n");
    FreeVec(text);
}

static void save_report(void)
{
    static const char *const file = "RAM:ShowConfig-Report.guide";
    char cmd[80];
    BPTR in, out;
    LONG rc;
    if (om_ask("GFOpenMonitor", "Save|Cancel", "Save ShowConfig's report (AmigaGuide) as %s?\n\nShowConfig writes it.", file) != 1) return;
    snprintf(cmd, sizeof cmd, "ShowConfig REPORT %s", file);
    in = Open((STRPTR)"NIL:", MODE_OLDFILE);
    out = Open((STRPTR)"NIL:", MODE_NEWFILE);
    rc = in && out ? SystemTags((STRPTR)cmd, SYS_Input, (ULONG)in, SYS_Output, (ULONG)out, TAG_DONE) : -1;
    if (in) Close(in);
    if (out) Close(out);
    if (rc == 0) om_status(0, "Saved %s.", file);
    else om_status(0, rc == -1 || rc >= 20 ? "ShowConfig couldn't be run (is it on the path?)." : "ShowConfig stopped with %ld.", (long)rc);
}

/* in compact mode: the boxes' lines in a requester */
static void summary(void)
{
    char *text = AllocVec(2400, MEMF_ANY | MEMF_CLEAR);
    int i, len = 0, last = -1;
    if (!text) return;
    for (i = 0; i < nlines && len < 2300; i++) {
        if (lines[i].box != last) {
            len += snprintf(text + len, 2400 - len, "%s%s\n", last < 0 ? "" : "\n", box_title[lines[i].box]);
            last = lines[i].box;
        }
        len += snprintf(text + len, 2400 - len, "  %-10s %s\n", lines[i].what, lines[i].value);
    }
    om_ask("GFOpenMonitor", "OK", "%s", text);
    FreeVec(text);
}

/* ---- the view ----------------------------------------------------------------------------------------- */

static void layout(int x, int y, int w, int h)
{
    int bh = U.fh + 8, ty, i, left, right;
    if (!read_done) take();
    gy = y + h - bh;
    gh = bh;
    if (U.compact) { box_y = -1; head_y = -1; ty = y; }
    else {
        box_y = y;
        left = box_height(B_CPU) + box_height(B_CHIPS) + box_height(B_SOFT) + 16;
        right = box_height(B_AC) + box_height(B_MEM) + 8;
        head_y = y + (left > right ? left : right) + 10;
        ty = head_y + U.fh + 6;
    }
    om_table_layout(&T, x, ty, w, gy - (U.compact ? 4 : 10) - ty);
    gw[G_DETAILS] = om_text_width(U.font, U.compact ? "Board" : "Board details...") + 30;
    gw[G_DRIVES] = om_text_width(U.font, U.compact ? "Drives" : "Drives...") + 30;
    gw[G_SUMMARY] = U.compact ? om_text_width(U.font, "Summary") + 30 : 0;
    gw[G_REPORT] = om_text_width(U.font, U.compact ? "Report" : "Save a report (as ShowConfig)") + 30;
    gx[G_DETAILS] = x;
    gx[G_DRIVES] = x + gw[G_DETAILS] + 8;
    gx[G_SUMMARY] = gx[G_DRIVES] + gw[G_DRIVES] + 8;
    gx[G_REPORT] = x + w - gw[G_REPORT];
    for (i = 0; i < G_COUNT; i++) gad[i] = NULL;
}

static struct Gadget *gadgets(struct Gadget *g)
{
    static const char *const big[G_COUNT] = { "Board details...", "Drives...", "Save a report (as ShowConfig)", "Summary" };
    static const char *const small[G_COUNT] = { "Board", "Drives", "Report", "Summary" };
    struct NewGadget ng;
    int i;
    memset(&ng, 0, sizeof ng);
    ng.ng_TextAttr = &U.ta; ng.ng_VisualInfo = U.vi; ng.ng_Flags = PLACETEXT_IN;
    for (i = 0; i < G_COUNT && g; i++) {
        if (!gw[i]) continue;
        ng.ng_LeftEdge = gx[i]; ng.ng_TopEdge = gy; ng.ng_Width = gw[i]; ng.ng_Height = gh;
        ng.ng_GadgetText = (STRPTR)(U.compact ? small[i] : big[i]); ng.ng_GadgetID = GID_HW + i;
        g = gad[i] = CreateGadget(BUTTON_KIND, g, &ng, TAG_DONE);
    }
    return g;
}

static void draw(void)
{
    draw_boxes();
    if (head_y >= 0) om_heading(U.px, head_y, "Expansion boards (AutoConfig)");
    om_table_set_count(&T, nboards);
    om_table_draw(&T);
}

static void status(void)
{
    int i, ours = 0;
    for (i = 0; i < nboards; i++) if (boards[i].maker == 0xDA15 || boards[i].maker == 2011) ours++;
    om_status(1, "%d boards, %d of them the AmigaChrome Team's", nboards, ours);
    om_status(2, "Read once; F5 reads again");
}

static int event(ULONG cls, UWORD code, UWORD qual, APTR ia, int mx, int my, ULONG secs, ULONG micros)
{
    int picked = 0, row = -1, sorted = 0;
    (void)qual;
    if (om_table_event(&T, cls, code, ia, mx, my, secs, micros, &picked, &row, &sorted)) {
        if (sorted) { take_boards(); om_table_draw_rows(&T); }
        if (picked == 2) board_details();
        return 1;
    }
    if (cls == IDCMP_GADGETUP && ia) {
        switch (((struct Gadget *)ia)->GadgetID - GID_HW) {
        case G_DETAILS: board_details(); return 1;
        case G_DRIVES: drives(); return 1;
        case G_REPORT: save_report(); return 1;
        case G_SUMMARY: summary(); return 1;
        }
    }
    if (cls == IDCMP_RAWKEY && code == 0x54) {                 /* F5 */
        take();
        om_rebuild_gadgets();
        status();
        om_status(0, "Read again.");
        return 1;
    }
    if (cls == IDCMP_RAWKEY && (code == 0x4C || code == 0x4D)) { ogt_list_step(T.list, code == 0x4C ? -1 : 1); return 1; }
    if (cls == IDCMP_VANILLAKEY) {
        switch (code) {
        case 13: board_details(); return 1;
        case 'd': case 'D': drives(); return 1;
        case 's': case 'S': summary(); return 1;
        case 'r': case 'R': save_report(); return 1;
        case 'c': case 'C': om_status(0, om_table_copy(&T) ? "The boards are on the clipboard." : "The clipboard could not be written."); return 1;
        }
    }
    return 0;
}

static void shown(int on)
{
    int i;
    if (!on) { om_table_free(&T); return; }
    for (i = 0; i < nlines; i++) if (!strcmp(lines[i].what, "Speed")) speed_value(lines[i].value, sizeof lines[i].value);
}

const struct om_view om_hw_view = { "Hardware", layout, gadgets, draw, NULL, event, shown, status };

void om_hw_free(void)
{
    om_table_free(&T);
    if (lines) FreeVec(lines);
    if (boards) FreeVec(boards);
    lines = NULL; boards = NULL;
    nlines = nboards = 0;
}

void om_hw_init(void)
{
    lines = AllocVec(MAXLN * sizeof *lines, MEMF_ANY | MEMF_CLEAR);
    boards = AllocVec(MAXB * sizeof *boards, MEMF_ANY | MEMF_CLEAR);
    read_done = 0;
    T.sort = -1;                        /* AutoConfig's order */
    om_table_init(&T, cols, C_N, cell, GID_HW + 50, "Hardware");
}
