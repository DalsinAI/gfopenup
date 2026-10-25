/* OpenSnap 1.0: pictures of the screen, by hotkey (the user, 24 October 2026).
 *
 *   Shift + Right Amiga + S   the whole front screen
 *   Shift + Right Amiga + W   the active window: its box on its screen, borders included, cut to the screen
 *
 * Each picture is saved in SYS:Snapshots (made, with a drawer icon, when it isn't there) as
 * Snapshot-2026-10-24-15.42.07.png, a window's with its title after the time (Snapshot-...-Shell.png), and
 * is put on the clipboard (clipboard.device unit 0) as an IFF ILBM, so RAmiga+V pastes it in a paint
 * program: 24 planes for a truecolour screen, its planes and a CMAP for a screen of 256 colours or fewer.
 * A short message on the screen's title bar says where it went, for three seconds; a requester only when
 * the file can't be saved.
 *
 * Screens: RTG truecolour screens (OpenRTG), 8-bit RTG screens with a palette, and the Amiga's own planar
 * screens, AGA's too (HAM is saved as its colours, Extra Half-Brite with its 64). An RTG screen's pixels are
 * read with cybergraphics.library's ReadPixelArray, which waits for the GPU's drawing to be done (OpenRTG
 * waits in ReadPixels, as the compositor's asynchronous ring will); video RAM is never read by address. A
 * planar screen's planes are read after WaitBlit.
 *
 * The PNG is written by openpng.datatype (OpenImage, opendatatypes 47.4: OIM_ENCODEPNG, deflate by
 * z.library). Without them the picture is saved as the IFF ILBM instead (Snapshot-....iff) and the message
 * says so.
 *
 * A screen can't close while it is read: a public screen (Workbench's, OpenRTG's) is locked with
 * LockPubScreen; any other is read with Intuition's lock held (LockIBase), which only holds up Intuition
 * for the moment the read takes. OpenSnap waits at priority 4, so a hotkey is answered at once while a
 * program is busy, and saves at priority 0.
 *
 * A commodity: Exchange shows it and can disable, enable or quit it. Tool types (in its icon, or as
 * KEY=VALUE on the command line, which wins):
 *   SCREENKEY=shift rcommand s     the hotkey for the front screen (commodities' words); NONE for none
 *   WINDOWKEY=shift rcommand w     the hotkey for the active window
 *   DRAWER=SYS:Snapshots           where the pictures go
 *   CLIPBOARD=YES                  NO: files only
 *   CX_PRIORITY=0                  the broker's priority
 *
 * MIT, Copyright (c) 2026 AmigaChrome Team. */

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/datetime.h>
#include <devices/clipboard.h>
#include <devices/timer.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/screens.h>
#include <intuition/classusr.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <graphics/view.h>
#include <graphics/modeid.h>
#include <graphics/displayinfo.h>
#include <graphics/rastport.h>
#include <libraries/commodities.h>
#include <utility/date.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>
#include <workbench/icon.h>
#include <datatypes/datatypes.h>
#include <datatypes/datatypesclass.h>
#include <datatypes/openimage.h>
#include <cybergraphx/cybergraphics.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/commodities.h>
#include <proto/utility.h>
#include <proto/icon.h>
#include <proto/cybergraphics.h>
#include <clib/alib_protos.h>
#include <string.h>

#include "opensnap_pic.h"

static const char version[] __attribute__((used)) = "$VER: OpenSnap 1.0 (24.10.2026) MIT, Copyright (c) 2026 AmigaChrome Team";
unsigned long __stack = 32768;      /* libnix swaps to this when Run gives the Shell's 4 KB */

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Library *CxBase, *IconBase, *CyberGfxBase;
struct Library *UtilityBase;

#define EV_SCREEN 1
#define EV_WINDOW 2
#define PRI_WAIT 4                  /* waiting for a hotkey: answered at once while a program is busy */
#define PRI_SAVE 0                  /* encoding and saving: no more than any other program */
#ifndef PNG_LEVEL
/* z.library's quickest deflate: in the lab (a 68040 on a busy PC) a 1920 x 1200 Workbench took 2.5 s at 1,
 * 3.5 s at 3 and 5.7 s at the default, 6, and its PNG was 23% bigger at 1 than at 6 */
#define PNG_LEVEL 1
#endif

static struct {
    char screenkey[64], windowkey[64], drawer[256];
    int clipboard;
    LONG cxpri;
} set = { "shift rcommand s", "shift rcommand w", "SYS:Snapshots", 1, 0 };

/* ---- the message on the title bar ------------------------------------------------------------- */

static char msgtext[200];
static struct Window *msgwin;         /* a screen title changed: the window it belongs to */
static UBYTE *msgold;
static struct Window *toast;          /* or a small window of ours over a public screen's title bar */
static int msgup, tpending;           /* the title is ours; the timer request is out */

