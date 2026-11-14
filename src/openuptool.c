/* GFOpenUpTool: what GFOpenUp's Installer scripts call (DESIGN.md section 3).
 *
 *   GFOpenUpTool DETECT                 what this machine is; ENV:GFOpenUp/... (ROM: GFOpenKick or Kickstart,
 *                                     0.8.7; GFOpenRTG: where gfopenrtg.library is, 0.8.7)
 *   GFOpenUpTool INSTALL <part drawer>  installs one part, as its Part file says; a file on the disk whose $VER is
 *                                     newer than the pack's stays, and is said (FORCE puts the pack's in; 14 Nov 2026)
 *   GFOpenUpTool REMOVE <part drawer>   takes one part out, putting back what it replaced (0.8.0)
 *   GFOpenUpTool REINSTALL <Parts drawer>  every part the manifest lists, in again from the pack: missing or
 *                                     changed files put back, the settings in ENVARC: and ENV: kept (9 Nov 2026)
 *   GFOpenUpTool STARTUP                writes the marked block in S:User-Startup
 *   GFOpenUpTool UNINSTALL              puts everything back
 *   GFOpenUpTool FIXTOOLS <table> [ALL] puts back default tools that GFOpenUp 0.6.14 to 0.7.0 changed
 *                                     to GFOpenView by mistake (see FIXTOOLS below)
 *   GFOpenUpTool LIST                   what is installed
 *   GFOpenUpTool SETTING <dest> <text>  a setting the Installer asked for, as a file
 *   GFOpenUpTool THEME <name> <look>    the look's theme: a saved look keeps its mode and other lines (see THEME)
 *   GFOpenUpTool BACKDROP <prefix>      Workbench's backdrop: <prefix>-strip-<H>.png for the
 *                                     screen's height, tiled, when there is one; else <prefix>-<W>x<H>.png, the
 *                                     one nearest the screen's shape, scaled to fit; KEEP (an upgrade from
 *                                     an OpenUp before the GF names): the backdrop the user had (as OLDNAMES
 *                                     found it) stays when its picture is there, or moves to the same picture
 *                                     in GFOpenUp's drawer
 *   GFOpenUpTool FONT <font> <size>     the desktop's text: the screens' font and the icons'
 *                                     text (Font prefs, "Screen text" and "Workbench icon text")
 *   GFOpenUpTool TITLEBARS <screen> <window>  extra pixels in the title bars (IControl
 *                                     prefs, Appearance Settings, Title Bar Extra Height)
 *   GFOpenUpTool WBBACKDROP             Workbench as a backdrop: no frame, scroll bars or size gadget on
 *                                     its window (Workbench's prefs, WBConfig.prefs)
 *   GFOpenUpTool OLDLOOK                takes out the look's files GFOpenUp up to 0.8.1 wrote (C:GFOpenLook,
 *                                     the theme) where they are as it wrote them (Font and IControl
 *                                     prefs stay: they are the user's settings, 0.8.7),
 *                                     and the default icons in ENVARC:Sys and SYS:Disk.info: OS 3.2.3's
 *                                     own and GFOpenUp's faces of them (0.8.3, 0.8.4), and Gorilla's faces
 *                                     on the other icons, each where it is still as installed (0.8.4)
 *   GFOpenUpTool WBGAUGE                No fill gauge in a drive's window (Workbench's prefs, "No
 *                                     volume gauge": the bar down the left border that shows how
 *                                     full the disk is; the border is slim like any window's)
 *   GFOpenUpTool P96LEFTOVER            5 (WARN) on a real Amiga with Picasso96's
 *                                     library left in LIBS: and no Picasso96 board running
 *   GFOpenUpTool RECONCILE [CHECK|APPLY [ONCE]|UNDO]  one desktop, the Open one: old prefs editors to
 *                                     SYS:Prefs/Classic, replaced tools retired, settings GFOpenUp owns kept in
 *                                     step; CHECK says what it would do (see the RECONCILE section)
 *   GFOpenUpTool TIDY [ONCE]            the system's drawers cleaned up, sized to fit (at most 800 x 600) and
 *                                     snapshotted by Workbench itself, so they open neat (see TIDY)
 *   GFOpenUpTool ICONS [CHECK|APPLY]    old icons (four colours, NewIcons) found on SYS: and Work: and given
 *                                     our faces, by name from GFOpenIcons' set, else by kind (see ICONS)
 *   GFOpenUpTool ICONSIG <icon|drawer>  each icon's picture and whole signatures (see OLDLOOK)
 *   GFOpenUpTool RETIRE <file>          moves a file to SYS:Storage/GFOpenUp-Superseded, to
 *                                     come back with UNINSTALL
 *   GFOpenUpTool OLDNAMES [DOCK]        an OpenUp from before the GF rename (0.8.11 and earlier, 0.8.13): a copy of
 *                                     its list and S:User-Startup kept, and its settings copied to the GF names
 *                                     (ENV: and ENVARC:); DOCK: the dock's buttons for its programs on the GF ones
 *   GFOpenUpTool UNPLACE <program>      drops the program's windows from GFOpenWindows' Places
 *                                     (ENV: and ENVARC:GFOpenPrefs/WindowPlaces), so they open
 *                                     at the program's own first size again
 *   GFOpenUpTool KICKSET [CHECK] [<part drawer>]  the slim ROM's module set (the KickstartSet part): installs it,
 *                                     or CHECK says whether SYS:Kickstart/default/ has it whole (9 Nov 2026, KICKSET below)
 *   GFOpenUpTool STARTUPSEQ [CHECK|COMPARE|KEEP|ROM]  the disk's S:Startup-Sequence against the ROM's default
 *                                     (amicradle-guest docs/design/rom-or-disk.md, section 6; 9 Nov 2026): CHECK
 *                                     says how they compare, WARN (5) when the disk's is older or not the ROM's and
 *                                     the user hasn't kept it against this ROM's version; COMPARE lists the lines
 *                                     that differ; KEEP remembers the user's choice (ENVARC:GFOpenAmiga/
 *                                     StartupSequence.kept, the ROM's version); ROM puts the ROM's default in its
 *                                     place, the user's kept in SYS:Storage/GFOpenUp-Superseded for UNINSTALL.
 *                                     Never at boot, only when the user chooses (Install-GFOpenUp asks)
 *   GFOpenUpTool UPDATE [CHECK|YES]     a newer pack Cradle put in DH1:GFOpenUp: CHECK says
 *                                     (WARN when one waits); UPDATE shows what changes and
 *                                     asks; YES doesn't ask. It never runs on its own
 *
 * The rules it keeps:
 * - The base OS stays as shipped. A file GFOpenUp replaces goes first to
 *   SYS:Storage/GFOpenUp-Superseded/ (its path, with the colon as a slash),
 *   and only the first time: that copy is the original.
 * - It never overwrites another stack: a part's "keep" lines name files that
 *   must already be ours (a marker string inside them) or absent.
 * - Every file, directory and startup line it adds is listed in the
 *   manifest, ENVARC:GFOpenAmiga/GFOpenUp.manifest (and ENV:), so UNINSTALL can
 *   take exactly those away and put the originals back.
 * - Startup lines go in one marked block in S:User-Startup:
 *   ";BEGIN GFOpenUp" to ";END GFOpenUp". A line listed in
 *   ENVARC:GFOpenAmiga/Startup.off (GFOpenUp Setup's switches) is written with
 *   ";OFF " in front, so AmigaDOS skips it, and a line whose program isn't on
 *   the disk with ";MISSING " (STARTUP writes it back in once it is there).
 *   A part's lines are the ones its Part file has now (0.7.5).
 * - Errors say what to do next, in plain words.
 *
 * A part's drawer holds its files, laid out as they go, and a Part file:
 *   part <name> <version>
 *   needs AmiCradle                  (only on an AmiCradle instance)
 *   needs ACRTG                        (only with an ACRTG graphics board)
 *   needs Kickstart                    (not on the GFOpenKick ROM: for OS 3.2's LoadModule, 0.8.7)
 *   needs part <name>                  (25 Oct 2026: only once the part <name> is installed, and REMOVE of <name>
 *                                       refuses while this part is installed; the GFOffice Suite's programs need
 *                                       the GFOffice part, their shared files; src/openup_lines.h)
 *   keep <dest> <marker> [<marker>...]  (dest must be absent or contain a marker)
 *   copy <file in the drawer> <dest>
 *   default <file in the drawer> <dest> (copied only when dest is missing)
 *   add <file in the drawer> <dest>     (copied when dest is missing or GFOpenUp
 *                                        put it there; another program's stays)
 *   retire <dest>                       (moved to SYS:Storage/GFOpenUp-Superseded,
 *                                        so it no longer runs; UNINSTALL puts it back)
 *   face <icon in the drawer> <dest .info>  (0.6: the icon's picture only; dest keeps
 *                                        its type, tools, tool types and window;
 *                                        left alone when the machine hasn't got it)
 *   tooltype <dest .info> <from> <to>   (24 Oct 2026: one tool type of an icon that stays the
 *                                        system's: where it is <from> exactly it becomes <to>, and
 *                                        the rest of the icon stays; REMOVE puts <from> back)
 *   startup <the rest of the line>
 *   when AmiCradle <copy|default|add|face|retire line>  (0.6.3: the line only on an AmiCradle instance)
 *   unless AmiCradle <line>, when GFOpenKick <line>, unless GFOpenKick <line>  (0.8.9, any line, startup lines
 *                                        too: GFOpenKick is the open ROM whose intuition applies the boot's
 *                                        settings, prefs.resource 47.101 or later; src/openup_lines.h)
 * Words with spaces are quoted.
 *
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <datatypes/datatypes.h>
#include <datatypes/datatypesclass.h>
#include <datatypes/pictureclass.h>
#include <proto/datatypes.h>
#include <clib/alib_protos.h>
#include <rexx/storage.h>
#include <rexx/rxslib.h>
#include <proto/rexxsyslib.h>
#include <libraries/configvars.h>
#include <libraries/expansionbase.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/expansion.h>
#include <proto/intuition.h>
#include <intuition/screens.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <proto/icon.h>

#include <string.h>
#include <stdlib.h>

#include "openup_lines.h"
#include "openup_startupseq.h"
#include "openup_kickset.h"
#include "openup_slimrom.h"            /* DETECT SlimROM (0.8.x) */
#include "openup_vers.h"               /* an older file never over a newer one */
#include "openup_sysfont.h"            /* Font prefs' system font: DejaVuSansMono 11 to GFOSMono 11 */
#include "openup_left.h"               /* REINSTALL: the parts the pack hasn't got, for the Installer */
#include "openup_unattended.h"          /* UPDATE AUTO (0.8.20) */

static int slim_rom(void);
static int kickset(const char *a1, const char *a2);

static const char version[] __attribute__((used)) = "$VER: GFOpenUpTool 0.8.21 (14.11.2026) MIT, Copyright (c) 2026 AmiCradle Team";

struct ExpansionBase *ExpansionBase;
struct Library *IconBase;

static int same_word(const char *a, const char *b)
{
    for (; *a && *b; a++, b++)
        if ((*a | 0x20) != (*b | 0x20)) return 0;
    return !*a && !*b;
}

#define MANIFEST "ENVARC:GFOpenAmiga/GFOpenUp.manifest"
#define MANIFEST_ENV "ENV:GFOpenAmiga/GFOpenUp.manifest"
#define SUPERSEDED "SYS:Storage/GFOpenUp-Superseded"
#define USER_STARTUP "S:User-Startup"
#define BLOCK_BEGIN ";BEGIN GFOpenUp"
#define BLOCK_END ";END GFOpenUp"
#define STARTUP_OFF "ENVARC:GFOpenAmiga/Startup.off"

#define AMICRADLE_MFR 0xDA15
#define AMICRADLE_MFR_OLD 2011

/* ---- small helpers ----------------------------------------------------------- */

/* strlcat, which libnix hasn't got: appends src to dst within size, always ending it. */
static size_t str_cat(char *dst, const char *src, size_t size)
{
    size_t d = strlen(dst), i = 0;
    if (d + 1 >= size) return d + strlen(src);
    while (src[i] && d + i + 1 < size) { dst[d + i] = src[i]; i++; }
    dst[d + i] = 0;
    return d + strlen(src);
}

static void say(const char *a, const char *b, const char *c)
{
    PutStr((STRPTR)a);
    if (b) PutStr((STRPTR)b);
    if (c) PutStr((STRPTR)c);
    PutStr((STRPTR)"\n");
}

/* No "Please insert volume" requester: a volume the machine hasn't got (a
 * face line's Work:, say) is simply not there. */
static int exists(const char *path)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    APTR win = me->pr_WindowPtr;
    BPTR l;
    me->pr_WindowPtr = (APTR)-1;
    l = Lock((STRPTR)path, ACCESS_READ);
    me->pr_WindowPtr = win;
    if (l) UnLock(l);
    return l != 0;
}

/* 1 when path is there and is a file, not a drawer (no requester either, as exists). */
static int is_file(const char *path)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    APTR win = me->pr_WindowPtr;
    struct FileInfoBlock *fib;
    BPTR l;
    int file = 0;
    me->pr_WindowPtr = (APTR)-1;
    l = Lock((STRPTR)path, ACCESS_READ);
    me->pr_WindowPtr = win;
    if (!l) return 0;
    if ((fib = AllocDosObject(DOS_FIB, NULL))) {
        if (Examine(l, fib)) file = fib->fib_DirEntryType < 0;
        FreeDosObject(DOS_FIB, fib);
    }
    UnLock(l);
    return file;
}

/* 0.7.8: a synchronous SystemTags never gets a NULL input or output (12 Oct 2026), as GFOpenDock 0.6.4 and
 * GFOpenMenus 0.4.4 give their programs (gfopenprefs #94). quiet: both are NIL:. Otherwise the command gets
 * GFOpenUpTool's own input and output when it has them (a Shell, or GFOpenUp Update's window, where the messages
 * belong) and NIL: in place of either it hasn't got (started from Workbench). A synchronous call leaves the
 * handles to the caller, so the NIL: ones opened here are closed here. -1 when NIL: can't be opened. */
static LONG run_sync(const char *cmd, int quiet)
{
    BPTR in = quiet ? 0 : Input(), out = quiet ? 0 : Output();
    BPTR nil_in = in ? 0 : Open((STRPTR)"NIL:", MODE_OLDFILE), nil_out = out ? 0 : Open((STRPTR)"NIL:", MODE_NEWFILE);
    LONG rc = -1;
    if ((in || nil_in) && (out || nil_out))
        rc = SystemTags((STRPTR)cmd, SYS_Input, (ULONG)(in ? in : nil_in), SYS_Output, (ULONG)(out ? out : nil_out),
                        TAG_DONE);
    if (nil_in) Close(nil_in);
    if (nil_out) Close(nil_out);
    return rc;
}

/* The whole file, NUL-ended, in memory of its own (FreeVec it); NULL when it can't be had, with IoErr saying
 * why: ERROR_OBJECT_NOT_FOUND only when there is no such file, so a caller starts a file afresh only then (1 Nov
 * 2026: a read that failed for any reason was taken as no file, and S:User-Startup was written anew, without the
 * user's lines). */
static char *read_file(const char *path, LONG *lenp)
{
    BPTR fh = Open((STRPTR)path, MODE_OLDFILE);
    LONG len, err = 0;
    char *buf = NULL;
    if (!fh)
        return NULL;
    Seek(fh, 0, OFFSET_END);
    len = Seek(fh, 0, OFFSET_BEGINNING);
    if (len < 0)
        err = IoErr();
    else if (!(buf = AllocVec(len + 1, MEMF_ANY)))
        err = ERROR_NO_FREE_STORE;
    else if (Read(fh, buf, len) != len) {
        err = IoErr();
        FreeVec(buf);
        buf = NULL;
    }
    Close(fh);
    if (!buf) {
        SetIoErr(err && err != ERROR_OBJECT_NOT_FOUND ? err : ERROR_SEEK_ERROR);
        return NULL;
    }
    buf[len] = 0;
    if (lenp) *lenp = len;
    return buf;
}

/* ---- writing a file so that a failure leaves the old one (1 Nov 2026) ------------------------------
 * A file was written over where it was: cut to nothing first, so a full SYS: left a library in use short, and a
 * short manifest lost the originals UNINSTALL puts back. Now the new bytes go to a short name beside the file and
 * take its name only once they are all written and closed (put_in_place); on the RAM disk they are written in
 * place as before (in_ram). Files are copied and compared 64 KB at a time, not read whole (OpenBrowser's 80 MB,
 * twice). */

#define CHUNK 65536

/* 1 when nothing has the name, 0 when something has, -1 when it can't be told (a volume not there): no
 * "Please insert volume" requester, as exists. */
static int name_free(const char *path)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    APTR win = me->pr_WindowPtr;
    BPTR l;
    LONG err;
    me->pr_WindowPtr = (APTR)-1;
    l = Lock((STRPTR)path, ACCESS_READ);
    err = l ? 0 : IoErr();
    me->pr_WindowPtr = win;
    if (l) {
        UnLock(l);
        return 0;
    }
    return err == ERROR_OBJECT_NOT_FOUND ? 1 : -1;
}

/* Two names beside path, nothing there with either: the new copy's (.tmp) and the old file's while the new one
 * takes its name (.bak). GFOpenUp, the task's address and a count in hex: 22 characters, for a 30-character file
 * system. 1, else 0. */
static int side_names(const char *path, char *temp, char *backup, int size)
{
    static const char hex[] = "0123456789abcdef";
    static UBYTE count;
    ULONG id = (ULONG)FindTask(NULL);
    int dir = strlen(path), tries, i, t, b = 0;
    while (dir && path[dir - 1] != '/' && path[dir - 1] != ':') dir--;
    if (dir + 23 > size)
        return 0;
    for (tries = 0; tries < 256; tries++) {
        unsigned n = count++;
        char *name = temp + dir;
        memcpy(temp, path, dir);
        memcpy(name, "GFOpenUp", 8);
        for (i = 0; i < 8; i++) name[8 + i] = hex[(id >> (28 - 4 * i)) & 15];
        name[16] = hex[(n >> 4) & 15];
        name[17] = hex[n & 15];
        strcpy(name + 18, ".tmp");
        strcpy(backup, temp);
        strcpy(backup + dir + 18, ".bak");
        if ((t = name_free(temp)) < 0 || (t && (b = name_free(backup)) < 0))
            return 0;
        if (t && b)
            return 1;
    }
    return 0;
}

/* The new copy, temp (written and closed), takes dest's name. A file there steps aside as backup first (Rename
 * doesn't replace a file) and goes once the new copy has its name; if the new copy can't take the name, the old
 * file gets it back at once. keep_bits: the new copy gets the old file's protection bits and comment (a file
 * written anew, as S:User-Startup). A drawer, or a file protected from writing or deleting, isn't replaced: it
 * couldn't be written over before either. temp goes whenever it doesn't take the name. 1 done. */
static int put_in_place(const char *temp, const char *backup, const char *dest, int keep_bits)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    APTR win = me->pr_WindowPtr;
    struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
    BPTR l;
    int ok = 0;
    me->pr_WindowPtr = (APTR)-1;
    l = fib ? Lock((STRPTR)dest, ACCESS_READ) : 0;
    me->pr_WindowPtr = win;
    if (!fib) {
        /* no memory to look at it with: nothing replaced */
    } else if (!l) {
        ok = Rename((STRPTR)temp, (STRPTR)dest) != 0;           /* refused when something is there after all */
    } else {
        ok = Examine(l, fib) && fib->fib_DirEntryType < 0 && !(fib->fib_Protection & (FIBF_WRITE | FIBF_DELETE));
        UnLock(l);
        if (ok && keep_bits) {
            SetProtection((STRPTR)temp, fib->fib_Protection & ~FIBF_ARCHIVE);    /* new bytes: not backed up yet */
            if (fib->fib_Comment[0]) SetComment((STRPTR)temp, (STRPTR)fib->fib_Comment);
        }
        if (ok && !Rename((STRPTR)dest, (STRPTR)backup)) {
            ok = 0;
        } else if (ok && !Rename((STRPTR)temp, (STRPTR)dest)) {
            Rename((STRPTR)backup, (STRPTR)dest);
            ok = 0;
        } else if (ok && !DeleteFile((STRPTR)backup)) {
            say("GFOpenUp: the old ", dest, " is still in use, so it is kept beside it until it can be deleted:");
            say("  ", backup, NULL);
        }
    }
    if (!ok) DeleteFile((STRPTR)temp);
    if (fib) FreeDosObject(DOS_FIB, fib);
    return ok;
}

/* 1 when path is on the RAM disk (ENV:, T:). A file there is written in place, as before: OS 3.2's RAM:ENV shows
 * ENVARC:'s files through a link, and while ENVARC: has one, its name in ENV: stays taken after Rename has moved
 * it (lab t4-openup-20261101: "object already exists"). The RAM disk is gone at a reboot anyway, and ENVARC: and
 * SYS: keep their own copies. */
static int in_ram(const char *path)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    APTR win = me->pr_WindowPtr;
    struct DevProc *p, *r;
    int ram;
    me->pr_WindowPtr = (APTR)-1;
    p = GetDeviceProc((STRPTR)path, NULL);
    r = GetDeviceProc((STRPTR)"RAM:", NULL);
    me->pr_WindowPtr = win;
    ram = p && r && p->dvp_Port == r->dvp_Port;
    if (p) FreeDeviceProc(p);
    if (r) FreeDeviceProc(r);
    return ram;
}

static int write_file(const char *path, const char *data, LONG len)
{
    char temp[260], backup[260];
    int ram = in_ram(path), ok;
    BPTR fh;
    if (ram) fh = Open((STRPTR)path, MODE_NEWFILE);
    else if (!side_names(path, temp, backup, sizeof temp)) return 0;
    else fh = Open((STRPTR)temp, MODE_NEWFILE);
    if (!fh)
        return 0;
    ok = Write(fh, (APTR)data, len) == len;
    if (!Close(fh)) ok = 0;
    if (ram) return ok;
    if (!ok) {
        DeleteFile((STRPTR)temp);
        return 0;
    }
    return put_in_place(temp, backup, path, 1);
}

/* ---- the manifest -------------------------------------------------------------- */

/* One line each: kind, then up to three tab-separated fields. Kinds:
 *   part <name> <version>   new <dest>   replaced <dest> <backup>
 *   same <dest>   dir <path>   bdir <path>   startup <part> <line>
 *   newfile <path> (a file GFOpenUp made, such as S:User-Startup)
 *   needpart <part> <the part it needs> (a "needs part" line of an installed part's Part file, 25 Oct 2026)
 * 1024 entries was too few (0.7.3 and earlier): every part ticked comes to about 1,150 lines in 0.7.5,
 * an upgrade keeps the lines of files earlier versions put in, and a lab copy of an instance upgraded
 * from 0.7.3 to 0.7.4 filled the table, so new files and a part's new startup line went unlisted. */
#define MAXE 4096
/* a and b are whole paths, each in an allocation of its own length (libnix
 * gives them all back at exit): a path is never cut short in the manifest,
 * where a cut path would send UNINSTALL to the wrong file, and the table
 * stays small. Neither is ever NULL: "" is no_text. */
#define ENTRY_PATH 260                       /* the install code's path buffers */
typedef oul_entry entry;                     /* kind, a, b (src/openup_lines.h, which the host test shares) */
static entry ents[MAXE];
static int nents;
static char no_text[] = "";
static int manifest_short;                   /* not read whole (cut short, unreadable, out of memory): never save it */

/* Sets a or b to a copy of s; 0 (and the field as it was) when out of memory. */
static int set_text(char **field, const char *s)
{
    char *d;
    if (!*s) d = no_text;
    else if (!(d = malloc(strlen(s) + 1))) return 0;
    else strcpy(d, s);
    if (*field && *field != no_text) free(*field);
    *field = d;
    return 1;
}

static void manifest_load(void)
{
    LONG len = 0;
    char *text = read_file(MANIFEST, &len), *p, *line;
    nents = 0;
    manifest_short = 0;
    if (!text) {
        if (IoErr() != ERROR_OBJECT_NOT_FOUND)
            manifest_short = 1;             /* there but not read: saving would put an empty list over it */
        return;
    }
    if (!oul_manifest_whole(text, len))
        manifest_short = 1;                 /* cut short (src/openup_lines.h): read what is there, never save it */
    for (p = text; *p && nents < MAXE; ) {
        char *f[3] = { NULL, NULL, NULL };
        int n = 0;
        line = p;
        while (*p && *p != '\n') p++;
        if (*p) *p++ = 0;
        f[n++] = line;
        for (char *q = line; *q && n < 3; q++)
            if (*q == '\t') { *q = 0; f[n++] = q + 1; }
        if (!strcmp(f[0], "GFOpenUp") || !f[0][0])
            continue;
        strlcpy(ents[nents].kind, f[0], sizeof ents[0].kind);
        if (!set_text(&ents[nents].a, f[1] ? f[1] : "") || !set_text(&ents[nents].b, f[2] ? f[2] : "")) {
            manifest_short = 1;
            break;
        }
        nents++;
    }
    while (*p == '\n') p++;
    if (*p && nents >= MAXE)
        manifest_short = 1;                 /* more lines than the table holds: saving would lose them */
    FreeVec(text);
}

static int ensure_dir(const char *path, int record);

static int manifest_save(void)
{
    static int said;
    LONG size = 64, len = 0;
    char *buf, *p;
    int i, ok;
    if (manifest_short) {                   /* a list loaded short would lose files */
        if (!said++)
            say("GFOpenUp: its list of installed files (" MANIFEST ") is cut short or can't be read, so nothing is "
                "written over it: that would lose what it lists. If " MANIFEST_ENV " is whole, copy it in its place.",
                NULL, NULL);
        return 0;
    }
    for (i = 0; i < nents; i++)
        size += strlen(ents[i].kind) + strlen(ents[i].a) + strlen(ents[i].b) + 3;
    if (!(buf = AllocVec(size, MEMF_ANY)))
        return 0;
    p = buf;
    strcpy(p, OUL_MANIFEST_HEAD "\n");
    p += strlen(p);
    for (i = 0; i < nents; i++) {
        if (!ents[i].kind[0])
            continue;                       /* taken out */
        strcpy(p, ents[i].kind); p += strlen(p);
        *p++ = '\t'; strcpy(p, ents[i].a); p += strlen(p);
        if (ents[i].b[0]) { *p++ = '\t'; strcpy(p, ents[i].b); p += strlen(p); }
        *p++ = '\n';
    }
    strcpy(p, OUL_MANIFEST_END "\n");     /* the list is whole (oul_manifest_whole) */
    p += strlen(p);
    len = p - buf;
    ensure_dir("ENVARC:GFOpenAmiga", 1);
    ensure_dir("ENV:GFOpenAmiga", 0);
    ok = write_file(MANIFEST, buf, len);
    write_file(MANIFEST_ENV, buf, len);
    FreeVec(buf);
    return ok;
}

static entry *find_entry(const char *kind, const char *a)
{
    int i;
    for (i = 0; i < nents; i++)
        if (!strcmp(ents[i].a, a) && (!kind || !strcmp(ents[i].kind, kind)))
            return &ents[i];
    return NULL;
}

static entry *add_entry(const char *kind, const char *a, const char *b)
{
    static int said_full;
    entry *e;
    if (nents >= MAXE) {
        if (!said_full++) say("GFOpenUp: its list of installed files (" MANIFEST ") is full.", NULL, NULL);
        return NULL;
    }
    e = &ents[nents];
    if (!set_text(&e->a, a) || !set_text(&e->b, b ? b : ""))
        return NULL;
    strlcpy(e->kind, kind, sizeof e->kind);
    nents++;
    return e;
}

/* ---- files ---------------------------------------------------------------------- */

/* Makes the directories up to path (not a volume), noting those it made. */
static int ensure_dir(const char *path, int record)
{
    char buf[ENTRY_PATH];
    char *p;
    strlcpy(buf, path, sizeof buf);
    for (p = strchr(buf, ':') ? strchr(buf, ':') + 1 : buf; ; p++) {
        if (*p == '/' || *p == 0) {
            char c = *p;
            *p = 0;
            if (buf[strlen(buf) - 1] != ':' && !exists(buf)) {
                BPTR l = CreateDir((STRPTR)buf);
                if (!l)
                    return 0;
                UnLock(l);
                if (record && !find_entry("dir", buf))
                    add_entry("dir", buf, NULL);
            }
            *p = c;
            if (!c)
                break;
        }
    }
    return 1;
}

static void parent_of(const char *path, char *out, int outlen)
{
    char *slash;
    strncpy(out, path, outlen - 1);
    out[outlen - 1] = 0;
    slash = strrchr(out, '/');
    if (slash) *slash = 0;
    else if ((slash = strchr(out, ':'))) slash[1] = 0;
}

/* 1 when both files have the same bytes, compared a chunk at a time. */
static int same_file(const char *a, const char *b)
{
    BPTR fa = Open((STRPTR)a, MODE_OLDFILE), fb = fa ? Open((STRPTR)b, MODE_OLDFILE) : 0;
    char *buf = fb ? AllocVec(2 * CHUNK, MEMF_ANY) : NULL;
    LONG na, nb;
    int same = 0;
    if (buf) {
        Seek(fa, 0, OFFSET_END);
        Seek(fb, 0, OFFSET_END);
        if ((na = Seek(fa, 0, OFFSET_BEGINNING)) >= 0 && na == Seek(fb, 0, OFFSET_BEGINNING))
            for (;;) {
                na = Read(fa, buf, CHUNK);
                nb = Read(fb, buf + CHUNK, CHUNK);
                if (na < 0 || na != nb || memcmp(buf, buf + CHUNK, na)) break;
                if (!na) { same = 1; break; }
            }
        FreeVec(buf);
    }
    if (fb) Close(fb);
    if (fa) Close(fa);
    return same;
}

/* Copies a file, its protection bits too, making the directories it needs: a chunk at a time into a new file
 * beside to, which takes to's name once it is all written (put_in_place). A copy that fails leaves to as it was.
 * On the RAM disk the copy goes straight to to, as before (in_ram). */
static int copy_file(const char *from, const char *to, int record_dirs)
{
    char dir[200], temp[260], backup[260];
    const char *out_name = temp;
    struct FileInfoBlock *fib;
    BPTR in, out, l;
    char *buf;
    LONG n = -1;
    int ok = 0, ram = in_ram(to);
    if (ram) out_name = to;
    if (!(in = Open((STRPTR)from, MODE_OLDFILE)))
        return 0;
    parent_of(to, dir, sizeof dir);
    if ((dir[strlen(dir) - 1] == ':' || ensure_dir(dir, record_dirs)) && (ram || side_names(to, temp, backup, sizeof temp))
        && (buf = AllocVec(CHUNK, MEMF_ANY))) {
        if ((out = Open((STRPTR)out_name, MODE_NEWFILE))) {
            while ((n = Read(in, buf, CHUNK)) > 0 && Write(out, buf, n) == n)
                ;
            ok = n == 0;                                /* to the end, every chunk written */
            if (!Close(out)) ok = 0;
            if (!ok && !ram) DeleteFile((STRPTR)temp);
        }
        FreeVec(buf);
    }
    Close(in);
    if (!ok || (!ram && !put_in_place(temp, backup, to, 0)))
        return 0;
    if ((fib = AllocDosObject(DOS_FIB, NULL))) {
        if ((l = Lock((STRPTR)from, ACCESS_READ))) {
            if (Examine(l, fib))
                SetProtection((STRPTR)to, fib->fib_Protection);
            UnLock(l);
        }
        FreeDosObject(DOS_FIB, fib);
    }
    return 1;
}

/* ---- what GFOpenUp wrote (9 Nov 2026; src/openup_lines.h, oul_mark) ---------------------------------------------- */

/* The file's size and CRC-32, as a "new" line's mark (out: 24 bytes); 0 when it isn't a file that can be read whole. */
static int file_mark(const char *path, char *out)
{
    BPTR fh;
    char *buf;
    LONG n;
    unsigned long crc = 0, size = 0;
    int ok = 0;
    if (!is_file(path) || !(fh = Open((STRPTR)path, MODE_OLDFILE)))
        return 0;
    if ((buf = AllocVec(CHUNK, MEMF_ANY))) {
        while ((n = Read(fh, buf, CHUNK)) > 0) { crc = oul_crc32(crc, (const unsigned char *)buf, n); size += n; }
        ok = n == 0;
        FreeVec(buf);
    }
    Close(fh);
    if (ok) oul_mark(out, size, crc);
    return ok;
}

/* A "new" line's mark set to the file as it is now (GFOpenUp's bytes, just written or found the same as its own). */
static void note_mark(entry *e)
{
    char mark[24];
    if (e && !strcmp(e->kind, "new") && file_mark(e->a, mark))
        set_text(&e->b, mark);
}

/* 1 when a "new" line's file is still what GFOpenUp wrote, or the line has no mark to tell by (oul_still_ours) */
static int still_ours(const entry *e)
{
    char mark[24];
    unsigned long size = 0, crc = 0;
    int there = file_mark(e->a, mark) && oul_mark_read(mark, &size, &crc);
    return oul_still_ours(e->b, there, size, crc);
}

/* Where a file's original goes: SYS:Storage/GFOpenUp-Superseded/LIBS/x. */
static void backup_path(const char *dest, char *out, int outlen)
{
    char *p;
    strcpy(out, SUPERSEDED "/");
    strncat(out, dest, outlen - strlen(out) - 1);
    for (p = out + strlen(SUPERSEDED) + 1; *p; p++)
        if (*p == ':') *p = '/';
}

/* 1 when the file holds one of the markers (it is ours, under any name). */
static int has_marker(const char *path, char **markers, int n)
{
    LONG len;
    char *data = read_file(path, &len);
    int i, found = 0;
    LONG k;
    if (!data)
        return 0;
    for (i = 0; i < n && !found; i++) {
        LONG ml = strlen(markers[i]);
        for (k = 0; k + ml <= len; k++)
            if (data[k] == markers[i][0] && !memcmp(data + k, markers[i], ml)) { found = 1; break; }
    }
    FreeVec(data);
    return found;
}

/* ---- the machine -------------------------------------------------------------------- */

static int board(UWORD product)
{
    return FindConfigDev(NULL, AMICRADLE_MFR, product) || FindConfigDev(NULL, AMICRADLE_MFR_OLD, product);
}

static int is_amicradle(void)
{
    return board(1) || board(2);        /* ACBridge, ACStorage */
}

/* An ACRTG graphics board: any of its products (src/openup_lines.h, oul_acrtg_products: 9 the 16 MiB board, 15
 * the 64 MiB one, 16 the 256 MiB one). Up to GFOpenUpTool 0.7.5 only 9 counted, so a 64 MiB machine had "RTG: none"
 * and no GFOpenRTG; up to 0.8.11 16 didn't, so Instance-32's 256 MiB board had the same. */
static int acrtg_board(void)
{
    static const ULONG mfrs[2] = { AMICRADLE_MFR, AMICRADLE_MFR_OLD };
    struct ConfigDev *cd;
    int i;
    for (i = 0; i < 2; i++)
        for (cd = NULL; (cd = FindConfigDev(cd, mfrs[i], -1)) != NULL; )
            if (oul_acrtg_product(cd->cd_Rom.er_Product)) return 1;
    return 0;
}

