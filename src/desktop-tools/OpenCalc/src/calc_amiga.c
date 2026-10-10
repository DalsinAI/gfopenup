/* OpenCalc: the calculator's Workbench window, on the engine in opencalc.c (OpenUp Desktop Tools 0.1,
 * received 10 October 2026). GadTools, styled by OpenLook as every Open app. It takes over
 * SYS:Tools/Calculator (OpenUp's RECONCILE retires that one).
 *
 *   OpenCalc [EXPRESSION]           from a Shell: prints the result and ends
 *
 * One editable expression line, the result under it, the keypad, the scientific keys, degrees or
 * radians, the memory keys (MC MR M+ M-), the last calculations as a list (a click puts one back
 * on the line). Typing goes to the line; Return works it out; Escape clears it. Nothing is shown as
 * NaN or infinity: the engine's errors are said in words.
 *
 * MIT, Copyright (c) 2026 Dalsin Limited. */
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "opencalc.h"

static const char version[] __attribute__((used)) = "$VER: OpenCalc 0.2 (10.10.2026) OpenUp, MIT, Copyright (c) 2026 Dalsin Limited";
#define TITLE "Calculator"
#define SCREEN_TITLE "OpenCalc 0.2"
#define EXPR_MAX 200
#define HIST_MAX 24

enum { G_EXPR = 1, G_RESULT, G_MODE, G_HIST, G_KEY0 = 100 };

/* a key: what is shown, and what it puts on the line ("" = an action of its own) */
typedef struct { const char *label, *put; char act; } key;
#define ROWS 6
#define COLS 4
static const key pad[ROWS][COLS] = {
    { { "MC", "", 'c' }, { "MR", "", 'r' }, { "M+", "", '+' }, { "M-", "", '-' } },
    { { "7", "7", 0 }, { "8", "8", 0 }, { "9", "9", 0 }, { "/", "/", 0 } },
    { { "4", "4", 0 }, { "5", "5", 0 }, { "6", "6", 0 }, { "*", "*", 0 } },
    { { "1", "1", 0 }, { "2", "2", 0 }, { "3", "3", 0 }, { "-", "-", 0 } },
    { { "0", "0", 0 }, { ".", ".", 0 }, { "(", "(", 0 }, { ")", ")", 0 } },
    { { "C", "", 'C' }, { "^", "^", 0 }, { "+", "+", 0 }, { "=", "", '=' } },
};
#define SROWS 4
#define SCOLS 3
static const key sci[SROWS][SCOLS] = {
    { { "sin", "sin(", 0 }, { "cos", "cos(", 0 }, { "tan", "tan(", 0 } },
    { { "sqrt", "sqrt(", 0 }, { "ln", "ln(", 0 }, { "log", "log(", 0 } },
    { { "abs", "abs(", 0 }, { "pi", "pi", 0 }, { "e", "e", 0 } },
    { { "ans", "ans", 0 }, { "mem", "mem", 0 }, { "<-", "", 'b' } },
};
static const char *const mode_labels[] = { "Degrees", "Radians", NULL };

static OC_Context ctx;
static struct Window *win;
static struct Gadget *gad[G_KEY0 + ROWS * COLS + SROWS * SCOLS];
static char expr[EXPR_MAX + 1], result[80];
static struct List hist;
static struct Node hnode[HIST_MAX];
static char htext[HIST_MAX][120];
static int hcount;

/* %g without the C library's floating-point printf (libnix's is not to be trusted with -noixemul):
 * up to 10 decimals, trailing zeros dropped, exponent form beyond 1e15 or under 1e-6 */
static void fmt(char *out, size_t cap, double v)
{
    char buf[64], *p = buf;
    double a = v < 0 ? -v : v;
    int ex = 0;
    if (a != 0.0 && (a >= 1e15 || a < 1e-6)) {
        while (a >= 10.0) { a /= 10.0; ex++; }
        while (a < 1.0) { a *= 10.0; ex--; }
    }
    if (v < 0) *p++ = '-';
    {
        unsigned long ip = (unsigned long)a;
        double frac = a - (double)ip;
        char digs[24]; int n = 0, i;
        unsigned long fr = (unsigned long)(frac * 1e9 + 0.5);     /* 9 decimals: fits a 32-bit long */
        if (fr >= 1000000000UL) { fr -= 1000000000UL; ip++; }
        do { digs[n++] = (char)('0' + ip % 10); ip /= 10; } while (ip);
        while (n) *p++ = digs[--n];
        if (fr) {
            char fd[12];
            for (i = 8; i >= 0; i--) { fd[i] = (char)('0' + fr % 10); fr /= 10; }
            n = 9; while (n > 0 && fd[n - 1] == '0') n--;
            *p++ = '.'; for (i = 0; i < n; i++) *p++ = fd[i];
        }
    }
    if (ex) {
        char ed[8]; int n = 0;
        *p++ = 'e';
        if (ex < 0) { *p++ = '-'; ex = -ex; }
        do { ed[n++] = (char)('0' + ex % 10); ex /= 10; } while (ex);
        while (n) *p++ = ed[--n];
    }
    *p = 0;
    strncpy(out, buf, cap - 1); out[cap - 1] = 0;
}