static int window_there(struct Window *w)
{
    struct Screen *s;
    struct Window *v;
    for (s = IntuitionBase->FirstScreen; s; s = s->NextScreen)
        for (v = s->FirstWindow; v; v = v->NextWindow)
            if (v == w) return 1;
    return 0;
}

static void unsay(void)
{
    if (toast) { CloseWindow(toast); toast = NULL; }
    if (!msgup) return;
    msgup = 0;
    Forbid();   /* the window can't close between the look and the call: Intuition takes the calls in turn */
    if (window_there(msgwin) && msgwin->ScreenTitle == (UBYTE *)msgtext)
        SetWindowTitles(msgwin, (UBYTE *)~0, msgold);
    Permit();
}

/* On a public screen (Workbench's, whatever window is active or none): a small borderless window over the
 * middle of its title bar that doesn't take the activation, as a visitor, so the screen's owner can't
 * close the screen under it. */
static struct Window *toast_open(const char *pubname, const char *text)
{
    struct Screen *scr = LockPubScreen((UBYTE *)pubname);
    struct DrawInfo *dri;
    struct Window *w = NULL;
    struct RastPort rp;
    LONG len, tw, th, n = strlen(text);
    if (!scr) return NULL;
    if ((dri = GetScreenDrawInfo(scr))) {
        InitRastPort(&rp);
        SetFont(&rp, dri->dri_Font);
        while (n > 0 && (len = TextLength(&rp, (STRPTR)text, n)) + 24 > scr->Width) n--;
        tw = TextLength(&rp, (STRPTR)text, n) + 24;
        th = scr->BarHeight + 1;
        if (th < dri->dri_Font->tf_YSize + 3) th = dri->dri_Font->tf_YSize + 3;
        w = OpenWindowTags(NULL, WA_PubScreen, (ULONG)scr, WA_Left, (scr->Width - tw) / 2, WA_Top, 0,
                           WA_Width, tw, WA_Height, th, WA_Borderless, TRUE, WA_Activate, FALSE, WA_RMBTrap, TRUE,
                           WA_SmartRefresh, TRUE, WA_NoCareRefresh, TRUE, WA_IDCMP, 0, TAG_DONE);
        if (w) {
            struct RastPort *r = w->RPort;
            SetFont(r, dri->dri_Font);
            SetAPen(r, dri->dri_Pens[FILLPEN]);
            RectFill(r, 0, 0, tw - 1, th - 1);
            SetAPen(r, dri->dri_Pens[SHADOWPEN]);
            Move(r, 0, th - 1); Draw(r, 0, 0); Draw(r, tw - 1, 0); Draw(r, tw - 1, th - 1); Draw(r, 0, th - 1);
            SetAPen(r, dri->dri_Pens[FILLTEXTPEN]);
            SetDrMd(r, JAM1);
            Move(r, 12, (th - dri->dri_Font->tf_YSize) / 2 + dri->dri_Font->tf_Baseline);
            Text(r, (STRPTR)text, n);
        }
        FreeScreenDrawInfo(scr, dri);
    }
    UnlockPubScreen(NULL, scr);
    return w;
}

/* The message for three seconds: over a public screen's title bar, else as the active window's screen
 * title when that window is on the screen taken. */
static void say(const char *pubname, struct Window *w, const char *text, struct timerequest *tr)
{
    unsay();
    strncpy(msgtext, text, sizeof msgtext - 1);
    msgtext[sizeof msgtext - 1] = 0;
    if (pubname && pubname[0]) toast = toast_open(pubname, msgtext);
    if (!toast && w) {
        Forbid();
        if (window_there(w)) {
            msgwin = w;
            msgold = w->ScreenTitle;
            SetWindowTitles(w, (UBYTE *)~0, (UBYTE *)msgtext);
            msgup = 1;
        }
        Permit();
    }
    if ((toast || msgup) && tr) {
        if (tpending) { AbortIO((struct IORequest *)tr); WaitIO((struct IORequest *)tr); }
        tr->tr_node.io_Command = TR_ADDREQUEST;
        tr->tr_time.tv_secs = 3;
        tr->tr_time.tv_micro = 0;
        SendIO((struct IORequest *)tr);
        tpending = 1;
    }
}

#ifdef OSDEBUG
/* lab builds: how long each step took, in T:OpenSnap.log */
static void lap(const char *what)
{
    struct DateStamp d;
    BPTR fh = Open((STRPTR)"T:OpenSnap.log", MODE_READWRITE);
    DateStamp(&d);
    if (fh) {
        Seek(fh, 0, OFFSET_END);
        FPrintf(fh, (STRPTR)"%ld.%02ld %s\n", d.ds_Minute * 60 + d.ds_Tick / 50, (d.ds_Tick % 50) * 2, (ULONG)what);
        Close(fh);
    }
}
#else
#define lap(what) ((void)0)
#endif