/* The GFOpenKick ROM (0.8.7): GFOpenKickROM's header mark, the long 'OKR1' at the main ROM's start + $14, $F80014
 * (amicradle-guest libraries/openexec/exec_boot.h, OEXB_OKR1), or its system-startup module's ID string, which
 * says "GFOpenKick". A Kickstart 3.2.3 ROM has neither. OS 3.2's LoadModule and S:Pre-LoadModule-Startup are for
 * Kickstart ROMs: on Instance-32 (GFOpenKick) the GFOpenLibs startup stopped the boot with "device not mounted". */
static int openkick_rom(void)
{
    const volatile ULONG *mark = (const volatile ULONG *)0xF80014;
    struct Resident *r;
    if (*mark == 0x4F4B5231UL) return 1;                           /* 'OKR1' */
    r = FindResident((STRPTR)"system-startup");
    return r && r->rt_IdString && strstr((const char *)r->rt_IdString, "GFOpenKick") != NULL;
}

/* 0.8.9: the open ROM whose intuition applies the boot's settings (the no-ROM-volume design of 24 October 2026,
 * sections 9 and 10): its prefs task applies Sound's levels, Gamepads' patch and the stack caps from ENV:, which
 * GFOpenPrefs APPLY did from S:User-Startup, and prefs.resource says so by its version, 47.101 or later. This is
 * "GFOpenKick" to a Part file's when and unless lines. OS 3.2.3's ROM has no prefs.resource. */
static int prefs_task_rom(void)
{
    struct Library *pr = (struct Library *)OpenResource((STRPTR)"prefs.resource");
    return oul_prefs_task(openkick_rom(), pr ? pr->lib_Version : 0, pr ? pr->lib_Revision : 0);
}

/* The OUL_ bits a Part file's conditions ask about (src/openup_lines.h) */
static unsigned machine_bits(void)
{
    return (is_amicradle() ? OUL_AMICRADLE : 0) | (prefs_task_rom() ? OUL_OPENKICK : 0);
}

static void set_env(const char *name, const char *value)
{
    ensure_dir("ENV:GFOpenUp", 0);
    SetVar((STRPTR)name, (STRPTR)value, -1, GVF_GLOBAL_ONLY);
}

#define LOOK_ENVARC "ENVARC:GFOpenGadTools/Look"
#define LOOK_ENV "ENV:GFOpenGadTools/Look"

/* The theme of the look saved in ENVARC:GFOpenGadTools/Look: the first word of its first line ("Graphite" of
 * "Graphite dark"), as GFOpenLook reads it (a first line that is a comment is Classic); "" when there is no
 * look saved, as before a new install. */
static void saved_theme(char *out, int size)
{
    char *text = read_file(LOOK_ENVARC, NULL), *p;
    int n = 0;
    out[0] = 0;
    if (!text) return;
    for (p = text; *p == ' ' || *p == '\t'; p++) ;
    if (*p == ';') strlcpy(out, "Classic", size);
    else {
        if (*p == '"') { p++; while (p[n] && p[n] != '"' && p[n] != '\n' && n < size - 1) n++; }
        else while (p[n] && p[n] != ' ' && p[n] != '\t' && p[n] != '\n' && p[n] != '\r' && n < size - 1) n++;
        memcpy(out, p, n);
        out[n] = 0;
        if (!n) strlcpy(out, "Classic", size);
    }
    FreeVec(text);
}

static int detect(void)
{
    char summary[600], theme[80];
    const char *machine;
    UWORD attn = ((struct ExecBase *)SysBase)->AttnFlags;
    int ac = is_amicradle(), okick = openkick_rom();
    if (ac) machine = "AmiCradle";
    else if (OpenResource((STRPTR)"devicetree.resource")) machine = "PiStorm";
    else machine = "Amiga";
    set_env("GFOpenUp/Machine", machine);
    /* the look this Amiga has (GFOpenUpTool 0.7.5): the Installer keeps it on an upgrade, and picks one only
     * for a new install */
    saved_theme(theme, sizeof theme);
    set_env("GFOpenUp/Theme", theme);
    set_env("GFOpenUp/RTG", acrtg_board() ? "ACRTG" : "");
    set_env("GFOpenUp/Network", board(6) ? "GFOpenSocket card" : "");
    /* 0.8.7: the ROM, so the Installer leaves OS 3.2's LoadModule (GFOpenLibs' ROM modules) out on the GFOpenKick ROM;
     * and where gfopenrtg.library is, so an upgrade (a Novice one never sees the questions) brings GFOpenRTG up to date
     * wherever GFOpenUp put it or the ROM has it, not only where LIBS:gfopenrtg.library is on the disk */
    set_env("GFOpenUp/ROM", okick ? "GFOpenKick" : "Kickstart");
    /* 9 Nov 2026: the slim ROM, whose module set the KickstartSet part brings (KICKSET) */
    set_env("GFOpenUp/SlimROM", slim_rom() ? "yes" : "no");
    manifest_load();
    set_env("GFOpenUp/GFOpenRTG", find_entry("part", "GFOpenRTG") ? "GFOpenUp"
                              : exists("SYS:Libs/gfopenrtg.library") ? "LIBS"
                              : FindResident((STRPTR)"gfopenrtg.library") ? "ROM" : "");
    /* 9 Nov 2026: the version of GFOpenUp installed ("" when none, or its list can't be read whole), so the
     * Installer offers Reinstall over the same version */
    {
        entry *pe = manifest_short ? NULL : find_entry("part", "GFOpenUp");
        set_env("GFOpenUp/Installed", pe ? pe->b : "");
    }
    strcpy(summary, "This is ");
    strcat(summary, ac ? "an AmiCradle instance" : !strcmp(machine, "PiStorm") ? "a PiStorm (Emu68)" : "an Amiga");
    strcat(summary, ", with a ");
    strcat(summary, attn & AFF_68060 ? "68060" : attn & AFF_68040 ? "68040" : attn & AFF_68030 ? "68030" : attn & AFF_68020 ? "68020" : "68000");
    strcat(summary, attn & (AFF_68881 | AFF_FPU40) ? " and an FPU" : " and no FPU");
    strcat(summary, okick ? ", on the GFOpenKick ROM.\n" : ".\n");
    if (ac) {
        strcat(summary, acrtg_board() ? "RTG: an ACRTG card.\n" : "RTG: none.\n");
        strcat(summary, board(6) ? "Network: the GFOpenSocket card is fitted.\n"
                                 : "Network: the GFOpenSocket card isn't fitted (turn the instance's Network switch on in Cradle).\n");
    } else {
        strcat(summary, "GFOpenUp 0.1 is for AmiCradle instances. On this machine it offers only the parts that need no AmiCradle boards.\n");
    }
    set_env("GFOpenUp/Summary", summary);
    PutStr((STRPTR)summary);
    return RETURN_OK;
}

/* ---- INSTALL ------------------------------------------------------------------------- */

/* Splits a line into words; quoted words keep their spaces (src/openup_lines.h, which the host test shares). */
static int words(char *line, char **w, int max)
{
    return oul_words(line, w, max);
}

/* One file out of a drawer's way (clear_way): one GFOpenUp put there ("new") goes; one GFOpenUp replaced goes, its
 * original staying listed in SYS:Storage; any other is kept in SYS:Storage now, listed as replaced. 0, or -1
 * (said why). */
static int out_of_way(const char *path)
{
    char backup[ENTRY_PATH], bdir[ENTRY_PATH];
    entry *e = find_entry(NULL, path);
    int made = 0;
    if (e && !strcmp(e->kind, "new") && !still_ours(e))
        set_text(&e->b, "");                                /* someone else's bytes now: kept, as any other file */
    else if (e && (!strcmp(e->kind, "new") || !strcmp(e->kind, "replaced"))) {
        if (!DeleteFile((STRPTR)path)) {
            say("GFOpenUp: can't move ", path, " out of the way of the drawer that goes there. Is it in use, or the disk write-protected?");
            return -1;
        }
        if (!strcmp(e->kind, "new")) e->kind[0] = 0;        /* GFOpenUp's own: gone */
        return 0;
    }
    backup_path(path, backup, sizeof backup);
    parent_of(backup, bdir, sizeof bdir);
    if (!exists(backup)) {
        if (!ensure_dir(bdir, 0) || !copy_file(path, backup, 0)) {
            say("GFOpenUp: can't keep a copy of ", path, " in SYS:Storage, so the drawer that goes there wasn't made. Is SYS: full or write-protected?");
            return -1;
        }
        made = 1;
    }
    if (!DeleteFile((STRPTR)path)) {
        if (made) DeleteFile((STRPTR)backup);
        say("GFOpenUp: can't move ", path, " out of the way of the drawer that goes there. Is it in use, or the disk write-protected?");
        return -1;
    }
    if (e ? !set_text(&e->b, backup) : !add_entry("replaced", path, backup)) {
        say("GFOpenUp: out of memory noting ", path, "; it is in SYS:Storage/GFOpenUp-Superseded.");
        return -1;
    }
    if (e) strcpy(e->kind, "replaced");
    return 0;
}

/* A file where a drawer of dest's path goes is taken out of the way (25 Oct 2026; src/openup_lines.h, oul_way),
 * with its icon: Instance-32's own calculator at SYS:Utilities/GFOpenCalc, where the GFOffice Suite's GFOpenCalc drawer
 * goes. Each is kept as a replaced file is (out_of_way). The drawer comes after it in the manifest; UNINSTALL takes
 * the drawer out once its files are out, then puts the file back, and REMOVE does when the drawer it made is gone.
 * Before, the part's files couldn't be written there and INSTALL listed them anyway. 0, or -1 (said why). */
static int clear_way(const char *dest)
{
    char dir[ENTRY_PATH], icon[ENTRY_PATH];
    int i;
    for (i = 0; oul_way(dest, i, dir, sizeof dir); i++) {
        if (!is_file(dir))
            continue;
        if (out_of_way(dir) < 0)
            return -1;
        if (strlcpy(icon, dir, sizeof icon) < sizeof icon && str_cat(icon, ".info", sizeof icon) < sizeof icon
            && is_file(icon) && out_of_way(icon) < 0)
            return -1;                              /* its icon: the drawer's goes there */
        say("GFOpenUp: ", dir, " was a file where a drawer goes; it is kept in SYS:Storage/GFOpenUp-Superseded.");
    }
    return 0;
}

static int reinstalling;                            /* REINSTALL (below): the settings in place stay */
static int force_older;                             /* INSTALL/REINSTALL ... FORCE: an older file may go over a newer */
static int kept_newer;                              /* files left because the disk's were newer (said, and counted) */

/* The file's version from its $VER string into out: 1, else 0 */
static int file_ver(const char *path, char *out, int size)
{
    LONG len = 0;
    char *buf = read_file(path, &len);
    int ok = buf ? ouv_ver(buf, len, out, size) : 0;
    if (buf) FreeVec(buf);
    return ok;
}

/* 1 when dest is a setting: in ENVARC: or ENV: */
static int env_setting(const char *dest)
{
    static const char *const vols[] = { "ENVARC:", "ENV:" };
    int v, i;
    for (v = 0; v < 2; v++) {
        for (i = 0; vols[v][i] && (dest[i] | 0x20) == (vols[v][i] | 0x20); i++)
            ;
        if (!vols[v][i]) return 1;
    }
    return 0;
}

/* Puts one file in place, keeping the original the first time GFOpenUp
 * replaces it, and notes it in the manifest (loaded by the caller).
 * only_if_missing: 1 leaves any file already there (the user's own setting);
 * 2 replaces only a file GFOpenUp itself added, so an upgrade reaches it.
 * 1 copied, 0 nothing to do, -1 failed (said why). */
static int install_file(const char *from, const char *dest, int only_if_missing)
{
    char backup[260];
    entry *e = find_entry(NULL, dest), *added = NULL;
    /* a file GFOpenUp put there that has other bytes now (its mark, src/openup_lines.h): someone else's */
    int theirs = e && !strcmp(e->kind, "new") && exists(dest) && !still_ours(e);
    if ((only_if_missing == 1 || (reinstalling && env_setting(dest))) && exists(dest))
        return 0;                                   /* the user's own setting stays (a reinstall: every setting) */
    if (only_if_missing == 2 && exists(dest) && (!e || strcmp(e->kind, "new") || theirs))
        return 0;                                   /* another program's file stays */
    if (!exists(dest) && clear_way(dest) < 0)       /* (a file there means its drawers are drawers) */
        return -1;                                  /* a file where its drawer goes, which couldn't be kept */
    if (exists(dest) && same_file(from, dest)) {
        if (!e) add_entry("same", dest, NULL);
        else note_mark(e);                          /* GFOpenUp's bytes: a line from before marks gets its mark */
        return 0;
    }
    if (exists(dest) && !force_older) {             /* an older file never goes over a newer one (14 Nov 2026) */
        char fv[24], tv[24];
        if (file_ver(from, fv, sizeof fv) && file_ver(dest, tv, sizeof tv) && ouv_keep_newer(fv, tv, 0)) {
            Printf((STRPTR)"GFOpenUp: kept %s %s, newer than the pack's %s (FORCE puts the pack's in)\n", (LONG)dest,
                   (LONG)tv, (LONG)fv);
            kept_newer++;
            return 0;
        }
    }
    if (exists(dest) && (!e || !strcmp(e->kind, "same") || theirs)) {
        /* The first time GFOpenUp replaces this file: keep the original. */
        char dir[260];
        backup_path(dest, backup, sizeof backup);
        parent_of(backup, dir, sizeof dir);
        if ((!exists(backup) || theirs) && (!ensure_dir(dir, 0) || !copy_file(dest, backup, 0))) {
            say("GFOpenUp: can't keep a copy of ", dest, " in SYS:Storage; it wasn't replaced. Is SYS: full or write-protected?");
            return -1;
        }
        if (e ? !set_text(&e->b, backup) : !add_entry("replaced", dest, backup)) {
            say("GFOpenUp: out of memory noting ", dest, "; it wasn't replaced.");
            return -1;
        }
        if (e) strcpy(e->kind, "replaced");
        if (theirs) say("GFOpenUp: ", dest, " had changed since GFOpenUp put it there; that copy is kept in SYS:Storage/GFOpenUp-Superseded.");
    } else if (!e) {
        added = add_entry("new", dest, NULL);
    }
    if (!copy_file(from, dest, 1)) {
        if (added) added->kind[0] = 0;              /* never there, so not listed (25 Oct 2026) */
        say("GFOpenUp: can't write ", dest, ". Is the disk full or write-protected?");
        return -1;
    }
    note_mark(added ? added : e);                   /* what GFOpenUp wrote, so UNINSTALL takes out only that */
    return 1;
}

/* Takes a file out of the way (another program's monitor driver, say),
 * keeping it in SYS:Storage/GFOpenUp-Superseded for UNINSTALL to put back.
 * 1 moved, 0 nothing to do, -1 failed (said why). */
static int retire_file(const char *dest)
{
    char backup[260], dir[260];
    entry *e, *theirs = NULL;
    if (!exists(dest))
        return 0;                                   /* absent */
    if ((e = find_entry(NULL, dest)) && !strcmp(e->kind, "new") && !still_ours(e)) {
        /* GFOpenUp put it there, but it has other bytes now (its mark): kept in SYS:Storage as any other file is,
         * its line becoming the "replaced" one */
        theirs = e;
        e = NULL;
    }
    if (e) {
        /* GFOpenUp's own file, from an earlier part that the retiring part has taken over: it goes. A "new"
         * one leaves the manifest with it; a "replaced" one keeps its entry, so UNINSTALL still puts the
         * original back from SYS:Storage. A "same" file is the user's own bytes and stays. Before, every
         * file in the manifest was left: on Dale's disk (GFOpenUp 0.7.9's parts) GFOpenPrefs's retire lines
         * left seven old editors' icons beside GFOpenPrefs's (lab t5-events, W53) */
        if (strcmp(e->kind, "new") && strcmp(e->kind, "replaced"))
            return 0;
        if (!DeleteFile((STRPTR)dest)) {
            say("GFOpenUp: can't move ", dest, " out of the way. Is it in use, or the disk write-protected?");
            return -1;
        }
        if (!strcmp(e->kind, "new"))
            e->kind[0] = 0;                         /* manifest_save leaves it out */
        return 1;
    }
    backup_path(dest, backup, sizeof backup);
    parent_of(backup, dir, sizeof dir);
    if (!ensure_dir(dir, 0) || !copy_file(dest, backup, 0)) {
        say("GFOpenUp: can't keep a copy of ", dest, " in SYS:Storage; it was left in place. Is SYS: full or write-protected?");
        return -1;
    }
    if (!DeleteFile((STRPTR)dest)) {
        DeleteFile((STRPTR)backup);
        say("GFOpenUp: can't move ", dest, " out of the way. Is it in use, or the disk write-protected?");
        return -1;
    }
    if (theirs && set_text(&theirs->b, backup)) strcpy(theirs->kind, "replaced");
    else add_entry("replaced", dest, backup);
    return 1;
}

/* 1 when icon b already shows icon a's picture, as icon.library reads them (OS 3.5 icons): the size, the
 * frame, and both images with their palettes and transparent colours. A face is only the picture, so an
 * icon that has it already is left as it is. Before GFOpenUpTool 0.7.5 every face line wrote its icon again
 * on every install and upgrade: the GorillaIcons part counted every icon it faces as changed each time
 * (59 on one lab copy of Instance-32, 76 on another), and Workbench redrew each of them. */
static int same_picture(struct DiskObject *a, struct DiskObject *b)
{
    struct DiskObject *o[2];
    LONG w[2] = { 0, 0 }, h[2] = { 0, 0 }, pm[2] = { 0, 0 }, fr[2] = { 0, 0 };
    int i, k;
    o[0] = a; o[1] = b;
    for (i = 0; i < 2; i++)
        IconControl(o[i], ICONCTRLA_GetWidth, (ULONG)&w[i], ICONCTRLA_GetHeight, (ULONG)&h[i],
                    ICONCTRLA_IsPaletteMapped, (ULONG)&pm[i], ICONCTRLA_GetFrameless, (ULONG)&fr[i], TAG_DONE);
    if (!pm[0] || !pm[1] || w[0] <= 0 || h[0] <= 0 || w[0] != w[1] || h[0] != h[1] || fr[0] != fr[1])
        return 0;
    for (k = 0; k < 2; k++) {
        UBYTE *img[2] = { NULL, NULL };
        struct ColorRegister *pal[2] = { NULL, NULL };
        LONG np[2] = { 0, 0 }, tc[2] = { -1, -1 };
        for (i = 0; i < 2; i++)
            IconControl(o[i], k ? ICONCTRLA_GetImageData2 : ICONCTRLA_GetImageData1, (ULONG)&img[i],
                        k ? ICONCTRLA_GetPalette2 : ICONCTRLA_GetPalette1, (ULONG)&pal[i],
                        k ? ICONCTRLA_GetPaletteSize2 : ICONCTRLA_GetPaletteSize1, (ULONG)&np[i],
                        k ? ICONCTRLA_GetTransparentColor2 : ICONCTRLA_GetTransparentColor1, (ULONG)&tc[i], TAG_DONE);
        if (!img[0] != !img[1] || !pal[0] != !pal[1] || np[0] != np[1] || tc[0] != tc[1])
            return 0;
        if (img[0] && memcmp(img[0], img[1], (size_t)(w[0] * h[0])))
            return 0;
        if (pal[0] && np[0] > 0 && memcmp(pal[0], pal[1], (size_t)np[0] * sizeof(struct ColorRegister)))
            return 0;
    }
    return 1;
}

/* Writes name.info: pic's picture with old's own type, default tool, tool types, stack, place and drawer window
 * (lent for the write, then given back). face_icon writes the icon this way, and OLDLOOK writes it again into T:
 * to know whether an icon is still as GFOpenUp left it. 1 written. */
static int put_face(struct DiskObject *pic, struct DiskObject *old, const char *name, int notify)
{
    UBYTE type = pic->do_Type;
    STRPTR tool = pic->do_DefaultTool, *tts = pic->do_ToolTypes;
    LONG stack = pic->do_StackSize, cx = pic->do_CurrentX, cy = pic->do_CurrentY;
    struct DrawerData *dd = pic->do_DrawerData;
    STRPTR tw = pic->do_ToolWindow;
    int ok;
    pic->do_Type = old->do_Type; pic->do_DefaultTool = old->do_DefaultTool; pic->do_ToolTypes = old->do_ToolTypes;
    pic->do_StackSize = old->do_StackSize; pic->do_CurrentX = old->do_CurrentX; pic->do_CurrentY = old->do_CurrentY;
    pic->do_DrawerData = old->do_DrawerData; pic->do_ToolWindow = old->do_ToolWindow;
    ok = PutIconTags((STRPTR)name, pic, ICONPUTA_NotifyWorkbench, (ULONG)notify, TAG_DONE) != 0;
    pic->do_Type = type; pic->do_DefaultTool = tool; pic->do_ToolTypes = tts;
    pic->do_StackSize = stack; pic->do_CurrentX = cx; pic->do_CurrentY = cy;
    pic->do_DrawerData = dd; pic->do_ToolWindow = tw;
    return ok;
}

/* A new face for an icon that stays the system's (GFOpenIcons' pictures on SYS:'s
 * drawers and programs and on the DefIcons file types, ICONS.md section 8): the
 * picture comes from the part's icon, and everything else (type, default
 * tool, tool types, stack, the drawer's window, its place) stays the icon's
 * own. The original goes to SYS:Storage/GFOpenUp-Superseded the first time, as
 * for a replaced file, so UNINSTALL puts it back. An icon the machine hasn't
 * got, or one that shows the picture already (same_picture), is left alone.
 * 1 changed, 0 nothing to do, -1 failed (said why). */
static int still_face(const char *dest, const char *pic_name, const char *fields_name);
static int face_icon(const char *from, const char *dest)
{
    char backup[260], dir[260], fname[260], dname[260];
    struct DiskObject *pic, *old;
    entry *e;
    int ok = 0, n;
    if (!exists(dest))
        return 0;
    strncpy(fname, from, sizeof fname - 1); fname[sizeof fname - 1] = 0;
    strncpy(dname, dest, sizeof dname - 1); dname[sizeof dname - 1] = 0;
    if ((n = strlen(fname)) > 5 && same_word(fname + n - 5, ".info")) fname[n - 5] = 0;
    if ((n = strlen(dname)) > 5 && same_word(dname + n - 5, ".info")) dname[n - 5] = 0;
    backup_path(dest, backup, sizeof backup);
    e = find_entry(NULL, dest);
    if (!(IconBase = OpenLibrary((STRPTR)"icon.library", 44))) {
        say("GFOpenUp: icon.library 44 or later is needed to give ", dest, " its new picture.");
        return -1;
    }
    pic = GetIconTags((STRPTR)fname, ICONGETA_FailIfUnavailable, TRUE, TAG_DONE);
    old = GetIconTags((STRPTR)dname, ICONGETA_FailIfUnavailable, TRUE, TAG_DONE);
    if (pic && old && same_picture(pic, old)) {     /* the picture is there already: nothing to write */
        FreeDiskObject(pic);
        FreeDiskObject(old);
        CloseLibrary(IconBase);
        IconBase = NULL;
        return 0;
    }
    /* 0.8.4: an icon GFOpenUp faced before gets a new face only while it is its original (OLDLOOK put it back
     * from Gorilla's picture): one the user changed since keeps what they gave it */
    if (e && !strcmp(e->kind, "replaced") && pic && old) {
        FreeDiskObject(pic);                        /* still_face opens icon.library for itself */
        FreeDiskObject(old);
        CloseLibrary(IconBase);
        IconBase = NULL;
        /* (still its original picture, over its original fields or over its own now: a tooltype line, the
         * Shell icon's, changes a field and not the picture, 24 October 2026) */
        if (!e->b || !exists(e->b) || (!still_face(dest, e->b, e->b) && !still_face(dest, e->b, dest))) {
            say("GFOpenUp: kept ", dest, ": it has been changed since GFOpenUp last gave it a picture");
            return 0;
        }
        if (!(IconBase = OpenLibrary((STRPTR)"icon.library", 44))) return -1;
        pic = GetIconTags((STRPTR)fname, ICONGETA_FailIfUnavailable, TRUE, TAG_DONE);
        old = GetIconTags((STRPTR)dname, ICONGETA_FailIfUnavailable, TRUE, TAG_DONE);
    }
    if (!e || !strcmp(e->kind, "same")) {          /* the first time: keep the original */
        parent_of(backup, dir, sizeof dir);
        if (!exists(backup) && (!ensure_dir(dir, 0) || !copy_file(dest, backup, 0))) {
            say("GFOpenUp: can't keep a copy of ", dest, " in SYS:Storage; its picture wasn't changed. Is SYS: full or write-protected?");
            if (pic) FreeDiskObject(pic);
            if (old) FreeDiskObject(old);
            CloseLibrary(IconBase);
            IconBase = NULL;
            return -1;
        }
    }
    if (pic && old)
        ok = put_face(pic, old, dname, 1);
    if (pic) FreeDiskObject(pic);
    if (old) FreeDiskObject(old);
    CloseLibrary(IconBase);
    IconBase = NULL;
    if (!ok) {
        say("GFOpenUp: can't give ", dest, " its new picture. Is the disk write-protected?");
        return -1;
    }
    if (!e) { if (!add_entry("replaced", dest, backup)) { say("GFOpenUp: out of memory noting ", dest, "'s new picture."); return -1; } }
    else if (!strcmp(e->kind, "same")) {
        if (!set_text(&e->b, backup)) { say("GFOpenUp: out of memory noting ", dest, "'s new picture."); return -1; }
        strcpy(e->kind, "replaced");
    }
    return 1;
}

/* One tool type of an icon that stays the system's ("tooltype" lines, 24 October 2026: the Shell icon's
 * WINDOW=CON:0///130/... without its 130, so a Shell from the icon opens 80 x 24 of the system font, as
 * NewShell's own window does). Where the icon has the tool type from, exactly, it becomes to; its picture, its
 * other tool types and everything else stay as they are. With keep (INSTALL), the icon as it was goes to
 * SYS:Storage/GFOpenUp-Superseded the first time, as for a face, and the manifest lists it as replaced, so
 * UNINSTALL puts it back; REMOVE calls this the other way round (to back to from), without keep, so the icon
 * keeps whatever picture it has then. An icon the machine hasn't got, or one without that tool type (it has
 * the other one already, or the user changed it), is left alone. 1 changed, 0 nothing to do, -1 failed. */
static void no_info(char *dst, const char *src, int size);
static int tooltype_icon(const char *dest, const char *from, const char *to, int keep)
{
    char backup[260], dir[260], dname[260];
    struct DiskObject *o;
    STRPTR *was, *tts;
    entry *e = find_entry(NULL, dest);
    int i, n, at = -1, ok;
    if (!exists(dest))
        return 0;
    no_info(dname, dest, sizeof dname);
    if (!(IconBase = OpenLibrary((STRPTR)"icon.library", 44))) {
        say("GFOpenUp: icon.library 44 or later is needed to change ", dest, "'s tool types.");
        return -1;
    }
    if (!(o = GetIconTags((STRPTR)dname, ICONGETA_FailIfUnavailable, TRUE, TAG_DONE))) {
        CloseLibrary(IconBase);
        IconBase = NULL;
        say("GFOpenUp: can't read the icon ", dest, NULL);
        return -1;
    }
    for (n = 0; o->do_ToolTypes && o->do_ToolTypes[n]; n++)
        if (at < 0 && !strcmp((const char *)o->do_ToolTypes[n], from)) at = n;
    ok = at < 0 ? 0 : 1;
    if (ok && keep && (!e || !strcmp(e->kind, "same"))) {     /* the first time: keep the icon as it was */
        backup_path(dest, backup, sizeof backup);
        parent_of(backup, dir, sizeof dir);
        if (!exists(backup) && (!ensure_dir(dir, 0) || !copy_file(dest, backup, 0))) {
            say("GFOpenUp: can't keep a copy of ", dest, " in SYS:Storage; its tool types weren't changed. Is SYS: full or write-protected?");
            ok = -1;
        }
    }
    if (ok == 1) {
        if (!(tts = AllocVec((n + 1) * sizeof *tts, MEMF_ANY | MEMF_CLEAR))) {
            ok = -1;
        } else {
            for (i = 0; i < n; i++) tts[i] = i == at ? (STRPTR)to : o->do_ToolTypes[i];
            was = o->do_ToolTypes;
            o->do_ToolTypes = tts;                          /* lent for the write, then given back */
            if (!PutIconTags((STRPTR)dname, o, ICONPUTA_NotifyWorkbench, TRUE, TAG_DONE)) ok = -1;
            o->do_ToolTypes = was;
            FreeVec(tts);
        }
        if (ok < 0) say("GFOpenUp: can't write ", dest, "'s tool types. Is the disk write-protected?");
    }
    FreeDiskObject(o);
    CloseLibrary(IconBase);
    IconBase = NULL;
    if (ok == 1 && keep) {
        if (!e) { if (!add_entry("replaced", dest, backup)) { say("GFOpenUp: out of memory noting ", dest, "'s new tool type."); return -1; } }
        else if (!strcmp(e->kind, "same")) {
            if (!set_text(&e->b, backup)) { say("GFOpenUp: out of memory noting ", dest, "'s new tool type."); return -1; }
            strcpy(e->kind, "replaced");
        }
    }
    return ok;
}

/* A part's startup lines are the ones its Part file has now. Installing it marks the lines it had before
 * ("ostartup"); each line it still has goes back in its place, and the rest are taken out. Before 0.7.5 a
 * line a newer part changed stayed too: "SYS:Prefs/Sound USE" beside 0.7.4's "SYS:Prefs/OpenSound USE",
 * and a boot that stopped where the old editor was no longer there (a lab copy of Instance-32).
 * oul_settle (src/openup_lines.h) ends it: the marked lines go, or, when the install failed, come back. */
static void mark_startup(const char *part)
{
    int i;
    for (i = 0; i < nents; i++)
        if (!strcmp(ents[i].kind, "startup") && !strcmp(ents[i].b, part))
            strcpy(ents[i].kind, "ostartup");
}

static unsigned kickset_host(void);

static int move_sysfont(void);

static int install(const char *drawer)
{
    char path[220], from[260], *text, *p, *line, partname[64] = "", partver[32] = "";
    int pass, refused = 0, files = 0, errors = 0, first = 0;
    unsigned have = machine_bits();
    unsigned hostcpu = kickset_host();

    strncpy(path, drawer, 200);
    path[200] = 0;
    AddPart((STRPTR)path, (STRPTR)"Part", sizeof path);
    if (!(text = read_file(path, NULL))) {
        say("GFOpenUp: can't read ", path, ". Run the Installer from the GFOpenUp drawer.");
        return RETURN_FAIL;
    }
    manifest_load();
    if (manifest_short) {                           /* nothing done that couldn't be listed (manifest_save says why) */
        manifest_save();
        FreeVec(text);
        return RETURN_ERROR;
    }
    /* Pass 0 checks (needs, keep); pass 1 copies. Lines are split in a copy each pass. */
    for (pass = 0; pass < 2 && !refused; pass++) {
        char *copy = AllocVec(strlen(text) + 1, MEMF_ANY);
        if (!copy) { FreeVec(text); return RETURN_FAIL; }
        strcpy(copy, text);
        if (pass == 1)
            first = nents;                          /* the lines this install adds come from here (oul_settle) */
        for (p = copy; *p; ) {
            char *w[12];
            const char *need;
            int n;
            line = p;
            while (*p && *p != '\n') p++;
            if (*p) *p++ = 0;
            /* a when or unless in front: the line only where it holds (0.8.9: any line, a startup line too) */
            if (!(line = (char *)oul_strip(line, have, 0)))
                continue;
            if (!strncmp(line, "startup ", 8)) {
                if (pass == 1) {
                    entry *old = find_entry("ostartup", line + 8);
                    if (old && !strcmp(old->b, partname)) strcpy(old->kind, "startup");   /* kept, in its place */
                    else if (!find_entry("startup", line + 8))
                        add_entry("startup", line + 8, partname);
                }
                continue;
            }
            n = words(line, w, 12);
            if (!n) continue;
            if (!strcmp(w[0], "part") && n >= 3) {
                strncpy(partname, w[1], sizeof partname - 1);
                strncpy(partver, w[2], sizeof partver - 1);
                if (pass == 1) {
                    mark_startup(partname);
                    oul_mark_needs(ents, nents, partname);   /* its needs are the ones its Part file has now */
                }
            } else if (pass == 0 && (need = oul_needs_part(w, n))) {
                /* another part first: the GFOffice Suite's programs need the GFOffice part, their shared files */
                if (!oul_installed(ents, nents, need)) {
                    Printf((STRPTR)"GFOpenUp: %s needs the %s part, which isn't installed, so it was left out.\n"
                           "        Install %s first (GFOpenUpTool INSTALL Parts/%s), then %s.\n",
                           (LONG)partname, (LONG)need, (LONG)need, (LONG)need, (LONG)partname);
                    refused = 1;
                }
            } else if (pass == 1 && (need = oul_needs_part(w, n))) {
                /* noted, so REMOVE of that part asks for this one to go first */
                if (!add_entry(OUL_NEEDPART, partname, need)) errors++;
            } else if (pass == 0 && !strcmp(w[0], "needs") && n >= 2) {
                if (!strcmp(w[1], "AmiCradle") && !is_amicradle()) {
                    say("GFOpenUp: ", partname, " is for AmiCradle instances; left out here.");
                    refused = 1;
                }
                if (!strcmp(w[1], "Kickstart") && openkick_rom()) {
                    say("GFOpenUp: ", partname, " is loaded by OS 3.2's LoadModule, which is for Kickstart ROMs; the GFOpenKick ROM has its own, so it is left out here.");
                    refused = 1;
                }
                /* 9 Nov 2026: the CPU floor, a 68040 or 68060 with its FPU and MMU (src/openup_lines.h, oul_full_040).
                 * The GFOpenGPU part's modules are built -m68040 -m68881: on less they crash the first program. */
                if (!strcmp(w[1], "68040") && !oul_full_040(((struct ExecBase *)SysBase)->AttnFlags)) {
                    say("GFOpenUp: ", partname, " needs a 68040 or 68060 with its FPU and MMU; left out here.");
                    refused = 1;
                }
                if (!strcmp(w[1], "ACRTG") && !acrtg_board()) {
                    say("GFOpenUp: ", partname, " needs an ACRTG graphics board; left out here. In AmiCradle, fit one in Cradle (Hardware, Display).");
                    refused = 1;
                }
            } else if (pass == 0 && !strcmp(w[0], "keep") && n >= 3) {
                if (exists(w[1]) && !has_marker(w[1], w + 2, n - 2) && !find_entry(NULL, w[1])) {
                    say("GFOpenUp: ", w[1], " belongs to another program, so GFOpenUp leaves it and this part out.");
                    say("        To use ", partname, ", remove that program first, then run GFOpenUp again.");
                    refused = 1;
                }
            } else if (pass == 1 && !strcmp(w[0], "face") && n >= 3) {
                strcpy(from, drawer);
                AddPart((STRPTR)from, (STRPTR)w[1], sizeof from);
                switch (face_icon(from, w[2])) {
                case 1: files++; break;
                case -1: errors++; break;
                }
            } else if (pass == 1 && !strcmp(w[0], "tooltype") && n >= 4) {
                switch (tooltype_icon(w[1], w[2], w[3], 1)) {
                case 1: files++; break;
                case -1: errors++; break;
                }
            } else if (pass == 1 && !strcmp(w[0], "retire") && n >= 2) {
                switch (retire_file(w[1])) {
                case 1: files++; break;
                case -1: errors++; break;
                }
            } else if (pass == 1 && (!strcmp(w[0], "copy") || !strcmp(w[0], "default") || !strcmp(w[0], "add"))
                       && n >= 3) {
                /* A body for another CPU is not this machine's. It stays in the pack and is not copied. */
                if (!ouk_copy(w[2], hostcpu)) continue;
                strcpy(from, drawer);
                AddPart((STRPTR)from, (STRPTR)w[1], sizeof from);
                switch (install_file(from, w[2], !strcmp(w[0], "default") ? 1 : !strcmp(w[0], "add") ? 2 : 0)) {
                case 1: files++; break;
                case -1: errors++; break;
                }
            }
        }
        FreeVec(copy);
    }
    FreeVec(text);
    if (refused)
        return RETURN_WARN;
    /* 25 Oct 2026: a part with an error isn't listed as installed (src/openup_lines.h, oul_settle). Its startup and
     * needpart lines stay as they were, an older version's part line too, so an upgrade is tried again; what its
     * files did stays listed for REMOVE and UNINSTALL. Before, the part was listed whatever happened. */
    oul_settle(ents, nents, first, partname, !errors);
    if (!errors) {
        entry *pe = find_entry("part", partname);
        if (pe ? !set_text(&pe->b, partver) : !add_entry("part", partname, partver)) {
            say("GFOpenUp: out of memory noting ", partname, "; run the install again.");
            return RETURN_ERROR;
        }
    }
    if (!manifest_save()) {
        say("GFOpenUp: can't write its list of installed files in ENVARC:GFOpenAmiga. Is SYS: full?", NULL, NULL);
        return RETURN_ERROR;
    }
    if (errors) {
        entry *pe = find_entry("part", partname);
        Printf((STRPTR)"GFOpenUp: %s %s didn't install completely (%ld file%s changed, %ld problem%s above), so ",
               (LONG)partname, (LONG)partver, files, (LONG)(files == 1 ? "" : "s"), errors, (LONG)(errors == 1 ? "" : "s"));
        if (pe) Printf((STRPTR)"the list still has %s %s.\n", (LONG)partname, (LONG)pe->b);
        else Printf((STRPTR)"it isn't listed as installed.\n");
        return RETURN_ERROR;
    }
    Printf((STRPTR)"GFOpenUp: %s %s installed (%ld file%s changed)\n", (LONG)partname, (LONG)partver, files, (LONG)(files == 1 ? "" : "s"));
    if (!strcmp(partname, "GFOSFonts")) move_sysfont();     /* GFOS Mono is in FONTS: now (openup_sysfont.h) */
    return RETURN_OK;
}