static void set_expr(const char *s)
{
    strncpy(expr, s, EXPR_MAX); expr[EXPR_MAX] = 0;
    if (win) GT_SetGadgetAttrs(gad[G_EXPR], win, NULL, GTST_String, (ULONG)expr, TAG_DONE);
}

static void set_result(const char *s)
{
    strncpy(result, s, sizeof result - 1); result[sizeof result - 1] = 0;
    if (win) GT_SetGadgetAttrs(gad[G_RESULT], win, NULL, GTTX_Text, (ULONG)result, TAG_DONE);
}

static void read_expr(void)
{
    if (win && gad[G_EXPR] && gad[G_EXPR]->SpecialInfo) {
        const char *s = (const char *)((struct StringInfo *)gad[G_EXPR]->SpecialInfo)->Buffer;
        strncpy(expr, s, EXPR_MAX); expr[EXPR_MAX] = 0;
    }
}

static void put(const char *s)
{
    read_expr();
    if (strlen(expr) + strlen(s) <= EXPR_MAX) strcat(expr, s);
    set_expr(expr);
    if (win) ActivateGadget(gad[G_EXPR], win, NULL);
}

/* The last calculations, newest first. No allocation: the texts are an array, and the list is
 * built again from them each time, which a listview is happy with as long as it is detached first. */
static void add_history(const char *line)
{
    int i;
    if (win) GT_SetGadgetAttrs(gad[G_HIST], win, NULL, GTLV_Labels, ~0, TAG_DONE);
    if (hcount == HIST_MAX) hcount--;
    for (i = hcount; i > 0; i--) strcpy(htext[i], htext[i - 1]);
    strncpy(htext[0], line, sizeof htext[0] - 1); htext[0][sizeof htext[0] - 1] = 0;
    hcount++;
    NewList(&hist);
    for (i = 0; i < hcount; i++) { hnode[i].ln_Name = htext[i]; AddTail(&hist, &hnode[i]); }
    if (win) GT_SetGadgetAttrs(gad[G_HIST], win, NULL, GTLV_Labels, (ULONG)&hist, GTLV_Top, 0, TAG_DONE);
}

static void evaluate(void)
{
    double v = 0;
    OC_Status st;
    read_expr();
    if (!expr[0]) return;
    st = OC_Eval(&ctx, expr, &v);
    if (st == OC_OK) {
        char num[40], line[120];
        fmt(num, sizeof num, v);
        snprintf(result, sizeof result, "= %s", num);
        set_result(result);
        snprintf(line, sizeof line, "%s = %s", expr, num);
        add_history(line);
    } else set_result(OC_StatusText(st));
}

static void action(char act)
{
    char num[40];
    switch (act) {
    case '=': evaluate(); break;
    case 'C': set_expr(""); set_result(""); if (win) ActivateGadget(gad[G_EXPR], win, NULL); break;
    case 'b': read_expr(); if (expr[0]) expr[strlen(expr) - 1] = 0; set_expr(expr); if (win) ActivateGadget(gad[G_EXPR], win, NULL); break;
    case 'c': ctx.memory = 0; set_result("Memory cleared"); break;
    case 'r': fmt(num, sizeof num, ctx.memory); put(num); break;
    case '+': ctx.memory += ctx.ans; fmt(num, sizeof num, ctx.memory); snprintf(result, sizeof result, "M = %s", num); set_result(result); break;
    case '-': ctx.memory -= ctx.ans; fmt(num, sizeof num, ctx.memory); snprintf(result, sizeof result, "M = %s", num); set_result(result); break;
    }
}