/* ---- the picture ------------------------------------------------------------------------------- */

struct shot {
    struct osp_pic pic;
    uint8_t *pens;                  /* indexed */
    uint32_t *argb;                 /* truecolour, or the indexed picture's colours for the PNG */
    char title[64];                 /* the window's, made safe for a file name */
    struct Window *active;          /* for the message: the active window, when it is on the screen taken */
    char pubname[MAXPUBSCREENNAME + 1];   /* the screen's public name, or "" */
};

static void free_shot(struct shot *s)
{
    if (s->pens) FreeVec(s->pens);
    if (s->argb) FreeVec(s->argb);
    s->pens = NULL; s->argb = NULL;
}

static void palette_of(struct ViewPort *vp, unsigned colours, uint32_t *palette)
{
    ULONG rgb[3 * 256];
    unsigned i, n = colours;
    memset(palette, 0, 256 * sizeof *palette);
    if (!vp->ColorMap) return;
    if (n > vp->ColorMap->Count) n = vp->ColorMap->Count;
    if (n > 256) n = 256;
    GetRGB32(vp->ColorMap, 0, n, rgb);
    for (i = 0; i < n; i++)
        palette[i] = ((rgb[3 * i] >> 24) << 16) | ((rgb[3 * i + 1] >> 24) << 8) | (rgb[3 * i + 2] >> 24);
}

/* The bitmap on show: the view port's (a double-buffered screen's front one), else the screen's. */
static struct BitMap *shown_bitmap(struct Screen *scr)
{
    struct ViewPort *vp = &scr->ViewPort;
    return (vp->RasInfo && vp->RasInfo->BitMap) ? vp->RasInfo->BitMap : scr->RastPort.BitMap;
}

/* The pixels of x, y, w, h on the screen. Called with the screen held (a public screen's lock, or
 * Intuition's). 0, or a reason. */