/* ---- REMOVE: one part taken out (0.8.0) ------------------------------------------------------ */

/* Takes out the files one part's Part file names, as UNINSTALL takes out everything: a file the part added
 * is deleted, a file it replaced or retired gets its original back from SYS:Storage/GFOpenUp-Superseded, and
 * its startup lines and its "part" line leave the manifest (STARTUP writes S:User-Startup again after). The
 * directories it made go when empty. The GFOpenLibs switches use it: unticking a library puts OS 3.2's own
 * back. Only a file the manifest lists is touched, so a file the part names but GFOpenUp never put there (the
 * user's own) stays. 0 when the part isn't installed.
 * A part another installed part needs (its "needs part" line, noted as needpart in the manifest) stays, and
 * nothing changes: GFOffice stays while GFOpenWrite or GFOpenCalc is installed (25 Oct 2026). */
static int remove_part(const char *drawer)
{
    char path[220], *text, *p, *line, partname[64] = "";
    const char *by;
    int removed = 0, restored = 0, problems = 0, i;

    strncpy(path, drawer, 200);
    path[200] = 0;
    AddPart((STRPTR)path, (STRPTR)"Part", sizeof path);
    if (!(text = read_file(path, NULL))) {
        say("GFOpenUp: can't read ", path, ". Run the Installer from the GFOpenUp drawer.");
        return RETURN_FAIL;
    }
    manifest_load();
    if (manifest_short) {                           /* nothing done that couldn't be listed (manifest_save says why) */
        manifest_save();
        FreeVec(text);
        return RETURN_ERROR;
    }
    if (oul_part_name(text, partname, sizeof partname) && oul_installed(ents, nents, partname) &&
        (by = oul_needed_by(ents, nents, partname))) {
        Printf((STRPTR)"GFOpenUp: %s stays: %s needs it. Take out %s first (GFOpenUpTool REMOVE Parts/%s), then %s.\n",
               (LONG)partname, (LONG)by, (LONG)by, (LONG)by, (LONG)partname);
        FreeVec(text);
        return RETURN_WARN;
    }
    for (p = text; *p; ) {
        char *w[12], dir[ENTRY_PATH];
        const char *dest = NULL;
        entry *e;
        int n;
        line = p;
        while (*p && *p != '\n') p++;
        if (*p) *p++ = 0;
        line = (char *)oul_strip(line, 0, 1);       /* whatever its condition: only what the manifest lists goes */
        if (!strncmp(line, "startup ", 8)) continue;
        n = words(line, w, 12);
        if (!n) continue;
        if (!strcmp(w[0], "part") && n >= 3) strncpy(partname, w[1], sizeof partname - 1);
        else if (!strcmp(w[0], "tooltype") && n >= 4) {
            /* the tool type back as it was; the icon stays (its copy in SYS:Storage too, for UNINSTALL) */
            switch (tooltype_icon(w[1], w[3], w[2], 0)) {
            case 1: restored++; break;
            case -1: problems++; break;
            }
            continue;
        }
        else if (!strcmp(w[0], "retire") && n >= 2) dest = w[1];
        else if ((!strcmp(w[0], "copy") || !strcmp(w[0], "default") || !strcmp(w[0], "add") || !strcmp(w[0], "face"))
                 && n >= 3) dest = w[2];
        if (!dest || !(e = find_entry(NULL, dest)))
            continue;
        if (!strcmp(e->kind, "new") && !still_ours(e)) {
            /* other bytes than GFOpenUp wrote (its mark): someone else's file now, so it stays, and leaves the list */
            say("GFOpenUp: ", e->a, " has changed since GFOpenUp put it there, so it stays.");
            e->kind[0] = 0;
            continue;
        } else if (!strcmp(e->kind, "new")) {
            if (DeleteFile((STRPTR)e->a) || !exists(e->a)) { removed++; e->kind[0] = 0; }
            else { say("GFOpenUp: can't delete ", e->a, "; is it in use?"); problems++; continue; }
        } else if (!strcmp(e->kind, "replaced")) {
            if (copy_file(e->b, e->a, 0)) { DeleteFile((STRPTR)e->b); restored++; e->kind[0] = 0; }
            else { say("GFOpenUp: can't put back ", e->a, "; its original is in SYS:Storage/GFOpenUp-Superseded."); problems++; continue; }
        } else if (!strcmp(e->kind, "same")) {
            e->kind[0] = 0;
            continue;
        } else continue;
        /* the directories GFOpenUp made for it, deepest first, while empty */
        parent_of(dest, dir, sizeof dir);
        while (dir[0] && dir[strlen(dir) - 1] != ':') {
            char up[ENTRY_PATH];
            entry *d = find_entry("dir", dir), *f;
            if (!d || !DeleteFile((STRPTR)dir)) break;
            d->kind[0] = 0;
            /* a file that was where the drawer went (INSTALL's clear_way, 25 Oct 2026) comes back now it's gone */
            if ((f = find_entry("replaced", dir))) {
                if (copy_file(f->b, f->a, 0)) { DeleteFile((STRPTR)f->b); restored++; f->kind[0] = 0; }
                else { say("GFOpenUp: can't put back ", f->a, "; it is in SYS:Storage/GFOpenUp-Superseded."); problems++; }
            }
            parent_of(dir, up, sizeof up);
            strcpy(dir, up);
        }
    }
    FreeVec(text);
    if (!partname[0] || !find_entry("part", partname)) {
        if (!removed && !restored) {
            Printf((STRPTR)"GFOpenUp: %s isn't installed\n", (LONG)(partname[0] ? partname : drawer));
            return RETURN_OK;
        }
    }
    for (i = 0; i < nents; i++)
        if ((!strcmp(ents[i].kind, "startup") && !strcmp(ents[i].b, partname)) ||
            (!strcmp(ents[i].kind, "part") && !strcmp(ents[i].a, partname)))
            ents[i].kind[0] = 0;
    oul_drop_needs(ents, nents, partname);          /* what it needed may go now */
    if (!manifest_save()) {
        say("GFOpenUp: can't write its list of installed files in ENVARC:GFOpenAmiga. Is SYS: full?", NULL, NULL);
        return RETURN_ERROR;
    }
    Printf((STRPTR)"GFOpenUp: %s taken out (%ld file%s removed, %ld put back as they were)\n", (LONG)partname, removed,
           (LONG)(removed == 1 ? "" : "s"), restored);
    return problems ? RETURN_ERROR : RETURN_OK;
}

/* ---- REINSTALL: every installed part's files put back (9 Nov 2026) ----------------------------------------------
 *   GFOpenUpTool REINSTALL <Parts drawer>
 * Dale (9 Nov 2026): "it needs a reinstall option". Each part the manifest lists as installed goes in again from the
 * pack's Parts drawer (and the drawers in it: Parts/GFOpenLibs/rexx...), in the order the manifest lists them, with
 * INSTALL's own rules: a file that is missing or not GFOpenUp's bytes is written again (an original GFOpenUp hasn't
 * kept yet is kept first), one that is GFOpenUp's already is left. Nothing is uninstalled first. The user's settings
 * stay: in a reinstall no line writes over a file in ENVARC: or ENV: that is there (reinstalling), as "default"
 * lines never do; S:User-Startup's block is the Installer's STARTUP's, after, which keeps the lines switched off. A
 * part this pack hasn't got is said and left as it is. RETURN_OK, or the worst of the parts' codes. */
#define MAXPARTS 128
static char part_names[MAXPARTS][48], part_drawers[MAXPARTS][160];
static int nparts;

/* each Part file in drawer and the drawers in it, one level down: its part's name and drawer */
static void find_parts(const char *drawer, int depth)
{
    BPTR lock = Lock((STRPTR)drawer, ACCESS_READ);
    struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
    char path[220], name[48], *text;
    if (lock && fib && Examine(lock, fib)) {
        strlcpy(path, drawer, sizeof path);
        AddPart((STRPTR)path, (STRPTR)"Part", sizeof path);
        if (depth > 0 && (text = read_file(path, NULL))) {
            if (oul_part_name(text, name, sizeof name) && nparts < MAXPARTS && strlen(drawer) < sizeof part_drawers[0]) {
                strcpy(part_names[nparts], name);
                strcpy(part_drawers[nparts], drawer);
                nparts++;
            }
            FreeVec(text);
        }
        if (depth < 2)
            while (ExNext(lock, fib))
                if (fib->fib_DirEntryType > 0) {
                    strlcpy(path, drawer, sizeof path);
                    if (AddPart((STRPTR)path, fib->fib_FileName, sizeof path)) find_parts(path, depth + 1);
                }
    }
    if (fib) FreeDosObject(DOS_FIB, fib);
    if (lock) UnLock(lock);
}

static int reinstalled;                             /* the parts REINSTALL put in again (UPDATE AUTO's log line) */

/* Before REINSTALL puts part j in: each part its Part file needs ("needs part") that the disk hasn't got and the pack
 * has goes in first, else INSTALL would leave part j out. GFOpenMUI 19.0 needs the GFOpenLayout part, which no earlier
 * pack had (Ivanova's lab, bundle C, 14 Nov 2026); before, an update kept the old GFOpenMUI. One level: a needed part's
 * own needs are its Part file's business, as on a first install. */
static void reinstall_needs(int j, int *rc, int *done)
{
    char path[200], need[48];
    LONG n = 0;
    char *text;
    int k, m;
    strcpy(path, part_drawers[j]);
    AddPart((STRPTR)path, (STRPTR)"Part", sizeof path);
    if (!(text = read_file(path, &n)))
        return;
    for (k = 0; oul_part_needs(text, k, need, sizeof need); k++) {
        if (oul_installed(ents, nents, need))
            continue;
        for (m = 0; m < nparts && strcmp(part_names[m], need); m++)
            ;
        if (m == nparts)
            continue;                               /* not in the pack: INSTALL says so, and leaves part j out */
        Printf((STRPTR)"GFOpenUp: %s needs the %s part, which this pack brings: it goes in first\n",
               (LONG)part_names[j], (LONG)need);
        {
            int r = install(part_drawers[m]);
            if (r > *rc) *rc = r;
            (*done)++;
        }
    }
    FreeVec(text);
}

static int reinstall(const char *parts)
{
    static char wanted[MAXPARTS][48];
    char left[200] = "";                            /* the installed parts the pack hasn't got (ENV:GFOpenUp/Left) */
    int nwanted = 0, i, j, rc = RETURN_OK, done = 0;
    reinstalled = 0;
    kept_newer = 0;
    set_env("GFOpenUp/Left", "");                   /* an early return never leaves an earlier run's list */
    manifest_load();
    if (manifest_short) {
        manifest_save();                            /* says why */
        return RETURN_ERROR;
    }
    if (!nents) {
        say("GFOpenUp isn't installed here (no " MANIFEST "): there is nothing to reinstall. Install it instead.", NULL, NULL);
        return RETURN_WARN;
    }
    for (i = 0; i < nents && nwanted < MAXPARTS; i++)
        if (!strcmp(ents[i].kind, "part") && strlen(ents[i].a) < sizeof wanted[0])
            strcpy(wanted[nwanted++], ents[i].a);
    nparts = 0;
    find_parts(parts, 0);
    if (!nparts) {
        say("GFOpenUp: no parts in ", parts, ". Run the Installer from the GFOpenUp drawer.");
        return RETURN_FAIL;
    }
    reinstalling = 1;
    for (i = 0; i < nwanted; i++) {
        for (j = 0; j < nparts && strcmp(part_names[j], wanted[i]); j++)
            ;
        if (j == nparts) {
            say("GFOpenUp: ", wanted[i], " is installed, but this pack hasn't got it; it was left as it is.");
            oul_left_add(left, sizeof left, wanted[i]);
            if (rc < RETURN_WARN) rc = RETURN_WARN;
            continue;
        }
        reinstall_needs(j, &rc, &done);
        {
            int r = install(part_drawers[j]);
            if (r > rc) rc = r;
            done++;
        }
    }
    reinstalling = 0;
    reinstalled = done;
    set_env("GFOpenUp/Left", left);                 /* "" when every installed part was in the pack */
    Printf((STRPTR)"GFOpenUp: reinstalled %ld part%s%s\n", (LONG)done, (LONG)(done == 1 ? "" : "s"),
           (LONG)(rc > RETURN_WARN ? "; see the messages above" : ""));
    if (kept_newer)
        Printf((STRPTR)"GFOpenUp: %ld file%s on the disk %s newer than the pack's and stayed (FORCE puts the pack's in)\n",
               (LONG)kept_newer, (LONG)(kept_newer == 1 ? "" : "s"), (LONG)(kept_newer == 1 ? "was" : "were"));
    return rc;
}

/* ---- S:User-Startup ---------------------------------------------------------------------- */

/* 1 when the line from p to e is exactly the marker (trailing blanks
 * allowed): ";BEGIN GFOpenUp" and not ";BEGIN OpenUpSomething", another
 * program's block whose name starts the same way. Any number of leading
 * semicolons counts as one: a block once marked ";;BEGIN GFOpenUp" (an edit by
 * hand or another tool) is ours too, so STARTUP replaces it instead of adding
 * a second block and starting every program twice (10 Oct 2026). */