static struct NewMenu menus[] = {
    { NM_TITLE, (STRPTR)"Project", 0, 0, 0, 0 },
    { NM_ITEM, (STRPTR)"Clear", (STRPTR)"K", 0, 0, (APTR)1 },
    { NM_ITEM, (STRPTR)"Clear memory", (STRPTR)"M", 0, 0, (APTR)2 },
    { NM_ITEM, NM_BARLABEL, 0, 0, 0, 0 },
    { NM_ITEM, (STRPTR)"Quit", (STRPTR)"Q", 0, 0, (APTR)3 },
    { NM_END, 0, 0, 0, 0, 0 }
};

static int gui(void)
{
    struct Screen *scr = LockPubScreen(NULL);
    APTR vi = scr ? GetVisualInfoA(scr, NULL) : NULL;
    struct Gadget *glist = NULL, *g;
    struct NewGadget ng;
    struct Menu *menu = NULL;
    int fh, lh, top, row, quit = 0, rc = RETURN_OK, W, bw, bh, r, c, sx, sw, lvh, hx, hw;
    if (!vi) { if (scr) UnlockPubScreen(NULL, scr); return RETURN_FAIL; }
    memset(gad, 0, sizeof gad);
    fh = scr->Font->ta_YSize;
    lh = fh + 6;
    bh = fh + 8;
    top = scr->WBorTop + fh + 1 + 8;
    bw = TextLength(&scr->RastPort, (STRPTR)"sqrt", 4) + 20;
    if (bw < 34) bw = 34;
    sx = 12 + COLS * (bw + 4) + 12;
    sw = SCOLS * (bw + 4) - 4;
    hx = sx + sw + 12;
    hw = TextLength(&scr->RastPort, (STRPTR)"12345678 * 3.14159 = 38785", 26) + 30;
    W = hx + hw + 12;
    lvh = ROWS * (bh + 4) - 4 + lh + 4;

    g = CreateContext(&glist);
    memset(&ng, 0, sizeof ng);
    ng.ng_TextAttr = scr->Font;
    ng.ng_VisualInfo = vi;
#define G(kind, id, x, y, w, h, text, flags, ...) \
    (ng.ng_LeftEdge = (x), ng.ng_TopEdge = (y), ng.ng_Width = (w), ng.ng_Height = (h), ng.ng_GadgetText = (STRPTR)(text), \
     ng.ng_Flags = (flags), ng.ng_GadgetID = (id), g = gad[id] = CreateGadget(kind, g, &ng, __VA_ARGS__, TAG_DONE))
    row = top;
    G(STRING_KIND, G_EXPR, 12, row, W - 24, lh, NULL, 0, GTST_String, (ULONG)expr, GTST_MaxChars, EXPR_MAX);
    row += lh + 4;
    G(TEXT_KIND, G_RESULT, 12, row, W - 24, lh, NULL, 0, GTTX_Text, (ULONG)result, GTTX_Border, TRUE, GTTX_Justification, GTJ_RIGHT);
    row += lh + 8;
    for (r = 0; r < ROWS; r++)
        for (c = 0; c < COLS; c++)
            G(BUTTON_KIND, G_KEY0 + r * COLS + c, 12 + c * (bw + 4), row + r * (bh + 4), bw, bh, pad[r][c].label, 0, GA_Disabled, FALSE);
    G(CYCLE_KIND, G_MODE, sx, row, sw, lh, NULL, 0, GTCY_Labels, (ULONG)mode_labels, GTCY_Active, ctx.degrees ? 0 : 1);
    for (r = 0; r < SROWS; r++)
        for (c = 0; c < SCOLS; c++)
            G(BUTTON_KIND, G_KEY0 + ROWS * COLS + r * SCOLS + c, sx + c * (bw + 4), row + lh + 8 + r * (bh + 4), bw, bh, sci[r][c].label, 0, GA_Disabled, FALSE);
    G(LISTVIEW_KIND, G_HIST, hx, row, hw, lvh, NULL, 0, GTLV_Labels, (ULONG)&hist, GTLV_ReadOnly, FALSE);
    row += ROWS * (bh + 4) + 8;
    if (!g) { rc = RETURN_FAIL; goto out; }
    win = OpenWindowTags(NULL, WA_Title, (ULONG)TITLE, WA_ScreenTitle, (ULONG)SCREEN_TITLE, WA_PubScreen, (ULONG)scr,
                         WA_Left, (scr->Width - W) / 2 > 0 ? (scr->Width - W) / 2 : 0, WA_Top, scr->BarHeight + 24,
                         WA_InnerWidth, W, WA_InnerHeight, row - scr->WBorTop - fh - 1,
                         WA_Gadgets, (ULONG)glist, WA_DragBar, TRUE, WA_DepthGadget, TRUE, WA_CloseGadget, TRUE, WA_Activate, TRUE,
                         WA_SmartRefresh, TRUE, WA_NewLookMenus, TRUE,
                         WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW | IDCMP_MENUPICK | IDCMP_VANILLAKEY | BUTTONIDCMP |
                                   STRINGIDCMP | CYCLEIDCMP | LISTVIEWIDCMP | TEXTIDCMP,
                         TAG_DONE);
    if (!win) { rc = RETURN_FAIL; goto out; }
    if ((menu = CreateMenus(menus, TAG_DONE)) && LayoutMenus(menu, vi, GTMN_NewLookMenus, TRUE, TAG_DONE)) SetMenuStrip(win, menu);
    GT_RefreshWindow(win, NULL);
    ActivateGadget(gad[G_EXPR], win, NULL);

    while (!quit) {
        struct IntuiMessage *m;
        WaitPort(win->UserPort);
        while (!quit && (m = GT_GetIMsg(win->UserPort))) {
            ULONG cls = m->Class;
            UWORD code = m->Code;
            struct Gadget *gg = (struct Gadget *)m->IAddress;
            GT_ReplyIMsg(m);
            if (cls == IDCMP_CLOSEWINDOW) quit = 1;
            else if (cls == IDCMP_REFRESHWINDOW) { GT_BeginRefresh(win); GT_EndRefresh(win, TRUE); }
            else if (cls == IDCMP_VANILLAKEY) {          /* the line is not active: keys go to it */
                char s[2] = { (char)code, 0 };
                if (code == 27) action('C');
                else if (code == 13) evaluate();
                else if (code == 8) action('b');
                else if (code >= 32 && code < 127) put(s);
            }
            else if (cls == IDCMP_MENUPICK) {
                while (code != MENUNULL && !quit) {
                    struct MenuItem *it = ItemAddress(menu, code);
                    if (!it) break;
                    switch ((ULONG)GTMENUITEM_USERDATA(it)) {
                    case 1: action('C'); break;
                    case 2: action('c'); break;
                    case 3: quit = 1; break;
                    }
                    code = it->NextSelect;
                }
            }
            else if (cls == IDCMP_GADGETUP) {
                int id = gg->GadgetID;
                if (id == G_EXPR) evaluate();
                else if (id == G_MODE) ctx.degrees = code == 0;
                else if (id == G_HIST) {
                    int i = code;
                    if (i >= 0 && i < hcount) {
                        char line[120], *eq;
                        strncpy(line, htext[i], sizeof line - 1); line[sizeof line - 1] = 0;
                        if ((eq = strstr(line, " = "))) *eq = 0;
                        set_expr(line);
                        ActivateGadget(gad[G_EXPR], win, NULL);
                    }
                }
                else if (id >= G_KEY0 && id < G_KEY0 + ROWS * COLS) {
                    const key *k = &pad[(id - G_KEY0) / COLS][(id - G_KEY0) % COLS];
                    if (k->act) action(k->act); else put(k->put);
                }
                else if (id >= G_KEY0 + ROWS * COLS && id < G_KEY0 + ROWS * COLS + SROWS * SCOLS) {
                    int n = id - G_KEY0 - ROWS * COLS;
                    const key *k = &sci[n / SCOLS][n % SCOLS];
                    if (k->act) action(k->act); else put(k->put);
                }
            }
        }
    }
out:
    if (win) { ClearMenuStrip(win); CloseWindow(win); win = NULL; }
    if (menu) FreeMenus(menu);
    FreeGadgets(glist);
    FreeVisualInfo(vi);
    UnlockPubScreen(NULL, scr);
    return rc;
}

int main(int argc, char **argv)
{
    int i, rc;
    (void)version;
    OC_Init(&ctx);
    NewList(&hist);
    if (argc > 1) {                                  /* OpenCalc 2+2: the answer on the Shell */
        double v = 0; OC_Status st; char num[40], line[EXPR_MAX + 1] = "";
        for (i = 1; i < argc; i++) { if (strlen(line) + strlen(argv[i]) + 1 <= EXPR_MAX) { if (i > 1) strcat(line, " "); strcat(line, argv[i]); } }
        st = OC_Eval(&ctx, line, &v);
        if (st == OC_OK) { fmt(num, sizeof num, v); Printf((STRPTR)"%s\n", (LONG)num); return RETURN_OK; }
        Printf((STRPTR)"%s\n", (LONG)OC_StatusText(st));
        return RETURN_WARN;
    }
    rc = gui();
    return rc;
}