static const char *read_pixels(struct Screen *scr, LONG x, LONG y, ULONG w, ULONG h, struct shot *s)
{
    struct ViewPort *vp = &scr->ViewPort;
    struct BitMap *bm = shown_bitmap(scr);
    ULONG mode = GetVPModeID(vp), depth, i;
    int rtg = 0;
    struct osp_pic *p = &s->pic;

    if (!bm) return "the screen has no picture";
    if (CyberGfxBase && GetCyberMapAttr(bm, CYBRMATTR_ISCYBERGFX)) rtg = 1;
    else if (CyberGfxBase && !(GetBitMapAttr(bm, BMA_FLAGS) & BMF_STANDARD)) rtg = 1;
    p->w = w; p->h = h; p->pagew = (uint16_t)w; p->pageh = (uint16_t)h;
    p->xaspect = p->yaspect = 1;

    if (rtg) {
        struct RastPort rp;
        depth = GetCyberMapAttr(bm, CYBRMATTR_DEPTH);
        if (!(s->argb = AllocVec(w * h * 4, MEMF_ANY))) return "not enough memory";
        InitRastPort(&rp);
        rp.BitMap = bm;
        /* the GPU's drawing is done before the pixels come back (OpenRTG's ReadPixels waits for it) */
        if (!ReadPixelArray(s->argb, 0, 0, (UWORD)(w * 4), &rp, (UWORD)x, (UWORD)y, (UWORD)w, (UWORD)h, RECTFMT_ARGB))
            return "the screen's pixels couldn't be read";
        if (depth <= 8) {
            /* a screen with a palette: each colour back to its pen, for an ILBM with a CMAP; the last pen of
             * each of 1024 colours is kept, so a dithered picture doesn't search the palette for every pixel */
            uint32_t *pal = p->palette;
            ULONG n = 1UL << depth;
            ULONG *seen = AllocVec(1024 * 2 * sizeof(ULONG), MEMF_ANY);
            palette_of(vp, n, pal);
            if (seen && (s->pens = AllocVec(w * h, MEMF_ANY))) {
                for (i = 0; i < 1024; i++) seen[2 * i] = 0xFFFFFFFF;
                for (i = 0; i < w * h; i++) {
                    ULONG c = s->argb[i] & 0xFFFFFF, slot = ((c >> 14) ^ (c >> 4) ^ c) & 1023, k;
                    if (seen[2 * slot] != c) {
                        for (k = 0; k < n && (pal[k] & 0xFFFFFF) != c; k++) ;
                        if (k == n) break;                /* a colour not in the palette: keep it truecolour */
                        seen[2 * slot] = c; seen[2 * slot + 1] = k;
                    }
                    s->pens[i] = (uint8_t)seen[2 * slot + 1];
                }
                if (i == w * h) { p->planes = depth ? depth : 8; p->pens = s->pens; }
                else { FreeVec(s->pens); s->pens = NULL; }
            }
            if (seen) FreeVec(seen);
        }
        for (i = 0; i < w * h; i++) s->argb[i] |= 0xFF000000;
        if (!p->planes) p->argb = s->argb;
        return 0;
    }

    /* the Amiga's own planar screen: the planes, after the blitter has finished */
    depth = GetBitMapAttr(bm, BMA_DEPTH);
    if (depth < 1 || depth > 8) return "a screen of more than 8 planes without OpenRTG";
    if (!(s->pens = AllocVec(w * h, MEMF_ANY | MEMF_CLEAR))) return "not enough memory";
    WaitBlit();
    {
        ULONG bw = GetBitMapAttr(bm, BMA_WIDTH), yy, xx, d;
        for (yy = 0; yy < h; yy++) {
            uint8_t *out = s->pens + yy * w;
            for (d = 0; d < depth; d++) {
                const UBYTE *row;
                uint8_t bit = (uint8_t)(1 << d);
                if (!bm->Planes[d]) continue;                 /* a plane of 0s */
                if (bm->Planes[d] == (PLANEPTR)-1) {          /* a plane of 1s */
                    for (xx = 0; xx < w; xx++) out[xx] |= bit;
                    continue;
                }
                row = (const UBYTE *)bm->Planes[d] + (y + yy) * bm->BytesPerRow;
                for (xx = 0; xx < w && x + xx < bw; xx++)
                    if (row[(x + xx) >> 3] & (0x80 >> ((x + xx) & 7))) out[xx] |= bit;
            }
        }
    }
    palette_of(vp, 1UL << depth, p->palette);
    if ((vp->Modes & HAM) || (mode != INVALID_ID && (mode & HAM_KEY))) {
        /* HAM: its colours, as a truecolour picture */
        ULONG yy;
        if (!(s->argb = AllocVec(w * h * 4, MEMF_ANY))) return "not enough memory";
        for (yy = 0; yy < h; yy++) osp_ham_row(s->pens + yy * w, w, depth, p->palette, s->argb + yy * w);
        FreeVec(s->pens); s->pens = NULL;
        p->argb = s->argb;
        return 0;
    }
    if ((vp->Modes & EXTRA_HALFBRITE) || (mode != INVALID_ID && (mode & EXTRAHALFBRITE_KEY))) osp_ehb(p->palette);
    p->planes = depth;
    p->pens = s->pens;
    if (mode != INVALID_ID) {
        struct DisplayInfo di;
        p->camg = mode & ~(ULONG)(HAM_KEY | EXTRAHALFBRITE_KEY);   /* EHB's colours are all in the CMAP */
        if (GetDisplayInfoData(NULL, (UBYTE *)&di, sizeof di, DTAG_DISP, mode) > 0
            && di.Resolution.x > 0 && di.Resolution.y > 0) {
            ULONG ax = di.Resolution.x, ay = di.Resolution.y;
            while (ax > 255 || ay > 255) { ax = (ax + 1) / 2; ay = (ay + 1) / 2; }
            p->xaspect = (uint8_t)ax; p->yaspect = (uint8_t)ay;
        }
    }
    return 0;
}

/* The public name of a screen, or "" (out: MAXPUBSCREENNAME + 1 bytes). */
static void pub_name(struct Screen *scr, char *out)
{
    struct List *l = LockPubScreenList();
    struct Node *n;
    out[0] = 0;
    for (n = l->lh_Head; n->ln_Succ; n = n->ln_Succ)
        if (((struct PubScreenNode *)n)->psn_Screen == scr && n->ln_Name) {
            strncpy(out, n->ln_Name, MAXPUBSCREENNAME);
            out[MAXPUBSCREENNAME] = 0;
            break;
        }
    UnlockPubScreenList();
}