static int is_marker_line(const char *p, const char *e, const char *marker)
{
    size_t ml;
    while (e > p && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r')) e--;
    if (p >= e || *p != ';')
        return 0;
    while (p < e && *p == ';') p++;
    while (*marker == ';') marker++;
    ml = strlen(marker);
    return (size_t)(e - p) == ml && !strncmp(p, marker, ml);
}

/* The file without our block; *had says whether there was one. */
static char *without_block(const char *text, int *had)
{
    char *out = AllocVec(strlen(text) + 2, MEMF_ANY), *o = out;
    const char *p = text;
    int in = 0;
    *had = 0;
    if (!out)
        return NULL;
    while (*p) {
        const char *e = p;
        while (*e && *e != '\n') e++;
        if (!in && is_marker_line(p, e, BLOCK_BEGIN)) { in = 1; *had = 1; }
        else if (in && is_marker_line(p, e, BLOCK_END)) in = 0;
        else if (!in) { memcpy(o, p, e - p); o += e - p; if (*e) *o++ = '\n'; }
        p = *e ? e + 1 : e;
    }
    *o = 0;
    return out;
}

/* 1 when line is listed in ENVARC:GFOpenAmiga/Startup.off (GFOpenUp Setup's
 * switches: one startup line per line, ";" comments, blanks ignored). */
static int switched_off(const char *off, const char *line)
{
    const char *p = off;
    size_t n = strlen(line);
    while (p && *p) {
        const char *e = p, *t;
        while (*e && *e != '\n') e++;
        t = e;
        while (t > p && (t[-1] == ' ' || t[-1] == '\r' || t[-1] == '\t')) t--;
        if (*p != ';' && (size_t)(t - p) == n && !strncmp(p, line, n)) return 1;
        p = *e ? e + 1 : e;
    }
    return 0;
}

/* ---- GFOpenWindows' places ------------------------------------------------------- */

/* The places text without the lines for a program's windows: each line is
 * "program:title" in quotes, then left, top, width and height. */
static char *without_places(const char *text, const char *prog, int *dropped)
{
    char *out = AllocVec(strlen(text) + 2, MEMF_ANY), *o = out;
    const char *p = text;
    size_t n = strlen(prog);
    *dropped = 0;
    if (!out)
        return NULL;
    while (*p) {
        const char *e = p;
        int ours = 0;
        while (*e && *e != '\n') e++;
        if (*p == '"' && (size_t)(e - p) > n + 2 && p[n + 1] == ':') {
            size_t i;
            ours = 1;
            for (i = 0; i < n; i++)
                if ((p[1 + i] | 0x20) != (prog[i] | 0x20)) { ours = 0; break; }
        }
        if (ours) (*dropped)++;
        else { memcpy(o, p, e - p); o += e - p; if (*e) *o++ = '\n'; }
        p = *e ? e + 1 : e;
    }
    *o = 0;
    return out;
}

/* Drops a program's windows from both copies of GFOpenWindows' places file,
 * so they open at the program's own first size again. A file that isn't
 * there, or has no line for it, is fine. */
static int unplace(const char *prog)
{
    static const char *const files[2] = { "ENV:GFOpenPrefs/WindowPlaces", "ENVARC:GFOpenPrefs/WindowPlaces" };
    int k, rc = RETURN_OK;
    for (k = 0; k < 2; k++) {
        char *text = read_file(files[k], NULL), *out;
        int dropped;
        if (!text)
            continue;
        out = without_places(text, prog, &dropped);
        if (out && dropped && !write_file(files[k], out, strlen(out))) {
            say("GFOpenUp: can't write ", files[k], NULL);
            rc = RETURN_ERROR;
        }
        if (out) FreeVec(out);
        FreeVec(text);
    }
    return rc;
}

/* 1 when a startup line runs a program named by its path ("SYS:Prefs/OpenSound USE", "Run >NIL: C:GFOpenDock")
 * and that program isn't on the disk. A command found on the path (Assign ...) isn't checked. Such a line,
 * written as it is, stops S:User-Startup and the boot with "Unknown command": a part not ticked in an upgrade
 * keeps its old line (0.7.3's "SYS:Prefs/Gamepads USE"), and a program can be moved by hand. */
static int program_missing(const char *line)
{
    char buf[ENTRY_PATH], *w[4];
    int n, k = 0;
    strlcpy(buf, line, sizeof buf);
    n = words(buf, w, 4);
    if (n && same_word(w[0], "Run")) {
        k = 1;
        while (k < n && w[k][0] == '>') k++;        /* Run's own redirections: >NIL: */
    }
    if (k >= n || !strchr(w[k], ':'))
        return 0;
    return !exists(w[k]);
}

static int write_startup(int remove_only)
{
    char *text, *rest, *buf;
    LONG len = 0;
    int had, i, lines = 0;
    text = read_file(USER_STARTUP, &len);
    if (!text) {
        if (IoErr() != ERROR_OBJECT_NOT_FOUND) {    /* there but not read: a new one would lose the user's lines */
            say("GFOpenUp: can't read " USER_STARTUP ", so it is left as it is.", NULL, NULL);
            return 0;
        }
        if (remove_only)
            return 1;
        if (!(text = AllocVec(1, MEMF_CLEAR)))
            return 0;
        manifest_load();
        if (!find_entry("newfile", USER_STARTUP)) { add_entry("newfile", USER_STARTUP, NULL); manifest_save(); }
    }
    rest = without_block(text, &had);
    FreeVec(text);
    if (!rest)
        return 0;
    if (!(buf = AllocVec(strlen(rest) + 64 + MAXE * 2 + 8192, MEMF_ANY))) {
        FreeVec(rest);
        return 0;
    }
    strcpy(buf, rest);
    FreeVec(rest);
    if (!remove_only) {
        manifest_load();
        for (i = 0; i < nents; i++)
            if (!strcmp(ents[i].kind, "startup")) lines++;
        if (lines) {
            char *off = read_file(STARTUP_OFF, NULL);
            if (buf[0] && buf[strlen(buf) - 1] != '\n') strcat(buf, "\n");
            strcat(buf, BLOCK_BEGIN "\n; Added by GFOpenUp. GFOpenUpTool UNINSTALL takes it out.\n");
            for (i = 0; i < nents; i++)
                if (!strcmp(ents[i].kind, "startup") && strlen(buf) + strlen(ents[i].a) + 32 < (ULONG)(64 + MAXE * 2 + 8192)) {
                    if (off && switched_off(off, ents[i].a)) strcat(buf, ";OFF ");     /* switched off in GFOpenUp Setup */
                    else if (program_missing(ents[i].a)) {                           /* back in when the program is */
                        say("GFOpenUp: left out of S:User-Startup, as its program isn't there: ", ents[i].a, NULL);
                        strcat(buf, ";MISSING ");
                    }
                    strcat(buf, ents[i].a);
                    strcat(buf, "\n");
                }
            strcat(buf, BLOCK_END "\n");
            if (off) FreeVec(off);
        }
    }
    i = write_file(USER_STARTUP, buf, strlen(buf));
    FreeVec(buf);
    return i;
}

/* ---- FIXTOOLS: default tools GFOpenTypes' SWITCH changed that it should not have --------------- */

/* GFOpenUp 0.6.14 to 0.7.0 ran "GFOpenTypes SWITCH MultiView TO GFOpenView IN SYS:". A bug in GFOpenTypes (its
 * Stricmp was called with A6 still holding dos.library) made it see every icon and every kind of file as
 * MultiView's, so about 40 of AmigaOS 3.2.3's own icons (the datatype descriptors', the pointer presets',
 * DOSDrivers', the Picasso96 guides', Help...) and 16 default icons opened GFOpenView. FIXTOOLS puts the
 * default tool back. It changes an icon only when that icon's default tool is GFOpenView now and its
 * original is known, from, in this order:
 *   1. GFOpenTypes' own record, ENVARC:GFOpenTypes/Changes and the icon it kept in Backup (exact);
 *   2. the table, StockDefaultTools: "<icon without .info><TAB><default tool>" for AmigaOS 3.2.3's icons.
 * The kinds of file GFOpenTypes switched on purpose (ENVARC:Sys/def_<kind> that named MultiView) stay on
 * GFOpenView, unless ALL is given (UNINSTALL gives it). Every change is logged. */
#define OT_CHANGES "ENVARC:GFOpenTypes/Changes"
#define STOCK_TOOLS "ENVARC:GFOpenAmiga/StockDefaultTools"
#define REPAIR_LOG "ENVARC:GFOpenAmiga/DefaultTools-repair.log"
#define REPAIRED_MARK "ENVARC:GFOpenAmiga/DefaultTools-repaired"

static char fix_log[20000];
static int fix_log_len;
static int fix_checked, fix_restored, fix_kept, fix_failed, fix_nkept;
static char fix_kept_paths[64][48];

static void fix_say(const char *a, const char *b, const char *c, const char *d)
{
    const char *part[4];
    int i;
    part[0] = a; part[1] = b; part[2] = c; part[3] = d;
    for (i = 0; i < 4; i++) {
        int n;
        if (!part[i]) continue;
        n = strlen(part[i]);
        if (fix_log_len + n + 2 < (int)sizeof fix_log) { memcpy(fix_log + fix_log_len, part[i], n); fix_log_len += n; }
    }
    if (fix_log_len + 2 < (int)sizeof fix_log) fix_log[fix_log_len++] = '\n';
    fix_log[fix_log_len] = 0;
}

/* The program a tool names: the part after the last ':' or '/'. */
static const char *tool_base(const char *tool)
{
    const char *p, *b = tool;
    for (p = tool; *p; p++) if (*p == ':' || *p == '/') b = p + 1;
    return b;
}

static int same_program(const char *a, const char *b)
{
    return same_word(tool_base(a), tool_base(b));
}

static int starts_with(const char *s, const char *prefix)
{
    for (; *prefix; s++, prefix++)
        if (!*s || (*s | 0x20) != (*prefix | 0x20)) return 0;
    return 1;
}

static int is_default_icon(const char *path)
{
    return starts_with(path, "ENVARC:Sys/def_") || starts_with(path, "ENV:Sys/def_");
}

/* One icon (path without .info): its default tool goes back to orig when it is GFOpenView now. */
static void fix_icon(const char *path, const char *orig, int all, const char *why)
{
    struct DiskObject *dob;
    STRPTR cur;
    char info[ENTRY_PATH];
    if (strlen(path) + 6 > sizeof info || !orig[0]) return;
    strcpy(info, path); strcat(info, ".info");
    if (!exists(info)) return;
    dob = GetIconTags((STRPTR)path, ICONGETA_FailIfUnavailable, TRUE, TAG_DONE);
    if (!dob) return;
    fix_checked++;
    cur = dob->do_DefaultTool;
    if (cur && same_program((const char *)cur, "GFOpenView") && !same_program(orig, "GFOpenView")) {
        if (!all && is_default_icon(path) && same_program(orig, "MultiView")) {
            int i, seen = 0;
            for (i = 0; i < fix_nkept; i++) if (same_word(fix_kept_paths[i], path)) seen = 1;     /* once, not for the table too */
            if (!seen) {
                fix_kept++;
                if (fix_nkept < 64 && strlen(path) < sizeof fix_kept_paths[0]) strcpy(fix_kept_paths[fix_nkept++], path);
                fix_say("kept: ", path, " opens with GFOpenView, as the switch of kinds of file made it", NULL);
            }
        } else {
            int ok;
            dob->do_DefaultTool = (STRPTR)orig;
            ok = PutIconTags((STRPTR)path, dob, ICONPUTA_NotifyWorkbench, TRUE, TAG_DONE) != 0;
            dob->do_DefaultTool = cur;
            if (ok) {
                fix_restored++;
                fix_say("restored: ", path, ": GFOpenView -> ", orig);
                if (why) fix_say("   (", why, ")", NULL);
            } else {
                fix_failed++;
                fix_say("FAILED: ", path, ": couldn't write it; the disk may be write-protected", NULL);
            }
        }
    }
    FreeDiskObject(dob);
    /* the kinds of file live in ENV: too */
    if (starts_with(path, "ENVARC:Sys/def_")) {
        char env[ENTRY_PATH];
        if (strlen(path) + 1 < sizeof env) { strcpy(env, "ENV:Sys/"); strcat(env, path + 11); fix_icon(env, orig, all, NULL); }
    }
}

/* The default tool of an icon file (path without .info), into buf; 0 when it has none. */
static int icon_tool(const char *path, char *buf, int size)
{
    struct DiskObject *dob = GetIconTags((STRPTR)path, ICONGETA_FailIfUnavailable, TRUE, TAG_DONE);
    int ok = 0;
    if (!dob) return 0;
    if (dob->do_DefaultTool && ((const char *)dob->do_DefaultTool)[0] && strlen((const char *)dob->do_DefaultTool) < (size_t)size) {
        strcpy(buf, (const char *)dob->do_DefaultTool);
        ok = 1;
    }
    FreeDiskObject(dob);
    return ok;
}

static int fix_tools(const char *table, int all)
{
    char *text;
    static char line_copy[ENTRY_PATH * 2], orig[200], path[ENTRY_PATH];
    char *p, *line;
    fix_log_len = 0; fix_log[0] = 0;
    fix_checked = fix_restored = fix_kept = fix_failed = fix_nkept = 0;
    if (!(IconBase = OpenLibrary((STRPTR)"icon.library", 44))) {
        say("GFOpenUp: icon.library 44 or later is needed to check the icons' default tools.", NULL, NULL);
        return RETURN_WARN;
    }
    fix_say("FIXTOOLS", all ? " ALL" : "", NULL, NULL);
    /* 1. what GFOpenTypes recorded */
    if ((text = read_file(OT_CHANGES, NULL))) {
        fix_say("from ", OT_CHANGES, NULL, NULL);
        for (p = text; *p; ) {
            char *tab, *bk;
            line = p;
            while (*p && *p != '\n') p++;
            if (*p) *p++ = 0;
            if (!(tab = strchr(line, '\t'))) continue;
            *tab = 0; bk = tab + 1;
            if (strlen(line) < 6 || strlen(bk) < 6 || strlen(line) >= sizeof path || strlen(bk) >= sizeof line_copy) continue;
            if (!same_word(line + strlen(line) - 5, ".info") || !same_word(bk + strlen(bk) - 5, ".info")) continue;
            strcpy(path, line); path[strlen(path) - 5] = 0;
            strcpy(line_copy, bk); line_copy[strlen(line_copy) - 5] = 0;
            if (icon_tool(line_copy, orig, sizeof orig))
                fix_icon(path, orig, all, "its original, kept by GFOpenTypes");
        }
        FreeVec(text);
    }
    /* 2. the table of AmigaOS 3.2.3's own icons */
    if (!(text = read_file(table, NULL))) fix_say("no table: ", table, NULL, NULL);
    else {
        fix_say("from ", table, NULL, NULL);
        for (p = text; *p; ) {
            char *tab;
            line = p;
            while (*p && *p != '\n') p++;
            if (*p) *p++ = 0;
            if (*line == ';' || !*line) continue;
            if (!(tab = strchr(line, '\t'))) continue;
            *tab = 0;
            if (strlen(line) >= sizeof path || strlen(tab + 1) >= sizeof orig) continue;
            strcpy(path, line); strcpy(orig, tab + 1);
            fix_icon(path, orig, all, "its stock default tool, from AmigaOS 3.2.3");
        }
        FreeVec(text);
    }
    CloseLibrary(IconBase);
    IconBase = NULL;
    Printf((STRPTR)"GFOpenUp: default tools: %ld icon%s checked, %ld put back, %ld kept on GFOpenView (kinds of file), %ld failed\n",
           (LONG)fix_checked, (LONG)(fix_checked == 1 ? "" : "s"), (LONG)fix_restored, (LONG)fix_kept, (LONG)fix_failed);
    /* the log, kept: what was put back is there to read, and the Installer's T:GFOpenUp.log has it too */
    if (fix_restored || fix_kept || fix_failed) {
        PutStr((STRPTR)fix_log);
        if (exists("ENVARC:GFOpenAmiga")) {
            BPTR fh = Open((STRPTR)REPAIR_LOG, MODE_READWRITE);
            if (fh) { Seek(fh, 0, OFFSET_END); Write(fh, fix_log, fix_log_len); Close(fh); }
        }
    }
    return fix_failed ? RETURN_WARN : RETURN_OK;
}

/* ---- SETTING and THEME --------------------------------------------------------------------- */

/* A setting as a file, installed like any file (the original kept the first time, and listed), through T:.
 * The manifest is loaded by the caller and saved here. RETURN_OK or RETURN_ERROR. */
static int install_text(const char *dest, const char *text)
{
    int rc;
    if (!write_file("T:GFOpenUp-setting", text, strlen(text))) return RETURN_ERROR;
    rc = install_file("T:GFOpenUp-setting", dest, 0) < 0 ? RETURN_ERROR : RETURN_OK;
    DeleteFile((STRPTR)"T:GFOpenUp-setting");
    if (rc == RETURN_OK && !manifest_save()) rc = RETURN_ERROR;
    return rc;
}

/* The look's theme (GFOpenUpTool 0.7.5). ENVARC:GFOpenGadTools/Look's first line is "<theme> <mode>", and the
 * lines after it are the user's own, written by GFOpenPrefs Look: dark mode by the clock, an accent, Lite,
 * "profile auto", which screens and programs are drawn in the theme, the windows' gadgets. Up to GFOpenUp 0.7.5
 * the Installer wrote the whole file again on every run, with the theme its question gave: a Novice never sees
 * the question, so every Novice upgrade put back Open, light, without the user's lines.
 *   GFOpenUpTool THEME <name> <look>   with a look saved already, only its theme becomes <name>, as GFOpenLook
 *                                    THEME <name> SAVE does (the mode and every other line stay); with none
 *                                    (a new install), <look> is the look. ENVARC: and ENV: both. */
/* The look THEME writes: with old (a look saved already) its first word becomes name and the rest stays; with
 * none, look. NULL when out of memory; FreeVec it. OLDLOOK uses it to know what THEME wrote. */
static char *look_text(const char *old, const char *name, const char *look)
{
    char *text;
    const char *p;
    ULONG n;
    if (!old) {
        n = strlen(look) + 2;
        if (!(text = AllocVec(n, MEMF_ANY))) return NULL;
        strcpy(text, look);
        if (!*text || text[strlen(text) - 1] != '\n') strcat(text, "\n");
        return text;
    }
    /* the first word goes; the rest of the first line (the mode) and every line after it stay */
    for (p = old; *p == ' ' || *p == '\t'; p++) ;
    if (*p == ';') p = NULL;                         /* a comment first: the name and light, before it */
    else if (*p == '"') { for (p++; *p && *p != '"' && *p != '\n'; p++) ; if (*p == '"') p++; }
    else while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') p++;
    n = strlen(name) + strlen(old) + 10;
    if (!(text = AllocVec(n, MEMF_ANY))) return NULL;
    strcpy(text, name);
    if (!p) { strcat(text, " light\n"); strcat(text, old); }
    else if (*p == '\n' || *p == '\r' || !*p) { strcat(text, " light"); strcat(text, p); }
    else strcat(text, p);
    if (text[strlen(text) - 1] != '\n') strcat(text, "\n");
    return text;
}

static int set_theme(const char *name, const char *look)
{
    char *old = read_file(LOOK_ENVARC, NULL), *text = look_text(old, name, look);
    int rc;
    if (old) FreeVec(old);
    if (!text) return RETURN_FAIL;
    manifest_load();
    ensure_dir("ENV:GFOpenGadTools", 0);
    rc = install_text(LOOK_ENVARC, text);
    if (rc == RETURN_OK) rc = install_text(LOOK_ENV, text);
    if (rc == RETURN_OK) say("GFOpenUp: the look's theme is ", name, "");
    else say("GFOpenUp: can't write ", LOOK_ENVARC, ". Is SYS: full or write-protected?");
    FreeVec(text);
    return rc;
}

/* ---- UNINSTALL ----------------------------------------------------------------------------- */

#define WBPATTERN "ENVARC:Sys/WBPattern.prefs"
#define WBPATTERN_ENV "ENV:Sys/WBPattern.prefs"

/* 0.7.7: Workbench's backdrop setting goes back first, before any file is deleted. Whoever shows the backdrop
 * (IPrefs for Hyperion's Workbench, GFOpenBench 0.1.19 for itself) keeps its picture's file open until the
 * setting changes; before, the setting went back in the same pass as the pictures, so the picture could
 * still be in use when its turn came. A setting GFOpenUp made from nothing is put back as a pattern file with
 * no picture in ENV: (ENV: is gone at the next boot), so the picture is let go now. */
static void backdrop_setting_back(int *restored, int *problems)
{
    static const char empty[] = "FORM\0\0\0\x12PREFPRHD\0\0\0\x06\0\0\0\0\0\0";
    int i;
    for (i = 0; i < nents; i++) {
        entry *e = &ents[i];
        if (strcmp(e->a, WBPATTERN) && strcmp(e->a, WBPATTERN_ENV)) continue;
        if (!strcmp(e->kind, "replaced")) {
            if (copy_file(e->b, e->a, 0)) { DeleteFile((STRPTR)e->b); (*restored)++; e->kind[0] = 0; }
            else { say("GFOpenUp: can't put back ", e->a, "; its original is in SYS:Storage/GFOpenUp-Superseded."); (*problems)++; }
        } else if (!strcmp(e->kind, "new") && !strcmp(e->a, WBPATTERN_ENV)) {
            if (write_file(e->a, empty, (LONG)sizeof empty - 1)) e->kind[0] = 0;
        }
    }
}

/* 1 when path is in GFOpenUp's own backdrop drawer. Everything there is GFOpenUp's: a picture an earlier install
 * put there before the manifest listed it is marked "same", and each picture's tone sits beside it as
 * "<picture>.tone", never listed (Daletop's lab, 12 Oct 2026: 80 logo pictures and 296 .tone files were left
 * after UNINSTALL). */
#define BACKDROP_DRAWER "SYS:Prefs/Presets/Backdrops/GFOpenUp/"
static int in_backdrop_drawer(const char *path)
{
    return !strncmp(path, BACKDROP_DRAWER, sizeof BACKDROP_DRAWER - 1);
}

/* The tone file beside a backdrop picture, when there is one. */
static void delete_tone(const char *pic)
{
    char tone[ENTRY_PATH];
    if (strlcpy(tone, pic, sizeof tone) >= sizeof tone || str_cat(tone, ".tone", sizeof tone) >= sizeof tone) return;
    DeleteFile((STRPTR)tone);
}

static int uninstall(void)
{
    int i, problems = 0, restored = 0, removed = 0, said_wait = 0, kept = 0;
    manifest_load();
    if (!nents && !exists(MANIFEST)) {
        say("GFOpenUp isn't installed here: there is no list of its files (" MANIFEST "), so nothing was changed.", NULL, NULL);
        return RETURN_WARN;
    }
    /* 9 Nov 2026: only from a whole list. A list cut short or not read whole names part of what GFOpenUp put in and
     * none of what it didn't, but it can't be told which drawers and originals its lost lines held: nothing is done. */
    if (manifest_short) {
        say("GFOpenUp: its list of installed files (" MANIFEST ") is cut short or can't be read, so nothing was taken out. "
            "If " MANIFEST_ENV " is whole, copy it in its place and run Uninstall-GFOpenUp again.", NULL, NULL);
        return RETURN_ERROR;
    }
    /* default tools still on GFOpenView (GFOpenTypes' UNDO has run before this when the Uninstall-GFOpenUp script
     * does it): put back, with the kinds of file too, before GFOpenView goes */
    if (exists(STOCK_TOOLS)) fix_tools(STOCK_TOOLS, 1);
    if (!write_startup(1)) {
        say("GFOpenUp: can't take its block out of S:User-Startup. Remove the lines from \"" BLOCK_BEGIN "\" to \"" BLOCK_END "\" by hand.", NULL, NULL);
        problems++;
    }
    backdrop_setting_back(&restored, &problems);
    for (i = nents - 1; i >= 0; i--) {
        entry *e = &ents[i];
        if (in_backdrop_drawer(e->a) && (!strcmp(e->kind, "new") || !strcmp(e->kind, "same")))
            delete_tone(e->a);
        if (!strcmp(e->kind, "same") && in_backdrop_drawer(e->a))
            strcpy(e->kind, "new");                         /* GFOpenUp's drawer: GFOpenUp's picture */
        if (!strcmp(e->kind, "new") || !strcmp(e->kind, "newfile") || !strcmp(e->kind, "rnew")) {
            if (!strcmp(e->kind, "newfile")) {
                LONG len = 0;
                char *t = read_file(e->a, &len);
                int blank = 1;
                for (LONG k = 0; t && k < len; k++) if (t[k] != '\n' && t[k] != ' ') { blank = 0; break; }
                if (t) FreeVec(t);
                if (!blank) { e->kind[0] = 0; continue; }   /* someone else wrote in it since: theirs now */
            } else if (!still_ours(e)) {
                /* other bytes than GFOpenUp wrote there (its mark, src/openup_lines.h): someone else's now */
                say("GFOpenUp: ", e->a, " has changed since GFOpenUp put it there, so it stays.");
                e->kind[0] = 0;
                kept++;
                continue;
            }
            if (DeleteFile((STRPTR)e->a)) { removed++; e->kind[0] = 0; }
            else if (!exists(e->a)) e->kind[0] = 0;          /* gone already */
            else {
                /* In use, as the backdrop picture is until whoever shows it has read the setting put back
                 * above and let it go; decoding the new one takes a while on a 68k: up to 30 seconds. */
                int tries;
                if (!said_wait++) say("GFOpenUp: waiting for ", e->a, " to be let go...");
                for (tries = 0; tries < 60 && !DeleteFile((STRPTR)e->a) && exists(e->a); tries++)
                    Delay(25);
                if (exists(e->a)) { say("GFOpenUp: can't delete ", e->a, " yet: it is in use."); problems++; }
                else { removed++; e->kind[0] = 0; }
            }
        } else if (!strcmp(e->kind, "moved")) {                /* RECONCILE's: moved back */
            LONG ok = exists(e->a) ? 0 : Rename((STRPTR)e->b, (STRPTR)e->a);
            if (ok) { restored++; e->kind[0] = 0; }
            else { say("GFOpenUp: can't put back ", e->a, "; it is in SYS:Prefs/Classic or SYS:Storage/GFOpenUp-Superseded"); problems++; }
        } else if (!strcmp(e->kind, "replaced") && !find_entry("dir", e->a)) {
            if (copy_file(e->b, e->a, 0)) { DeleteFile((STRPTR)e->b); restored++; e->kind[0] = 0; }
            else { say("GFOpenUp: can't put back ", e->a, "; its original is in SYS:Storage/GFOpenUp-Superseded."); problems++; }
        }
    }
    for (i = nents - 1; i >= 0; i--)
        if (!strcmp(ents[i].kind, "dir"))
            DeleteFile((STRPTR)ents[i].a);            /* only an empty one goes */
    /* A file that was where a drawer of GFOpenUp's went (INSTALL's clear_way, 25 Oct 2026; listed as replaced, with a
     * dir line of the same name): back now the drawer is gone. */
    for (i = nents - 1; i >= 0; i--)
        if (!strcmp(ents[i].kind, "replaced") && find_entry("dir", ents[i].a)) {
            entry *e = &ents[i];
            if (copy_file(e->b, e->a, 0)) { DeleteFile((STRPTR)e->b); restored++; e->kind[0] = 0; }
            else { say("GFOpenUp: can't put back ", e->a, "; it is in SYS:Storage/GFOpenUp-Superseded, and its drawer isn't empty."); problems++; }
        }
    /* The originals' drawers, deepest first: they go once empty. */
    for (i = nents - 1; i >= 0; i--)
        if (!strcmp(ents[i].kind, "replaced")) {
            char dir[ENTRY_PATH];
            if (strlcpy(dir, ents[i].b, sizeof dir) >= sizeof dir) continue;   /* too long to be GFOpenUp's */
            for (;;) {
                char *s = strrchr(dir, '/');
                if (!s || strlen(dir) <= strlen(SUPERSEDED)) break;
                *s = 0;
                if (!DeleteFile((STRPTR)dir)) break;
            }
        }
    DeleteFile((STRPTR)SUPERSEDED);
    if (!problems) {
        /* GFOpenUp's own marks in ENVARC:GFOpenAmiga go with it (GFOpenUpTool 0.7.5). Each records that a step of
         * GFOpenUp's has run; with GFOpenUp gone they mean nothing, and a later install would believe them and skip
         * the step (Setup's first-start questions, RECONCILE, TIDY). The user's settings stay, as any program's
         * settings in ENVARC: stay when the program goes, and a later install finds them: Sound (GFOpenPrefs
         * Sound's levels and beep), PrefsView (simple or advanced, for every GFOpenPrefs editor) and
         * Startup.off (the startup lines switched off in GFOpenUp Setup). */
        static const char *const marks[] = {
            "ENVARC:GFOpenAmiga/Reconcile.done",      /* RECONCILE APPLY has run (its moves are put back above) */
            "ENVARC:GFOpenAmiga/Tidy.done",           /* TIDY has tidied the system's drawers */
            "ENVARC:GFOpenAmiga/Setup.done",          /* GFOpenUp Setup, the first-start wizard, has run or won't come back */
            "ENVARC:GFOpenAmiga/Backdrop.tone",       /* the backdrop's tone, remembered by GFOpenUpTool */
        };
        for (i = 0; i < (int)(sizeof marks / sizeof marks[0]); i++) {
            char env[ENTRY_PATH];
            DeleteFile((STRPTR)marks[i]);
            strlcpy(env, "ENV:", sizeof env);
            str_cat(env, marks[i] + 7, sizeof env);    /* the copy in ENV:, if there is one */
            DeleteFile((STRPTR)env);
        }
        DeleteFile((STRPTR)REPAIRED_MARK);
        DeleteFile((STRPTR)REPAIR_LOG);
        DeleteFile((STRPTR)MANIFEST);
        DeleteFile((STRPTR)MANIFEST_ENV);
        DeleteFile((STRPTR)"ENVARC:GFOpenAmiga");
        DeleteFile((STRPTR)"ENV:GFOpenAmiga");
    } else if (manifest_save()) {
        /* 0.7.7: what is left stays listed (what was done is taken out), so UNINSTALL can be run again to
         * finish: after a reboot nothing holds a backdrop picture. Before, the list kept everything, and a
         * second run failed on the originals already put back. */
        say("GFOpenUp: reboot, then run GFOpenUpTool UNINSTALL again to finish.", NULL, NULL);
    }
    Printf((STRPTR)"GFOpenUp: uninstalled: %ld file%s removed, %ld put back as they were%s\n", removed, (LONG)(removed == 1 ? "" : "s"),
           restored, (LONG)(problems ? "; see the messages above" : ". Reboot to finish."));
    if (kept)
        Printf((STRPTR)"GFOpenUp: %ld file%s GFOpenUp put in had changed since, so %s left as %s.\n", kept,
               (LONG)(kept == 1 ? "" : "s"), (LONG)(kept == 1 ? "it was" : "they were"), (LONG)(kept == 1 ? "it is" : "they are"));
    return problems ? RETURN_ERROR : RETURN_OK;
}

/* ---- BACKDROP ------------------------------------------------------------------------------ */

static ULONG get32(const UBYTE *p) { return (ULONG)p[0] << 24 | (ULONG)p[1] << 16 | (ULONG)p[2] << 8 | p[3]; }
static void put32(UBYTE *p, ULONG v) { p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v; }

/* Workbench's backdrop (the ROOT pattern of WBPattern.prefs) becomes the
 * picture nearest the screen's shape, scaled to fit; the drawer and screen
 * patterns are kept as they are. The old file is kept, as every file GFOpenUp
 * replaces is. */
/* The picture WBPattern.prefs' ROOT pattern names, and its flags: 1 when there is one. */
static int root_picture(const char *file, char *out, int outlen, UWORD *flags)
{
    LONG len = 0, k = 12;
    UBYTE *p = (UBYTE *)read_file(file, &len);
    int found = 0;
    if (!p) return 0;
    if (len > 12 && !memcmp(p, "FORM", 4) && !memcmp(p + 8, "PREF", 4)) {
        while (!found && k + 8 <= len) {
            ULONG n = get32(p + k + 4), whole = 8 + n + (n & 1);
            if (k + (LONG)whole > len) break;
            if (!memcmp(p + k, "PTRN", 4) && n > 24 && (p[k + 8 + 16] << 8 | p[k + 8 + 17]) == 0) {
                ULONG dl = p[k + 8 + 22] << 8 | p[k + 8 + 23];
                if (dl > n - 24) dl = n - 24;
                if (dl > 0 && dl < (ULONG)outlen) {
                    memcpy(out, p + k + 8 + 24, dl);
                    out[dl] = 0;
                    *flags = p[k + 8 + 18] << 8 | p[k + 8 + 19];
                    found = out[0] != 0;
                }
            }
            k += whole;
        }
    }
    FreeVec(p);
    return found;
}

/* BACKDROP <prefix> KEEP (0.8.14: an upgrade from an OpenUp before the GF names, which has just been taken out):
 * the backdrop the user had (WBPattern.prefs as OLDNAMES found it, T:GFOpenUp-WBPattern.prefs, before the old
 * OpenUp's UNINSTALL put back the one from before it) stays when its picture is still there (one of their own,
 * such as Cradle's), and one from OpenUp's old drawer (SYS:Prefs/Presets/Backdrops/OpenUp, gone with that
 * OpenUp) becomes the same picture in GFOpenUp's drawer. Only when neither is there does the theme's come. */
#define OLD_BACKDROP_DRAWER "SYS:Prefs/Presets/Backdrops/OpenUp/"
#define WBPATTERN_BEFORE "T:GFOpenUp-WBPattern.prefs"
static int keep_backdrop(char *pic, UWORD *flags)
{
    char cur[220];
    int i, n = sizeof OLD_BACKDROP_DRAWER - 1;
    if (!root_picture(exists(WBPATTERN_BEFORE) ? WBPATTERN_BEFORE : WBPATTERN, cur, sizeof cur, flags)) return 0;
    if (exists(cur)) {
        strcpy(pic, cur);
        return 1;
    }
    for (i = 0; i < n; i++)
    {
        char a = cur[i], b = OLD_BACKDROP_DRAWER[i];
        if (a >= 'a' && a <= 'z') a -= 32;
        if (b >= 'a' && b <= 'z') b -= 32;
        if (!a || a != b) return 0;
    }
    strcpy(pic, BACKDROP_DRAWER);
    strncat(pic, cur + n, 219 - strlen(pic));
    if (!exists(pic)) return 0;
    return 2;
}

static int backdrop(const char *prefix, int keep)
{
    static const UWORD sizes[4][2] = { { 1920, 1080 }, { 1920, 1200 }, { 1600, 1200 }, { 1280, 1024 } };
    char pic[220];
    struct Screen *scr;
    long sw, sh, best = -1, bestd = 0;
    int i;
    LONG oldlen = 0, len;
    UBYTE *old, *buf, *o;
    UWORD flags = 0x3a00;                                    /* scale (good), image precision, good dither */
    if (keep) {
        UWORD kf = flags;
        int k = keep_backdrop(pic, &kf);
        if (k) { flags = kf; best = 0; goto write; }
    }
    scr = LockPubScreen((STRPTR)"Workbench");
    sw = scr ? scr->Width : 640; sh = scr ? scr->Height : 480;
    if (scr) UnlockPubScreen(NULL, scr);
    {
        /* a strip as tall as the screen, tiled across from the top left: it
         * decodes 16 x H pixels and scales nothing, where a whole picture
         * took most of a minute to decode and scale without a JIT (6 Oct 2026) */
        char name[220], tail[24];
        int k = 0, t;
        long h = sh;
        char digits[8];
        strncpy(name, prefix, 180);
        name[180] = 0;
        strcpy(tail, "-strip-");
        k = 7;
        for (t = 0; h; h /= 10) digits[t++] = '0' + h % 10;
        while (t) tail[k++] = digits[--t];
        strcpy(tail + k, ".png");
        strcat(name, tail);
        if (exists(name)) { best = 0; strcpy(pic, name); flags = 0x4a00; }   /* tile, left top, image precision, good dither */
    }
    for (i = 0; flags != 0x4a00 && i < 4; i++) {
        /* how far the picture's shape is from the screen's, in thousandths */
        long d = (sw * sizes[i][1] * 1000) / (sh * sizes[i][0]) - 1000;
        char name[220];
        if (d < 0) d = -d;
        strncpy(name, prefix, 180);
        name[180] = 0;
        {                                                    /* "-1920x1080.png" */
            char tail[24];
            int a = sizes[i][0], b = sizes[i][1], k = 0, t;
            char digits[8];
            tail[k++] = '-';
            for (t = 0; a; a /= 10) digits[t++] = '0' + a % 10;
            while (t) tail[k++] = digits[--t];
            tail[k++] = 'x';
            for (t = 0; b; b /= 10) digits[t++] = '0' + b % 10;
            while (t) tail[k++] = digits[--t];
            strcpy(tail + k, ".png");
            strcat(name, tail);
        }
        if (exists(name) && (best < 0 || d < bestd)) { best = i; bestd = d; strcpy(pic, name); }
    }
    if (best < 0) {
        say("GFOpenUp: no backdrop pictures named ", prefix, "-<width>x<height>.png. Install the Backdrops part first.");
        return RETURN_ERROR;
    }
write:
    old = (UBYTE *)read_file(WBPATTERN, &oldlen);
    if (!(buf = AllocVec(oldlen + 512, MEMF_CLEAR))) { if (old) FreeVec(old); return RETURN_FAIL; }
    o = buf;
    memcpy(o, "FORM\0\0\0\0PREF", 12); o += 12;
    /* the header: the old file's, or an empty one */
    if (old && oldlen >= 12 && !memcmp(old, "FORM", 4) && !memcmp(old + 8, "PREF", 4) && oldlen >= 22 && !memcmp(old + 12, "PRHD", 4)) {
        ULONG n = get32(old + 16);
        memcpy(o, old + 12, 8 + n + (n & 1)); o += 8 + n + (n & 1);
    } else {
        memcpy(o, "PRHD\0\0\0\6\0\0\0\0\0\0", 14); o += 14;
    }
    /* the backdrop */
    {
        ULONG n = strlen(pic) + 1, size = 24 + n;
        memcpy(o, "PTRN", 4); put32(o + 4, size); o += 8;
        memset(o, 0, 24);
        o[16] = 0; o[17] = 0;                                /* wbp_Which: WBP_ROOT */
        o[18] = flags >> 8; o[19] = flags & 255;             /* wbp_Flags: placement, precision, dither */
        o[22] = n >> 8; o[23] = n & 255;                     /* wbp_DataLength */
        memcpy(o + 24, pic, n);
        o += 24 + n + (size & 1);
    }
    /* the drawer and screen patterns, as they were */
    if (old && oldlen > 12 && !memcmp(old, "FORM", 4)) {
        LONG k = 12;
        while (k + 8 <= oldlen) {
            ULONG n = get32(old + k + 4), whole = 8 + n + (n & 1);
            if (k + (LONG)whole > oldlen) break;
            if (!memcmp(old + k, "PTRN", 4) && n >= 18 && (old[k + 8 + 16] << 8 | old[k + 8 + 17]) != 0) {
                memcpy(o, old + k, whole); o += whole;
            }
            k += whole;
        }
    }
    len = o - buf;
    put32(buf + 4, len - 8);
    if (old) FreeVec(old);
    manifest_load();
    ensure_dir("ENVARC:Sys", 0);
    ensure_dir("ENV:Sys", 0);
    i = write_file("T:GFOpenUp-wbpattern", (char *)buf, len)
        && install_file("T:GFOpenUp-wbpattern", WBPATTERN, 0) >= 0
        && install_file("T:GFOpenUp-wbpattern", WBPATTERN_ENV, 0) >= 0
        && manifest_save();
    DeleteFile((STRPTR)"T:GFOpenUp-wbpattern");
    FreeVec(buf);
    if (!i)
        return RETURN_ERROR;
    say("GFOpenUp: Workbench's backdrop is ", pic, "");
    return RETURN_OK;
}

/* ---- FONT and TITLEBARS: OS 3.2's own prefs files, changed in one place ------------------ */

/* Puts a prefs file built in memory in place in ENVARC:, the original kept
 * as any file GFOpenUp replaces. Not ENV:: IPrefs would reset the Workbench
 * screen at once, while the Installer's window is open on it ("Intuition is
 * attempting to reset the Workbench screen"); ENV: gets it at the next boot. */
static int install_prefs(const char *name, const UBYTE *data, LONG len)
{
    char envarc[64];
    int ok;
    strcpy(envarc, "ENVARC:Sys/"); strcat(envarc, name);
    manifest_load();
    ensure_dir("ENVARC:Sys", 0);
    ok = write_file("T:GFOpenUp-prefs", (const char *)data, len)
         && install_file("T:GFOpenUp-prefs", envarc, 0) >= 0
         && manifest_save();
    DeleteFile((STRPTR)"T:GFOpenUp-prefs");
    return ok;
}

/* One line added to GFOpenUpTool's log, ENVARC:GFOpenUp/GFOpenUpTool.log (openup_unattended.h). */
static void log_append(const char *line)
{
    BPTR fh, l;
    if (!exists("ENVARC:GFOpenUp") && (l = CreateDir((STRPTR)"ENVARC:GFOpenUp"))) UnLock(l);
    if ((fh = Open((STRPTR)OU_LOG, MODE_READWRITE))) {
        Seek(fh, 0, OFFSET_END);
        FPuts(fh, (STRPTR)line);
        FPutC(fh, '\n');
        Close(fh);
    }
}

/* Font prefs' system default text: DejaVuSansMono.font 11 becomes GFOSMono.font 11, that one setting only (Dale,
 * 14 November 2026: openup_sysfont.h), in ENVARC: and, while Workbench isn't open yet, ENV:. 1 moved, 0 nothing. */
static int move_sysfont(void)
{
    LONG len = 0;
    UBYTE *d;
    int ok;
    struct MsgPort *wbport;
    d = (UBYTE *)read_file("ENVARC:Sys/Font.prefs", &len);
    if (!ous_migrate(d, len, exists("FONTS:" OUS_NEW_FONT) && exists(OUS_NEW_SIZE_FILE))) { if (d) FreeVec(d); return 0; }
    /* the old Font.prefs, kept beside it (once: a later run finds GFOS Mono there and changes nothing) */
    if (!exists(OUS_KEEP) && !copy_file("ENVARC:Sys/Font.prefs", OUS_KEEP, 0)) {
        say("GFOpenUp: can't keep a copy of ENVARC:Sys/Font.prefs; the system font stays DejaVu Sans Mono 11", NULL, NULL);
        FreeVec(d);
        return 0;
    }
    {
        /* UPDATE AUTO reinstalls with the settings in place kept (reinstalling), which left ENVARC: as it was;
         * this one setting is the move Dale asked for at update (16:23 "Move it at update") */
        int was = reinstalling;
        reinstalling = 0;
        ok = install_prefs("Font.prefs", d, len);
        reinstalling = was;
    }
    if (ok) {                                       /* read it back: said only when ENVARC: holds the new one */
        LONG blen = 0;
        UBYTE *back = (UBYTE *)read_file("ENVARC:Sys/Font.prefs", &blen);
        ok = back && ous_landed(back, blen);
        if (back) FreeVec(back);
    }
    if (!ok) {
        say("GFOpenUp: couldn't write ENVARC:Sys/Font.prefs; the system font stays DejaVu Sans Mono 11", NULL, NULL);
        FreeVec(d);
        return 0;
    }
    Forbid();
    wbport = FindPort((STRPTR)"WORKBENCH");
    Permit();
    if (!wbport) write_file("ENV:Sys/Font.prefs", (const char *)d, len);
    FreeVec(d);
    log_append(OUS_LOG_LINE);
    say("GFOpenUp: the system font (Font prefs) is now GFOS Mono 11, was DejaVu Sans Mono 11",
                " (GFOS Mono keeps every letter whole in OS 3.2's lists; from the next reboot). The old prefs: " OUS_KEEP, NULL);
    return ok;
}

/* Copies the chunks of an IFF PREF file that keep() wants, after the header. */
static UBYTE *prefs_start(const UBYTE *old, LONG oldlen, LONG extra, UBYTE **o)
{
    UBYTE *buf = AllocVec(oldlen + extra + 64, MEMF_CLEAR);
    if (!buf) return NULL;
    *o = buf;
    memcpy(*o, "FORM\0\0\0\0PREF", 12); *o += 12;
    if (old && oldlen >= 22 && !memcmp(old, "FORM", 4) && !memcmp(old + 8, "PREF", 4) && !memcmp(old + 12, "PRHD", 4)) {
        ULONG n = get32(old + 16);
        memcpy(*o, old + 12, 8 + n + (n & 1)); *o += 8 + n + (n & 1);
    } else {
        memcpy(*o, "PRHD\0\0\0\6\0\0\0\0\0\0", 14); *o += 14;
    }
    return buf;
}

#define FONT_CHUNK 156               /* struct FontPrefs */

/* One FontPrefs chunk: fp_Type, pens, draw modes, the font. */
static UBYTE *font_chunk(UBYTE *o, int type, int front, int back, int mode, int special, const char *font, LONG size)
{
    memcpy(o, "FONT", 4); put32(o + 4, FONT_CHUNK); o += 8;
    memset(o, 0, FONT_CHUNK);
    o[15] = (UBYTE)type;                             /* fp_Type */
    o[16] = (UBYTE)front; o[17] = (UBYTE)back;       /* fp_FrontPen, fp_BackPen */
    o[18] = (UBYTE)mode; o[19] = (UBYTE)special;     /* fp_DrawMode, fp_SpecialDrawMode (3.2: 1 shadow, 2 outline) */
    o[24] = size >> 8; o[25] = size & 255;           /* fp_TextAttr.ta_YSize */
    o[27] = 0x02;                                    /* ta_Flags: FPF_DISKFONT */
    strcpy((char *)o + 28, font);                    /* fp_Name */
    return o + FONT_CHUNK;
}

/* The desktop's text: the screens' font (fp_Type 2, FP_SCREENFONT) and the
 * icons' text (fp_Type 0, FP_WBFONT) in the same font and size. GFOpenUp owns
 * the desktop's text, so the icons no longer keep Topaz 8 in a filled box
 * under the new screen text (the user, 10 October 2026): the label is drawn
 * as text only (JAM1) in the pens it had; a shadow or an outline chosen in
 * GFOpenPrefs Look is kept. The system font (fp_Type 1, fixed width for the
 * Shell) stays as it is. */
/* The Font.prefs FONT makes from old (NULL: none): the length in *lenp. NULL when out of memory; FreeVec it. */
static UBYTE *font_prefs(const UBYTE *old, LONG oldlen, const char *font, LONG size, LONG *lenp)
{
    LONG k;
    UBYTE *buf, *o;
    int front = 1, back = 0, special = 0;
    if (!(buf = prefs_start(old, oldlen, 2 * (FONT_CHUNK + 8), &o))) return NULL;
    if (old && oldlen > 12 && !memcmp(old, "FORM", 4)) {
        for (k = 12; k + 8 <= oldlen; ) {
            ULONG n = get32(old + k + 4), whole = 8 + n + (n & 1);
            if (k + (LONG)whole > oldlen) break;
            if (!memcmp(old + k, "FONT", 4) && n >= 20 && old[k + 8 + 14] == 0 && old[k + 8 + 15] == 0) {
                front = old[k + 8 + 16]; back = old[k + 8 + 17];     /* the icons' pens and effect, kept */
                if (old[k + 8 + 19] == 1 || old[k + 8 + 19] == 2) special = old[k + 8 + 19];
            } else if (!memcmp(old + k, "FONT", 4) && !(n >= 16 && old[k + 8 + 14] == 0 && old[k + 8 + 15] == 2)) {
                memcpy(o, old + k, whole); o += whole;               /* the system font stays */
            }
            k += whole;
        }
    }
    o = font_chunk(o, 0, front, back, 0 /* JAM1 */, special, font, size);
    o = font_chunk(o, 2, 1, 0, 1 /* JAM2 */, 0, font, size);
    *lenp = o - buf;
    put32(buf + 4, *lenp - 8);
    return buf;
}

static int set_font(const char *font, LONG size)
{
    LONG oldlen = 0, len = 0;
    UBYTE *old, *buf;
    int ok;
    if (size < 6 || size > 64 || strlen(font) > 120) { say("GFOpenUp: FONT wants a font name and a size from 6 to 64", NULL, NULL); return RETURN_FAIL; }
    old = (UBYTE *)read_file("ENVARC:Sys/Font.prefs", &oldlen);
    buf = font_prefs(old, oldlen, font, size, &len);
    if (old) FreeVec(old);
    if (!buf) return RETURN_FAIL;
    ok = install_prefs("Font.prefs", buf, len);
    FreeVec(buf);
    if (!ok) return RETURN_ERROR;
    say("GFOpenUp: the screens' and the icons' text are ", font, " (from the next reboot)");
    return RETURN_OK;
}

#define ICTL_CHUNK 38                /* struct IControlPrefs, version 2 */

/* Title bar extra height: the last two bytes of ic_GUIGeometry (screen,
 * window), read by IPrefs only when ic_Flags has ICF_VERSIONED. */
static UBYTE *ictl_prefs(const UBYTE *old, LONG oldlen, LONG screen, LONG window, LONG *lenp)
{
    static const UBYTE defaults[ICTL_CHUNK] = {    /* as IControl saves with nothing changed */
        0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  0,0x32, 0,0x40,  0x80,0,0,0x1e,  'N','M','V','B',  0,2, 0,0, 0,0, 0,0,0,0 };
    LONG k;
    UBYTE *buf, *o, *mine = NULL;
    if (!(buf = prefs_start(old, oldlen, ICTL_CHUNK + 8, &o))) return NULL;
    if (old && oldlen > 12 && !memcmp(old, "FORM", 4)) {
        for (k = 12; k + 8 <= oldlen; ) {
            ULONG n = get32(old + k + 4), whole = 8 + n + (n & 1);
            if (k + (LONG)whole > oldlen) break;
            if (!memcmp(old + k, "ICTL", 4) && !mine) {
                /* the user's own settings, made the current version's size */
                memcpy(o, "ICTL", 4); put32(o + 4, n > ICTL_CHUNK ? n : ICTL_CHUNK);
                memcpy(o + 8, old + k + 8, n);
                if (n < ICTL_CHUNK) { o[8 + 28] = 0; o[8 + 29] = 2; }      /* an old file: version 2 now */
                mine = o + 8;
                o += 8 + (n > ICTL_CHUNK ? n + (n & 1) : ICTL_CHUNK);
            } else if (memcmp(old + k, "PRHD", 4)) {
                memcpy(o, old + k, whole); o += whole;                     /* IEXC exceptions and the rest */
            }
            k += whole;
        }
    }
    if (!mine) {
        memcpy(o, "ICTL", 4); put32(o + 4, ICTL_CHUNK);
        memcpy(o + 8, defaults, ICTL_CHUNK);
        mine = o + 8;
        o += 8 + ICTL_CHUNK;
    }
    mine[20] |= 0x80;                               /* ICF_VERSIONED */
    if (!mine[28] && mine[29] < 2) mine[29] = 2;
    mine[36] = (UBYTE)screen;
    mine[37] = (UBYTE)window;
    *lenp = o - buf;
    put32(buf + 4, *lenp - 8);
    return buf;
}

static int set_titlebars(LONG screen, LONG window)
{
    LONG oldlen = 0, len = 0;
    UBYTE *old, *buf;
    int ok;
    if (screen < 0 || screen > 16 || window < 0 || window > 16) { say("GFOpenUp: TITLEBARS wants two numbers from 0 to 16", NULL, NULL); return RETURN_FAIL; }
    old = (UBYTE *)read_file("ENVARC:Sys/IControl.prefs", &oldlen);
    buf = ictl_prefs(old, oldlen, screen, window, &len);
    if (old) FreeVec(old);
    if (!buf) return RETURN_FAIL;
    ok = install_prefs("IControl.prefs", buf, len);
    FreeVec(buf);
    if (!ok) return RETURN_ERROR;
    say("GFOpenUp: title bars have extra height (from the next reboot)", NULL, NULL);
    return RETURN_OK;
}

/* ---- OLDLOOK: the look's files earlier OpenUps wrote, taken out (0.8.2) ------------------------------------------
 *
 * The GFOpenKick ROM draws Workbench's look itself (a 24-pixel screen bar in CGTriumvirate on a 1080-line screen,
 * the icons on the backdrop), and on OS 3.2.3 the look is the OS's own; the files below only changed that (the
 * user, 16 October 2026: "it should be default, straight from the rom"). GFOpenUp up to 0.8.1 wrote them through
 * its GFOpenLook part; 0.8.2 writes none, and an upgrade takes out what an earlier one wrote:
 *   C:GFOpenLook and its line in S:User-Startup
 *   ENVARC: and ENV:GFOpenGadTools/Theme   (SETTING, a theme's name)
 *   ENVARC: and ENV:GFOpenGadTools/Look    (THEME)
 * A file goes only when the manifest lists it as GFOpenUp's and its bytes are what GFOpenUp wrote, worked out again
 * from the original GFOpenUp kept (or from none, where there was none). The original comes back where there was
 * one. A file the user has changed since stays as it is, and the log says so.
 *
 * Font prefs and IControl prefs (FONT CGTriumvirate.font 13, TITLEBARS 0 2) are no longer taken out (0.8.7). Up to
 * 0.8.6 OLDLOOK put back the stock Font.prefs and deleted IControl.prefs where their bytes were still FONT's and
 * TITLEBARS's, but bytes alone can't say whether the user chose them since: on Instance-32 (20 October 2026) the
 * 0.8.6 upgrade put the stock font back and took IControl prefs away from under the user. They are the user's
 * settings now and stay as they are; the originals stay in SYS:Storage/GFOpenUp-Superseded, for UNINSTALL. */

static UBYTE *font_of_type(UBYTE *d, LONG len, int type);

static const char *const old_themes[] = { "Open", "Glass", "Graphite", "Ember", "Clear", "Sky", "Classic", NULL };

static int same_bytes(const UBYTE *a, LONG la, const UBYTE *b, LONG lb) { return a && b && la == lb && !memcmp(a, b, la); }

/* Theme: SETTING wrote a theme's name and a new line */
static int was_theme(const UBYTE *cur, LONG len, const UBYTE *orig, LONG olen)
{
    int i;
    (void)orig; (void)olen;
    for (i = 0; old_themes[i]; i++) {
        LONG n = strlen(old_themes[i]);
        if (len == n + 1 && !memcmp(cur, old_themes[i], n) && cur[n] == '\n') return 1;
    }
    return 0;
}

/* Look: THEME's text for any theme, from the original or (a new install) from the Installer's own look */
static int was_look(const UBYTE *cur, LONG len, const UBYTE *orig, LONG olen)
{
    char look[120];
    int i, j, same = 0;
    (void)olen;
    for (i = 0; old_themes[i] && !same; i++)
        for (j = 0; j < 2 && !same; j++) {
            char *t;
            if (!strcmp(old_themes[i], "Classic")) strcpy(look, "Classic light");
            else {
                strcpy(look, old_themes[i]);
                strcat(look, " light\nzoom off\nscrollers auto");
                if (j) strcat(look, "\nsizegadget off");
            }
            if (!(t = look_text((const char *)orig, old_themes[i], look))) return 0;
            same = same_bytes(cur, len, (const UBYTE *)t, strlen(t));
            FreeVec(t);
        }
    return same;
}

/* One file: 1 taken out (or its original back), 0 left. */
static int take_out_old(const char *dest, int (*was)(const UBYTE *, LONG, const UBYTE *, LONG))
{
    entry *e = find_entry(NULL, dest);
    UBYTE *cur, *orig = NULL;
    LONG len = 0, olen = 0;
    int replaced, ok;
    if (!e || !exists(dest)) return 0;
    replaced = !strcmp(e->kind, "replaced");
    if (!replaced && strcmp(e->kind, "new")) return 0;              /* "same": the user's own bytes already */
    if (!(cur = (UBYTE *)read_file(dest, &len))) return 0;
    if (replaced && !(orig = (UBYTE *)read_file(e->b, &olen))) {
        FreeVec(cur);
        say("GFOpenUp: kept ", dest, ": its original isn't in SYS:Storage/GFOpenUp-Superseded any more");
        return 0;
    }
    if (!was(cur, len, orig, olen)) {
        say("GFOpenUp: kept ", dest, ": it has been changed since GFOpenUp wrote it");
        FreeVec(cur);
        if (orig) FreeVec(orig);
        return 0;
    }
    ok = replaced ? write_file(dest, (const char *)orig, olen) : (DeleteFile((STRPTR)dest) || !exists(dest));
    if (ok) {
        if (replaced) DeleteFile((STRPTR)e->b);
        e->kind[0] = 0;
        say("GFOpenUp: ", dest, replaced ? ": its original is back" : ": taken out");
    } else say("GFOpenUp: can't take out ", dest, "; is the disk write-protected?");
    FreeVec(cur);
    if (orig) FreeVec(orig);
    return ok;
}

/* ---- Icon signatures (0.8.4) ----------------------------------------------------------------------------------
 *
 * What an icon shows and what it is, as icon.library reads it, in 64 bits: the picture (its size, frame, both
 * images with their palettes and transparent colours; the planar images when it has no colour ones) and, for
 * the whole icon, its type, default tool, tool types and stack too (a program's or project's stack: a disk's,
 * drawer's or trashcan's means nothing, and Workbench's snapshot writes it anew). Its place and drawer window are left out:
 * Workbench writes those whenever it is snapshot. OLDLOOK tells by them the pictures GFOpenUp gave up to 0.8.3
 * (Gorilla's, which the pack no longer carries) and OS 3.2.3's own default icons as its install leaves them.
 *   GFOpenUpTool ICONSIG <icon or drawer>   prints each icon's picture and whole signatures
 *   GFOpenUpTool ICONSIG <icon> <fields>   says whether the icon is its own picture over the other icon's fields,
 *                                         as OLDLOOK asks, and leaves what it compared with in T:GFOpenUp-oldface.info */
typedef struct { ULONG a, b; } isig;

static void sig_add(isig *s, const void *p, ULONG n)
{
    const UBYTE *b = (const UBYTE *)p;
    while (n--) {
        s->a = (s->a ^ *b) * 16777619UL;                    /* FNV-1a */
        s->b = (s->b ^ *b++) * 0x5bd1e995UL + 0x9e3779b9UL;  /* and a second, unrelated one */
    }
}

static void sig_long(isig *s, LONG v)
{
    UBYTE b[4];
    b[0] = (UBYTE)(v >> 24); b[1] = (UBYTE)(v >> 16); b[2] = (UBYTE)(v >> 8); b[3] = (UBYTE)v;
    sig_add(s, b, 4);
}

static void sig_text(isig *s, const char *t)
{
    if (!t) { sig_long(s, -1); return; }
    sig_long(s, (LONG)strlen(t));
    sig_add(s, t, strlen(t));
}

static void sig_planar(isig *s, struct Image *im)
{
    if (!im) { sig_long(s, -1); return; }
    sig_long(s, im->Width); sig_long(s, im->Height); sig_long(s, im->Depth);
    if (im->ImageData && im->Width > 0 && im->Height > 0 && im->Depth > 0)
        sig_add(s, im->ImageData, (ULONG)((im->Width + 15) >> 4) * 2 * im->Height * im->Depth);
}

/* the picture's signature in pic; with whole, the icon's in whole */
static void icon_sig(struct DiskObject *o, isig *pic, isig *whole)
{
    LONG w = 0, h = 0, pm = 0, fr = 0;
    int k;
    pic->a = 2166136261UL; pic->b = 0x12345678UL;
    IconControl(o, ICONCTRLA_GetWidth, (ULONG)&w, ICONCTRLA_GetHeight, (ULONG)&h,
                ICONCTRLA_IsPaletteMapped, (ULONG)&pm, ICONCTRLA_GetFrameless, (ULONG)&fr, TAG_DONE);
    sig_long(pic, pm ? 1 : 0);
    if (pm && w > 0 && h > 0) {
        sig_long(pic, w); sig_long(pic, h); sig_long(pic, fr ? 1 : 0);
        for (k = 0; k < 2; k++) {
            UBYTE *img = NULL;
            struct ColorRegister *pal = NULL;
            LONG np = 0, tc = -1;
            IconControl(o, k ? ICONCTRLA_GetImageData2 : ICONCTRLA_GetImageData1, (ULONG)&img,
                        k ? ICONCTRLA_GetPalette2 : ICONCTRLA_GetPalette1, (ULONG)&pal,
                        k ? ICONCTRLA_GetPaletteSize2 : ICONCTRLA_GetPaletteSize1, (ULONG)&np,
                        k ? ICONCTRLA_GetTransparentColor2 : ICONCTRLA_GetTransparentColor1, (ULONG)&tc, TAG_DONE);
            sig_long(pic, img ? 1 : 0); sig_long(pic, np); sig_long(pic, tc);
            if (img) sig_add(pic, img, (ULONG)(w * h));
            if (pal && np > 0) sig_add(pic, pal, (ULONG)np * sizeof(struct ColorRegister));
        }
    } else {                                                    /* a planar icon: its images */
        sig_long(pic, o->do_Gadget.Width); sig_long(pic, o->do_Gadget.Height);
        sig_planar(pic, (struct Image *)o->do_Gadget.GadgetRender);
        sig_planar(pic, (o->do_Gadget.Flags & GFLG_GADGHIGHBITS) == GFLG_GADGHIMAGE ? (struct Image *)o->do_Gadget.SelectRender : NULL);
    }
    if (whole) {
        *whole = *pic;
        sig_long(whole, o->do_Type);
        sig_text(whole, (const char *)o->do_DefaultTool);
        if (o->do_ToolTypes) {
            STRPTR *t;
            for (t = o->do_ToolTypes; *t; t++) sig_text(whole, (const char *)*t);
        }
        sig_long(whole, -2);
        if (o->do_Type != WBDISK && o->do_Type != WBDRAWER && o->do_Type != WBGARBAGE)
            sig_long(whole, o->do_StackSize);           /* a window's icon: Workbench's snapshot rewrites it */
    }
}

/* name with or without .info; 1 read */
static int file_sig(const char *name, isig *pic, isig *whole)
{
    char n[260];
    struct DiskObject *o;
    int l, ok = 0;
    strlcpy(n, name, sizeof n);
    if ((l = strlen(n)) > 5 && same_word(n + l - 5, ".info")) n[l - 5] = 0;
    if (!(IconBase = OpenLibrary((STRPTR)"icon.library", 44))) return 0;
    if ((o = GetIconTags((STRPTR)n, ICONGETA_FailIfUnavailable, TRUE, TAG_DONE))) {
        icon_sig(o, pic, whole);
        FreeDiskObject(o);
        ok = 1;
    }
    CloseLibrary(IconBase);
    IconBase = NULL;
    return ok;
}

static int sig_in(const isig *s, const ULONG (*table)[2], int n)
{
    int i;
    for (i = 0; i < n; i++)
        if (table[i][0] == s->a && table[i][1] == s->b) return 1;
    return 0;
}

static int print_sig(const char *path)
{
    isig p, w;
    if (!file_sig(path, &p, &w)) { Printf((STRPTR)"%s: not an icon\n", (LONG)path); return 0; }
    Printf((STRPTR)"0x%08lx,0x%08lx 0x%08lx,0x%08lx %s\n", p.a, p.b, w.a, w.b, (LONG)path);
    return 1;
}

static int icon_sigs(const char *path)
{
    BPTR lock = Lock((STRPTR)path, ACCESS_READ);
    struct FileInfoBlock *fib;
    char sub[256];
    if (!lock) return print_sig(path) ? RETURN_OK : RETURN_WARN;
    if ((fib = AllocDosObject(DOS_FIB, NULL))) {
        if (Examine(lock, fib) && fib->fib_DirEntryType > 0) {
            while (ExNext(lock, fib)) {
                const char *n = (const char *)fib->fib_FileName;
                int l = strlen(n);
                if (fib->fib_DirEntryType > 0 || l <= 5 || !same_word(n + l - 5, ".info")) continue;
                strlcpy(sub, path, sizeof sub);
                AddPart((STRPTR)sub, (STRPTR)n, sizeof sub);
                print_sig(sub);
            }
        } else print_sig(path);
        FreeDosObject(DOS_FIB, fib);
    }
    UnLock(lock);
    return RETURN_OK;
}

/* ---- The look up to 0.8.3 taken out, GFOpenIcons' put on (0.8.3, 0.8.4) --------------------------------------
 *
 * The user, 16 October 2026: "we should remove them and use ours". Up to 0.8.3 the GorillaIcons part gave the
 * system's icons AROS's Gorilla pictures; 0.8.4's GFOpenIcons part gives them our own (GFOpenIcons look B), and the
 * pack carries no Gorilla picture any more. So OLDLOOK tells Gorilla's pictures by their signatures (gorilla_pics,
 * made with ICONSIG from 0.8.3's pack: every picture the part gave, its faces and the ones 0.8.3 retired):
 *   - An icon GFOpenUp faced (listed "replaced", its original in SYS:Storage/GFOpenUp-Superseded) that is still as
 *     GFOpenUp left it gets its original back. "As GFOpenUp left it" is W25's rule: the icon written again into T:
 *     from its own picture over the original's fields is the same, byte for byte, as the file (its place and
 *     the pointers an icon file stores left out), and that picture is one of Gorilla's. Where its colour images
 *     are packed otherwise than icon.library packs them today, the same picture and fields as icon.library
 *     reads them count (same_fields). The GFOpenIcons part then
 *     gives it our picture; one GFOpenIcons has none for keeps its original.
 *   - An icon the user changed since stays as it is, and so does its listing; face_icon leaves it too.
 *
 * The default icons. The GFOpenKick ROM's icon.library has its own default icons, GFOpenIcons look B (RAM, hard
 * disk, floppy, CD, RAD, card, Kickstart, drawer, tool, project, trashcan), and a def_<kind>.info in ENVARC:Sys,
 * or SYS:'s own Disk.info, wins over them. Each of those below goes when it is:
 *   - GFOpenUp's face of it, as above (up to 0.8.2 the GorillaIcons part faced def_RAM, def_Harddisk... and SYS:);
 *   - or OS 3.2.3's original: the original GFOpenUp kept, unchanged but for its place; or, never listed, an icon
 *     whose whole signature is one of OS 3.2.3's (os_defaults, made with ICONSIG on a new 3.2.3 install). That
 *     one is kept in SYS:Storage/GFOpenUp-Superseded first and listed, as RETIRE does.
 * Every original stays in SYS:Storage/GFOpenUp-Superseded, listed, so UNINSTALL puts it back. A user's own
 * default icon (any other picture, tool or tool types) stays, and T:GFOpenUp.log says so. Kickstart 3.2.3 has
 * built-in icons for all of them, so the stock ROM shows a disk and a drawer still. Work:Disk.info may be
 * unlisted and still carry Gorilla's hard disk (from the faced def_Harddisk, when the volume was snapshot):
 * then it goes when it is that picture over its own fields. The DefIcons file types (def_ascii, def_picture...)
 * stay, with GFOpenIcons' pictures. */
static const ULONG gorilla_pics[][2] = {
#include "gorilla_sigs.h"
};
static const ULONG os_defaults[][2] = {
#include "os323_default_sigs.h"
};
#define NGORILLA ((int)(sizeof gorilla_pics / sizeof gorilla_pics[0]))
#define NOSDEF ((int)(sizeof os_defaults / sizeof os_defaults[0]))

static const char *const default_icons[] = {
    "SYS:Disk.info", "Work:Disk.info",
    "ENVARC:Sys/def_RAM.info", "ENVARC:Sys/def_Harddisk.info", "ENVARC:Sys/def_Disk.info",
    "ENVARC:Sys/def_Drawer.info", "ENVARC:Sys/def_Tool.info", "ENVARC:Sys/def_Project.info",
    "ENVARC:Sys/def_Trashcan.info", "ENVARC:Sys/def_cd0.info", "ENVARC:Sys/def_rad.info",
    "ENVARC:Sys/def_card.info", "ENVARC:Sys/def_kickstart.info", "ENVARC:Sys/def_sys.info",
    "ENVARC:Sys/def_work.info", "ENVARC:Sys/def_CDROM.info", "ENVARC:Sys/def_Kick.info",
};
#define FACE_TMP "T:GFOpenUp-oldface"
static int keep_face_tmp;                          /* ICONSIG <icon> <fields>: T:GFOpenUp-oldface.info stays to look at */

/* An icon file holds memory addresses where the structures had pointers: icon.library reads them only as "there"
 * or "not there", and each write has new ones. These are the DiskObject's (its gadget's, the default tool's, the
 * tool types', the drawer's, the tool window's), the drawer window's, and each image's (its data, the next one). */
static UWORD get16(const UBYTE *p) { return (UWORD)(p[0] << 8 | p[1]); }
static const UBYTE dobj_ptrs[] = { 4, 22, 26, 30, 34, 38, 44, 50, 54, 66, 70 };

/* 1 when the two icon files are the same byte for byte, those addresses aside (both there, or both not) */
static int same_icon_bytes(const UBYTE *a, LONG la, const UBYTE *b, LONG lb)
{
    LONG ptr[32], off = 78, k;
    int np = 0, i, img;
    if (!a || !b || la != lb || la < 78) return 0;
    for (i = 0; i < (int)sizeof dobj_ptrs; i++) ptr[np++] = dobj_ptrs[i];
    if (get32(b + 66)) {                                        /* the drawer: its window's five pointers */
        if (off + 56 > lb) return 0;
        for (i = 18; i <= 34; i += 4) ptr[np++] = off + i;
        off += 56;
    }
    for (img = 0; img < 2; img++) {                             /* the image, and the selected one */
        if (!get32(b + (img ? 26 : 22))) continue;
        if (off + 20 > lb) return 0;
        ptr[np++] = off + 10;
        ptr[np++] = off + 16;
        off += 20 + (LONG)((get16(b + off + 4) + 15) >> 4) * 2 * get16(b + off + 6) * get16(b + off + 8);
        if (off > lb) return 0;
    }
    for (k = 0; k < la; k++) {
        for (i = 0; i < np; i++)
            if (k == ptr[i]) break;
        if (i < np) {
            if (!get32(a + k) != !get32(b + k)) return 0;
            k += 3;
        } else if (a[k] != b[k]) return 0;
    }
    return 1;
}

static void no_info(char *dst, const char *src, int size)
{
    int n;
    strlcpy(dst, src, size);
    if ((n = strlen(dst)) > 5 && same_word(dst + n - 5, ".info")) dst[n - 5] = 0;
}

/* 1 when dest is still the icon put_face makes from the picture of icon pic_name and the fields of fields_name
 * (either with or without .info; pic_name may be dest itself). Its place stays out of it: where it sits and its
 * drawer window are Workbench's to write (a Snapshot of RAM Disk writes ENVARC:Sys/def_RAM.info, through the
 * RAM:Disk.info link), so they come from dest. */
static int still_face(const char *dest, const char *pic_name, const char *fields_name)
{
    struct DiskObject *pic = NULL, *own = NULL, *cur = NULL;
    UBYTE *now = NULL, *want = NULL;
    LONG len = 0, wlen = 0;
    char dname[260], pname[260], fname[260];
    int same = 0;
    no_info(dname, dest, sizeof dname);
    no_info(pname, pic_name, sizeof pname);
    no_info(fname, fields_name, sizeof fname);
    if (!(IconBase = OpenLibrary((STRPTR)"icon.library", 44))) return 0;
    pic = GetIconTags((STRPTR)pname, ICONGETA_FailIfUnavailable, TRUE, TAG_DONE);
    own = GetIconTags((STRPTR)fname, ICONGETA_FailIfUnavailable, TRUE, TAG_DONE);
    cur = GetIconTags((STRPTR)dname, ICONGETA_FailIfUnavailable, TRUE, TAG_DONE);
    if (pic && own && cur) {
        LONG x = own->do_CurrentX, y = own->do_CurrentY, st = own->do_StackSize;
        struct DrawerData *dd = own->do_DrawerData;
        WORD gl = pic->do_Gadget.LeftEdge, gt = pic->do_Gadget.TopEdge;
        own->do_CurrentX = cur->do_CurrentX; own->do_CurrentY = cur->do_CurrentY; own->do_DrawerData = cur->do_DrawerData;
        if (own->do_Type == WBDISK || own->do_Type == WBDRAWER || own->do_Type == WBGARBAGE)
            own->do_StackSize = cur->do_StackSize;  /* a window's icon: nothing runs with it; Workbench's snapshot
                                                       writes 4096 where OS 3.2.3's own had 8192 */
        pic->do_Gadget.LeftEdge = cur->do_Gadget.LeftEdge; pic->do_Gadget.TopEdge = cur->do_Gadget.TopEdge;
        if (put_face(pic, own, FACE_TMP, 0)) {
            now = (UBYTE *)read_file(dest, &len);
            want = (UBYTE *)read_file(FACE_TMP ".info", &wlen);
            same = same_icon_bytes(now, len, want, wlen);
        }
        own->do_CurrentX = x; own->do_CurrentY = y; own->do_DrawerData = dd; own->do_StackSize = st;
        pic->do_Gadget.LeftEdge = gl; pic->do_Gadget.TopEdge = gt;
    }
    if (!keep_face_tmp) DeleteFile((STRPTR)FACE_TMP ".info");
    if (now) FreeVec(now);
    if (want) FreeVec(want);
    if (pic) FreeDiskObject(pic);
    if (own) FreeDiskObject(own);
    if (cur) FreeDiskObject(cur);
    CloseLibrary(IconBase);
    IconBase = NULL;
    return same;
}

static int same_text(const char *a, const char *b) { return (!a && !b) || (a && b && !strcmp(a, b)); }

/* 1 when the two icons have the same type, default tool, tool types and (a program's or project's) stack, as
 * icon.library reads them */
static int same_fields(const char *a_name, const char *b_name)
{
    struct DiskObject *a, *b;
    char an[260], bn[260];
    int same = 0;
    no_info(an, a_name, sizeof an);
    no_info(bn, b_name, sizeof bn);
    if (!(IconBase = OpenLibrary((STRPTR)"icon.library", 44))) return 0;
    a = GetIconTags((STRPTR)an, ICONGETA_FailIfUnavailable, TRUE, TAG_DONE);
    b = GetIconTags((STRPTR)bn, ICONGETA_FailIfUnavailable, TRUE, TAG_DONE);
    if (a && b && a->do_Type == b->do_Type && same_text((char *)a->do_DefaultTool, (char *)b->do_DefaultTool) &&
        (a->do_Type == WBDISK || a->do_Type == WBDRAWER || a->do_Type == WBGARBAGE || a->do_StackSize == b->do_StackSize)) {
        STRPTR *ta = a->do_ToolTypes, *tb = b->do_ToolTypes;
        same = 1;
        while (ta && tb && *ta && *tb && same) same = !strcmp((char *)*ta++, (char *)*tb++);
        if ((ta && *ta) || (tb && *tb)) same = 0;
    }
    if (a) FreeDiskObject(a);
    if (b) FreeDiskObject(b);
    CloseLibrary(IconBase);
    IconBase = NULL;
    return same;
}

/* 1 when dest is still one of Gorilla's pictures, as GFOpenUp gave it, over the fields of fields_name: byte for
 * byte (still_face), or, where the icon's colour images are packed otherwise than icon.library packs them now
 * (SYS:Expansion.info on Instance-32, faced by an older GFOpenUp: the same pixels and palette, other bytes), the
 * same picture and fields as icon.library reads them */
static int still_gorilla(const char *dest, const char *fields_name)
{
    isig p;
    if (!file_sig(dest, &p, NULL) || !sig_in(&p, gorilla_pics, NGORILLA)) return 0;
    return still_face(dest, dest, fields_name) || same_fields(dest, fields_name);
}

/* dest written again from its original (backup), keeping dest's place and drawer window. 1 done. */
static int put_original(const char *dest, const char *backup)
{
    struct DiskObject *own, *cur;
    char dname[260], bname[260];
    int ok = 0;
    no_info(dname, dest, sizeof dname);
    no_info(bname, backup, sizeof bname);
    if (!(IconBase = OpenLibrary((STRPTR)"icon.library", 44))) return 0;
    own = GetIconTags((STRPTR)bname, ICONGETA_FailIfUnavailable, TRUE, TAG_DONE);
    cur = GetIconTags((STRPTR)dname, ICONGETA_FailIfUnavailable, TRUE, TAG_DONE);
    if (own && cur) {
        LONG x = own->do_CurrentX, y = own->do_CurrentY;
        struct DrawerData *dd = own->do_DrawerData;
        WORD gl = own->do_Gadget.LeftEdge, gt = own->do_Gadget.TopEdge;
        own->do_CurrentX = cur->do_CurrentX; own->do_CurrentY = cur->do_CurrentY; own->do_DrawerData = cur->do_DrawerData;
        own->do_Gadget.LeftEdge = cur->do_Gadget.LeftEdge; own->do_Gadget.TopEdge = cur->do_Gadget.TopEdge;
        ok = PutIconTags((STRPTR)dname, own, ICONPUTA_NotifyWorkbench, TRUE, TAG_DONE) != 0;
        own->do_CurrentX = x; own->do_CurrentY = y; own->do_DrawerData = dd;
        own->do_Gadget.LeftEdge = gl; own->do_Gadget.TopEdge = gt;
    }
    if (own) FreeDiskObject(own);
    if (cur) FreeDiskObject(cur);
    CloseLibrary(IconBase);
    IconBase = NULL;
    return ok;
}

static int is_default_icon_name(const char *dest)
{
    int i;
    for (i = 0; i < (int)(sizeof default_icons / sizeof default_icons[0]); i++)
        if (same_word(dest, default_icons[i])) return 1;
    return 0;
}

/* Gorilla's faces on everything but the default icons: each one still as GFOpenUp left it gets its original back
 * (the GFOpenIcons part faces it after). Returns how many. */
static int gorilla_out(void)
{
    int i, n = 0, l;
    for (i = 0; i < nents; i++) {
        entry *e = &ents[i];
        if (strcmp(e->kind, "replaced") || !e->b || (l = strlen(e->a)) <= 5 || !same_word(e->a + l - 5, ".info") ||
            is_default_icon_name(e->a) || !exists(e->a) || !exists(e->b))
            continue;
        if (!still_gorilla(e->a, e->b)) {
            isig p;
            if (file_sig(e->a, &p, NULL) && sig_in(&p, gorilla_pics, NGORILLA))
                say("GFOpenUp: kept ", e->a, ": it has been changed since GFOpenUp gave it Gorilla's picture");
            continue;
        }
        if (put_original(e->a, e->b)) { say("GFOpenUp: ", e->a, ": Gorilla's picture taken out"); n++; }
        else say("GFOpenUp: can't put back ", e->a, "'s own picture; is the disk write-protected?");
    }
    return n;
}

/* One default icon: 1 taken out, 0 left. */
static int default_out(const char *dest)
{
    entry *e = find_entry(NULL, dest);
    isig p, w;
    int theirs;
    if (!exists(dest)) return 0;
    if (e && !strcmp(e->kind, "replaced") && e->b && exists(e->b)) {
        if (!still_face(dest, e->b, e->b) && !still_gorilla(dest, e->b)) {
            say("GFOpenUp: kept ", dest, ": it has been changed since GFOpenUp listed it");
            return 0;
        }
        if (!DeleteFile((STRPTR)dest)) { say("GFOpenUp: can't take out ", dest, "; is the disk write-protected?"); return 0; }
        say("GFOpenUp: ", dest, ": taken out (the ROM's icon shows; its original stays in SYS:Storage/GFOpenUp-Superseded)");
        return 1;
    }
    if (e) return 0;                                            /* "same" or "new": GFOpenUp's own file */
    theirs = !file_sig(dest, &p, &w);
    if (!theirs && sig_in(&w, os_defaults, NOSDEF)) {           /* OS 3.2.3's own, as installed */
        if (retire_file(dest) != 1) return 0;
        say("GFOpenUp: ", dest, ": OS 3.2.3's, taken out (the ROM's icon shows; kept in SYS:Storage/GFOpenUp-Superseded)");
        return 1;
    }
    if (!theirs && same_word(dest, "Work:Disk.info") && sig_in(&p, gorilla_pics, NGORILLA) && still_face(dest, dest, dest)) {
        if (!DeleteFile((STRPTR)dest)) return 0;
        say("GFOpenUp: ", dest, ": taken out (the ROM's icon shows)");
        return 1;
    }
    say("GFOpenUp: kept ", dest, ": not OS 3.2.3's own or GFOpenUp's picture");
    return 0;
}

/* Gorilla's pictures GFOpenUp's list doesn't name where they are: icons RECONCILE moved (SYS:Prefs/Classic's old prefs
 * editors), drawers and programs an older GFOpenUp or a hand change made from a faced icon (SYS:Prefs/Classic,
 * SYS:Developer/GFOpenGPU-SDK...). Found on lab copies of Instance-32, 16 October 2026. Each icon on SYS: (three
 * drawers deep, as ICONS walks) that shows one of Gorilla's pictures and isn't listed, or is listed as GFOpenUp's
 * own file (SYS:Developer/GFOpenGPU-SDK.info, which the GFOpenGPUDev part makes), gets GFOpenIcons' picture
 * instead: the part's own for that place (its face line), else by its name, else of its kind (face_icon: its
 * fields and place stay, the
 * icon as it was goes to SYS:Storage/GFOpenUp-Superseded for UNINSTALL). Returns how many. */
#define OI_PART "Parts/GFOpenIcons"
static int old_icon(const char *path, int *type);
static int sweep_n;

static char *oi_part_text;                         /* Parts/GFOpenIcons/Part, read once for gorilla_sweep */

/* the GFOpenIcons part's own picture for dest (its face line), in from; 1 found */
static int part_face_for(const char *dest, char *from, int size)
{
    char *p, *copy, *line;
    int found = 0;
    if (!oi_part_text || !(copy = AllocVec(strlen(oi_part_text) + 1, MEMF_ANY))) return 0;
    strcpy(copy, oi_part_text);
    for (p = copy; *p && !found; ) {
        char *w[12];
        int n;
        line = p;
        while (*p && *p != '\n') p++;
        if (*p) *p++ = 0;
        n = words(line, w, 12);
        if (n >= 3 && !strcmp(w[0], "face") && same_word(w[2], dest)) {
            strlcpy(from, OI_PART "/", size);
            str_cat(from, w[1], size);
            found = exists(from);
        }
    }
    FreeVec(copy);
    return found;
}

/* 0.8.16: GFOpenIcons' picture for an icon by its name, into from: Icons/<name>, else, for a name from before the GF
 * rename, Icons/GF<name>. Instance-32's SYS:Utilities/OpenClock and OpenCalc keep their old names, and GFOpenIcons
 * has GFOpenClock.info and GFOpenCalc.info: they got the plain tool picture (a gear) instead. 1 when there is one. */
static int gf_renamed(const char *p, size_t n);
static int icon_by_name(const char *name, char *from, int size)
{
    strlcpy(from, OI_PART "/Icons/", size);
    str_cat(from, name, size);
    if (exists(from)) return 1;
    if (gf_renamed(name, strlen(name))) {
        strlcpy(from, OI_PART "/Icons/GF", size);
        str_cat(from, name, size);
        if (exists(from)) return 1;
        strlcpy(from, OI_PART "/Icons/", size);
        str_cat(from, name, size);
    }
    return 0;
}

static void gorilla_sweep(const char *dir, int depth)
{
    BPTR lock = Lock((STRPTR)dir, ACCESS_READ);
    struct FileInfoBlock *fib;
    char sub[256], from[256];
    if (!lock) return;
    if ((fib = AllocDosObject(DOS_FIB, NULL))) {
        if (Examine(lock, fib)) {
            while (ExNext(lock, fib)) {
                const char *n = (const char *)fib->fib_FileName;
                int l = strlen(n), type = 0;
                isig p;
                entry *e;
                strlcpy(sub, dir, sizeof sub);
                if (sub[strlen(sub) - 1] != ':') str_cat(sub, "/", sizeof sub);
                str_cat(sub, n, sizeof sub);
                if (fib->fib_DirEntryType > 0) {
                    if (depth < 3 && strcmp(n, "GFOpenUp-Superseded") && strcmp(n, "Parts") && strcmp(n, "T") &&
                        strcmp(n, "Trashcan") && strncmp(n, ".", 1))
                        gorilla_sweep(sub, depth + 1);
                    continue;
                }
                if (l <= 5 || !same_word(n + l - 5, ".info") || is_default_icon_name(sub) ||
                    ((e = find_entry(NULL, sub)) && strcmp(e->kind, "new")) ||   /* GFOpenUp's own file is ours to face */
                    !file_sig(sub, &p, NULL) || !sig_in(&p, gorilla_pics, NGORILLA))
                    continue;
                if (!part_face_for(sub, from, sizeof from))     /* the part's face for it, else by name */
                    icon_by_name(n, from, sizeof from);
                if (!exists(from)) {
                    static const char *const kinds[] = { NULL, "Disk", "Drawer", "Tool", "Project", "Trashcan", NULL, NULL };
                    old_icon(sub, &type);
                    if (type < 1 || type > 5) continue;
                    strlcpy(from, OI_PART "/Kinds/", sizeof from);
                    str_cat(from, kinds[type], sizeof from);
                    str_cat(from, ".info", sizeof from);
                    if (!exists(from)) continue;
                }
                if (face_icon(from, sub) > 0) {
                    say("GFOpenUp: ", sub, ": Gorilla's picture where the list has none: GFOpenIcons' given instead");
                    sweep_n++;
                }
            }
        }
        FreeDosObject(DOS_FIB, fib);
    }
    UnLock(lock);
}

/* 0.8.16: an icon GFOpenUp gave the plain picture of its kind (GFOpenIcons' Kinds: a tool's is a gear) because no
 * picture by its name was found, and that has one now (icon_by_name: the GF name of a program from before the
 * rename), gets it. Instance-32 after 0.8.14 and 0.8.15: SYS:Utilities/OpenClock and OpenCalc showed the tool
 * picture, on Workbench and on the dock, where GFOpenIcons has GFOpenClock's and GFOpenCalc's. Only icons GFOpenUp
 * faced (listed replaced) that still show exactly a Kinds picture change: one the user changed since stays.
 * Returns how many. */
static int kind_faces_back(void)
{
    static const char *const kinds[] = { "Disk", "Drawer", "Tool", "Project", "Trashcan" };
    isig ks[5], p;
    int nk = 0, i, k, n = 0;
    char path[256];
    for (k = 0; k < 5; k++) {
        strlcpy(path, OI_PART "/Kinds/", sizeof path);
        str_cat(path, kinds[k], sizeof path);
        str_cat(path, ".info", sizeof path);
        if (exists(path) && file_sig(path, &ks[nk], NULL)) nk++;
    }
    if (!nk) return 0;
    for (i = 0; i < nents; i++) {
        entry *e = &ents[i];
        int l, kind_pic = 0, r;
        if (strcmp(e->kind, "replaced") || !e->a || (l = strlen(e->a)) <= 5 || !same_word(e->a + l - 5, ".info")) continue;
        if (is_default_icon_name(e->a) || !exists(e->a) || !file_sig(e->a, &p, NULL)) continue;
        for (k = 0; k < nk; k++) if (ks[k].a == p.a && ks[k].b == p.b) kind_pic = 1;
        if (!kind_pic) continue;
        if (!part_face_for(e->a, path, sizeof path) && !icon_by_name((const char *)FilePart((STRPTR)e->a), path, sizeof path))
            continue;
        strcpy(e->kind, "same");                     /* GFOpenUp's own face: face_icon's "changed since" rule is for the user's */
        r = face_icon(path, e->a);
        strcpy(e->kind, "replaced");                 /* its original in SYS:Storage stays the one UNINSTALL puts back */
        if (r > 0) {
            say("GFOpenUp: ", e->a, ": GFOpenIcons' picture by its name, in place of the plain one of its kind");
            n++;
        }
    }
    return n;
}

static int old_faces_out(void)
{
    int i, n = 0;
    for (i = 0; i < (int)(sizeof default_icons / sizeof default_icons[0]); i++)
        n += default_out(default_icons[i]);     /* no Work: volume: exists() says so, with no requester */
    n += gorilla_out();
    if (exists(OI_PART "/Part")) {                  /* run from the pack's drawer, as the Installer does */
        sweep_n = 0;
        oi_part_text = read_file(OI_PART "/Part", NULL);
        gorilla_sweep("SYS:", 0);
        n += sweep_n;
        n += kind_faces_back();                     /* 0.8.16 */
        if (oi_part_text) { FreeVec(oi_part_text); oi_part_text = NULL; }
    }
    /* what the GorillaIcons part copied (its PNGs and licence texts in SYS:Storage/Icons/Gorilla, its licence note)
     * and its part line: GFOpenUp's own, listed as new */
    for (i = 0; i < nents; i++) {
        entry *e = &ents[i];
        if (!strcmp(e->kind, "part") && !strcmp(e->a, "GorillaIcons")) { e->kind[0] = 0; continue; }
        if (strcmp(e->kind, "new") || !e->a) continue;
        if (!strncmp(e->a, "SYS:Storage/Icons/Gorilla/", 26) || same_word(e->a, "SYS:Utilities/GFOpenUp/Licences/Gorilla.txt")) {
            if (DeleteFile((STRPTR)e->a) || !exists(e->a)) { e->kind[0] = 0; n++; }
        }
    }
    for (i = 0; i < nents; i++)
        if (!strcmp(ents[i].kind, "dir") && ents[i].a && same_word(ents[i].a, "SYS:Storage/Icons/Gorilla") &&
            (DeleteFile((STRPTR)ents[i].a) || !exists(ents[i].a)))
            ents[i].kind[0] = 0;
    return n;
}

static int old_look(void)
{
    int n = 0, i, lines = 0;
    entry *e;
    manifest_load();
    /* C:GFOpenLook, GFOpenUp's own copy (listed as new, and GFOpenLook's by its version string) */
    if ((e = find_entry("new", "SYS:C/GFOpenLook")) && exists("SYS:C/GFOpenLook")) {
        /* put together here: as one string in the program, Version found it first and said "GFOpenLook" */
        static char look[24], *mark[] = { look };
        look[0] = '$';
        strcpy(look + 1, "VER: GFOpenLook ");
        if (has_marker("SYS:C/GFOpenLook", mark, 1) && DeleteFile((STRPTR)"SYS:C/GFOpenLook")) {
            e->kind[0] = 0;
            say("GFOpenUp: SYS:C/GFOpenLook: taken out", NULL, NULL);
            n++;
        }
    }
    for (i = 0; i < nents; i++)
        if ((!strcmp(ents[i].kind, "startup") && strstr(ents[i].a, "C:GFOpenLook")) ||
            (!strcmp(ents[i].kind, "part") && !strcmp(ents[i].a, "GFOpenLook"))) {
            if (!strcmp(ents[i].kind, "startup")) lines++;
            ents[i].kind[0] = 0;
        }
    n += lines;
    n += take_out_old("ENVARC:GFOpenGadTools/Theme", was_theme);
    n += take_out_old("ENV:GFOpenGadTools/Theme", was_theme);
    n += take_out_old(LOOK_ENVARC, was_look);
    n += take_out_old(LOOK_ENV, was_look);
    /* ENVARC:Sys/Font.prefs and IControl.prefs stay: the user's settings (0.8.7, above) */
    if (exists("ENVARC:Sys/Font.prefs") && find_entry(NULL, "ENVARC:Sys/Font.prefs"))
        say("GFOpenUp: kept ENVARC:Sys/Font.prefs", ": the user's settings stay", NULL);
    if (exists("ENVARC:Sys/IControl.prefs") && find_entry(NULL, "ENVARC:Sys/IControl.prefs"))
        say("GFOpenUp: kept ENVARC:Sys/IControl.prefs", ": the user's settings stay", NULL);
    n += old_faces_out();                           /* 0.8.3, 0.8.4: the default icons, and Gorilla's faces */
    if (!manifest_save()) {
        say("GFOpenUp: can't write its list of installed files in ENVARC:GFOpenAmiga. Is SYS: full?", NULL, NULL);
        return RETURN_ERROR;
    }
    if (lines && !write_startup(0)) {
        say("GFOpenUp: can't write S:User-Startup. Is SYS: write-protected?", NULL, NULL);
        return RETURN_ERROR;
    }
    Printf((STRPTR)"GFOpenUp: the old look: %ld thing%s taken out\n", (LONG)n, (LONG)(n == 1 ? "" : "s"));
    return RETURN_OK;
}

/* Workbench as a backdrop: the word at offset 16 of WBConfig.prefs' WBCF chunk (flags, 0, the window's
 * box, then the backdrop word). The user's other settings in the file stay. With no file, the one a
 * stock 3.2.3 writes (a box of 0, 11, 640, 389) with the word set. In ENVARC: only, like FONT: Workbench
 * reads it at the next boot (the original is kept for UNINSTALL). */
static int set_wb_backdrop(void)
{
    LONG oldlen = 0, k, len;
    UBYTE *old = (UBYTE *)read_file("ENVARC:Sys/WBConfig.prefs", &oldlen), *buf, *o;
    int ok, done = 0;
    if (!(buf = prefs_start(old, oldlen, 40, &o))) { if (old) FreeVec(old); return RETURN_FAIL; }
    if (old && oldlen > 12 && !memcmp(old, "FORM", 4)) {
        for (k = 12; k + 8 <= oldlen; ) {
            ULONG n = get32(old + k + 4), whole = 8 + n + (n & 1);
            if (k + (LONG)whole > oldlen) break;
            if (!memcmp(old + k, "WBCF", 4) && n >= 18 && !done) {
                memcpy(o, old + k, whole);
                o[8 + 16] = 0; o[8 + 17] = 1;                          /* the backdrop word */
                o += whole; done = 1;
            } else if (memcmp(old + k, "PRHD", 4)) {
                memcpy(o, old + k, whole); o += whole;
            }
            k += whole;
        }
    }
    if (!done) {
        static const UBYTE wbcf[28] = { 'W','B','C','F', 0,0,0,20,  0,0,0,0, 0,0,0,0,  0,0, 0,11, 2,0x80, 1,0x85,  0,1, 0,0 };
        memcpy(o, wbcf, sizeof wbcf);
        o += sizeof wbcf;
    }
    len = o - buf;
    put32(buf + 4, len - 8);
    if (old) FreeVec(old);
    ok = install_prefs("WBConfig.prefs", buf, len);
    FreeVec(buf);
    if (!ok) return RETURN_ERROR;
    say("GFOpenUp: Workbench is a backdrop (from the next reboot)", NULL, NULL);
    return RETURN_OK;
}

/* No volume gauge: the word at offset 38 of Workbench.prefs' WBNC chunk (wbe_DisableVolumeGauge; the
 * struct of prefs/workbench.h with BOOL a word). Every other chunk, the hidden devices and the title
 * format among them, and the rest of the settings stay. With no file, a stock 3.2.3's WBNC with the
 * word set. ENVARC: only, as FONT: Workbench reads it at the next boot. */
static int set_wb_gauge(void)
{
    LONG oldlen = 0, k, len;
    UBYTE *old = (UBYTE *)read_file("ENVARC:Sys/Workbench.prefs", &oldlen), *buf, *o;
    int ok, done = 0;
    if (!(buf = prefs_start(old, oldlen, 64, &o))) { if (old) FreeVec(old); return RETURN_FAIL; }
    if (old && oldlen > 12 && !memcmp(old, "FORM", 4)) {
        for (k = 12; k + 8 <= oldlen; ) {
            ULONG n = get32(old + k + 4), whole = 8 + n + (n & 1);
            if (k + (LONG)whole > oldlen) break;
            if (!memcmp(old + k, "WBNC", 4) && n >= 40 && !done) {
                memcpy(o, old + k, whole);
                o[8 + 38] = 0; o[8 + 39] = 1;                          /* the gauge is off */
                o += whole; done = 1;
            } else if (memcmp(old + k, "PRHD", 4)) {
                memcpy(o, old + k, whole); o += whole;
            }
            k += whole;
        }
    }
    if (!done) {
        static const UBYTE wbnc[58] = { 'W','B','N','C', 0,0,0,50,
            0,0,0x10,0, 0,0,0,3, 0,0,0,16, 0xff,0xff,0xff,0xff,0,1,0,1,   /* stack 4096, restart 3, precision 16, emboss -1 -1 1 1 */
            0,0, 0,0,0,25, 0,1, 0,1,                                        /* borderless 0, name length 25, new icons, colour icons */
            0,0,0,0, 0,0, 0,0, 0,1,                                         /* icon memory, lock pens, title bar, GAUGE OFF */
            0,50, 0,0,0,0, 0,0,0,0 };                                       /* title delay 50, copy buffer, flags */
        memcpy(o, wbnc, sizeof wbnc);
        o += sizeof wbnc;
    }
    len = o - buf;
    put32(buf + 4, len - 8);
    if (old) FreeVec(old);
    ok = install_prefs("Workbench.prefs", buf, len);
    FreeVec(buf);
    if (!ok) return RETURN_ERROR;
    say("GFOpenUp: drives' windows have no fill gauge (from the next reboot)", NULL, NULL);
    return RETURN_OK;
}

/* ---- RECONCILE: one desktop, the Open one ---------------------------------------------- */

/* The user, 10 October 2026: "we own the config, and the legacy is just that". RECONCILE makes the desktop
 * coherent in one pass, from one map: each old tool is moved where it no longer competes with its Open
 * home, and nothing is ever deleted.
 *   classic  an OS prefs editor goes to SYS:Prefs/Classic (one drawer in, still runs from there); its
 *            settings files stay, since programs read them
 *   retire   goes to SYS:Storage/GFOpenUp-Superseded, only once its Open replacement is on the disk
 *   replace  goes to SYS:Storage/GFOpenUp-Superseded and its Open replacement takes its name, so every icon
 *            and script that starts it starts the replacement (TextEdit: GFOpenEdit; no icon is changed)
 *   wait     stays: its Open home is still to build (listed, so the list is the build list)
 *   keep     stays: the OS's own, and wanted (said why)
 * Each move is a "moved" line in the manifest: RECONCILE UNDO and UNINSTALL put it back.
 * It also keeps the settings GFOpenUp owns in step: the icons' text is the screens' text (Font prefs), in a
 * light colour with a shadow over a dark backdrop and a dark one over a light backdrop (the backdrop's tone
 * from its theme's desktop colours), and the Open apps are on the dock (Dock ADD).
 *   GFOpenUpTool RECONCILE [CHECK]       says what it would do (changes nothing)
 *   GFOpenUpTool RECONCILE APPLY [ONCE]  does it; ONCE (at boot) only when this map is newer than the last run
 *   GFOpenUpTool RECONCILE UNDO          puts back everything RECONCILE moved */
#define RECONCILE_MAP_VERSION "1"
#define RECONCILE_DONE "ENVARC:GFOpenAmiga/Reconcile.done"
#define CLASSIC "SYS:Prefs/Classic"

typedef struct { const char *path, *action, *home, *what; } rmap;
static const rmap reconcile_map[] = {
    { "SYS:Prefs/Asl",          "classic", NULL, "GFOpenLook: file requesters (to build)" },
    { "SYS:Prefs/Font",         "classic", NULL, "GFOpenLook: fonts (to build); GFOpenUp sets the screens' and icons' text" },
    { "SYS:Prefs/Palette",      "classic", NULL, "GFOpenLook: colours (to build)" },
    { "SYS:Prefs/Pointer",      "classic", NULL, "GFOpenLook: pointer, with GFOpenMouseAccel (to build); the old editor crashes on this desktop" },
    { "SYS:Prefs/ReAction",     "classic", NULL, "GFOpenLook: ReAction's look (to build)" },
    { "SYS:Prefs/WBPattern",    "classic", NULL, "GFOpenLook: backdrop (to build)" },
    { "SYS:Prefs/Workbench",    "classic", NULL, "GFOpenLook: Workbench (to build)" },
    { "SYS:Prefs/ScreenMode",   "classic", NULL, "GFOpenLook Screen: mode and overscan (to build)" },
    { "SYS:Prefs/Overscan",     "classic", NULL, "GFOpenLook Screen: mode and overscan (to build)" },
    { "SYS:Prefs/Input",        "classic", NULL, "GFOpenInput's editor: input and IControl (to build)" },
    { "SYS:Prefs/IControl",     "classic", NULL, "GFOpenInput's editor: input and IControl (to build)" },
    { "SYS:Prefs/Time",         "classic", NULL, "Open Time and Locale (to build)" },
    { "SYS:Prefs/Locale",       "classic", NULL, "Open Time and Locale (to build)" },
    { "SYS:Prefs/Printer",      "classic", NULL, "GFOpenPrint's printer settings (to build)" },
    { "SYS:Prefs/PrinterGfx",   "classic", NULL, "GFOpenPrint's printer settings (to build)" },
    { "SYS:Prefs/PrinterPS",    "classic", NULL, "GFOpenPrint's printer settings (to build)" },
    { "SYS:Prefs/DefaultIcons", "classic", "SYS:Preferences/GFOpenPrefs", "GFOpenPrefs's File Types, which takes over default icons" },
    { "SYS:Prefs/Serial",       "classic", "SYS:Preferences/GFOpenPrefs", "GFOpenPrefs's Serial" },
    { "SYS:Prefs/Sound",        "classic", "SYS:Preferences/GFOpenPrefs", "GFOpenPrefs's Sound (a Sound in Prefs is the OS's)" },
    { "SYS:Tools/Commodities/Blanker", "retire", "SYS:Tools/Commodities/GFOpenBlanker", "GFOpenBlanker" },
    { "SYS:Tools/Calculator",   "wait",    NULL, "GFOpenCalculator (source in GFOpenUp, to build)" },
    { "SYS:Tools/TextEdit",     "replace", "C:GFOpenEdit", "GFOpenEdit, under TextEdit's name, so every icon that opens TextEdit opens GFOpenEdit" },
    { "SYS:WBStartup/AsyncWB",  "keep",    NULL, "OS 3.2.3's: Workbench copies and deletes in processes of their own" },
    { "SYS:WBStartup/RAWBInfo", "keep",    NULL, "OS 3.2.3's Information window" },
};
#define NRMAP (int)(sizeof reconcile_map / sizeof reconcile_map[0])

/* Moves a file (and its icon) from a to b on the same volume, noted as "moved". 1 moved, 0 not there, -1 failed. */
static int move_noted(const char *a, const char *b)
{
    char ai[ENTRY_PATH], bi[ENTRY_PATH], dir[ENTRY_PATH];
    LONG ok;
    if (!exists(a)) {
        /* moved by an earlier run that stopped before it saved its list: noted now, so UNDO knows */
        if (exists(b) && !find_entry("moved", a)) {
            add_entry("moved", a, b);
            strlcpy(ai, a, sizeof ai); str_cat(ai, ".info", sizeof ai);
            strlcpy(bi, b, sizeof bi); str_cat(bi, ".info", sizeof bi);
            if (!exists(ai) && exists(bi)) add_entry("moved", ai, bi);
            return 2;
        }
        return 0;
    }
    if (exists(b)) { say("GFOpenUp: ", b, " is there already; left both as they are."); return -1; }
    parent_of(b, dir, sizeof dir);
    if (!ensure_dir(dir, 1)) { say("GFOpenUp: can't make ", dir, "."); return -1; }
    ok = Rename((STRPTR)a, (STRPTR)b);
    if (!ok) {
        if (!copy_file(a, b, 0)) { say("GFOpenUp: can't move ", a, "; left in place."); return -1; }
        ok = DeleteFile((STRPTR)a);
        if (!ok) { DeleteFile((STRPTR)b); say("GFOpenUp: can't move ", a, ". Is it running?"); return -1; }
    }
    add_entry("moved", a, b);
    strlcpy(ai, a, sizeof ai); str_cat(ai, ".info", sizeof ai);
    strlcpy(bi, b, sizeof bi); str_cat(bi, ".info", sizeof bi);
    if (exists(ai) && !exists(bi)) {
        ok = Rename((STRPTR)ai, (STRPTR)bi);
        if (ok) add_entry("moved", ai, bi);
    }
    return 1;
}

/* The FONT chunk of one fp_Type in a Font.prefs file (its data), or NULL. */
static UBYTE *font_of_type(UBYTE *d, LONG len, int type)
{
    LONG k;
    for (k = 12; d && k + 8 <= len; ) {
        ULONG n = get32(d + k + 4);
        if (k + 8 + (LONG)n > len) break;
        if (!memcmp(d + k, "FONT", 4) && n >= FONT_CHUNK && d[k + 8 + 14] == 0 && d[k + 8 + 15] == type) return d + k + 8;
        k += 8 + n + (n & 1);
    }
    return NULL;
}

/* A picture's tone from its own pixels, through picture.datatype (V44: PDTM_READPIXELARRAY as ARGB):
 * twelve rows, every eighth pixel. 1 dark, 0 light, -1 couldn't read it. Remembered per picture in
 * ENVARC:GFOpenAmiga/Backdrop.tone, as decoding a big picture takes a moment on a 68k. */
#define TONE_CACHE "ENVARC:GFOpenAmiga/Backdrop.tone"
static int picture_tone(const char *name)
{
    struct Library *DataTypesBase;
    Object *o;
    struct BitMapHeader *bmh = NULL;
    ULONG *row;
    LONG len = 0, w, h, y, x, sum = 0, count = 0;
    char *cache, side[300];
    int tone = -1, nl = (int)strlen(name);
    /* a .tone file beside the picture ("dark" or "light"), as GFOpenUp's backdrops carry */
    strlcpy(side, name, sizeof side);
    str_cat(side, ".tone", sizeof side);
    if ((cache = read_file(side, &len))) {
        tone = len > 0 && (cache[0] == 'd' || cache[0] == 'D') ? 1 : len > 0 ? 0 : -1;
        FreeVec(cache);
        if (tone >= 0) return tone;
    }
    /* never a big picture at boot: decoding a full-screen PNG takes minutes on a 68k, and S:User-Startup
     * waited for it (the black screen of 10 October 2026, 18:15). Only GFOpenUp's strips (16 pixels wide). */
    if (!strstr(name, "-strip-")) return -1;
    cache = read_file(TONE_CACHE, &len);
    if (cache) {
        if (len > nl + 1 && !strncmp(cache, name, nl) && cache[nl] == '\n') tone = cache[nl + 1] == 'd' ? 1 : cache[nl + 1] == 'l' ? 0 : -1;
        FreeVec(cache);
        if (tone >= 0) return tone;
    }
    if (!(DataTypesBase = OpenLibrary((STRPTR)"datatypes.library", 44))) return -1;
    o = NewDTObject((APTR)name, DTA_GroupID, GID_PICTURE, PDTA_Remap, FALSE, PDTA_DestMode, PMODE_V43, TAG_DONE);
    if (!o) { CloseLibrary(DataTypesBase); return -1; }
    GetDTAttrs(o, PDTA_BitMapHeader, (ULONG)&bmh, TAG_DONE);
    w = bmh ? bmh->bmh_Width : 0;
    h = bmh ? bmh->bmh_Height : 0;
    if (w > 0 && h > 0 && w <= 8192 && (row = AllocVec(w * 4, MEMF_ANY))) {
        for (y = 0; y < 12; y++) {
            struct pdtBlitPixelArray m;
            m.MethodID = PDTM_READPIXELARRAY;
            m.pbpa_PixelData = row;
            m.pbpa_PixelFormat = PBPAFMT_ARGB;
            m.pbpa_PixelArrayMod = w * 4;
            m.pbpa_Left = 0;
            m.pbpa_Top = (h - 1) * y / 11;
            m.pbpa_Width = w;
            m.pbpa_Height = 1;
            /* DoMethodA, not DoDTMethodA: DoDTMethodA puts a GadgetInfo (NULL here) in the message's second
             * longword, which for this method is pbpa_PixelData, and the row went to address 0 (found on
             * Instance-32, 11 October 2026). */
            if (!DoMethodA(o, (Msg)&m)) continue;
            for (x = 0; x < w; x += 8) {
                ULONG p = row[x];
                sum += (2126L * ((p >> 16) & 255) + 7152L * ((p >> 8) & 255) + 722L * (p & 255)) / 10000;
                count++;
            }
        }
        FreeVec(row);
    }
    DisposeDTObject(o);
    CloseLibrary(DataTypesBase);
    if (!count) return -1;
    tone = sum / count < 115;                             /* 45 per cent of 255 */
    {
        char buf[300];
        strlcpy(buf, name, sizeof buf - 4);
        str_cat(buf, tone ? "\ndark\n" : "\nlight\n", sizeof buf);
        ensure_dir("ENVARC:GFOpenAmiga", 1);
        write_file(TONE_CACHE, buf, strlen(buf));
    }
    return tone;
}

/* The backdrop's tone: 1 dark, 0 light, -1 not known. GFOpenUp's backdrops are named <Theme>-<mode>[-chrome]-...
 * (gfopengadtools' make_backdrops.py); the theme's desktop colours for that mode give the tone: the mean of
 * their luminance below 45 per cent is dark. Another picture is dark when its name says so. */
static int backdrop_tone(void)
{
    LONG len = 0, k, tlen = 0;
    UBYTE *d = (UBYTE *)read_file("ENVARC:Sys/WBPattern.prefs", &len);
    char name[256], theme[48], mode[16], path[120], *t, *line;
    const char *base;
    int i, tone = -1;
    name[0] = 0;
    for (k = 12; d && k + 8 <= len; ) {
        ULONG n = get32(d + k + 4);
        if (k + 8 + (LONG)n > len) break;
        if (!memcmp(d + k, "PTRN", 4) && n >= 24 && d[k + 8 + 16] == 0 && d[k + 8 + 17] == 0 && !(d[k + 8 + 19] & 1)) {
            /* wbp_Which 0: the desktop; not WBPF_PATTERN: a picture's name follows */
            LONG dl = (d[k + 8 + 22] << 8) | d[k + 8 + 23];
            if (dl > 0 && dl < (LONG)sizeof name && 24 + dl <= (LONG)n) { memcpy(name, d + k + 8 + 24, dl); name[dl] = 0; }
        }
        k += 8 + n + (n & 1);
    }
    if (d) FreeVec(d);
    if (!name[0]) return -1;
    if ((tone = picture_tone(name)) >= 0) return tone;   /* the picture itself, best */
    base = strrchr(name, '/') ? strrchr(name, '/') + 1 : strchr(name, ':') ? strchr(name, ':') + 1 : name;
    for (i = 0; base[i] && base[i] != '-' && i < (int)sizeof theme - 1; i++) theme[i] = base[i];
    theme[i] = 0;
    mode[0] = 0;
    if (base[i] == '-') {
        const char *m = base + i + 1;
        for (i = 0; m[i] && m[i] != '-' && m[i] != '.' && i < (int)sizeof mode - 1; i++) mode[i] = m[i];
        mode[i] = 0;
    }
    if (strcmp(mode, "light") && strcmp(mode, "dark"))
        return strstr(name, "dark") || strstr(name, "Dark") ? 1 : -1;
    tone = !strcmp(mode, "dark");                   /* the mode's word, unless the theme's colours say more */
    strlcpy(path, "SYS:Prefs/Presets/Themes/", sizeof path);
    str_cat(path, theme, sizeof path);
    str_cat(path, ".theme", sizeof path);
    t = read_file(path, &tlen);
    if (t) {
        char want[20];
        int in = 0;
        strlcpy(want, "[", sizeof want); str_cat(want, mode, sizeof want); str_cat(want, "]", sizeof want);
        for (line = t; line && *line; line = strchr(line, '\n') ? strchr(line, '\n') + 1 : NULL) {
            if (*line == '[') { in = !strncmp(line, want, strlen(want)); continue; }
            if (in && !strncmp(line, "desktop", 7)) {
                long sum = 0, count = 0;
                char *p = line;
                while ((p = strchr(p, '#')) && (!strchr(line, '\n') || p < strchr(line, '\n'))) {
                    int c[3], j, ok = 1;
                    if (strlen(p) < 7) break;
                    for (j = 0; j < 3; j++) {
                        int hi = p[1 + 2 * j], lo = p[2 + 2 * j], v = 0, q;
                        for (q = 0; q < 2; q++) {
                            int ch = q ? lo : hi;
                            v <<= 4;
                            if (ch >= '0' && ch <= '9') v |= ch - '0';
                            else if ((ch | 32) >= 'a' && (ch | 32) <= 'f') v |= (ch | 32) - 'a' + 10;
                            else ok = 0;
                        }
                        c[j] = v;
                    }
                    if (ok) { sum += (2126L * c[0] + 7152L * c[1] + 722L * c[2]) / 10000; count++; }
                    p++;
                }
                if (count) tone = sum / count < 115;          /* 45 per cent of 255 */
                break;
            }
        }
        FreeVec(t);
    }
    return tone;
}

/* The icons' text's pens and effect in Font prefs (ENVARC:, and ENV: while Workbench isn't open yet, as at
 * boot before LoadWB, so this boot shows it). */
static int set_icon_pens(int front, int back, int special)
{
    LONG len = 0;
    UBYTE *d = (UBYTE *)read_file("ENVARC:Sys/Font.prefs", &len), *wb;
    int ok;
    struct MsgPort *wbport;
    if (!d) return 0;
    if (!(wb = font_of_type(d, len, 0))) { FreeVec(d); return 0; }
    wb[16] = (UBYTE)front; wb[17] = (UBYTE)back; wb[18] = 0; wb[19] = (UBYTE)special;
    ok = install_prefs("Font.prefs", d, len);
    Forbid();
    wbport = FindPort((STRPTR)"WORKBENCH");
    Permit();
    if (ok && !wbport) write_file("ENV:Sys/Font.prefs", (const char *)d, len);
    FreeVec(d);
    return ok;
}

/* The icons' text in the screens' text (Font prefs): 1 already, 0 changed or would change, -1 failed. */
static int reconcile_icon_text(int apply)
{
    LONG len = 0, size;
    UBYTE *d = (UBYTE *)read_file("ENVARC:Sys/Font.prefs", &len), *wb, *sc;
    char font[130];
    int same, rc;
    if (!d) { say("  icon text: no Font prefs saved yet; left to the OS", NULL, NULL); return 1; }
    wb = font_of_type(d, len, 0);
    sc = font_of_type(d, len, 2);
    if (!sc) { FreeVec(d); say("  icon text: no screen text set; left as it is", NULL, NULL); return 1; }
    same = wb && !memcmp(wb + 24, sc + 24, 2) && !strncmp((char *)wb + 28, (char *)sc + 28, 128) && wb[18] != 1;
    strlcpy(font, (char *)sc + 28, sizeof font);
    size = (sc[24] << 8) | sc[25];
    FreeVec(d);
    if (!same) {
        if (!apply) Printf((STRPTR)"  icon text: would become %s %ld, as the screens' text\n", (LONG)font, size);
        else if ((rc = set_font(font, size)) != RETURN_OK) return -1;
    } else Printf((STRPTR)"  icon text: %s %ld, as the screens' text\n", (LONG)font, size);
    {
        int tone = backdrop_tone(), front, back, special, nf, nb, ns;
        UBYTE *d2, *w2;
        LONG len2 = 0;
        if (tone < 0) return same ? 1 : 0;
        d2 = (UBYTE *)read_file("ENVARC:Sys/Font.prefs", &len2);
        w2 = d2 ? font_of_type(d2, len2, 0) : NULL;
        if (!w2) { if (d2) FreeVec(d2); return same ? 1 : 0; }
        front = w2[16]; back = w2[17]; special = w2[19];
        FreeVec(d2);
        if (tone) { nf = 2; nb = 1; ns = special ? special : 1; }    /* dark backdrop: white text, a black shadow */
        else { nf = 1; nb = 2; ns = special; }                        /* light: black text, a white shadow if any */
        if (nf == front && nb == back && ns == special) {
            Printf((STRPTR)"  icon text: %s, for a %s backdrop\n", (LONG)(tone ? "light" : "dark"), (LONG)(tone ? "dark" : "light"));
            return same ? 1 : 0;
        }
        if (!apply) {
            Printf((STRPTR)"  icon text: would become %s, for a %s backdrop\n", (LONG)(tone ? "light with a shadow" : "dark"), (LONG)(tone ? "dark" : "light"));
            return 0;
        }
        if (!set_icon_pens(nf, nb, ns)) return -1;
        Printf((STRPTR)"  icon text: now %s, for a %s backdrop\n", (LONG)(tone ? "light with a shadow" : "dark"), (LONG)(tone ? "dark" : "light"));
        return 0;
    }
}

static int reconcile(const char *how, const char *once)
{
    int apply = how && same_word(how, "APPLY"), undo = how && same_word(how, "UNDO");
    int i, moved = 0, failed = 0, waiting = 0;
    char dest[ENTRY_PATH];
    if (how && !apply && !undo && !same_word(how, "CHECK")) {
        PutStr((STRPTR)"Usage: GFOpenUpTool RECONCILE [CHECK | APPLY [ONCE] | UNDO]\n");
        return RETURN_FAIL;
    }
    manifest_load();
    if (undo) {
        for (i = nents - 1; i >= 0; i--) {
            entry *e = &ents[i];
            LONG ok;
            if (!strcmp(e->kind, "rnew")) {             /* a replacement RECONCILE put in an old program's place */
                if (DeleteFile((STRPTR)e->a) || !exists(e->a)) e->kind[0] = 0;
                else { say("GFOpenUp: can't take away ", e->a, "."); failed++; }
                continue;
            }
            if (strcmp(e->kind, "moved")) continue;
            if (exists(e->a)) { say("GFOpenUp: ", e->a, " is back already; its moved copy stays where it is."); failed++; continue; }
            ok = Rename((STRPTR)e->b, (STRPTR)e->a);
            if (ok) { e->kind[0] = 0; moved++; }
            else { say("GFOpenUp: can't put back ", e->a, "."); failed++; }
        }
        DeleteFile((STRPTR)RECONCILE_DONE);
        if (!manifest_save()) return RETURN_ERROR;
        Printf((STRPTR)"GFOpenUp: %ld put back where they were\n", (LONG)moved);
        return failed ? RETURN_WARN : RETURN_OK;
    }
    if (apply && once && same_word(once, "ONCE")) {
        LONG len = 0;
        char *done = read_file(RECONCILE_DONE, &len);
        int same = done && len >= 1 && !strncmp(done, RECONCILE_MAP_VERSION, strlen(RECONCILE_MAP_VERSION)) &&
                   (len == (LONG)strlen(RECONCILE_MAP_VERSION) || done[strlen(RECONCILE_MAP_VERSION)] == '\n');
        if (done) FreeVec(done);
        if (same) return reconcile_icon_text(1) < 0 ? RETURN_WARN : RETURN_OK;   /* the backdrop may have changed */
    }
    Printf((STRPTR)"GFOpenUp: reconcile (map %s)%s\n", (LONG)RECONCILE_MAP_VERSION, (LONG)(apply ? "" : ": what it would do; APPLY does it"));
    for (i = 0; i < NRMAP; i++) {
        const rmap *m = &reconcile_map[i];
        const char *name = (const char *)FilePart((STRPTR)m->path);
        int there = exists(m->path), home = m->home && exists(m->home);
        if (!strcmp(m->action, "classic")) {
            strlcpy(dest, CLASSIC "/", sizeof dest); str_cat(dest, name, sizeof dest);
            if (!there) {
                if (apply) move_noted(m->path, dest);           /* notes one moved before (2), else nothing */
                continue;
            }
            if (!apply) { Printf((STRPTR)"  to Classic: %-22s %s\n", (LONG)m->path, (LONG)m->what); continue; }
            if (move_noted(m->path, dest) > 0) { Printf((STRPTR)"  to Classic: %s\n", (LONG)m->path); moved++; }
            else failed++;
        } else if (!strcmp(m->action, "retire")) {
            if (!there) { if (apply) { backup_path(m->path, dest, sizeof dest); move_noted(m->path, dest); } continue; }
            if (!home) { Printf((STRPTR)"  waits:      %-22s for %s\n", (LONG)m->path, (LONG)m->what); waiting++; continue; }
            backup_path(m->path, dest, sizeof dest);
            if (!apply) { Printf((STRPTR)"  retire:     %-22s %s replaces it\n", (LONG)m->path, (LONG)m->what); continue; }
            if (move_noted(m->path, dest) > 0) { Printf((STRPTR)"  retired:    %s\n", (LONG)m->path); moved++; }
            else failed++;
        } else if (!strcmp(m->action, "replace")) {
            if (!there) continue;
            if (!home) { Printf((STRPTR)"  waits:      %-22s for %s\n", (LONG)m->path, (LONG)m->what); waiting++; continue; }
            backup_path(m->path, dest, sizeof dest);
            if (exists(dest) && !find_entry("rnew", m->path)) {
                /* replaced by an earlier run that stopped before it saved its list: noted now */
                if (apply) { add_entry("moved", m->path, dest); add_entry("rnew", m->path, NULL); }
                continue;
            }
            if (find_entry("rnew", m->path)) continue;          /* replaced already */
            if (!apply) { Printf((STRPTR)"  replace:    %-22s by %s\n", (LONG)m->path, (LONG)m->what); continue; }
            if (move_noted(m->path, dest) > 0) {
                if (copy_file(m->home, m->path, 0)) {
                    add_entry("rnew", m->path, NULL);
                    Printf((STRPTR)"  replaced:   %s by %s\n", (LONG)m->path, (LONG)m->home);
                    moved++;
                } else { say("GFOpenUp: can't put ", m->home, " in its place; RECONCILE UNDO puts the old one back."); failed++; }
            } else failed++;
        } else if (!strcmp(m->action, "wait")) {
            if (!there) continue;
            Printf((STRPTR)"  waits:      %-22s for %s%s\n", (LONG)m->path, (LONG)m->what, (LONG)(home ? " (on the disk)" : ""));
            waiting++;
        } else if (there) {
            Printf((STRPTR)"  keeps:      %-22s %s\n", (LONG)m->path, (LONG)m->what);
        }
    }
    if (reconcile_icon_text(apply) < 0) failed++;
    /* the Open apps on the dock (the user, 10 October 2026): Dock ADD adds only those installed and not there. The
     * editor is GFOpenPrefs's Dock page: SYS:Preferences/GFOpenPrefs DOCK since GFOpenUpTool 0.8.9 (the no-ROM-volume
     * design, section 8), SYS:Prefs/OpenDock, its name's stub, before */
    const char *dock_ed = exists("SYS:Preferences/GFOpenPrefs") ? "SYS:Preferences/GFOpenPrefs DOCK"
                          : exists("SYS:Prefs/OpenDock") ? "SYS:Prefs/OpenDock" : NULL;
    if (dock_ed) {
        if (!apply) PutStr((STRPTR)"  dock: GFOpenWrite, GFOpenPlay, GFOpenCompress, GFOpenFTP and GFOpenPuTTY added if installed and missing\n");
        else {
            char cmd[400];
            LONG r;
            strlcpy(cmd, dock_ed, sizeof cmd);
            str_cat(cmd, " ADD SYS:Utilities/GFOpenFiles SYS:Utilities/GFOpenPrint/GFOpenView "
                         "SYS:Utilities/GFOpenWrite/GFOpenWrite SYS:Utilities/GFOpenPlay SYS:Utilities/GFOpenCompress "
                         "SYS:Utilities/GFOpenFTP/GFOpenFTP SYS:Utilities/GFOpenPuTTY/GFOpenPuTTY SAVE", sizeof cmd);
            r = run_sync(cmd, 1);
            PutStr((STRPTR)(r == 0 ? "  dock: the Open apps are on it\n" : "  dock: Dock ADD didn't run; the dock is as it was\n"));
        }
    }
    if (apply) {
        if (!manifest_save()) return RETURN_ERROR;
        if (!failed) write_file(RECONCILE_DONE, RECONCILE_MAP_VERSION "\n", strlen(RECONCILE_MAP_VERSION) + 1);
        Printf((STRPTR)"GFOpenUp: %ld moved, %ld waiting for their Open home%s\n", (LONG)moved, (LONG)waiting,
               (LONG)(moved ? ". Choose Update in the drawers to see it." : ""));
    }
    return failed ? RETURN_WARN : RETURN_OK;
}

/* ---- TIDY: drawers that open looking finished --------------------------------------------- */

/* The user, 10 October 2026: "the windows look pants when opening because we haven't sized them and made them
 * pretty". TIDY has Workbench itself clean up each of the system's drawers (by name), size the window to fit
 * its icons (never larger than 800 x 600 or the screen), and snapshot it all, so every drawer opens neat, in
 * the screen's own font. Through Workbench's ARexx port (WORKBENCH), as GFOpenMenus talks to it; nothing of
 * Workbench's is patched.
 *   GFOpenUpTool TIDY [ONCE]    ONCE (at boot): waits for Workbench, and does it once per tidy version */
#define TIDY_VERSION "1"
#define TIDY_DONE "ENVARC:GFOpenAmiga/Tidy.done"
struct RxsLib *RexxSysBase;

/* One command to Workbench; 0 is RC_OK. Waits up to ticks fiftieths of a second for the answer. */
static LONG wb_cmd(const char *cmd, char *out, int size, int ticks)
{
    struct MsgPort *wb, *rp;
    struct RexxMsg *rm;
    LONG rc = 20;
    if (out && size) out[0] = 0;
    if (!RexxSysBase && !(RexxSysBase = (struct RxsLib *)OpenLibrary((STRPTR)"rexxsyslib.library", 36))) return 20;
    if (!(rp = CreateMsgPort())) return 20;
    if ((rm = CreateRexxMsg(rp, NULL, NULL))) {
        rm->rm_Action = RXCOMM | RXFF_RESULT;
        if ((rm->rm_Args[0] = (STRPTR)CreateArgstring((STRPTR)cmd, strlen(cmd)))) {
            Forbid();
            if ((wb = FindPort((STRPTR)"WORKBENCH"))) PutMsg(wb, (struct Message *)rm);
            Permit();
            if (wb) {
                int t;
                struct Message *got = NULL;
                for (t = 0; t < ticks && !(got = GetMsg(rp)); t++) Delay(1);
                if (!got) return 20;                 /* no answer: the message and port stay, for a late reply */
                rc = rm->rm_Result1;
                if (!rc && rm->rm_Result2) {
                    if (out) strlcpy(out, (const char *)rm->rm_Result2, size);
                    DeleteArgstring((UBYTE *)rm->rm_Result2);
                }
            }
            DeleteArgstring((UBYTE *)rm->rm_Args[0]);
        }
        DeleteRexxMsg(rm);
    }
    DeleteMsgPort(rp);
    return rc;
}

/* "verb "name" rest" into buf */
static const char *wb_line(char *buf, int size, const char *verb, const char *name, const char *rest)
{
    strlcpy(buf, verb, size);
    if (name) { str_cat(buf, " \"", size); str_cat(buf, name, size); str_cat(buf, "\"", size); }
    if (rest) { str_cat(buf, " ", size); str_cat(buf, rest, size); }
    return buf;
}

static LONG wb_num(const char *attr, const char *name)
{
    char cmd[300], out[32];
    strlcpy(cmd, "GETATTR ", sizeof cmd);
    str_cat(cmd, attr, sizeof cmd);
    str_cat(cmd, " NAME \"", sizeof cmd);
    str_cat(cmd, name, sizeof cmd);
    str_cat(cmd, "\"", sizeof cmd);
    if (wb_cmd(cmd, out, sizeof out, 250)) return -1;
    return atol(out);
}

/* The drawer's name as Workbench knows its windows: the volume's name, not SYS:. */
static int wb_name(const char *path, char *out, int size)
{
    BPTR l = Lock((STRPTR)path, ACCESS_READ);
    LONG ok;
    if (!l) return 0;
    ok = NameFromLock(l, (STRPTR)out, size);
    UnLock(l);
    return ok != 0;
}

static int tidy(const char *once)
{
    static const char *const drawers[] = {
        "SYS:", "SYS:Prefs", "SYS:Prefs/Classic", "SYS:Prefs/Presets", "SYS:Tools", "SYS:Tools/Commodities",
        "SYS:Utilities", "SYS:System", "SYS:WBStartup", "SYS:Storage", "SYS:Developer", "SYS:Expansion", "Work:",
    };
    char name[256], cmd[400], num[16];
    LONG scrw = 640, scrh = 480, w, h;
    int i, done = 0, t;
    if (once && same_word(once, "ONCE")) {
        LONG len = 0;
        char *d = read_file(TIDY_DONE, &len);
        int same = d && len >= 1 && d[0] == TIDY_VERSION[0];
        if (d) FreeVec(d);
        if (same) return RETURN_OK;
        for (t = 0; t < 90; t++) {                   /* at boot: Workbench comes after S:User-Startup */
            struct MsgPort *wb;
            Forbid();
            wb = FindPort((STRPTR)"WORKBENCH");
            Permit();
            if (wb) break;
            Delay(50);
        }
        Delay(250);                                  /* let Workbench open its windows and read its icons */
    }
    if (wb_cmd("GETATTR SCREEN.WIDTH", num, sizeof num, 250) == 0) scrw = atol(num);
    if (wb_cmd("GETATTR SCREEN.HEIGHT", num, sizeof num, 250) == 0) scrh = atol(num);
    if (scrw <= 0 || scrh <= 0) { say("GFOpenUp: Workbench isn't answering; nothing tidied.", NULL, NULL); return RETURN_WARN; }
    for (i = 0; i < (int)(sizeof drawers / sizeof drawers[0]); i++) {
        if (!exists(drawers[i]) || !wb_name(drawers[i], name, sizeof name)) continue;
        if (wb_cmd(wb_line(cmd, sizeof cmd, "WINDOW", name, "OPEN"), NULL, 0, 500)) continue;
        wb_cmd(wb_line(cmd, sizeof cmd, "WINDOW", name, "ACTIVATE"), NULL, 0, 250);
        wb_cmd("MENU INVOKE WINDOW.CLEANUPBY.NAME", NULL, 0, 500);
        wb_cmd("MENU INVOKE WINDOW.RESIZETOFIT", NULL, 0, 500);
        w = wb_num("WINDOW.WIDTH", name);
        h = wb_num("WINDOW.HEIGHT", name);
        if (w > 0 && h > 0) {
            LONG mw = scrw < 800 ? scrw : 800, mh = scrh - 40 < 600 ? scrh - 40 : 600;
            if (w > mw || h > mh) {
                char size[32];
                LONG nw = w > mw ? mw : w, nh = h > mh ? mh : h;
                size[0] = 0;
                {   /* "w h" without stdio */
                    char a[12], b[12];
                    int k = 0, j;
                    LONG v = nw;
                    do { a[k++] = (char)('0' + v % 10); v /= 10; } while (v && k < 11);
                    for (j = 0; j < k; j++) size[j] = a[k - 1 - j];
                    size[k] = ' ';
                    v = nh; j = 0;
                    do { b[j++] = (char)('0' + v % 10); v /= 10; } while (v && j < 11);
                    for (t = 0; t < j; t++) size[k + 1 + t] = b[j - 1 - t];
                    size[k + 1 + j] = 0;
                }
                wb_cmd(wb_line(cmd, sizeof cmd, "SIZEWINDOW", name, size), NULL, 0, 250);
            }
        }
        wb_cmd("MENU INVOKE WINDOW.SNAPSHOT.ALL", NULL, 0, 500);
        if (strcmp(drawers[i], "SYS:") && strcmp(drawers[i], "Work:"))
            wb_cmd(wb_line(cmd, sizeof cmd, "WINDOW", name, "CLOSE"), NULL, 0, 250);
        Printf((STRPTR)"  tidied: %s\n", (LONG)name);
        done++;
    }
    if (RexxSysBase) { CloseLibrary((struct Library *)RexxSysBase); RexxSysBase = NULL; }
    if (!done) { say("GFOpenUp: Workbench didn't open any drawer for TIDY; nothing tidied, it tries again next time.", NULL, NULL); return RETURN_WARN; }
    ensure_dir("ENVARC:GFOpenAmiga", 1);
    write_file(TIDY_DONE, TIDY_VERSION "\n", strlen(TIDY_VERSION) + 1);
    Printf((STRPTR)"GFOpenUp: %ld drawers tidied: cleaned up by name, sized to fit, snapshotted\n", (LONG)done);
    return RETURN_OK;
}

/* ---- ICONS: every icon in the house style ------------------------------------------------------ */

/* The user, 10 October 2026: an icon find and replace that hunts for icons that aren't AmiCradle's and
 * offers ours, with the drawers tidied after. ICONS walks SYS: and Work: (three drawers deep, GFOpenUp's own
 * Storage and Parts left out) for old icons: the classic four-colour pictures and NewIcons, anything that
 * isn't an OS 3.5 colour icon or a PNG icon and isn't one GFOpenUp gave already. Each gets a new face, the
 * icon keeping its type, tool, tool types, stack, place and drawer window (face_icon, as the install does):
 *   - by its name, from the GFOpenIcons set GFOpenUp carries (Work:GFOpenUp/Parts/GFOpenIcons/Icons/<name>.info);
 *   - else by its kind, from GFOpenIcons' disk, drawer, tool, project and trashcan (Parts/GFOpenIcons/Kinds), the
 *     pictures the ROM's own default icons have; with no GFOpenIcons part, from the system's default icons
 *     where there are any (an icon with neither keeps its picture).
 * The originals go to SYS:Storage/GFOpenUp-Superseded; UNINSTALL puts them back.
 *   GFOpenUpTool ICONS [CHECK]    lists what it would change
 *   GFOpenUpTool ICONS APPLY      changes them, then has Workbench tidy the system's drawers (TIDY) */
#define ICON_LIB "Work:GFOpenUp/Parts/GFOpenIcons/Icons"
#define ICON_KINDS "Work:GFOpenUp/Parts/GFOpenIcons/Kinds"
#define ICONS_MAX 400
static char *icon_found[ICONS_MAX], *icon_from[ICONS_MAX];
static int icon_count;

/* 1 when the icon file is an old one: no FORM ICON (OS 3.5) and no PNG. */
static int old_icon(const char *path, int *type)
{
    LONG len = 0, k;
    UBYTE *d = (UBYTE *)read_file(path, &len);
    int old = 0;
    *type = 0;
    if (!d) return 0;
    if (len > 78 && d[0] == 0xE3 && d[1] == 0x10) {             /* a DiskObject */
        old = 1;
        *type = d[48];
        for (k = 78; k + 12 <= len; k++)
            if (!memcmp(d + k, "FORM", 4) && !memcmp(d + k + 8, "ICON", 4)) { old = 0; break; }
    }
    if (len > 8 && !memcmp(d, "\x89PNG", 4)) old = 0;
    FreeVec(d);
    return old;
}

/* the picture for an icon of this kind: GFOpenIcons' (the part's Kinds drawer, the ROM's look), else the
 * system's default icon where there is one */
static const char *kind_default(int type)
{
    static const char *const ours[] = { NULL, ICON_KINDS "/Disk.info", ICON_KINDS "/Drawer.info", ICON_KINDS "/Tool.info",
                                        ICON_KINDS "/Project.info", ICON_KINDS "/Trashcan.info", NULL, NULL };
    if (type >= WBDISK && type <= WBGARBAGE && exists(ours[type])) return ours[type];
    switch (type) {
    case WBDISK: return "ENVARC:Sys/def_Disk.info";
    case WBDRAWER: return "ENVARC:Sys/def_Drawer.info";
    case WBTOOL: return "ENVARC:Sys/def_Tool.info";
    case WBPROJECT: return "ENVARC:Sys/def_Project.info";
    case WBGARBAGE: return "ENVARC:Sys/def_Trashcan.info";
    case WBKICK: return "ENVARC:Sys/def_Kick.info";
    }
    return NULL;
}

static char *dup_text(const char *s)
{
    char *d = malloc(strlen(s) + 1);
    if (d) strcpy(d, s);
    return d;
}

static void icons_walk(const char *dir, int depth)
{
    BPTR lock = Lock((STRPTR)dir, ACCESS_READ);
    struct FileInfoBlock *fib;
    char sub[256], lib[256];
    if (!lock) return;
    if ((fib = AllocDosObject(DOS_FIB, NULL))) {
        if (Examine(lock, fib)) {
            while (icon_count < ICONS_MAX && ExNext(lock, fib)) {
                const char *n = (const char *)fib->fib_FileName;
                int l = strlen(n), type = 0;
                strlcpy(sub, dir, sizeof sub);
                if (sub[strlen(sub) - 1] != ':') str_cat(sub, "/", sizeof sub);
                str_cat(sub, n, sizeof sub);
                if (fib->fib_DirEntryType > 0) {
                    if (depth < 3 && strcmp(n, "GFOpenUp-Superseded") && strcmp(n, "Parts") && strcmp(n, "T") &&
                        strcmp(n, "Trashcan") && strncmp(n, ".", 1))
                        icons_walk(sub, depth + 1);
                    continue;
                }
                if (l <= 5 || !same_word(n + l - 5, ".info") || find_entry(NULL, sub) || !old_icon(sub, &type)) continue;
                /* the replacement: by name from GFOpenIcons' set, else by kind */
                strlcpy(lib, ICON_LIB "/", sizeof lib);
                str_cat(lib, n, sizeof lib);
                if (!exists(lib) && gf_renamed(n, l - 5)) {   /* 0.8.16: a program from before the GF rename */
                    strlcpy(lib, ICON_LIB "/GF", sizeof lib);
                    str_cat(lib, n, sizeof lib);
                }
                if (!exists(lib)) {
                    const char *k = kind_default(type);
                    if (!k || !exists(k)) continue;
                    strlcpy(lib, k, sizeof lib);
                }
                icon_found[icon_count] = dup_text(sub);
                icon_from[icon_count] = dup_text(lib);
                if (icon_found[icon_count] && icon_from[icon_count]) icon_count++;
            }
        }
        FreeDosObject(DOS_FIB, fib);
    }
    UnLock(lock);
}

static int icons(const char *how)
{
    int apply = how && same_word(how, "APPLY"), i, done = 0, failed = 0;
    if (how && !apply && !same_word(how, "CHECK")) { PutStr((STRPTR)"Usage: GFOpenUpTool ICONS [CHECK | APPLY]\n"); return RETURN_FAIL; }
    manifest_load();
    icon_count = 0;
    icons_walk("SYS:", 0);
    if (exists("Work:")) icons_walk("Work:", 0);
    if (!icon_count) { PutStr((STRPTR)"GFOpenUp: every icon is in the house style already\n"); return RETURN_OK; }
    for (i = 0; i < icon_count; i++) {
        if (!apply) { Printf((STRPTR)"  old: %s  (new face: %s)\n", (LONG)icon_found[i], (LONG)icon_from[i]); continue; }
        if (face_icon(icon_from[i], icon_found[i]) > 0) { Printf((STRPTR)"  new face: %s\n", (LONG)icon_found[i]); done++; }
        else failed++;
    }
    if (!apply) { Printf((STRPTR)"GFOpenUp: %ld old icons; ICONS APPLY gives them our faces\n", (LONG)icon_count); return RETURN_OK; }
    if (!manifest_save()) return RETURN_ERROR;
    Printf((STRPTR)"GFOpenUp: %ld icons given our faces, %ld left as they were\n", (LONG)done, (LONG)failed);
    if (done) {
        struct MsgPort *wb;
        Forbid();
        wb = FindPort((STRPTR)"WORKBENCH");
        Permit();
        if (wb) tidy(NULL);                            /* new pictures, new sizes: the drawers tidied again */
    }
    return failed ? RETURN_WARN : RETURN_OK;
}

static int list(void)
{
    int i, files = 0;
    manifest_load();
    if (!nents) { PutStr((STRPTR)"GFOpenUp isn't installed here.\n"); return RETURN_WARN; }
    for (i = 0; i < nents; i++)
        if (!strcmp(ents[i].kind, "part")) Printf((STRPTR)"%s %s\n", (LONG)ents[i].a, (LONG)ents[i].b);
        else if (!strcmp(ents[i].kind, "new") || !strcmp(ents[i].kind, "replaced")) files++;
    Printf((STRPTR)"%ld files are GFOpenUp's (" MANIFEST ")\n", files);
    return RETURN_OK;
}

/* ---- UPDATE: a newer pack from Cradle (amicradle docs/design/AutoDocs-GFOpenUp-Update.md) ---- */

#define CRADLE_DRAWER "DH1:GFOpenUp"
#define CRADLE_PACK "DH1:GFOpenUp/Cradle-Pack"
#define RELEASES "https://github.com/DalsinAI/gfopenup/releases"

/* Versions compare as numbers, dot by dot: 0.6.14 is newer than 0.6.9, and a release candidate is older than
 * its release (0.8.29-rc1 < 0.8.29-rc2 < 0.8.29): openup_vers.h's ouv_cmp, so UPDATE offers each day's candidate. */
static int version_cmp(const char *a, const char *b)
{
    return ouv_cmp(a, b);
}

/* "version 0.6.14" from Cradle-Pack (key value lines, ";" comments). */
static int cradle_version(char *out, int outlen)
{
    char *text = read_file(CRADLE_PACK, NULL), *p, *line;
    int found = 0;
    if (!text) return 0;
    for (p = text; *p && !found; ) {
        line = p;
        while (*p && *p != '\n') p++;
        if (*p) *p++ = 0;
        if (!strncmp(line, "version", 7) && (line[7] == ' ' || line[7] == '\t')) {
            char *v = line + 8;
            int n = 0;
            while (*v == ' ' || *v == '\t') v++;
            while (v[n] && v[n] != ' ' && v[n] != '\t' && v[n] != '\r' && n < outlen - 1) { out[n] = v[n]; n++; }
            out[n] = 0;
            found = n > 0;
        }
    }
    FreeVec(text);
    return found;
}

/* Each part in the new pack against the manifest: what an update changes. */
static int show_changes(void)
{
    struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
    BPTR dir = Lock((STRPTR)CRADLE_DRAWER "/Parts", ACCESS_READ);
    int changes = 0;
    if (!dir || !fib) { if (dir) UnLock(dir); if (fib) FreeDosObject(DOS_FIB, fib); return 0; }
    if (Examine(dir, fib)) {
        while (ExNext(dir, fib)) {
            char path[160], *text, *w[4];
            entry *e;
            if (fib->fib_DirEntryType <= 0) continue;
            strcpy(path, CRADLE_DRAWER "/Parts/");
            strncat(path, (const char *)fib->fib_FileName, sizeof path - strlen(path) - 6);
            strcat(path, "/Part");
            if (!(text = read_file(path, NULL))) continue;
            {
                char *nl = strchr(text, '\n');
                if (nl) *nl = 0;
            }
            if (words(text, w, 4) >= 3 && !strcmp(w[0], "part")) {
                e = find_entry("part", w[1]);
                if (!e) { Printf((STRPTR)"  %s %s: new (installed if you pick it)\n", (LONG)w[1], (LONG)w[2]); changes++; }
                else if (version_cmp(w[2], e->b) > 0) { Printf((STRPTR)"  %s: %s to %s\n", (LONG)w[1], (LONG)e->b, (LONG)w[2]); changes++; }
            }
            FreeVec(text);
        }
    }
    UnLock(dir);
    FreeDosObject(DOS_FIB, fib);
    return changes;
}

/* UPDATE AUTO's GFOSFonts (ou_auto_gfosfonts): installed once when it isn't, which moves the system font
 * (move_sysfont, from install()). The return code, or RETURN_OK when there was nothing to add. */
#define PACK_GFOSFONTS CRADLE_DRAWER "/Parts/GFOSFonts"
static int auto_gfosfonts(void)
{
    manifest_load();
    if (!ou_auto_gfosfonts(find_entry("part", "GFOSFonts") != NULL, exists(PACK_GFOSFONTS "/Part"))) return RETURN_OK;
    say("GFOpenUp: adding GFOSFonts (GFOS's own fonts; the system font moves to GFOS Mono 11)", NULL, NULL);
    return install(PACK_GFOSFONTS);
}

/* The unattended UPDATE's one line, in ENVARC:GFOpenUp/GFOpenUpTool.log and on the Shell; rc back. */
static int auto_kick;                               /* UPDATE AUTO's Kickstart set: 1 in, -1 failed, 0 not tried */

static int update_logged(int by, const char *installed, const char *newer, int done, int rc)
{
    char line[200];
    ou_log_line(line, sizeof line, auto_kick, by, installed, newer, done, rc);
    say(line, NULL, NULL);
    log_append(line);
    return rc;
}

/* UPDATE [CHECK|YES|AUTO]: CHECK only says (WARN when a newer pack is waiting);
 * plain UPDATE shows what will change and asks; YES doesn't ask (scripts) but
 * runs the Installer. AUTO (openup_unattended.h; only AUTO, so the menu's
 * plain UPDATE still asks on a card with the host flag) is unattended: no requester and no Installer,
 * the installed parts reinstalled from the pack (the settings in place stay,
 * no part added but GFOSFonts once, Dale 14 Nov: ou_auto_gfosfonts), the startup block written again, one line in the log.
 * It never runs on its own, and it never changes anything until asked. */
#ifndef KICKSET_DIR
#define KICKSET_DIR "SYS:Kickstart/default/"
#endif
#define PACK_KICK_MANIFEST CRADLE_DRAWER "/Parts/KickstartSet/Kickstart/default/Manifest"   /* build_pack's */

static int update(const char *how)
{
    char newer[24] = "", installed[24] = "";
    entry *e;
    int by, mode = ou_update_mode(how, &by);
    int check = mode == OU_CHECK, yes = mode == OU_INSTALLER, unattended = mode == OU_AUTO;
    manifest_load();
    if ((e = find_entry("part", "GFOpenUp")) != NULL)
        strlcpy(installed, e->b, sizeof installed);
    if (!installed[0]) {
        say("GFOpenUp isn't installed here, so there is nothing to update. Run Install-GFOpenUp to install it.", NULL, NULL);
        return unattended ? update_logged(by, installed, newer, -1, RETURN_WARN) : RETURN_WARN;
    }
    if (!cradle_version(newer, sizeof newer)) {
        say("GFOpenUp ", installed, " is installed. No newer GFOpenUp from Cradle: this Amiga isn't an AmiCradle instance with GFOpenUp's drawer (DH1:GFOpenUp).");
        say("On a real Amiga, newer packs are at " RELEASES ": unpack one and run its Install-GFOpenUp.", NULL, NULL);
        newer[0] = 0;
        return unattended ? update_logged(by, installed, newer, -1, RETURN_OK) : RETURN_OK;
    }
    if (version_cmp(newer, installed) <= 0) {
        int rc = RETURN_OK;
        auto_kick = 0;
        /* an older GFOpenUpTool did the update and left the Kickstart set out (ou_set_stale): the set now */
        if (unattended && ou_auto_kickset(slim_rom(), exists(CRADLE_DRAWER "/Parts/KickstartSet/Part"))) {
            LONG nh = 0, np = 0;
            char *have = read_file(KICKSET_DIR "Manifest", &nh), *pack = read_file(PACK_KICK_MANIFEST, &np);
            int stale = ou_set_stale(have, nh, pack, np);
            if (have) FreeVec(have);
            if (pack) FreeVec(pack);
            if (stale) {
                int r = kickset(CRADLE_DRAWER "/Parts/KickstartSet", NULL);
                auto_kick = r > RETURN_WARN ? -1 : 1;
                rc = r;
            }
        }
        if (unattended && !find_entry("part", "GFOSFonts") && exists(PACK_GFOSFONTS "/Part")) {
            int r = auto_gfosfonts();               /* an older GFOpenUpTool did the update, without GFOSFonts */
            if (r > rc) rc = r;
        } else if (!auto_kick)
            say("GFOpenUp ", installed, " is installed, and it is the newest Cradle has. Nothing to do.");
        return unattended ? update_logged(by, installed, newer, -1, rc) : rc;
    }
    Printf((STRPTR)"GFOpenUp %s is waiting in " CRADLE_DRAWER " (%s is installed).\n", (LONG)newer, (LONG)installed);
    if (check) return RETURN_WARN;
    if (!show_changes()) say("  (the same parts, updated in place)", NULL, NULL);
    if (unattended) {
        int rc = reinstall(CRADLE_DRAWER "/Parts");
        auto_kick = 0;
        if (rc < RETURN_ERROR) {
            int r = auto_gfosfonts();
            if (r > rc) rc = r;
        }
        if (rc < RETURN_ERROR && ou_auto_kickset(slim_rom(), exists(CRADLE_DRAWER "/Parts/KickstartSet/Part"))) {
            int r = kickset(CRADLE_DRAWER "/Parts/KickstartSet", NULL);
            auto_kick = r > RETURN_WARN ? -1 : 1;
            if (r > rc) rc = r;
        }
        if (rc < RETURN_ERROR && !write_startup(0)) {
            say("GFOpenUp: can't write S:User-Startup. Is SYS: write-protected?", NULL, NULL);
            rc = RETURN_ERROR;
        }
        return update_logged(by, installed, newer, reinstalled, rc);
    }
    if (!yes) {
        struct EasyStruct es = { sizeof(struct EasyStruct), 0, (UBYTE *)"GFOpenUp Update",
            (UBYTE *)"GFOpenUp %s is ready to install (%s is installed now).\n\n"
                     "GFOpenUp's Installer runs with your answers from last time;\n"
                     "what you have changed yourself stays.",
            (UBYTE *)"Not now|Update" };
        LONG answer = EasyRequest(NULL, &es, NULL, (LONG)newer, (LONG)installed);
        if (answer != 0) {           /* "Not now" is 1, the left; "Update" is 0, the right */
            say("Not updated. GFOpenUpTool UPDATE asks again whenever you like.", NULL, NULL);
            return RETURN_OK;
        }
    }
    {
        const char *inst = exists("C:Installer") ? "C:Installer" : exists("SYS:System/Installer") ? "SYS:System/Installer" : "SYS:Utilities/Installer";
        char cmd[200];
        LONG rc;
        strcpy(cmd, "CD " CRADLE_DRAWER "\n");
        strcat(cmd, inst);
        strcat(cmd, " Install-GFOpenUp APPNAME GFOpenUp NOLOG\n");       /* no install_log_file left in SYS: */
        if (!write_file("T:GFOpenUp-update", cmd, strlen(cmd))) {
            say("GFOpenUp: can't write T:GFOpenUp-update. Is T: full?", NULL, NULL);
            return RETURN_ERROR;
        }
        rc = run_sync("Execute T:GFOpenUp-update", 0);
        DeleteFile((STRPTR)"T:GFOpenUp-update");
        return rc == 0 ? RETURN_OK : RETURN_WARN;
    }
}

/* ---- an OpenUp from before the GF rename (GFOpenUp 0.8.13) -----------------------------------------------------
 * GFOpenUp 0.8.11 and earlier were OpenUp: their list is ENVARC:OpenAmiga/OpenUp.manifest, their programs and
 * settings the names without GF (C:OpenDock, ENVARC:OpenDock). The Installer takes that OpenUp out with its own
 * OpenUpTool UNINSTALL, which puts back what it replaced and leaves the settings in ENVARC:, so the two never start
 * side by side. Before that, OLDNAMES keeps a copy of its list and S:User-Startup, and copies each setting to the
 * GF name GFOpenUp's programs read, where that has none yet; OLDNAMES DOCK, after the parts are in, points the
 * dock's buttons at the GF programs that took the old ones' places. Nothing old is deleted here. */
#define OLD_MANIFEST "ENVARC:OpenAmiga/OpenUp.manifest"
#define OLD_KEPT "SYS:Storage/GFOpenUp-Superseded/OpenUp-before-GF"

/* the settings' names after ENV: and ENVARC: that the GF programs read as "GF" + the old name */
static const char *const old_settings[] = {
    "OpenApps", "OpenBench", "OpenBlanker", "OpenCalc", "OpenClock", "OpenDock", "OpenDrawer", "OpenEdit",
    "OpenFTP", "OpenFiles", "OpenFont", "OpenGadTools", "OpenImage", "OpenInput", "OpenIntuition", "OpenLocale",
    "OpenMail", "OpenMenus", "OpenMeters.place", "OpenMonitor", "OpenMulticore", "OpenPlay", "OpenPrefs",
    "OpenPrint", "OpenPuTTY", "OpenService", "OpenSnap", "OpenSocket", "OpenTLS", "OpenTitle", "OpenTypes",
    "OpenWeb", "OpenWindows", "OpenWrite", NULL
};
/* OpenUp's own marks in ENVARC:OpenAmiga that are the user's choices or say a step is done */
static const char *const old_marks[][2] = {
    { "Setup.done", "Setup.done" }, { "Startup.off", "Startup.off" }, { "Backdrop.tone", "Backdrop.tone" },
    { "Wazp3D-taken", "Wazp3D-taken" }, { "DefaultTools-repaired", "DefaultTools-repaired" },
    { "OpenFiles-place-reset", "GFOpenFiles-place-reset" }, { "Sound", "Sound" }, { "PrefsView", "PrefsView" },
    { NULL, NULL }
};

/* 1 when a name of a program or drawer took "GF" in front at the rename: Open..., but not the ones that kept
 * their names (OpenBrowser, the WebKit one, and OpenAmigaPython). OpenBrowserLite is GFOpenBrowserLite. */
static int gf_renamed(const char *p, size_t n)
{
    if (n < 5 || strncmp(p, "Open", 4) || !(p[4] >= 'A' && p[4] <= 'Z')) return 0;
    if (n == 11 && !strncmp(p, "OpenBrowser", 11)) return 0;
    if (n >= 15 && !strncmp(p, "OpenAmigaPython", 15)) return 0;
    return 1;
}

/* path with "GF" before each part of it that took it at the rename (SYS:Utilities/OpenPrint/OpenView ->
 * SYS:Utilities/GFOpenPrint/GFOpenView) into out; 0 when nothing changed or it doesn't fit. */
static int gf_path(const char *path, char *out, size_t size)
{
    const char *p = path;
    size_t o = 0;
    int changed = 0;
    while (*p) {
        const char *e = p;
        while (*e && *e != '/' && *e != ':') e++;
        if (gf_renamed(p, e - p)) {
            if (o + 2 >= size) return 0;
            out[o++] = 'G'; out[o++] = 'F';
            changed = 1;
        }
        while (p < e) { if (o + 1 >= size) return 0; out[o++] = *p++; }
        if (*p) { if (o + 1 >= size) return 0; out[o++] = *p++; }
    }
    out[o] = 0;
    return changed;
}

/* Copy from to, all of it (a drawer with what is in it), as the Shell's Copy does; 1 when it worked. */
static int copy_all(const char *from, const char *to)
{
    char cmd[600];
    strlcpy(cmd, "C:Copy >NIL: \"", sizeof cmd);
    str_cat(cmd, from, sizeof cmd); str_cat(cmd, "\" \"", sizeof cmd); str_cat(cmd, to, sizeof cmd);
    str_cat(cmd, is_file(from) ? "\" CLONE QUIET" : "\" ALL CLONE QUIET", sizeof cmd);
    return run_sync(cmd, 1) == 0;
}

/* Startup.off's lines with the GF names (C:OpenDock -> C:GFOpenDock), as GFOpenUp's startup lines have them */
static char *gf_lines(const char *text)
{
    size_t n = strlen(text);
    char *out = AllocVec(n * 3 + 16, MEMF_ANY), *o = out;
    const char *p = text;
    if (!out) return NULL;
    while (*p) {
        const char *e = p;
        char word[260], gf[300];
        while (*e && *e != ' ' && *e != '\t' && *e != '\n' && *e != '"') e++;
        if (e > p && (size_t)(e - p) < sizeof word) {
            memcpy(word, p, e - p); word[e - p] = 0;
            if (strchr(word, ':') && gf_path(word, gf, sizeof gf)) { strcpy(o, gf); o += strlen(gf); }
            else { memcpy(o, p, e - p); o += e - p; }
        } else { memcpy(o, p, e - p); o += e - p; }
        if (*e) *o++ = *e++;
        p = e;
    }
    *o = 0;
    return out;
}

/* 1 when GFOpenUp's own list names path (any kind but a blank one) */
static int gf_listed(const char *path)
{
    int i;
    for (i = 0; i < nents; i++)
        if (ents[i].kind[0] && same_word(ents[i].a, path)) return 1;
    return 0;
}

/* 9 Nov 2026: the old list without its "new" lines for files GFOpenUp's list names (src/openup_lines.h,
 * oul_old_line_taken), so the old OpenUpTool UNINSTALL the Installer runs next leaves GFOpenUp's files alone. Only
 * with the copy of the old list kept (OLD_KEPT), and only from a whole GFOpenUp list. 1 done (or nothing to do). */
static int old_list_prune(void)
{
    LONG len = 0;
    char *text, *out, *o, *p;
    int taken = 0, ok = 1;
    manifest_load();
    if (manifest_short) {
        say("GFOpenUp: its own list (" MANIFEST ") can't be read whole, so the old OpenUp can't be told which files are GFOpenUp's now.", NULL, NULL);
        return 0;
    }
    if (!nents || !(text = read_file(OLD_MANIFEST, &len)))
        return 1;
    if (!(out = AllocVec(len + 1, MEMF_ANY))) { FreeVec(text); return 0; }
    for (o = out, p = text; *p; ) {
        char *e = p;
        char c;
        while (*e && *e != '\n') e++;
        c = *e; *e = 0;
        if (oul_old_line_taken(p, gf_listed)) taken++;
        else { strcpy(o, p); o += strlen(p); if (c) *o++ = '\n'; }
        *e = c;
        p = c ? e + 1 : e;
    }
    *o = 0;
    if (taken) {
        ok = write_file(OLD_MANIFEST, out, o - out);
        Printf((STRPTR)(ok ? "GFOpenUp: OpenUp's list no longer names the %ld files GFOpenUp has put in their places since, so "
                             "OpenUp's UNINSTALL leaves them.\n"
                           : "GFOpenUp: couldn't take the %ld files GFOpenUp has put in their places since out of OpenUp's list, "
                             "so OpenUp wasn't taken out.\n"), (LONG)taken);
    }
    FreeVec(out);
    FreeVec(text);
    return ok;
}

static int old_settings_copy(void)
{
    static const char *const vols[2] = { "ENVARC:", "ENV:" };
    char from[200], to[200], ver[32] = "";
    int v, i, copied = 0, failed = 0;
    LONG len;
    char *m;
    if (!exists(OLD_MANIFEST)) {
        say("GFOpenUp: there is no OpenUp from before the GF names here (no " OLD_MANIFEST ").", NULL, NULL);
        return RETURN_WARN;
    }
    if ((m = read_file(OLD_MANIFEST, &len))) {         /* its version, from "part<tab>OpenUp<tab>0.8.11" */
        char *p = strstr(m, "\npart\tOpenUp\t");
        if (p) { p += 13; for (i = 0; i < 31 && p[i] && p[i] != '\n' && p[i] != '\r'; i++) ver[i] = p[i]; ver[i] = 0; }
        FreeVec(m);
    }
    say("GFOpenUp: OpenUp ", ver[0] ? ver : "(its version unknown)", " is here, from before the GF names.");
    /* a copy of its list, its own marks and S:User-Startup, beside what GFOpenUp keeps */
    if (!exists(OLD_KEPT "/OpenAmiga")) {
        ensure_dir(OLD_KEPT, 0);
        if (copy_all("ENVARC:OpenAmiga", OLD_KEPT "/OpenAmiga") && copy_all(USER_STARTUP, OLD_KEPT "/User-Startup"))
            say("GFOpenUp: kept a copy of OpenUp's list (ENVARC:OpenAmiga) and S:User-Startup in ", OLD_KEPT, NULL);
        else {
            say("GFOpenUp: couldn't keep a copy of OpenUp's list in ", OLD_KEPT, ". Is SYS: full? Nothing was changed.");
            return RETURN_ERROR;
        }
    }
    /* GFOpenUp's files where the old OpenUp's were: out of the old list, before the old UNINSTALL runs */
    if (!old_list_prune())
        return RETURN_ERROR;                         /* the Installer stops: the old UNINSTALL would delete them */
    /* 0.8.14: the backdrop the user has now, for BACKDROP KEEP after the old OpenUp's UNINSTALL puts back the
     * WBPattern.prefs from before it (lab t5-up0814-32, a copy of Instance-32) */
    if (exists(WBPATTERN)) copy_all(WBPATTERN, WBPATTERN_BEFORE);
    for (v = 0; v < 2; v++)
        for (i = 0; old_settings[i]; i++) {
            strlcpy(from, vols[v], sizeof from); str_cat(from, old_settings[i], sizeof from);
            strlcpy(to, vols[v], sizeof to); str_cat(to, "GF", sizeof to); str_cat(to, old_settings[i], sizeof to);
            if (!exists(from) || exists(to)) continue;
            if (copy_all(from, to)) { copied++; say("GFOpenUp: settings ", from, v ? "" : " copied to their GF name"); }
            else { failed++; say("GFOpenUp: couldn't copy ", from, " to its GF name"); }
        }
    ensure_dir("ENVARC:GFOpenAmiga", 0);
    for (i = 0; old_marks[i][0]; i++) {
        strlcpy(from, "ENVARC:OpenAmiga/", sizeof from); str_cat(from, old_marks[i][0], sizeof from);
        strlcpy(to, "ENVARC:GFOpenAmiga/", sizeof to); str_cat(to, old_marks[i][1], sizeof to);
        if (!exists(from) || exists(to)) continue;
        if (!strcmp(old_marks[i][0], "Startup.off")) {      /* its lines name the programs: their GF names */
            char *t = read_file(from, &len), *g = t ? gf_lines(t) : NULL;
            if (g && write_file(to, g, strlen(g))) { copied++; say("GFOpenUp: ", from, " copied, with the GF names"); }
            else { failed++; say("GFOpenUp: couldn't copy ", from, NULL); }
            if (t) FreeVec(t);
            if (g) FreeVec(g);
        } else if (copy_all(from, to)) { copied++; say("GFOpenUp: ", from, " copied"); }
        else { failed++; say("GFOpenUp: couldn't copy ", from, NULL); }
    }
    if (!copied && !failed) say("GFOpenUp: OpenUp's settings have their GF names already.", NULL, NULL);
    return failed ? RETURN_WARN : RETURN_OK;
}

/* The dock's buttons that start a program by its old name, where that is gone and the GF one is there: the GF one,
 * and its label too when the label was the old name. ENVARC: and ENV:. */
/* 0.8.16: an old program with no GF program of its own name any more: GFOpenLook (SYS:Prefs/OpenLook, SYS:Prefs/
 * GFOpenLook; gone since GFOpenUp 0.8.2) is GFOpenPrefs' Theme page, SYS:Preferences/Theme (its project icon opens the
 * page). Instance-32's dock had "SYS:Prefs/OpenLook" (label "Look"), which showed the plain tool picture. 1 mapped. */
static int old_dock_moved(const char *path, char *out, size_t size)
{
    static const char *const map[][3] = { { "SYS:Prefs/OpenLook", "SYS:Preferences/Theme", "SYS:Preferences/Theme.info" },
                                          { "SYS:Prefs/GFOpenLook", "SYS:Preferences/Theme", "SYS:Preferences/Theme.info" },
                                          { NULL, NULL, NULL } };
    int i;
    for (i = 0; map[i][0]; i++)                     /* the page's icon is a project with no file of its own */
        if (same_word(path, map[i][0]) && exists(map[i][2]) && strlen(map[i][1]) < size) {
            strcpy(out, map[i][1]);
            return 1;
        }
    return 0;
}

static int old_dock(void)
{
    static const char *const files[2] = { "ENVARC:GFOpenDock/Dock", "ENV:GFOpenDock/Dock" };
    int f, fixed = 0;
    for (f = 0; f < 2; f++) {
        LONG len;
        char *text = read_file(files[f], &len), *out, *o;
        const char *p;
        int here = 0;
        if (!text) continue;
        if (!(out = AllocVec(len * 2 + 64, MEMF_ANY))) { FreeVec(text); return RETURN_ERROR; }
        o = out;
        for (p = text; *p; ) {
            const char *e = p, *q1, *q2;
            while (*e && *e != '\n') e++;
            /* button <kind> "<path>" [label "<label>"] */
            if (!strncmp(p, "button ", 7) && (q1 = memchr(p, '"', e - p)) && (q2 = memchr(q1 + 1, '"', e - q1 - 1))) {
                char path[260], gf[300];
                size_t n = q2 - q1 - 1;
                if (n < sizeof path) {
                    memcpy(path, q1 + 1, n); path[n] = 0;
                    if (!exists(path) && ((gf_path(path, gf, sizeof gf) && exists(gf)) || old_dock_moved(path, gf, sizeof gf))) {
                        const char *r = q2 + 1, *l1, *l2;
                        memcpy(o, p, q1 + 1 - p); o += q1 + 1 - p;
                        strcpy(o, gf); o += strlen(gf);
                        *o++ = '"';                         /* the path's closing quote, q2 */
                        /* the rest of the line, the label with GF where it was the old program's name */
                        if ((l1 = memchr(r, '"', e - r)) && (l2 = memchr(l1 + 1, '"', e - l1 - 1)) &&
                            gf_renamed(l1 + 1, l2 - l1 - 1) && memchr(l1 + 1, ' ', l2 - l1 - 1) == NULL) {
                            memcpy(o, r, l1 + 1 - r); o += l1 + 1 - r;
                            *o++ = 'G'; *o++ = 'F';
                            r = l1 + 1;
                        }
                        memcpy(o, r, e - r); o += e - r;
                        if (*e) *o++ = '\n';
                        say("GFOpenUp: the dock's button for ", path, " starts its GF program now");
                        here++;
                        p = *e ? e + 1 : e;
                        continue;
                    }
                }
            }
            memcpy(o, p, e - p); o += e - p;
            if (*e) *o++ = '\n';
            p = *e ? e + 1 : e;
        }
        *o = 0;
        if (here && !write_file(files[f], out, o - out)) {
            say("GFOpenUp: couldn't write ", files[f], NULL);
            FreeVec(out); FreeVec(text);
            return RETURN_ERROR;
        }
        fixed += here;
        FreeVec(out);
        FreeVec(text);
    }
    if (!fixed) say("GFOpenUp: no button of the dock names a program by its old name.", NULL, NULL);
    return RETURN_OK;
}

/* ---- KICKSET: the slim ROM's module set (9 November 2026) ---------------------------------------------------
 *
 * The slim ROM (amicradle-guest's thin profile, docs/design/thin-loader.md) keeps only what a machine needs to
 * reach its disk; the rest (workbench.library, icon.library, GFOpenRTG, audio, the maths libraries, ...) is a
 * module set on SYS:Kickstart/default/, which the ROM's kickload loads at each power-on, and the files the full
 * ROM held on demand (asl, locale, version, datatypes, realtime, the handlers, CDFileSystem...) are on the disk.
 * The KickstartSet part brings both, built from the same tree as the ROM. Without them the slim ROM boots only
 * to its rescue Shell, so the set goes on before a machine moves to that ROM:
 *   GFOpenUpTool KICKSET <Parts/KickstartSet>   installs the 68k modules and only this host's
 *                                               .bodies/<e_machine>/ (none on a 68k), moves another
 *                                               CPU's drawer left from earlier to SYS:Kickstart/.retired/
 *                                               without deleting it, then checks the set
 *   GFOpenUpTool KICKSET CHECK [<part drawer>]  0 when SYS:Kickstart/default/Manifest is there and every
 *                                               68k module it lists is there at its size, and the host's
 *                                               listed bodies too (another CPU's may be absent); it says
 *                                               which CPU's bodies are installed; 10 when the host's are
 *                                               missing, which kickload would refuse
 * ENV:GFOpenUp/KickstartSet says "ready" or "missing"; DETECT's ENV:GFOpenUp/SlimROM says whether this ROM is the
 * slim one (kickload in it and no workbench.library of its own), and the Installer then installs the part. Cradle
 * refuses to start an instance or build a card on the slim ROM without the set (amicradle openkick_rom.py). */
#ifndef KICKSET_DIR
#define KICKSET_DIR "SYS:Kickstart/default/"
#endif

static int in_rom_window(ULONG a)
{
    return (a >= 0x00F80000UL && a < 0x01000000UL) || (a >= 0x00E00000UL && a < 0x00E80000UL);
}

/* 1 on the slim ROM: kickload there, and the ROM's own header says the thin profile (openup_slimrom.h); a ROM of
 * ours without that header: kickload in the ROM, and workbench.library not a RomTag of the ROM */
static int slim_rom(void)
{
    struct Resident *k, *wb;
    Forbid();
    k = FindResident((STRPTR)"kickload");
    wb = FindResident((STRPTR)"workbench.library");
    Permit();
    return ous_slim_rom(k != NULL, (const unsigned char *)0x00F80000UL, k && in_rom_window((ULONG)k), wb != NULL,
                        wb && in_rom_window((ULONG)wb));
}

/* The file's size, or -1 when it isn't a file there */
static LONG file_size(const char *path)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    APTR win = me->pr_WindowPtr;
    struct FileInfoBlock *fib;
    BPTR l;
    LONG size = -1;
    me->pr_WindowPtr = (APTR)-1;
    l = Lock((STRPTR)path, ACCESS_READ);
    me->pr_WindowPtr = win;
    if (!l) return -1;
    if ((fib = AllocDosObject(DOS_FIB, NULL))) {
        if (Examine(l, fib) && fib->fib_DirEntryType < 0) size = fib->fib_Size;
        FreeDosObject(DOS_FIB, fib);
    }
    UnLock(l);
    return size;
}