/* Takes the front screen or the active window's box. 0, or a reason. */
static const char *grab(int kind, struct shot *s)
{
    struct Screen *scr, *locked = NULL;
    struct Window *win;
    LONG x, y, x1, y1;
    ULONG ib;
    char pubname[MAXPUBSCREENNAME + 1];
    const char *why;
    int pub = 0;

    memset(s, 0, sizeof *s);
    ib = LockIBase(0);
    win = IntuitionBase->ActiveWindow;
    if (kind == EV_WINDOW) {
        if (!win) {
            scr = IntuitionBase->FirstScreen;
            UnlockIBase(ib);
            if (scr) pub_name(scr, s->pubname);      /* for the message */
            return "no window is active";
        }
        scr = win->WScreen;
        x = win->LeftEdge; y = win->TopEdge; x1 = x + win->Width; y1 = y + win->Height;
        osp_safe_title((const char *)win->Title, s->title, sizeof s->title);
    } else {
        scr = IntuitionBase->FirstScreen;
        if (!scr) { UnlockIBase(ib); return "no screen"; }
        x = 0; y = 0; x1 = scr->Width; y1 = scr->Height;
    }
    s->active = (win && win->WScreen == scr) ? win : NULL;
    UnlockIBase(ib);

    /* a public screen: locked, so it can't close while it's read */
    pub_name(scr, pubname);
    pub = pubname[0] != 0;
    if (pub) {
        locked = LockPubScreen((UBYTE *)pubname);
        if (locked != scr) { if (locked) UnlockPubScreen(NULL, locked); locked = NULL; }
        else strcpy(s->pubname, pubname);
    }
    if (!locked) {
        /* any other screen: read with Intuition's lock held, after making sure it is still open */
        struct Screen *t;
        ib = LockIBase(0);
        for (t = IntuitionBase->FirstScreen; t && t != scr; t = t->NextScreen) ;
        if (!t) { UnlockIBase(ib); return "the screen closed"; }
    }
    {
        struct BitMap *bm = shown_bitmap(scr);
        LONG bw = bm ? (LONG)GetBitMapAttr(bm, BMA_WIDTH) : 0, bh = bm ? (LONG)GetBitMapAttr(bm, BMA_HEIGHT) : 0;
        if (x1 > scr->Width) x1 = scr->Width;
        if (y1 > scr->Height) y1 = scr->Height;
        if (x1 > bw) x1 = bw;
        if (y1 > bh) y1 = bh;
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        why = (x1 <= x || y1 <= y) ? "the window is off the screen"
                                   : read_pixels(scr, x, y, (ULONG)(x1 - x), (ULONG)(y1 - y), s);
    }
    if (locked) UnlockPubScreen(NULL, locked);
    else UnlockIBase(ib);
    if (why) free_shot(s);
    return why;
}

/* ---- saving ------------------------------------------------------------------------------------ */

/* The drawer, made with a drawer icon (showing every file) when it isn't there. */
static int make_drawer(const char *path)
{
    BPTR l = Lock((STRPTR)path, ACCESS_READ);
    if (l) { UnLock(l); return 1; }
    if (!(l = CreateDir((STRPTR)path))) return 0;
    UnLock(l);
    if (IconBase) {
        struct DiskObject *d = GetDefDiskObject(WBDRAWER);
        if (d) {
            if (d->do_DrawerData) {
                d->do_DrawerData->dd_Flags = DDFLAGS_SHOWALL;      /* the PNGs have no icons of their own */
                d->do_DrawerData->dd_ViewModes = DDVM_BYICON;
            }
            if (IconBase->lib_Version >= 44)
                PutIconTags((STRPTR)path, d, ICONPUTA_NotifyWorkbench, TRUE, TAG_DONE);
            else
                PutDiskObject((STRPTR)path, d);
            FreeDiskObject(d);
        }
    }
    return 1;
}

/* The PNG, by openpng.datatype 47.4 or later; NULL without it (or z.library). */
static UBYTE *encode_png(const struct shot *s, ULONG *size)
{
    struct Library *png = OpenLibrary((STRPTR)"datatypes/openpng.datatype", 47);
    UBYTE *data = NULL;
    *size = 0;
    if (!png) return NULL;
    if (png->lib_Version > 47 || png->lib_Revision >= 4) {     /* 47.4: OIM_ENCODEPNG */
        Object *o = NewObject(NULL, (STRPTR)"openpng.datatype", DTA_SourceType, DTST_RAM, TAG_DONE);
        if (o) {
            struct oiEncodePNG m;
            uint32_t *argb = s->argb;
            int own = 0;
            if (!argb && s->pens) {                    /* an indexed picture's colours */
                ULONG i, n = s->pic.w * s->pic.h;
                if ((argb = AllocVec(n * 4, MEMF_ANY))) {
                    own = 1;
                    for (i = 0; i < n; i++) argb[i] = 0xFF000000 | s->pic.palette[s->pens[i]];
                }
            }
            if (argb) {
                m.MethodID = OIM_ENCODEPNG;
                m.oiep_GInfo = NULL;
                m.oiep_Pixels = (const UBYTE *)argb;
                m.oiep_Width = s->pic.w;
                m.oiep_Height = s->pic.h;
                m.oiep_Stride = s->pic.w * 4;
                m.oiep_Flags = OIEF_OPAQUE;
                m.oiep_Level = PNG_LEVEL;
                m.oiep_Data = NULL;
                m.oiep_Size = 0;
                m.oiep_Error = 0;
                if (DoMethodA(o, (Msg)&m) && m.oiep_Data) { data = m.oiep_Data; *size = m.oiep_Size; }
                if (own) FreeVec(argb);
            }
            DisposeObject(o);
        }
    }
    CloseLibrary(png);
    return data;
}

static int to_clipboard(const UBYTE *data, ULONG size)
{
    struct MsgPort *mp = CreateMsgPort();
    struct IOClipReq *io = mp ? (struct IOClipReq *)CreateIORequest(mp, sizeof *io) : NULL;
    int ok = 0;
    if (io && !OpenDevice((STRPTR)"clipboard.device", 0, (struct IORequest *)io, 0)) {
        io->io_Command = CMD_WRITE;
        io->io_Offset = 0;
        io->io_ClipID = 0;
        io->io_Data = (STRPTR)data;
        io->io_Length = size;
        ok = !DoIO((struct IORequest *)io) && io->io_Actual == size;
        io->io_Command = CMD_UPDATE;
        io->io_Length = 0;
        DoIO((struct IORequest *)io);
        ok = ok && !io->io_Error;
        CloseDevice((struct IORequest *)io);
    }
    if (io) DeleteIORequest((struct IORequest *)io);
    if (mp) DeleteMsgPort(mp);
    return ok;
}

static int write_file(const char *path, const UBYTE *data, ULONG size)
{
    BPTR fh = Open((STRPTR)path, MODE_NEWFILE);
    LONG err;
    if (!fh) return 0;
    if (Write(fh, (APTR)data, size) != (LONG)size) {
        err = IoErr();
        Close(fh);
        DeleteFile((STRPTR)path);           /* a part-written picture: our own, just made */
        SetIoErr(err);
        return 0;
    }
    if (!Close(fh)) return 0;
    return 1;
}

static void cant_save(const char *what)
{
    struct EasyStruct es = { sizeof(struct EasyStruct), 0, (UBYTE *)"OpenSnap",
                             (UBYTE *)"The snapshot couldn't be saved.\n\n%s: %s", (UBYTE *)"OK" };
    char why[96];
    Fault(IoErr(), NULL, (STRPTR)why, sizeof why);
    EasyRequest(NULL, &es, NULL, (ULONG)what, (ULONG)why);
}

static void snap(int kind, struct timerequest *tr)
{
    struct shot s;
    const char *why;
    struct DateStamp ds;
    struct ClockData cd;
    char name[96], path[400];
    UBYTE *png, *ilbm = NULL, *row = NULL;
    ULONG pngsize = 0, ilbmsize = 0, n;
    int clip = 0, saved = 0, asiff = 0;
    BPTR l;

    unsay();                                /* not in the picture */
    DateStamp(&ds);                         /* the moment of the key, not of the save */
    lap(kind == EV_WINDOW ? "window key" : "screen key");
    why = grab(kind, &s);
    lap("read");
    SetTaskPri(FindTask(NULL), PRI_SAVE);
    if (why) {
        char t[120];
        strcpy(t, "OpenSnap: ");
        strncat(t, why, sizeof t - 12);
        say(s.pubname, IntuitionBase->ActiveWindow, t, tr);
        SetTaskPri(FindTask(NULL), PRI_WAIT);
        return;
    }
    Amiga2Date(ds.ds_Days * 86400 + ds.ds_Minute * 60 + ds.ds_Tick / TICKS_PER_SECOND, &cd);

    png = encode_png(&s, &pngsize);
    lap(png ? "PNG" : "no PNG");
    asiff = !png;
    /* the ILBM: for the clipboard, and the file when there is no PNG */
    if ((set.clipboard || asiff) && (ilbm = AllocVec(osp_ilbm_max(&s.pic), MEMF_ANY))
        && (row = AllocVec(osp_rowbytes(s.pic.w) + 4, MEMF_ANY)))
        ilbmsize = osp_ilbm(&s.pic, ilbm, row);
    lap("ILBM");

    if (!make_drawer(set.drawer)) {
        cant_save(set.drawer);
    } else {
        for (n = 1; n < 100; n++) {         /* two in the same second: -2, -3... */
            osp_name(name, sizeof name, cd.year, cd.month, cd.mday, cd.hour, cd.min, cd.sec,
                     kind == EV_WINDOW ? s.title : "", n, asiff ? "iff" : "png");
            strncpy(path, set.drawer, sizeof path - 1); path[sizeof path - 1] = 0;
            AddPart((STRPTR)path, (STRPTR)name, sizeof path);
            if (!(l = Lock((STRPTR)path, ACCESS_READ))) break;
            UnLock(l);
        }
        if (asiff ? (ilbmsize && write_file(path, ilbm, ilbmsize)) : write_file(path, png, pngsize)) saved = 1;
        else if (asiff && !ilbmsize) { SetIoErr(ERROR_NO_FREE_STORE); cant_save(name); }
        else cant_save(name);
    }
    lap("file");
    if (set.clipboard && ilbmsize) clip = to_clipboard(ilbm, ilbmsize);
    lap("clipboard");

    if (saved || clip) {
        char t[200];
        t[0] = 0;
        if (saved) {
            strcpy(t, name);
            strcat(t, " saved");
            if (asiff) strcat(t, " (no PNG writer: OpenImage)");
        }
        if (set.clipboard) {
            if (saved) strcat(t, clip ? ", and on the clipboard" : "; not on the clipboard");
            else strcpy(t, clip ? "OpenSnap: on the clipboard" : "OpenSnap: not saved");
        }
        say(s.pubname, s.active, t, tr);
    }
    if (png) FreeVec(png);
    if (row) FreeVec(row);
    if (ilbm) FreeVec(ilbm);
    free_shot(&s);
    SetTaskPri(FindTask(NULL), PRI_WAIT);
}