/* ElfHostCPU(0), LVO -108. An older elf.library, or none, is a 68k: no bodies. */
static unsigned kickset_host(void)
{
    struct Library *elf;
    unsigned cpu = 4;
    elf = OpenLibrary((STRPTR)"elf.library", 1);
    if (elf && elf->lib_NegSize >= 108) {
        register ULONG d0 __asm("d0") = 0;
        register struct Library *a6 __asm("a6") = elf;
        __asm volatile("jsr -108(%%a6)" : "+r"(d0) : "r"(a6) : "d1", "a0", "a1", "cc", "memory");
        cpu = ouk_host_cpu((unsigned)d0);
    }
    if (elf) CloseLibrary(elf);
    return cpu;
}

/* Which CPU's drawers are in the set now. */
static void kickset_say_bodies(unsigned host)
{
    struct FileInfoBlock *fib;
    BPTR l;
    int any = 0;
    l = Lock((STRPTR)KICKSET_DIR ".bodies", ACCESS_READ);
    if (!l) {
        Printf((STRPTR)"GFOpenUp: no native bodies installed%s.\n",
               (LONG)(host == 4 ? " (this host is 68k)" : ""));
        return;
    }
    if ((fib = AllocDosObject(DOS_FIB, NULL))) {
        if (Examine(l, fib) && fib->fib_DirEntryType > 0) {
            while (ExNext(l, fib)) {
                unsigned cpu;
                if (fib->fib_DirEntryType <= 0) continue;
                if (!ouk_u((const char *)fib->fib_FileName, &cpu) || !cpu) continue;
                Printf((STRPTR)"GFOpenUp: native bodies installed for CPU %ld.\n", (LONG)cpu);
                any = 1;
            }
        }
        FreeDosObject(DOS_FIB, fib);
    }
    UnLock(l);
    if (!any)
        Printf((STRPTR)"GFOpenUp: no native bodies installed%s.\n",
               (LONG)(host == 4 ? " (this host is 68k)" : ""));
}