/* ---- settings: the icon's tool types, then the command line's KEY=VALUE ------------------------ */

static void take(const char *key, const char *value)
{
    if (!value) return;
    if (!Stricmp((STRPTR)key, (STRPTR)"SCREENKEY")) { strncpy(set.screenkey, value, sizeof set.screenkey - 1); }
    else if (!Stricmp((STRPTR)key, (STRPTR)"WINDOWKEY")) { strncpy(set.windowkey, value, sizeof set.windowkey - 1); }
    else if (!Stricmp((STRPTR)key, (STRPTR)"DRAWER")) { strncpy(set.drawer, value, sizeof set.drawer - 1); }
    else if (!Stricmp((STRPTR)key, (STRPTR)"CLIPBOARD")) set.clipboard = !(value[0] == 'n' || value[0] == 'N' || value[0] == '0');
    else if (!Stricmp((STRPTR)key, (STRPTR)"CX_PRIORITY")) { LONG v; if (StrToLong((STRPTR)value, &v) > 0) set.cxpri = v; }
}

static const char *KEYS[] = { "SCREENKEY", "WINDOWKEY", "DRAWER", "CLIPBOARD", "CX_PRIORITY" };

static void from_icon(struct DiskObject *d)
{
    unsigned i;
    if (!d || !d->do_ToolTypes) return;
    for (i = 0; i < sizeof KEYS / sizeof KEYS[0]; i++)
        take(KEYS[i], (const char *)FindToolType((CONST_STRPTR *)d->do_ToolTypes, (STRPTR)KEYS[i]));
}

static void settings(int argc, char **argv)
{
    if (!IconBase) return;
    if (argc == 0) {                        /* from Workbench: its icon */
        struct WBStartup *wbs = (struct WBStartup *)argv;
        if (wbs && wbs->sm_NumArgs > 0) {
            BPTR old = CurrentDir(wbs->sm_ArgList[0].wa_Lock);
            struct DiskObject *d = GetDiskObject((STRPTR)wbs->sm_ArgList[0].wa_Name);
            from_icon(d);
            if (d) FreeDiskObject(d);
            CurrentDir(old);
        }
    } else {                                /* from a Shell or S:User-Startup: the icon beside it, then the line */
        char prog[256], icon[300];
        LONG a[5] = { 0, 0, 0, 0, 0 };
        struct RDArgs *rd;
        struct DiskObject *d;
        unsigned i;
        if (GetProgramName((STRPTR)prog, sizeof prog)) {
            strcpy(icon, "PROGDIR:");
            strncat(icon, (const char *)FilePart((STRPTR)prog), sizeof icon - 10);
            if ((d = GetDiskObject((STRPTR)icon))) { from_icon(d); FreeDiskObject(d); }
        }
        if ((rd = ReadArgs((STRPTR)"SCREENKEY/K,WINDOWKEY/K,DRAWER/K,CLIPBOARD/K,CX_PRIORITY/K", a, NULL))) {
            for (i = 0; i < 5; i++) take(KEYS[i], (const char *)a[i]);
            FreeArgs(rd);
        }
    }
}

/* ---- the commodity ----------------------------------------------------------------------------- */

static char descr[CBD_DESCRLEN];
static struct NewBroker nb = {
    NB_VERSION, (STRPTR)"OpenSnap", (STRPTR)"OpenSnap 1.0: pictures of the screen", (STRPTR)descr,
    NBU_UNIQUE | NBU_NOTIFY, 0, 0, NULL, 0
};

static int add_key(CxObj *broker, const char *key, struct MsgPort *port, LONG id)
{
    CxObj *hk;
    if (!key[0] || !Stricmp((STRPTR)key, (STRPTR)"NONE")) return 1;
    if (!(hk = HotKey((STRPTR)key, port, id))) return 0;
    AttachCxObj(broker, hk);
    if (CxObjError(hk)) { ClearCxObjError(hk); return 0; }
    return 1;
}

int main(int argc, char **argv)
{
    struct MsgPort *port = NULL, *tport = NULL;
    struct timerequest *tr = NULL;
    CxObj *broker = NULL;
    LONG err = 0;
    int quit = 0, timer = 0, rc = 20;
    (void)version;

    IntuitionBase = (struct IntuitionBase *)OpenLibrary((STRPTR)"intuition.library", 39);
    GfxBase = (struct GfxBase *)OpenLibrary((STRPTR)"graphics.library", 39);
    CxBase = OpenLibrary((STRPTR)"commodities.library", 37);
    UtilityBase = (void *)OpenLibrary((STRPTR)"utility.library", 37);
    IconBase = OpenLibrary((STRPTR)"icon.library", 37);
    CyberGfxBase = OpenLibrary((STRPTR)"cybergraphics.library", 41);   /* RTG screens; none on a plain Amiga */
    if (!IntuitionBase || !GfxBase || !CxBase || !UtilityBase) goto out;
    settings(argc, argv);

    if (!(port = CreateMsgPort())) goto out;
    {   /* GCC 16's -Wstringop-truncation stops a strncpy here */
        size_t c = strlen(set.screenkey);
        if (c > sizeof descr - 1) c = sizeof descr - 1;
        memcpy(descr, set.screenkey, c); descr[c] = 0;
    }
    if (strlen(descr) + strlen(set.windowkey) + 3 < sizeof descr) { strcat(descr, ", "); strcat(descr, set.windowkey); }
    nb.nb_Port = port;
    nb.nb_Pri = (BYTE)set.cxpri;
    if (!(broker = CxBroker(&nb, &err))) { rc = err == CBERR_DUP ? 0 : 20; goto out; }   /* running already */
    if (!add_key(broker, set.screenkey, port, EV_SCREEN) && argc) Printf("OpenSnap: \"%s\" isn't a hotkey commodities knows\n", (ULONG)set.screenkey);
    if (!add_key(broker, set.windowkey, port, EV_WINDOW) && argc) Printf("OpenSnap: \"%s\" isn't a hotkey commodities knows\n", (ULONG)set.windowkey);
    ActivateCxObj(broker, 1);
    if ((tport = CreateMsgPort()) && (tr = (struct timerequest *)CreateIORequest(tport, sizeof *tr)))
        timer = !OpenDevice((STRPTR)TIMERNAME, UNIT_VBLANK, (struct IORequest *)tr, 0);
    SetTaskPri(FindTask(NULL), PRI_WAIT);
    rc = 0;

    while (!quit) {
        ULONG got = Wait((1UL << port->mp_SigBit) | SIGBREAKF_CTRL_C | (timer ? 1UL << tport->mp_SigBit : 0));
        CxMsg *m;
        if (got & SIGBREAKF_CTRL_C) quit = 1;
        if (tpending && CheckIO((struct IORequest *)tr)) {   /* the message's three seconds are up */
            WaitIO((struct IORequest *)tr);
            tpending = 0;
            unsay();
        }
        while ((m = (CxMsg *)GetMsg(port))) {
            ULONG type = CxMsgType(m), id = CxMsgID(m);
            ReplyMsg((struct Message *)m);
            if (type == CXM_IEVENT && (id == EV_SCREEN || id == EV_WINDOW)) snap((int)id, timer ? tr : NULL);
            else if (type == CXM_COMMAND) switch (id) {
            case CXCMD_DISABLE: ActivateCxObj(broker, 0); break;
            case CXCMD_ENABLE: ActivateCxObj(broker, 1); break;
            case CXCMD_KILL: quit = 1; break;
            case CXCMD_UNIQUE: break;       /* started again: the one running stays */
            }
        }
    }
out:
    unsay();
    if (broker) DeleteCxObjAll(broker);
    if (port) { struct Message *msg; while ((msg = GetMsg(port))) ReplyMsg(msg); DeleteMsgPort(port); }
    if (tpending) { AbortIO((struct IORequest *)tr); WaitIO((struct IORequest *)tr); }
    if (timer) CloseDevice((struct IORequest *)tr);
    if (tr) DeleteIORequest((struct IORequest *)tr);
    if (tport) DeleteMsgPort(tport);
    if (CyberGfxBase) CloseLibrary(CyberGfxBase);
    if (IconBase) CloseLibrary(IconBase);
    if (UtilityBase) CloseLibrary((struct Library *)UtilityBase);
    if (CxBase) CloseLibrary(CxBase);
    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    return rc;
}