/* A drawer for another CPU, left from an earlier install, moves under SYS:Kickstart/.retired/.
 * Nothing is deleted: a name already there gets -2, -3, ... */
static void kickset_retire_one(const char *fn)
{
    char from[160], to[160], rev[8];
    int n, k, v;
    strcpy(from, KICKSET_DIR ".bodies/");
    strncat(from, fn, 40);
    UnLock(CreateDir((STRPTR)"SYS:Kickstart/.retired"));
    strcpy(to, "SYS:Kickstart/.retired/bodies-");
    strncat(to, fn, 40);
    for (n = 2; exists(to) && n < 100; n++) {
        strcpy(to, "SYS:Kickstart/.retired/bodies-");
        strncat(to, fn, 40);
        strcat(to, "-");
        v = n;
        k = 0;
        do rev[k++] = (char)('0' + v % 10); while ((v /= 10) && k < 7);
        while (k) {
            int L = (int)strlen(to);
            to[L] = rev[--k];
            to[L + 1] = 0;
        }
    }
    if (exists(to)) {
        say("GFOpenUp: ", from, " is another CPU's drawer, and SYS:Kickstart/.retired has no free name for it");
        return;
    }
    if (Rename((STRPTR)from, (STRPTR)to))
        say("GFOpenUp: ", from, ": another CPU's bodies from an earlier install, moved to SYS:Kickstart/.retired");
}

static void kickset_retire_bodies(unsigned host)
{
    struct FileInfoBlock *fib;
    BPTR l;
    char names[16][32];
    int count = 0, i;
    l = Lock((STRPTR)KICKSET_DIR ".bodies", ACCESS_READ);
    if (!l) return;
    if ((fib = AllocDosObject(DOS_FIB, NULL))) {
        if (Examine(l, fib)) {
            while (ExNext(l, fib) && count < 16) {
                unsigned cpu;
                const char *fn = (const char *)fib->fib_FileName;
                if (fib->fib_DirEntryType <= 0) continue;
                if (!ouk_u(fn, &cpu) || !ouk_retire_drawer(cpu, host)) continue;
                {   /* copied by hand: GCC 16's -Wstringop-truncation refuses strncpy into 32 from FileName[108] */
                    int i;
                    for (i = 0; i < 31 && fn[i]; i++) names[count][i] = fn[i];
                    names[count][i] = 0;
                }
                count++;
            }
        }
        FreeDosObject(DOS_FIB, fib);
    }
    UnLock(l);
    for (i = 0; i < count; i++) kickset_retire_one(names[i]);
}

static int kickset_check(const char *drawer)
{
    char *text, *p, *line, path[260], *w[8];
    int n, modules = 0, bad = 0, v2 = 0;
    unsigned host = kickset_host();
    if (!(text = read_file(KICKSET_DIR "Manifest", NULL))) {
        say("GFOpenUp: there is no module set for the slim ROM (" KICKSET_DIR "Manifest is missing).",
            " Install it with GFOpenUpTool KICKSET Parts/KickstartSet from GFOpenUp's drawer before",
            " this machine moves to the slim ROM.");
        set_env("GFOpenUp/KickstartSet", "missing");
        return RETURN_ERROR;
    }
    for (p = text; *p && !bad; ) {
        unsigned cpu = 0;
        const char *bytes = 0;
        int kind;
        line = p;
        while (*p && *p != '\n') p++;
        if (*p) *p++ = 0;
        n = oul_words(line, w, 8);
        kind = ouk_manifest(n, w, v2, &cpu, &bytes);
        if (kind == OUK_SKIP) continue;
        if (kind == OUK_VER) {
            if (modules) { say("GFOpenUp: " KICKSET_DIR "Manifest has its version line after a module.", NULL, NULL); bad = 1; break; }
            v2 = 1;
            continue;
        }
        if (kind != OUK_FILE) { say("GFOpenUp: " KICKSET_DIR "Manifest has a line that isn't a Manifest line: ", line, NULL); bad = 1; break; }
        /* A body for another CPU is not installed here. A missing one is not a failure. */
        if (cpu != 4 && !(host != 4 && cpu == host)) continue;
        strcpy(path, KICKSET_DIR);
        strncat(path, w[0], 180);
        if (file_size(path) != atol(bytes)) {
            say("GFOpenUp: the module set for the slim ROM is incomplete: ", path,
                file_size(path) < 0 ? " is missing" : " is not the size its Manifest lists");
            if (cpu != 4)
                say("GFOpenUp: the host's listed body is missing, so kickload would refuse the set.", NULL, NULL);
            bad = 1;
        }
        modules++;
    }
    FreeVec(text);
    if (!bad && !modules) { say("GFOpenUp: " KICKSET_DIR "Manifest lists no module.", NULL, NULL); bad = 1; }
    if (!bad && drawer) {
        /* the part's other files: what the full ROM held on demand, on the disk now */
        strncpy(path, drawer, 200);
        path[200] = 0;
        AddPart((STRPTR)path, (STRPTR)"Part", sizeof path);
        if ((text = read_file(path, NULL))) {
            for (p = text; *p && !bad; ) {
                const char *rest;
                line = p;
                while (*p && *p != '\n') p++;
                if (*p) *p++ = 0;
                if (!(rest = oul_strip(line, machine_bits(), 0))) continue;
                strncpy(path, rest, sizeof path - 1);
                path[sizeof path - 1] = 0;
                n = oul_words(path, w, 8);
                if (n >= 3 && (!strcmp(w[0], "copy") || !strcmp(w[0], "add") || !strcmp(w[0], "default")) &&
                    !ouk_copy(w[2], host))
                    continue;
                if (n >= 3 && (!strcmp(w[0], "copy") || !strcmp(w[0], "add") || !strcmp(w[0], "default")) && !exists(w[2])) {
                    say("GFOpenUp: the slim ROM's files on the disk are incomplete: ", w[2], " is missing");
                    bad = 1;
                }
            }
            FreeVec(text);
        }
    }
    set_env("GFOpenUp/KickstartSet", bad ? "missing" : "ready");
    if (bad) {
        kickset_say_bodies(host);
        say("GFOpenUp: install GFOpenUp's KickstartSet part again (GFOpenUpTool KICKSET Parts/KickstartSet)",
            " before this machine moves to the slim ROM.", NULL);
        return RETURN_ERROR;
    }
    Printf((STRPTR)"GFOpenUp: the module set for the slim ROM is ready: %ld modules in " KICKSET_DIR "%s.\n", (LONG)modules,
           (LONG)(slim_rom() ? " (this machine runs the slim ROM)" : ""));
    kickset_say_bodies(host);
    return RETURN_OK;
}

/* A module of an older set that the new Manifest doesn't list: kickload would refuse the whole set for it (a set
 * is one unit), so it moves to SYS:Kickstart/.retired/ (kickload passes over names that start with a dot) */
static void kickset_prune(void)
{
    char *text, name[108], from[160], to[160];
    struct FileInfoBlock *fib;
    BPTR l;
    if (!(text = read_file(KICKSET_DIR "Manifest", NULL))) return;
    if (!(l = Lock((STRPTR)KICKSET_DIR, ACCESS_READ))) { FreeVec(text); return; }
    if ((fib = AllocDosObject(DOS_FIB, NULL))) {
        if (Examine(l, fib)) {
            while (ExNext(l, fib)) {
                const char *fn = (const char *)fib->fib_FileName;
                if (fib->fib_DirEntryType >= 0 || !ouk_prunable(fn) || ouk_listed(text, fn))
                    continue;
                strncpy(name, fn, sizeof name - 1);
                name[sizeof name - 1] = 0;
                strcpy(from, KICKSET_DIR);
                strncat(from, name, 100);
                UnLock(CreateDir((STRPTR)"SYS:Kickstart/.retired"));
                strcpy(to, "SYS:Kickstart/.retired/");
                strncat(to, name, 100);
                DeleteFile((STRPTR)to);
                if (Rename((STRPTR)from, (STRPTR)to))
                    say("GFOpenUp: ", from, ": an older set's module the new set doesn't have, moved to SYS:Kickstart/.retired");
            }
        }
        FreeDosObject(DOS_FIB, fib);
    }
    UnLock(l);
    FreeVec(text);
}

static int kickset(const char *a1, const char *a2)
{
    int rc;
    if (a1 && same_word(a1, "CHECK")) return kickset_check(a2);
    if (!a1) { say("GFOpenUpTool KICKSET <Parts/KickstartSet> | KICKSET CHECK [<part drawer>]", NULL, NULL); return RETURN_FAIL; }
    if ((rc = install(a1)) > RETURN_WARN) return rc;
    kickset_prune();
    kickset_retire_bodies(kickset_host());
    return kickset_check(a1);
}

/* ---- STARTUPSEQ: the disk's S:Startup-Sequence against the ROM's default (amicradle-guest
 * docs/design/rom-or-disk.md, section 6). The ROM's bytes and dos's weighing are GFOpenDOS's private calls,
 * ODRomDefault (-1176) and ODRomDefaultState (-1182), on GFOpenDOS 47.100 and up (OS 3.2.3's is 47.30) with
 * them; elsewhere there is no ROM default to offer. ---- */

#define STARTUPSEQ "S:Startup-Sequence"
#define STARTUPSEQ_KEPT "ENVARC:GFOpenAmiga/StartupSequence.kept"

static int rom_defaults_here(void)
{
    return DOSBase->dl_lib.lib_Revision >= 100 && DOSBase->dl_lib.lib_NegSize >= 1182;
}

static const UBYTE *rom_default(const char *name, ULONG *len)
{
    register const UBYTE *d0 __asm("d0");
    register const char *d1 __asm("d1") = name;
    register ULONG *d2 __asm("d2") = len;
    register struct DosLibrary *a6 __asm("a6") = DOSBase;
    *len = 0;
    if (!rom_defaults_here()) return NULL;
    __asm volatile("jsr -1176(%%a6)" : "=r"(d0), "+r"(d1), "+r"(d2) : "r"(a6) : "a0", "a1", "cc", "memory");
    return d0;
}

static ULONG rom_default_state(const char *name, ULONG *romvr, ULONG *diskvr)
{
    register ULONG d0 __asm("d0");
    register const char *d1 __asm("d1") = name;
    register ULONG *d2 __asm("d2") = romvr;
    register ULONG *d3 __asm("d3") = diskvr;
    register struct DosLibrary *a6 __asm("a6") = DOSBase;
    *romvr = *diskvr = 0;
    if (!rom_defaults_here()) return OSS_NONE;
    __asm volatile("jsr -1182(%%a6)" : "=r"(d0), "+r"(d1), "+r"(d2) : "r"(d3), "r"(a6) : "a0", "a1", "cc", "memory");
    return d0;
}

static void put_out(void *ctx, const char *s, unsigned long len)
{
    (void)ctx;
    Write(Output(), (APTR)s, (LONG)len);
}

static int startupseq(const char *how)
{
    ULONG romvr, diskvr, len = 0, st = rom_default_state(STARTUPSEQ, &romvr, &diskvr);
    const UBYTE *rom = rom_default(STARTUPSEQ, &len);
    char rv[16], dv[16], *kept;
    if (!how || same_word(how, "CHECK")) {
        unsigned long k = 0;
        if ((kept = read_file(STARTUPSEQ_KEPT, NULL))) { k = oss_kept(kept); FreeVec(kept); }
        say("GFOpenUp: ", oss_says(st), NULL);
        if (st != OSS_NONE) say("GFOpenUp: the ROM's default is ", oss_vr(rv, romvr), ".");
        if (diskvr) say("GFOpenUp: S:Startup-Sequence is ", oss_vr(dv, diskvr), ".");
        if ((st == OSS_OLDER || st == OSS_DIFFERS) && k == romvr) say("GFOpenUp: you chose to keep yours against this version.", NULL, NULL);
        return oss_offer(st, romvr, k) ? RETURN_WARN : RETURN_OK;
    }
    if (st == OSS_NONE || !rom) {
        say("GFOpenUp: this ROM has no default Startup-Sequence.", NULL, NULL);
        return RETURN_WARN;
    }
    if (same_word(how, "COMPARE")) {
        LONG dl = 0;
        char *disk = read_file(STARTUPSEQ, &dl);
        unsigned long n;
        if (!disk) { say("GFOpenUp: can't read S:Startup-Sequence.", NULL, NULL); return RETURN_ERROR; }
        say("; - the ROM's default's lines, + yours, where they differ (", oss_vr(rv, romvr), ")");
        n = oss_compare((const char *)rom, len, disk, (unsigned long)dl, put_out, NULL);
        if (!n) say("; none differ", NULL, NULL);
        FreeVec(disk);
        return RETURN_OK;
    }
    if (same_word(how, "KEEP")) {
        char line[16];
        int ok;
        UnLock(CreateDir((STRPTR)"ENVARC:GFOpenAmiga"));
        oss_vr(line, romvr);
        strcat(line, "\n");
        ok = write_file(STARTUPSEQ_KEPT, line, (LONG)strlen(line));
        UnLock(CreateDir((STRPTR)"ENV:GFOpenAmiga"));
        write_file("ENV:GFOpenAmiga/StartupSequence.kept", line, (LONG)strlen(line));
        say("GFOpenUp: your S:Startup-Sequence stays; not asked again until the ROM's default changes.", NULL, NULL);
        return ok ? RETURN_OK : RETURN_ERROR;
    }
    if (same_word(how, "ROM")) {
        int r;
        manifest_load();
        if ((r = retire_file(STARTUPSEQ)) < 0) return RETURN_ERROR;
        if (!write_file(STARTUPSEQ, (const char *)rom, (LONG)len)) {
            say("GFOpenUp: can't write S:Startup-Sequence. Is SYS: write-protected?", NULL, NULL);
            manifest_save();
            return RETURN_ERROR;
        }
        if (!manifest_save()) return RETURN_ERROR;
        say("GFOpenUp: S:Startup-Sequence is now the ROM's default ", oss_vr(rv, romvr),
            r ? "; yours is in SYS:Storage/GFOpenUp-Superseded." : ".");
        return RETURN_OK;
    }
    say("Usage: GFOpenUpTool STARTUPSEQ [CHECK | COMPARE | KEEP | ROM]", NULL, NULL);
    return RETURN_FAIL;
}

static int real_main(void);

/* GFOpenUpTool needs a big stack: FIXTOOLS reads and writes icons through icon.library, which takes far more
 * than the 4096 bytes a Shell, an Installer's Run Program step or an Execute script has by default (it did its
 * work and then never returned). When the stack is smaller than 32 KB, the whole program runs on one of 64 KB.
 * The swap's own variables are static: after StackSwap() the old stack can't be reached from the stack pointer. */
static struct StackSwapStruct big_stack;
static APTR big_stack_mem;
static int big_stack_rc;

static void run_on_big_stack(void)
{
    big_stack.stk_Lower = big_stack_mem;
    big_stack.stk_Upper = (ULONG)big_stack_mem + 65536;
    big_stack.stk_Pointer = (APTR)big_stack.stk_Upper;
    StackSwap(&big_stack);
    big_stack_rc = real_main();
    StackSwap(&big_stack);
}

int main(void)
{
    struct Task *me = FindTask(NULL);
    if ((ULONG)me->tc_SPUpper - (ULONG)me->tc_SPLower < 32768 && (big_stack_mem = AllocVec(65536, MEMF_PUBLIC))) {
        int rc;
        run_on_big_stack();
        rc = big_stack_rc;
        FreeVec(big_stack_mem);
        return rc;
    }
    return real_main();
}

static int real_main(void)
{
    LONG args[3] = { 0, 0, 0 };
    struct RDArgs *rda = ReadArgs((STRPTR)"COMMAND/A,DRAWER,TEXT", args, NULL);
    const char *cmd;
    int rc;
    if (!rda) {
        PutStr((STRPTR)"Usage: GFOpenUpTool DETECT | INSTALL <part drawer> [FORCE] | REMOVE <part drawer> | REINSTALL <Parts drawer> [FORCE] | STARTUP | UNINSTALL | FIXTOOLS <table> [ALL] | LIST | SETTING <dest> <text> | THEME <name> <look> | BACKDROP <prefix> [KEEP] | FONT <font> <size> | TITLEBARS <screen> <window> | WBBACKDROP | WBGAUGE | OLDLOOK | RECONCILE [CHECK|APPLY [ONCE]|UNDO] | TIDY [ONCE] | ICONS [CHECK|APPLY] | ICONSIG <icon or drawer> [<fields icon>] | UPDATE [CHECK|YES|AUTO] | UNPLACE <program> | OLDNAMES [DOCK] | KICKSET [CHECK] [<part drawer>] | STARTUPSEQ [CHECK|COMPARE|KEEP|ROM]\n");
        return RETURN_FAIL;
    }
    if (!(ExpansionBase = (struct ExpansionBase *)OpenLibrary((STRPTR)"expansion.library", 37))) {
        FreeArgs(rda);
        return RETURN_FAIL;
    }
    cmd = (const char *)args[0];
    if (same_word(cmd, "DETECT")) rc = detect();
    else if (same_word(cmd, "INSTALL") && args[1]) {
        force_older = args[2] && same_word((const char *)args[2], "FORCE");
        rc = install((const char *)args[1]);
    }
    else if (same_word(cmd, "REMOVE") && args[1]) rc = remove_part((const char *)args[1]);
    else if (same_word(cmd, "REINSTALL") && args[1]) {
        force_older = args[2] && same_word((const char *)args[2], "FORCE");
        rc = reinstall((const char *)args[1]);
    }
    else if (same_word(cmd, "STARTUP")) rc = write_startup(0) ? RETURN_OK : (say("GFOpenUp: can't write S:User-Startup. Is SYS: write-protected?", NULL, NULL), RETURN_ERROR);
    else if (same_word(cmd, "UNINSTALL")) rc = uninstall();
    else if (same_word(cmd, "FIXTOOLS") && args[1]) rc = fix_tools((const char *)args[1], args[2] && same_word((const char *)args[2], "ALL"));
    else if (same_word(cmd, "LIST")) rc = list();
    else if (same_word(cmd, "BACKDROP") && args[1]) rc = backdrop((const char *)args[1], args[2] && same_word((const char *)args[2], "KEEP"));
    else if (same_word(cmd, "FONT") && args[1] && args[2]) rc = set_font((const char *)args[1], atol((const char *)args[2]));
    else if (same_word(cmd, "TITLEBARS") && args[1] && args[2]) rc = set_titlebars(atol((const char *)args[1]), atol((const char *)args[2]));
    else if (same_word(cmd, "WBBACKDROP")) rc = set_wb_backdrop();
    else if (same_word(cmd, "WBGAUGE")) rc = set_wb_gauge();
    else if (same_word(cmd, "OLDLOOK")) rc = old_look();
    else if (same_word(cmd, "RECONCILE")) rc = reconcile((const char *)args[1], (const char *)args[2]);
    else if (same_word(cmd, "TIDY")) rc = tidy((const char *)args[1]);
    else if (same_word(cmd, "ICONS")) rc = icons((const char *)args[1]);
    else if (same_word(cmd, "ICONSIG") && args[1] && args[2]) {
        /* is the icon its own picture over these fields, as OLDLOOK asks? The icon it compared with stays in T: */
        int same;
        keep_face_tmp = 1;
        same = still_face((const char *)args[1], (const char *)args[1], (const char *)args[2]);
        Printf((STRPTR)"%s: %s the picture over %s's fields (T:GFOpenUp-oldface.info)\n", args[1],
               (LONG)(same ? "is" : "is not"), args[2]);
        rc = same ? RETURN_OK : RETURN_WARN;
    }
    else if (same_word(cmd, "ICONSIG") && args[1]) rc = icon_sigs((const char *)args[1]);
    else if (same_word(cmd, "SETTING") && args[1] && args[2]) {
        /* The text goes through T: so it is installed like any file. */
        char line[200];
        strncpy(line, (const char *)args[2], sizeof line - 2);
        line[sizeof line - 2] = 0;
        strcat(line, "\n");
        manifest_load();
        rc = install_text((const char *)args[1], line);
    }
    else if (same_word(cmd, "THEME") && args[1] && args[2]) rc = set_theme((const char *)args[1], (const char *)args[2]);
    else if (same_word(cmd, "P96LEFTOVER")) {
        /* Picasso96API.library in LIBS: with no Picasso96 board running (its
         * monitor driver would have opened rtg.library at boot) hangs SDL's
         * video start (6 Oct 2026). AmiCradle's instances are never asked. */
        int rtg;
        Forbid();
        rtg = FindName(&SysBase->LibList, (STRPTR)"rtg.library") != NULL;
        Permit();
        rc = (!is_amicradle() && !rtg && exists("LIBS:Picasso96API.library")) ? RETURN_WARN : RETURN_OK;
    }
    else if (same_word(cmd, "UPDATE")) rc = update((const char *)args[1]);
    else if (same_word(cmd, "STARTUPSEQ")) rc = startupseq((const char *)args[1]);
    else if (same_word(cmd, "KICKSET")) rc = kickset((const char *)args[1], (const char *)args[2]);
    else if (same_word(cmd, "RETIRE") && args[1]) {
        manifest_load();
        rc = retire_file((const char *)args[1]) < 0 ? RETURN_ERROR : RETURN_OK;
        if (rc == RETURN_OK && !manifest_save()) rc = RETURN_ERROR;
    }
    else if (same_word(cmd, "UNPLACE") && args[1]) rc = unplace((const char *)args[1]);
    else if (same_word(cmd, "OLDNAMES")) rc = args[1] && same_word((const char *)args[1], "DOCK") ? old_dock() : old_settings_copy();
    else { PutStr((STRPTR)"Usage: GFOpenUpTool DETECT | INSTALL <part drawer> [FORCE] | REMOVE <part drawer> | REINSTALL <Parts drawer> [FORCE] | STARTUP | UNINSTALL | FIXTOOLS <table> [ALL] | LIST | SETTING <dest> <text> | THEME <name> <look> | BACKDROP <prefix> [KEEP] | FONT <font> <size> | TITLEBARS <screen> <window> | WBBACKDROP | WBGAUGE | OLDLOOK | RECONCILE [CHECK|APPLY [ONCE]|UNDO] | TIDY [ONCE] | ICONS [CHECK|APPLY] | ICONSIG <icon or drawer> [<fields icon>] | UPDATE [CHECK|YES|AUTO] | UNPLACE <program> | OLDNAMES [DOCK] | KICKSET [CHECK] [<part drawer>] | STARTUPSEQ [CHECK|COMPARE|KEEP|ROM]\n"); rc = RETURN_FAIL; }
    CloseLibrary((struct Library *)ExpansionBase);
    FreeArgs(rda);
    (void)version;
    return rc;
}
