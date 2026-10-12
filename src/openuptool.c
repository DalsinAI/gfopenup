/* OpenUpTool: what OpenUp's Installer scripts call (DESIGN.md section 3).
 *
 *   OpenUpTool DETECT                 what this machine is; ENV:OpenUp/...
 *   OpenUpTool INSTALL <part drawer>  installs one part, as its Part file says
 *   OpenUpTool STARTUP                writes the marked block in S:User-Startup
 *   OpenUpTool UNINSTALL              puts everything back
 *   OpenUpTool FIXTOOLS <table> [ALL] puts back default tools that OpenUp 0.6.14 to 0.7.0 changed
 *                                     to OpenView by mistake (see FIXTOOLS below)
 *   OpenUpTool LIST                   what is installed
 *   OpenUpTool SETTING <dest> <text>  a setting the Installer asked for, as a file
 *   OpenUpTool BACKDROP <prefix>      Workbench's backdrop: <prefix>-strip-<H>.png for the
 *                                     screen's height, tiled, when there is one; else <prefix>-<W>x<H>.png, the
 *                                     one nearest the screen's shape, scaled to fit
 *   OpenUpTool FONT <font> <size>     the desktop's text: the screens' font and the icons'
 *                                     text (Font prefs, "Screen text" and "Workbench icon text")
 *   OpenUpTool TITLEBARS <screen> <window>  extra pixels in the title bars (IControl
 *                                     prefs, Appearance Settings, Title Bar Extra Height)
 *   OpenUpTool WBBACKDROP             Workbench as a backdrop: no frame, scroll bars or size gadget on
 *                                     its window (Workbench's prefs, WBConfig.prefs)
 *   OpenUpTool WBGAUGE                No fill gauge in a drive's window (Workbench's prefs, "No
 *                                     volume gauge": the bar down the left border that shows how
 *                                     full the disk is; the border is slim like any window's)
 *   OpenUpTool P96LEFTOVER            5 (WARN) on a real Amiga with Picasso96's
 *                                     library left in LIBS: and no Picasso96 board running
 *   OpenUpTool RECONCILE [CHECK|APPLY [ONCE]|UNDO]  one desktop, the Open one: old prefs editors to
 *                                     SYS:Prefs/Classic, replaced tools retired, settings OpenUp owns kept in
 *                                     step; CHECK says what it would do (see the RECONCILE section)
 *   OpenUpTool TIDY [ONCE]            the system's drawers cleaned up, sized to fit (at most 800 x 600) and
 *                                     snapshotted by Workbench itself, so they open neat (see TIDY)
 *   OpenUpTool ICONS [CHECK|APPLY]    old icons (four colours, NewIcons) found on SYS: and Work: and given
 *                                     our faces, by name from Gorilla's set, else by kind (see ICONS)
 *   OpenUpTool RETIRE <file>          moves a file to SYS:Storage/OpenUp-Superseded, to
 *                                     come back with UNINSTALL
 *   OpenUpTool UPDATE [CHECK|YES]     a newer pack Cradle put in DH1:OpenUp: CHECK says
 *                                     (WARN when one waits); UPDATE shows what changes and
 *                                     asks; YES doesn't ask. It never runs on its own
 *
 * The rules it keeps:
 * - The base OS stays as shipped. A file OpenUp replaces goes first to
 *   SYS:Storage/OpenUp-Superseded/ (its path, with the colon as a slash),
 *   and only the first time: that copy is the original.
 * - It never overwrites another stack: a part's "keep" lines name files that
 *   must already be ours (a marker string inside them) or absent.
 * - Every file, directory and startup line it adds is listed in the
 *   manifest, ENVARC:OpenAmiga/OpenUp.manifest (and ENV:), so UNINSTALL can
 *   take exactly those away and put the originals back.
 * - Startup lines go in one marked block in S:User-Startup:
 *   ";BEGIN OpenUp" to ";END OpenUp". A line listed in
 *   ENVARC:OpenAmiga/Startup.off (OpenUp Setup's switches) is written with
 *   ";OFF " in front, so AmigaDOS skips it, and a line whose program isn't on
 *   the disk with ";MISSING " (STARTUP writes it back in once it is there).
 *   A part's lines are the ones its Part file has now (0.7.5).
 * - Errors say what to do next, in plain words.
 *
 * A part's drawer holds its files, laid out as they go, and a Part file:
 *   part <name> <version>
 *   needs AmigaChrome                  (only on an AmigaChrome instance)
 *   needs ACRTG                        (only with an ACRTG graphics board)
 *   keep <dest> <marker> [<marker>...]  (dest must be absent or contain a marker)
 *   copy <file in the drawer> <dest>
 *   default <file in the drawer> <dest> (copied only when dest is missing)
 *   add <file in the drawer> <dest>     (copied when dest is missing or OpenUp
 *                                        put it there; another program's stays)
 *   retire <dest>                       (moved to SYS:Storage/OpenUp-Superseded,
 *                                        so it no longer runs; UNINSTALL puts it back)
 *   face <icon in the drawer> <dest .info>  (0.6: the icon's picture only; dest keeps
 *                                        its type, tools, tool types and window;
 *                                        left alone when the machine hasn't got it)
 *   startup <the rest of the line>
 *   when AmigaChrome <copy|default|add|face|retire line>  (0.6.3: the line only on an AmigaChrome instance)
 * Words with spaces are quoted.
 *
 * MIT, Copyright (c) 2026 AmigaChrome Team. */
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

static const char version[] __attribute__((used)) = "$VER: OpenUpTool 0.7.3 (12.10.2026) MIT, Copyright (c) 2026 AmigaChrome Team";

struct ExpansionBase *ExpansionBase;
struct Library *IconBase;

static int same_word(const char *a, const char *b)
{
    for (; *a && *b; a++, b++)
        if ((*a | 0x20) != (*b | 0x20)) return 0;
    return !*a && !*b;
}

#define MANIFEST "ENVARC:OpenAmiga/OpenUp.manifest"
#define MANIFEST_ENV "ENV:OpenAmiga/OpenUp.manifest"
#define SUPERSEDED "SYS:Storage/OpenUp-Superseded"
#define USER_STARTUP "S:User-Startup"
#define BLOCK_BEGIN ";BEGIN OpenUp"
#define BLOCK_END ";END OpenUp"
#define STARTUP_OFF "ENVARC:OpenAmiga/Startup.off"

#define DALSIN 0xDA15
#define DALSIN_OLD 2011

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

static char *read_file(const char *path, LONG *lenp)
{
    BPTR fh = Open((STRPTR)path, MODE_OLDFILE);
    LONG len;
    char *buf;
    if (!fh)
        return NULL;
    Seek(fh, 0, OFFSET_END);
    len = Seek(fh, 0, OFFSET_BEGINNING);
    if (len < 0 || !(buf = AllocVec(len + 1, MEMF_ANY))) {
        Close(fh);
        return NULL;
    }
    if (Read(fh, buf, len) != len) {
        FreeVec(buf);
        Close(fh);
        return NULL;
    }
    buf[len] = 0;
    Close(fh);
    if (lenp) *lenp = len;
    return buf;
}

static int write_file(const char *path, const char *data, LONG len)
{
    BPTR fh = Open((STRPTR)path, MODE_NEWFILE);
    int ok;
    if (!fh)
        return 0;
    ok = Write(fh, (APTR)data, len) == len;
    Close(fh);
    return ok;
}

/* ---- the manifest -------------------------------------------------------------- */

/* One line each: kind, then up to three tab-separated fields. Kinds:
 *   part <name> <version>   new <dest>   replaced <dest> <backup>
 *   same <dest>   dir <path>   bdir <path>   startup <part> <line>
 *   newfile <path> (a file OpenUp made, such as S:User-Startup)
 * 1024 entries was too few (0.7.3 and earlier): every part ticked comes to about 1,150 lines in 0.7.5,
 * an upgrade keeps the lines of files earlier versions put in, and a lab copy of an instance upgraded
 * from 0.7.3 to 0.7.4 filled the table, so new files and a part's new startup line went unlisted. */
#define MAXE 4096
/* a and b are whole paths, each in an allocation of its own length (libnix
 * gives them all back at exit): a path is never cut short in the manifest,
 * where a cut path would send UNINSTALL to the wrong file, and the table
 * stays small. Neither is ever NULL: "" is no_text. */
#define ENTRY_PATH 260                       /* the install code's path buffers */
typedef struct { char kind[10]; char *a; char *b; } entry;
static entry ents[MAXE];
static int nents;
static char no_text[] = "";
static int manifest_short;                   /* out of memory while loading: never save a short list */

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
    char *text = read_file(MANIFEST, NULL), *p, *line;
    nents = 0;
    manifest_short = 0;
    if (!text)
        return;
    for (p = text; *p && nents < MAXE; ) {
        char *f[3] = { NULL, NULL, NULL };
        int n = 0;
        line = p;
        while (*p && *p != '\n') p++;
        if (*p) *p++ = 0;
        f[n++] = line;
        for (char *q = line; *q && n < 3; q++)
            if (*q == '\t') { *q = 0; f[n++] = q + 1; }
        if (!strcmp(f[0], "OpenUp") || !f[0][0])
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
    LONG size = 32, len = 0;
    char *buf, *p;
    int i, ok;
    if (manifest_short)
        return 0;                           /* a list loaded short would lose files */
    for (i = 0; i < nents; i++)
        size += strlen(ents[i].kind) + strlen(ents[i].a) + strlen(ents[i].b) + 3;
    if (!(buf = AllocVec(size, MEMF_ANY)))
        return 0;
    p = buf;
    strcpy(p, "OpenUp\t0.1\n");
    p += strlen(p);
    for (i = 0; i < nents; i++) {
        if (!ents[i].kind[0])
            continue;                       /* taken out */
        strcpy(p, ents[i].kind); p += strlen(p);
        *p++ = '\t'; strcpy(p, ents[i].a); p += strlen(p);
        if (ents[i].b[0]) { *p++ = '\t'; strcpy(p, ents[i].b); p += strlen(p); }
        *p++ = '\n';
    }
    len = p - buf;
    ensure_dir("ENVARC:OpenAmiga", 1);
    ensure_dir("ENV:OpenAmiga", 0);
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
        if (!said_full++) say("OpenUp: its list of installed files (" MANIFEST ") is full.", NULL, NULL);
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

/* 1 when both files have the same bytes. */
static int same_file(const char *a, const char *b)
{
    LONG la, lb;
    char *da = read_file(a, &la), *db;
    int same = 0;
    if (!da)
        return 0;
    if ((db = read_file(b, &lb))) {
        same = la == lb && !memcmp(da, db, la);
        FreeVec(db);
    }
    FreeVec(da);
    return same;
}

/* Copies a file, its protection bits too, making the directories it needs. */
static int copy_file(const char *from, const char *to, int record_dirs)
{
    char dir[200];
    LONG len;
    char *data = read_file(from, &len);
    struct FileInfoBlock *fib;
    BPTR l;
    int ok;
    if (!data)
        return 0;
    parent_of(to, dir, sizeof dir);
    if (dir[strlen(dir) - 1] != ':' && !ensure_dir(dir, record_dirs)) {
        FreeVec(data);
        return 0;
    }
    ok = write_file(to, data, len);
    FreeVec(data);
    if (ok && (fib = AllocDosObject(DOS_FIB, NULL))) {
        if ((l = Lock((STRPTR)from, ACCESS_READ))) {
            if (Examine(l, fib))
                SetProtection((STRPTR)to, fib->fib_Protection);
            UnLock(l);
        }
        FreeDosObject(DOS_FIB, fib);
    }
    return ok;
}

/* Where a file's original goes: SYS:Storage/OpenUp-Superseded/LIBS/x. */
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
    return FindConfigDev(NULL, DALSIN, product) || FindConfigDev(NULL, DALSIN_OLD, product);
}

static int is_amigachrome(void)
{
    return board(1) || board(2);        /* ACBridge, ACStorage */
}

static void set_env(const char *name, const char *value)
{
    ensure_dir("ENV:OpenUp", 0);
    SetVar((STRPTR)name, (STRPTR)value, -1, GVF_GLOBAL_ONLY);
}

static int detect(void)
{
    char summary[600];
    const char *machine;
    UWORD attn = ((struct ExecBase *)SysBase)->AttnFlags;
    int ac = is_amigachrome();
    if (ac) machine = "AmigaChrome";
    else if (OpenResource((STRPTR)"devicetree.resource")) machine = "PiStorm";
    else machine = "Amiga";
    set_env("OpenUp/Machine", machine);
    set_env("OpenUp/RTG", board(9) ? "ACRTG" : "");
    set_env("OpenUp/Network", board(6) ? "OpenSocket card" : "");
    strcpy(summary, "This is ");
    strcat(summary, ac ? "an AmigaChrome instance" : !strcmp(machine, "PiStorm") ? "a PiStorm (Emu68)" : "an Amiga");
    strcat(summary, ", with a ");
    strcat(summary, attn & AFF_68060 ? "68060" : attn & AFF_68040 ? "68040" : attn & AFF_68030 ? "68030" : attn & AFF_68020 ? "68020" : "68000");
    strcat(summary, attn & (AFF_68881 | AFF_FPU40) ? " and an FPU" : " and no FPU");
    strcat(summary, ".\n");
    if (ac) {
        strcat(summary, board(9) ? "RTG: an ACRTG card.\n" : "RTG: none.\n");
        strcat(summary, board(6) ? "Network: the OpenSocket card is fitted.\n"
                                 : "Network: the OpenSocket card isn't fitted (turn the instance's Network switch on in Cradle).\n");
    } else {
        strcat(summary, "OpenUp 0.1 is for AmigaChrome instances. On this machine it offers only the parts that need no AmigaChrome boards.\n");
    }
    set_env("OpenUp/Summary", summary);
    PutStr((STRPTR)summary);
    return RETURN_OK;
}

/* ---- INSTALL ------------------------------------------------------------------------- */

/* Splits a line into words; quoted words keep their spaces. */
static int words(char *line, char **w, int max)
{
    int n = 0;
    char *p = line;
    while (*p && n < max) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p || *p == ';' || *p == '#') break;
        if (*p == '"') {
            w[n++] = ++p;
            while (*p && *p != '"') p++;
        } else {
            w[n++] = p;
            while (*p && *p != ' ' && *p != '\t') p++;
        }
        if (*p) *p++ = 0;
    }
    return n;
}

/* Puts one file in place, keeping the original the first time OpenUp
 * replaces it, and notes it in the manifest (loaded by the caller).
 * only_if_missing: 1 leaves any file already there (the user's own setting);
 * 2 replaces only a file OpenUp itself added, so an upgrade reaches it.
 * 1 copied, 0 nothing to do, -1 failed (said why). */
static int install_file(const char *from, const char *dest, int only_if_missing)
{
    char backup[260];
    entry *e = find_entry(NULL, dest);
    if (only_if_missing == 1 && exists(dest))
        return 0;                                   /* the user's own setting stays */
    if (only_if_missing == 2 && exists(dest) && (!e || strcmp(e->kind, "new")))
        return 0;                                   /* another program's file stays */
    if (exists(dest) && same_file(from, dest)) {
        if (!e) add_entry("same", dest, NULL);
        return 0;
    }
    if (exists(dest) && (!e || !strcmp(e->kind, "same"))) {
        /* The first time OpenUp replaces this file: keep the original. */
        char dir[260];
        backup_path(dest, backup, sizeof backup);
        parent_of(backup, dir, sizeof dir);
        if (!exists(backup) && (!ensure_dir(dir, 0) || !copy_file(dest, backup, 0))) {
            say("OpenUp: can't keep a copy of ", dest, " in SYS:Storage; it wasn't replaced. Is SYS: full or write-protected?");
            return -1;
        }
        if (e ? !set_text(&e->b, backup) : !add_entry("replaced", dest, backup)) {
            say("OpenUp: out of memory noting ", dest, "; it wasn't replaced.");
            return -1;
        }
        if (e) strcpy(e->kind, "replaced");
    } else if (!e) {
        add_entry("new", dest, NULL);
    }
    if (!copy_file(from, dest, 1)) {
        say("OpenUp: can't write ", dest, ". Is the disk full or write-protected?");
        return -1;
    }
    return 1;
}

/* Takes a file out of the way (another program's monitor driver, say),
 * keeping it in SYS:Storage/OpenUp-Superseded for UNINSTALL to put back.
 * 1 moved, 0 nothing to do, -1 failed (said why). */
static int retire_file(const char *dest)
{
    char backup[260], dir[260];
    if (!exists(dest) || find_entry(NULL, dest))
        return 0;                                   /* absent, or OpenUp's already */
    backup_path(dest, backup, sizeof backup);
    parent_of(backup, dir, sizeof dir);
    if (!ensure_dir(dir, 0) || !copy_file(dest, backup, 0)) {
        say("OpenUp: can't keep a copy of ", dest, " in SYS:Storage; it was left in place. Is SYS: full or write-protected?");
        return -1;
    }
    if (!DeleteFile((STRPTR)dest)) {
        DeleteFile((STRPTR)backup);
        say("OpenUp: can't move ", dest, " out of the way. Is it in use, or the disk write-protected?");
        return -1;
    }
    add_entry("replaced", dest, backup);
    return 1;
}

/* A new face for an icon that stays the system's (Gorilla's pictures on SYS:'s
 * drawers and programs and on the default icons, ICONS.md section 8): the
 * picture comes from the part's icon, and everything else (type, default
 * tool, tool types, stack, the drawer's window, its place) stays the icon's
 * own. The original goes to SYS:Storage/OpenUp-Superseded the first time, as
 * for a replaced file, so UNINSTALL puts it back. An icon the machine hasn't
 * got is left alone. 1 changed, 0 nothing to do, -1 failed (said why). */
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
    if (!e || !strcmp(e->kind, "same")) {          /* the first time: keep the original */
        parent_of(backup, dir, sizeof dir);
        if (!exists(backup) && (!ensure_dir(dir, 0) || !copy_file(dest, backup, 0))) {
            say("OpenUp: can't keep a copy of ", dest, " in SYS:Storage; its picture wasn't changed. Is SYS: full or write-protected?");
            return -1;
        }
    }
    if (!(IconBase = OpenLibrary((STRPTR)"icon.library", 44))) {
        say("OpenUp: icon.library 44 or later is needed to give ", dest, " its new picture.");
        return -1;
    }
    pic = GetIconTags((STRPTR)fname, ICONGETA_FailIfUnavailable, TRUE, TAG_DONE);
    old = GetIconTags((STRPTR)dname, ICONGETA_FailIfUnavailable, TRUE, TAG_DONE);
    if (pic && old) {
        /* the new picture with the old icon's own: lent for the write, then given back */
        UBYTE type = pic->do_Type;
        STRPTR tool = pic->do_DefaultTool, *tts = pic->do_ToolTypes;
        LONG stack = pic->do_StackSize, cx = pic->do_CurrentX, cy = pic->do_CurrentY;
        struct DrawerData *dd = pic->do_DrawerData;
        STRPTR tw = pic->do_ToolWindow;
        pic->do_Type = old->do_Type; pic->do_DefaultTool = old->do_DefaultTool; pic->do_ToolTypes = old->do_ToolTypes;
        pic->do_StackSize = old->do_StackSize; pic->do_CurrentX = old->do_CurrentX; pic->do_CurrentY = old->do_CurrentY;
        pic->do_DrawerData = old->do_DrawerData; pic->do_ToolWindow = old->do_ToolWindow;
        ok = PutIconTags((STRPTR)dname, pic, ICONPUTA_NotifyWorkbench, TRUE, TAG_DONE) != 0;
        pic->do_Type = type; pic->do_DefaultTool = tool; pic->do_ToolTypes = tts;
        pic->do_StackSize = stack; pic->do_CurrentX = cx; pic->do_CurrentY = cy;
        pic->do_DrawerData = dd; pic->do_ToolWindow = tw;
    }
    if (pic) FreeDiskObject(pic);
    if (old) FreeDiskObject(old);
    CloseLibrary(IconBase);
    IconBase = NULL;
    if (!ok) {
        say("OpenUp: can't give ", dest, " its new picture. Is the disk write-protected?");
        return -1;
    }
    if (!e) { if (!add_entry("replaced", dest, backup)) { say("OpenUp: out of memory noting ", dest, "'s new picture."); return -1; } }
    else if (!strcmp(e->kind, "same")) {
        if (!set_text(&e->b, backup)) { say("OpenUp: out of memory noting ", dest, "'s new picture."); return -1; }
        strcpy(e->kind, "replaced");
    }
    return 1;
}

/* A part's startup lines are the ones its Part file has now. Installing it marks the lines it had before
 * ("ostartup"); each line it still has goes back in its place, and the rest are taken out. Before 0.7.5 a
 * line a newer part changed stayed too: "SYS:Prefs/Sound USE" beside 0.7.4's "SYS:Prefs/OpenSound USE",
 * and a boot that stopped where the old editor was no longer there (a lab copy of Instance-32). */
static void mark_startup(const char *part)
{
    int i;
    for (i = 0; i < nents; i++)
        if (!strcmp(ents[i].kind, "startup") && !strcmp(ents[i].b, part))
            strcpy(ents[i].kind, "ostartup");
}

static void drop_marked_startup(void)
{
    int i;
    for (i = 0; i < nents; i++)
        if (!strcmp(ents[i].kind, "ostartup"))
            ents[i].kind[0] = 0;                    /* taken out: manifest_save leaves it out */
}

static int install(const char *drawer)
{
    char path[220], from[260], *text, *p, *line, partname[64] = "", partver[32] = "";
    int pass, refused = 0, files = 0, errors = 0;

    strncpy(path, drawer, 200);
    path[200] = 0;
    AddPart((STRPTR)path, (STRPTR)"Part", sizeof path);
    if (!(text = read_file(path, NULL))) {
        say("OpenUp: can't read ", path, ". Run the Installer from the OpenUp drawer.");
        return RETURN_FAIL;
    }
    manifest_load();
    /* Pass 0 checks (needs, keep); pass 1 copies. Lines are split in a copy each pass. */
    for (pass = 0; pass < 2 && !refused; pass++) {
        char *copy = AllocVec(strlen(text) + 1, MEMF_ANY);
        if (!copy) { FreeVec(text); return RETURN_FAIL; }
        strcpy(copy, text);
        for (p = copy; *p; ) {
            char *w[12];
            int n;
            line = p;
            while (*p && *p != '\n') p++;
            if (*p) *p++ = 0;
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
            if (!strcmp(w[0], "when") && n >= 4 && !strcmp(w[1], "AmigaChrome")) {
                if (!is_amigachrome()) continue;        /* the line is for AmigaChrome only */
                n -= 2;
                memmove(w, w + 2, n * sizeof w[0]);
            }
            if (!strcmp(w[0], "part") && n >= 3) {
                strncpy(partname, w[1], sizeof partname - 1);
                strncpy(partver, w[2], sizeof partver - 1);
                if (pass == 1) mark_startup(partname);
            } else if (pass == 0 && !strcmp(w[0], "needs") && n >= 2) {
                if (!strcmp(w[1], "AmigaChrome") && !is_amigachrome()) {
                    say("OpenUp: ", partname, " is for AmigaChrome instances; left out here.");
                    refused = 1;
                }
                if (!strcmp(w[1], "ACRTG") && !board(9)) {
                    say("OpenUp: ", partname, " needs an ACRTG graphics board; left out here. In AmigaChrome, fit one in Cradle (Hardware, Display).");
                    refused = 1;
                }
            } else if (pass == 0 && !strcmp(w[0], "keep") && n >= 3) {
                if (exists(w[1]) && !has_marker(w[1], w + 2, n - 2) && !find_entry(NULL, w[1])) {
                    say("OpenUp: ", w[1], " belongs to another program, so OpenUp leaves it and this part out.");
                    say("        To use ", partname, ", remove that program first, then run OpenUp again.");
                    refused = 1;
                }
            } else if (pass == 1 && !strcmp(w[0], "face") && n >= 3) {
                strcpy(from, drawer);
                AddPart((STRPTR)from, (STRPTR)w[1], sizeof from);
                switch (face_icon(from, w[2])) {
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
    drop_marked_startup();
    if (refused)
        return RETURN_WARN;
    {
        entry *pe = find_entry("part", partname);
        if (pe ? !set_text(&pe->b, partver) : !add_entry("part", partname, partver)) {
            say("OpenUp: out of memory noting ", partname, "; run the install again.");
            return RETURN_ERROR;
        }
    }
    if (!manifest_save()) {
        say("OpenUp: can't write its list of installed files in ENVARC:OpenAmiga. Is SYS: full?", NULL, NULL);
        return RETURN_ERROR;
    }
    Printf((STRPTR)"OpenUp: %s %s installed (%ld file%s changed)\n", (LONG)partname, (LONG)partver, files, (LONG)(files == 1 ? "" : "s"));
    return errors ? RETURN_ERROR : RETURN_OK;
}

/* ---- S:User-Startup ---------------------------------------------------------------------- */

/* 1 when the line from p to e is exactly the marker (trailing blanks
 * allowed): ";BEGIN OpenUp" and not ";BEGIN OpenUpSomething", another
 * program's block whose name starts the same way. Any number of leading
 * semicolons counts as one: a block once marked ";;BEGIN OpenUp" (an edit by
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

/* 1 when line is listed in ENVARC:OpenAmiga/Startup.off (OpenUp Setup's
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

/* 1 when a startup line runs a program named by its path ("SYS:Prefs/OpenSound USE", "Run >NIL: C:OpenDock")
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
            strcat(buf, BLOCK_BEGIN "\n; Added by OpenUp. OpenUpTool UNINSTALL takes it out.\n");
            for (i = 0; i < nents; i++)
                if (!strcmp(ents[i].kind, "startup") && strlen(buf) + strlen(ents[i].a) + 32 < (ULONG)(64 + MAXE * 2 + 8192)) {
                    if (off && switched_off(off, ents[i].a)) strcat(buf, ";OFF ");     /* switched off in OpenUp Setup */
                    else if (program_missing(ents[i].a)) {                           /* back in when the program is */
                        say("OpenUp: left out of S:User-Startup, as its program isn't there: ", ents[i].a, NULL);
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

/* ---- FIXTOOLS: default tools OpenTypes' SWITCH changed that it should not have --------------- */

/* OpenUp 0.6.14 to 0.7.0 ran "OpenTypes SWITCH MultiView TO OpenView IN SYS:". A bug in OpenTypes (its
 * Stricmp was called with A6 still holding dos.library) made it see every icon and every kind of file as
 * MultiView's, so about 40 of AmigaOS 3.2.3's own icons (the datatype descriptors', the pointer presets',
 * DOSDrivers', the Picasso96 guides', Help...) and 16 default icons opened OpenView. FIXTOOLS puts the
 * default tool back. It changes an icon only when that icon's default tool is OpenView now and its
 * original is known, from, in this order:
 *   1. OpenTypes' own record, ENVARC:OpenTypes/Changes and the icon it kept in Backup (exact);
 *   2. the table, StockDefaultTools: "<icon without .info><TAB><default tool>" for AmigaOS 3.2.3's icons.
 * The kinds of file OpenTypes switched on purpose (ENVARC:Sys/def_<kind> that named MultiView) stay on
 * OpenView, unless ALL is given (UNINSTALL gives it). Every change is logged. */
#define OT_CHANGES "ENVARC:OpenTypes/Changes"
#define STOCK_TOOLS "ENVARC:OpenAmiga/StockDefaultTools"
#define REPAIR_LOG "ENVARC:OpenAmiga/DefaultTools-repair.log"
#define REPAIRED_MARK "ENVARC:OpenAmiga/DefaultTools-repaired"

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

/* One icon (path without .info): its default tool goes back to orig when it is OpenView now. */
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
    if (cur && same_program((const char *)cur, "OpenView") && !same_program(orig, "OpenView")) {
        if (!all && is_default_icon(path) && same_program(orig, "MultiView")) {
            int i, seen = 0;
            for (i = 0; i < fix_nkept; i++) if (same_word(fix_kept_paths[i], path)) seen = 1;     /* once, not for the table too */
            if (!seen) {
                fix_kept++;
                if (fix_nkept < 64 && strlen(path) < sizeof fix_kept_paths[0]) strcpy(fix_kept_paths[fix_nkept++], path);
                fix_say("kept: ", path, " opens with OpenView, as the switch of kinds of file made it", NULL);
            }
        } else {
            int ok;
            dob->do_DefaultTool = (STRPTR)orig;
            ok = PutIconTags((STRPTR)path, dob, ICONPUTA_NotifyWorkbench, TRUE, TAG_DONE) != 0;
            dob->do_DefaultTool = cur;
            if (ok) {
                fix_restored++;
                fix_say("restored: ", path, ": OpenView -> ", orig);
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
        say("OpenUp: icon.library 44 or later is needed to check the icons' default tools.", NULL, NULL);
        return RETURN_WARN;
    }
    fix_say("FIXTOOLS", all ? " ALL" : "", NULL, NULL);
    /* 1. what OpenTypes recorded */
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
                fix_icon(path, orig, all, "its original, kept by OpenTypes");
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
    Printf((STRPTR)"OpenUp: default tools: %ld icon%s checked, %ld put back, %ld kept on OpenView (kinds of file), %ld failed\n",
           (LONG)fix_checked, (LONG)(fix_checked == 1 ? "" : "s"), (LONG)fix_restored, (LONG)fix_kept, (LONG)fix_failed);
    /* the log, kept: what was put back is there to read, and the Installer's T:OpenUp.log has it too */
    if (fix_restored || fix_kept || fix_failed) {
        PutStr((STRPTR)fix_log);
        if (exists("ENVARC:OpenAmiga")) {
            BPTR fh = Open((STRPTR)REPAIR_LOG, MODE_READWRITE);
            if (fh) { Seek(fh, 0, OFFSET_END); Write(fh, fix_log, fix_log_len); Close(fh); }
        }
    }
    return fix_failed ? RETURN_WARN : RETURN_OK;
}

/* ---- UNINSTALL ----------------------------------------------------------------------------- */

static int uninstall(void)
{
    int i, problems = 0, restored = 0, removed = 0;
    manifest_load();
    if (!nents && !exists(MANIFEST)) {
        say("OpenUp isn't installed here: there is no list of its files (" MANIFEST ").", NULL, NULL);
        return RETURN_WARN;
    }
    /* default tools still on OpenView (OpenTypes' UNDO has run before this when the Uninstall-OpenUp script
     * does it): put back, with the kinds of file too, before OpenView goes */
    if (exists(STOCK_TOOLS)) fix_tools(STOCK_TOOLS, 1);
    if (!write_startup(1)) {
        say("OpenUp: can't take its block out of S:User-Startup. Remove the lines from \"" BLOCK_BEGIN "\" to \"" BLOCK_END "\" by hand.", NULL, NULL);
        problems++;
    }
    for (i = nents - 1; i >= 0; i--) {
        entry *e = &ents[i];
        if (!strcmp(e->kind, "new") || !strcmp(e->kind, "newfile") || !strcmp(e->kind, "rnew")) {
            if (!strcmp(e->kind, "newfile")) {
                LONG len = 0;
                char *t = read_file(e->a, &len);
                int blank = 1;
                for (LONG k = 0; t && k < len; k++) if (t[k] != '\n' && t[k] != ' ') { blank = 0; break; }
                if (t) FreeVec(t);
                if (!blank) continue;                       /* someone else wrote in it since */
            }
            if (DeleteFile((STRPTR)e->a)) removed++;
            else if (exists(e->a)) {
                /* In use, as Workbench's backdrop picture is until it has read
                 * the setting put back above: give it up to ten seconds. */
                int tries;
                for (tries = 0; tries < 20 && !DeleteFile((STRPTR)e->a) && exists(e->a); tries++)
                    Delay(25);
                if (exists(e->a)) { say("OpenUp: can't delete ", e->a, ". Is it in use? Delete it by hand."); problems++; }
                else removed++;
            }
        } else if (!strcmp(e->kind, "moved")) {                /* RECONCILE's: moved back */
            LONG ok = exists(e->a) ? 0 : Rename((STRPTR)e->b, (STRPTR)e->a);
            if (ok) restored++;
            else { say("OpenUp: can't put back ", e->a, "; it is in SYS:Prefs/Classic or SYS:Storage/OpenUp-Superseded"); problems++; }
        } else if (!strcmp(e->kind, "replaced")) {
            if (copy_file(e->b, e->a, 0)) { DeleteFile((STRPTR)e->b); restored++; }
            else { say("OpenUp: can't put back ", e->a, "; its original is in SYS:Storage/OpenUp-Superseded."); problems++; }
        }
    }
    for (i = nents - 1; i >= 0; i--)
        if (!strcmp(ents[i].kind, "dir"))
            DeleteFile((STRPTR)ents[i].a);            /* only an empty one goes */
    /* The originals' drawers, deepest first: they go once empty. */
    for (i = nents - 1; i >= 0; i--)
        if (!strcmp(ents[i].kind, "replaced")) {
            char dir[ENTRY_PATH];
            if (strlcpy(dir, ents[i].b, sizeof dir) >= sizeof dir) continue;   /* too long to be OpenUp's */
            for (;;) {
                char *s = strrchr(dir, '/');
                if (!s || strlen(dir) <= strlen(SUPERSEDED)) break;
                *s = 0;
                if (!DeleteFile((STRPTR)dir)) break;
            }
        }
    DeleteFile((STRPTR)SUPERSEDED);
    if (!problems) {
        DeleteFile((STRPTR)REPAIRED_MARK);
        DeleteFile((STRPTR)REPAIR_LOG);
        DeleteFile((STRPTR)MANIFEST);
        DeleteFile((STRPTR)MANIFEST_ENV);
        DeleteFile((STRPTR)"ENVARC:OpenAmiga");
        DeleteFile((STRPTR)"ENV:OpenAmiga");
    }
    Printf((STRPTR)"OpenUp: uninstalled: %ld file%s removed, %ld put back as they were%s\n", removed, (LONG)(removed == 1 ? "" : "s"),
           restored, (LONG)(problems ? "; see the messages above" : ". Reboot to finish."));
    return problems ? RETURN_ERROR : RETURN_OK;
}

/* ---- BACKDROP ------------------------------------------------------------------------------ */

#define WBPATTERN "ENVARC:Sys/WBPattern.prefs"
#define WBPATTERN_ENV "ENV:Sys/WBPattern.prefs"

static ULONG get32(const UBYTE *p) { return (ULONG)p[0] << 24 | (ULONG)p[1] << 16 | (ULONG)p[2] << 8 | p[3]; }
static void put32(UBYTE *p, ULONG v) { p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v; }

/* Workbench's backdrop (the ROOT pattern of WBPattern.prefs) becomes the
 * picture nearest the screen's shape, scaled to fit; the drawer and screen
 * patterns are kept as they are. The old file is kept, as every file OpenUp
 * replaces is. */
static int backdrop(const char *prefix)
{
    static const UWORD sizes[4][2] = { { 1920, 1080 }, { 1920, 1200 }, { 1600, 1200 }, { 1280, 1024 } };
    char pic[220];
    struct Screen *scr = LockPubScreen((STRPTR)"Workbench");
    long sw = scr ? scr->Width : 640, sh = scr ? scr->Height : 480, best = -1, bestd = 0;
    int i;
    LONG oldlen = 0, len;
    UBYTE *old, *buf, *o;
    UWORD flags = 0x3a00;                                    /* scale (good), image precision, good dither */
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
        say("OpenUp: no backdrop pictures named ", prefix, "-<width>x<height>.png. Install the Backdrops part first.");
        return RETURN_ERROR;
    }
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
    i = write_file("T:OpenUp-wbpattern", (char *)buf, len)
        && install_file("T:OpenUp-wbpattern", WBPATTERN, 0) >= 0
        && install_file("T:OpenUp-wbpattern", WBPATTERN_ENV, 0) >= 0
        && manifest_save();
    DeleteFile((STRPTR)"T:OpenUp-wbpattern");
    FreeVec(buf);
    if (!i)
        return RETURN_ERROR;
    say("OpenUp: Workbench's backdrop is ", pic, "");
    return RETURN_OK;
}

/* ---- FONT and TITLEBARS: OS 3.2's own prefs files, changed in one place ------------------ */

/* Puts a prefs file built in memory in place in ENVARC:, the original kept
 * as any file OpenUp replaces. Not ENV:: IPrefs would reset the Workbench
 * screen at once, while the Installer's window is open on it ("Intuition is
 * attempting to reset the Workbench screen"); ENV: gets it at the next boot. */
static int install_prefs(const char *name, const UBYTE *data, LONG len)
{
    char envarc[64];
    int ok;
    strcpy(envarc, "ENVARC:Sys/"); strcat(envarc, name);
    manifest_load();
    ensure_dir("ENVARC:Sys", 0);
    ok = write_file("T:OpenUp-prefs", (const char *)data, len)
         && install_file("T:OpenUp-prefs", envarc, 0) >= 0
         && manifest_save();
    DeleteFile((STRPTR)"T:OpenUp-prefs");
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
 * icons' text (fp_Type 0, FP_WBFONT) in the same font and size. OpenUp owns
 * the desktop's text, so the icons no longer keep Topaz 8 in a filled box
 * under the new screen text (the user, 10 October 2026): the label is drawn
 * as text only (JAM1) in the pens it had; a shadow or an outline chosen in
 * OpenPrefs Look is kept. The system font (fp_Type 1, fixed width for the
 * Shell) stays as it is. */
static int set_font(const char *font, LONG size)
{
    LONG oldlen = 0, k, len;
    UBYTE *old = (UBYTE *)read_file("ENVARC:Sys/Font.prefs", &oldlen), *buf, *o;
    int front = 1, back = 0, special = 0, ok;
    if (size < 6 || size > 64 || strlen(font) > 120) { say("OpenUp: FONT wants a font name and a size from 6 to 64", NULL, NULL); if (old) FreeVec(old); return RETURN_FAIL; }
    if (!(buf = prefs_start(old, oldlen, 2 * (FONT_CHUNK + 8), &o))) { if (old) FreeVec(old); return RETURN_FAIL; }
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
    len = o - buf;
    put32(buf + 4, len - 8);
    if (old) FreeVec(old);
    ok = install_prefs("Font.prefs", buf, len);
    FreeVec(buf);
    if (!ok) return RETURN_ERROR;
    say("OpenUp: the screens' and the icons' text are ", font, " (from the next reboot)");
    return RETURN_OK;
}

#define ICTL_CHUNK 38                /* struct IControlPrefs, version 2 */

/* Title bar extra height: the last two bytes of ic_GUIGeometry (screen,
 * window), read by IPrefs only when ic_Flags has ICF_VERSIONED. */
static int set_titlebars(LONG screen, LONG window)
{
    static const UBYTE defaults[ICTL_CHUNK] = {    /* as IControl saves with nothing changed */
        0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  0,0x32, 0,0x40,  0x80,0,0,0x1e,  'N','M','V','B',  0,2, 0,0, 0,0, 0,0,0,0 };
    LONG oldlen = 0, k, len;
    UBYTE *old = (UBYTE *)read_file("ENVARC:Sys/IControl.prefs", &oldlen), *buf, *o, *mine = NULL;
    int ok;
    if (screen < 0 || screen > 16 || window < 0 || window > 16) { say("OpenUp: TITLEBARS wants two numbers from 0 to 16", NULL, NULL); if (old) FreeVec(old); return RETURN_FAIL; }
    if (!(buf = prefs_start(old, oldlen, ICTL_CHUNK + 8, &o))) { if (old) FreeVec(old); return RETURN_FAIL; }
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
    len = o - buf;
    put32(buf + 4, len - 8);
    if (old) FreeVec(old);
    ok = install_prefs("IControl.prefs", buf, len);
    FreeVec(buf);
    if (!ok) return RETURN_ERROR;
    say("OpenUp: title bars have extra height (from the next reboot)", NULL, NULL);
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
    say("OpenUp: Workbench is a backdrop (from the next reboot)", NULL, NULL);
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
    say("OpenUp: drives' windows have no fill gauge (from the next reboot)", NULL, NULL);
    return RETURN_OK;
}

/* ---- RECONCILE: one desktop, the Open one ---------------------------------------------- */

/* The user, 10 October 2026: "we own the config, and the legacy is just that". RECONCILE makes the desktop
 * coherent in one pass, from one map: each old tool is moved where it no longer competes with its Open
 * home, and nothing is ever deleted.
 *   classic  an OS prefs editor goes to SYS:Prefs/Classic (one drawer in, still runs from there); its
 *            settings files stay, since programs read them
 *   retire   goes to SYS:Storage/OpenUp-Superseded, only once its Open replacement is on the disk
 *   replace  goes to SYS:Storage/OpenUp-Superseded and its Open replacement takes its name, so every icon
 *            and script that starts it starts the replacement (TextEdit: OpenEdit; no icon is changed)
 *   wait     stays: its Open home is still to build (listed, so the list is the build list)
 *   keep     stays: the OS's own, and wanted (said why)
 * Each move is a "moved" line in the manifest: RECONCILE UNDO and UNINSTALL put it back.
 * It also keeps the settings OpenUp owns in step: the icons' text is the screens' text (Font prefs), in a
 * light colour with a shadow over a dark backdrop and a dark one over a light backdrop (the backdrop's tone
 * from its theme's desktop colours), and the Open apps are on the dock (Dock ADD).
 *   OpenUpTool RECONCILE [CHECK]       says what it would do (changes nothing)
 *   OpenUpTool RECONCILE APPLY [ONCE]  does it; ONCE (at boot) only when this map is newer than the last run
 *   OpenUpTool RECONCILE UNDO          puts back everything RECONCILE moved */
#define RECONCILE_MAP_VERSION "1"
#define RECONCILE_DONE "ENVARC:OpenAmiga/Reconcile.done"
#define CLASSIC "SYS:Prefs/Classic"

typedef struct { const char *path, *action, *home, *what; } rmap;
static const rmap reconcile_map[] = {
    { "SYS:Prefs/Asl",          "classic", NULL, "OpenLook: file requesters (to build)" },
    { "SYS:Prefs/Font",         "classic", NULL, "OpenLook: fonts (to build); OpenUp sets the screens' and icons' text" },
    { "SYS:Prefs/Palette",      "classic", NULL, "OpenLook: colours (to build)" },
    { "SYS:Prefs/Pointer",      "classic", NULL, "OpenLook: pointer, with OpenMouseAccel (to build); the old editor crashes on this desktop" },
    { "SYS:Prefs/ReAction",     "classic", NULL, "OpenLook: ReAction's look (to build)" },
    { "SYS:Prefs/WBPattern",    "classic", NULL, "OpenLook: backdrop (to build)" },
    { "SYS:Prefs/Workbench",    "classic", NULL, "OpenLook: Workbench (to build)" },
    { "SYS:Prefs/ScreenMode",   "classic", NULL, "OpenLook Screen: mode and overscan (to build)" },
    { "SYS:Prefs/Overscan",     "classic", NULL, "OpenLook Screen: mode and overscan (to build)" },
    { "SYS:Prefs/Input",        "classic", NULL, "OpenInput's editor: input and IControl (to build)" },
    { "SYS:Prefs/IControl",     "classic", NULL, "OpenInput's editor: input and IControl (to build)" },
    { "SYS:Prefs/Time",         "classic", NULL, "Open Time and Locale (to build)" },
    { "SYS:Prefs/Locale",       "classic", NULL, "Open Time and Locale (to build)" },
    { "SYS:Prefs/Printer",      "classic", NULL, "OpenPrint's printer settings (to build)" },
    { "SYS:Prefs/PrinterGfx",   "classic", NULL, "OpenPrint's printer settings (to build)" },
    { "SYS:Prefs/PrinterPS",    "classic", NULL, "OpenPrint's printer settings (to build)" },
    { "SYS:Prefs/DefaultIcons", "classic", "SYS:Prefs/OpenTypes", "OpenTypes, which takes over default icons" },
    { "SYS:Prefs/Serial",       "classic", "SYS:Prefs/OpenSerial", "OpenSerial" },
    { "SYS:Prefs/Sound",        "classic", "SYS:Prefs/OpenSound", "OpenSound (the Sound part; a Sound in Prefs is the OS's once OpenSound is there)" },
    { "SYS:Tools/Commodities/Blanker", "retire", "SYS:Tools/Commodities/OpenBlanker", "OpenBlanker" },
    { "SYS:Tools/Calculator",   "wait",    NULL, "OpenCalc (source in OpenUp, to build)" },
    { "SYS:Tools/TextEdit",     "replace", "C:OpenEdit", "OpenEdit, under TextEdit's name, so every icon that opens TextEdit opens OpenEdit" },
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
    if (exists(b)) { say("OpenUp: ", b, " is there already; left both as they are."); return -1; }
    parent_of(b, dir, sizeof dir);
    if (!ensure_dir(dir, 1)) { say("OpenUp: can't make ", dir, "."); return -1; }
    ok = Rename((STRPTR)a, (STRPTR)b);
    if (!ok) {
        if (!copy_file(a, b, 0)) { say("OpenUp: can't move ", a, "; left in place."); return -1; }
        ok = DeleteFile((STRPTR)a);
        if (!ok) { DeleteFile((STRPTR)b); say("OpenUp: can't move ", a, ". Is it running?"); return -1; }
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
 * ENVARC:OpenAmiga/Backdrop.tone, as decoding a big picture takes a moment on a 68k. */
#define TONE_CACHE "ENVARC:OpenAmiga/Backdrop.tone"
static int picture_tone(const char *name)
{
    struct Library *DataTypesBase;
    Object *o;
    struct BitMapHeader *bmh = NULL;
    ULONG *row;
    LONG len = 0, w, h, y, x, sum = 0, count = 0;
    char *cache, side[300];
    int tone = -1, nl = (int)strlen(name);
    /* a .tone file beside the picture ("dark" or "light"), as OpenUp's backdrops carry */
    strlcpy(side, name, sizeof side);
    str_cat(side, ".tone", sizeof side);
    if ((cache = read_file(side, &len))) {
        tone = len > 0 && (cache[0] == 'd' || cache[0] == 'D') ? 1 : len > 0 ? 0 : -1;
        FreeVec(cache);
        if (tone >= 0) return tone;
    }
    /* never a big picture at boot: decoding a full-screen PNG takes minutes on a 68k, and S:User-Startup
     * waited for it (the black screen of 10 October 2026, 18:15). Only OpenUp's strips (16 pixels wide). */
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
        ensure_dir("ENVARC:OpenAmiga", 1);
        write_file(TONE_CACHE, buf, strlen(buf));
    }
    return tone;
}

/* The backdrop's tone: 1 dark, 0 light, -1 not known. OpenUp's backdrops are named <Theme>-<mode>[-chrome]-...
 * (opengadtools' make_backdrops.py); the theme's desktop colours for that mode give the tone: the mean of
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
        PutStr((STRPTR)"Usage: OpenUpTool RECONCILE [CHECK | APPLY [ONCE] | UNDO]\n");
        return RETURN_FAIL;
    }
    manifest_load();
    if (undo) {
        for (i = nents - 1; i >= 0; i--) {
            entry *e = &ents[i];
            LONG ok;
            if (!strcmp(e->kind, "rnew")) {             /* a replacement RECONCILE put in an old program's place */
                if (DeleteFile((STRPTR)e->a) || !exists(e->a)) e->kind[0] = 0;
                else { say("OpenUp: can't take away ", e->a, "."); failed++; }
                continue;
            }
            if (strcmp(e->kind, "moved")) continue;
            if (exists(e->a)) { say("OpenUp: ", e->a, " is back already; its moved copy stays where it is."); failed++; continue; }
            ok = Rename((STRPTR)e->b, (STRPTR)e->a);
            if (ok) { e->kind[0] = 0; moved++; }
            else { say("OpenUp: can't put back ", e->a, "."); failed++; }
        }
        DeleteFile((STRPTR)RECONCILE_DONE);
        if (!manifest_save()) return RETURN_ERROR;
        Printf((STRPTR)"OpenUp: %ld put back where they were\n", (LONG)moved);
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
    Printf((STRPTR)"OpenUp: reconcile (map %s)%s\n", (LONG)RECONCILE_MAP_VERSION, (LONG)(apply ? "" : ": what it would do; APPLY does it"));
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
                } else { say("OpenUp: can't put ", m->home, " in its place; RECONCILE UNDO puts the old one back."); failed++; }
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
    /* the Open apps on the dock (the user, 10 October 2026): Dock ADD adds only those installed and not there */
    if (exists("SYS:Prefs/OpenDock")) {                 /* 0.7.1: the editor is OpenDock (every editor of ours is Open...) */
        if (!apply) PutStr((STRPTR)"  dock: OpenWrite, OpenPlay, OpenCompress, OpenFTP and OpenPuTTY added if installed and missing\n");
        else {
            BPTR nil = Open((STRPTR)"NIL:", MODE_NEWFILE);
            LONG r = SystemTags((STRPTR)"SYS:Prefs/OpenDock ADD SYS:Utilities/OpenFiles SYS:Utilities/OpenPrint/OpenView "
                                "SYS:Utilities/OpenWrite/OpenWrite SYS:Utilities/OpenPlay SYS:Utilities/OpenCompress "
                                "SYS:Utilities/OpenFTP/OpenFTP SYS:Utilities/OpenPuTTY/OpenPuTTY SAVE",
                                SYS_Input, (ULONG)nil, SYS_Output, (ULONG)NULL, TAG_DONE);
            if (nil) Close(nil);
            PutStr((STRPTR)(r == 0 ? "  dock: the Open apps are on it\n" : "  dock: Dock ADD didn't run; the dock is as it was\n"));
        }
    }
    if (apply) {
        if (!manifest_save()) return RETURN_ERROR;
        if (!failed) write_file(RECONCILE_DONE, RECONCILE_MAP_VERSION "\n", strlen(RECONCILE_MAP_VERSION) + 1);
        Printf((STRPTR)"OpenUp: %ld moved, %ld waiting for their Open home%s\n", (LONG)moved, (LONG)waiting,
               (LONG)(moved ? ". Choose Update in the drawers to see it." : ""));
    }
    return failed ? RETURN_WARN : RETURN_OK;
}

/* ---- TIDY: drawers that open looking finished --------------------------------------------- */

/* The user, 10 October 2026: "the windows look pants when opening because we haven't sized them and made them
 * pretty". TIDY has Workbench itself clean up each of the system's drawers (by name), size the window to fit
 * its icons (never larger than 800 x 600 or the screen), and snapshot it all, so every drawer opens neat, in
 * the screen's own font. Through Workbench's ARexx port (WORKBENCH), as OpenMenus talks to it; nothing of
 * Workbench's is patched.
 *   OpenUpTool TIDY [ONCE]    ONCE (at boot): waits for Workbench, and does it once per tidy version */
#define TIDY_VERSION "1"
#define TIDY_DONE "ENVARC:OpenAmiga/Tidy.done"
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
    if (scrw <= 0 || scrh <= 0) { say("OpenUp: Workbench isn't answering; nothing tidied.", NULL, NULL); return RETURN_WARN; }
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
    if (!done) { say("OpenUp: Workbench didn't open any drawer for TIDY; nothing tidied, it tries again next time.", NULL, NULL); return RETURN_WARN; }
    ensure_dir("ENVARC:OpenAmiga", 1);
    write_file(TIDY_DONE, TIDY_VERSION "\n", strlen(TIDY_VERSION) + 1);
    Printf((STRPTR)"OpenUp: %ld drawers tidied: cleaned up by name, sized to fit, snapshotted\n", (LONG)done);
    return RETURN_OK;
}

/* ---- ICONS: every icon in the house style ------------------------------------------------------ */

/* The user, 10 October 2026: an icon find and replace that hunts for icons that aren't AmigaChrome's and
 * offers ours, with the drawers tidied after. ICONS walks SYS: and Work: (three drawers deep, OpenUp's own
 * Storage and Parts left out) for old icons: the classic four-colour pictures and NewIcons, anything that
 * isn't an OS 3.5 colour icon or a PNG icon and isn't one OpenUp gave already. Each gets a new face, the
 * icon keeping its type, tool, tool types, stack, place and drawer window (face_icon, as the install does):
 *   - by its name, from the Gorilla set OpenUp carries (Work:OpenUp/Parts/GorillaIcons/Icons/<name>.info);
 *   - else by its kind, from the system's default icons (ENVARC:Sys/def_Drawer, def_Tool, def_Project...),
 *     which OpenUp has given Gorilla's faces.
 * The originals go to SYS:Storage/OpenUp-Superseded; UNINSTALL puts them back.
 *   OpenUpTool ICONS [CHECK]    lists what it would change
 *   OpenUpTool ICONS APPLY      changes them, then has Workbench tidy the system's drawers (TIDY) */
#define ICON_LIB "Work:OpenUp/Parts/GorillaIcons/Icons"
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

static const char *kind_default(int type)
{
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
                    if (depth < 3 && strcmp(n, "OpenUp-Superseded") && strcmp(n, "Parts") && strcmp(n, "T") &&
                        strcmp(n, "Trashcan") && strncmp(n, ".", 1))
                        icons_walk(sub, depth + 1);
                    continue;
                }
                if (l <= 5 || !same_word(n + l - 5, ".info") || find_entry(NULL, sub) || !old_icon(sub, &type)) continue;
                /* the replacement: by name from Gorilla's set, else by kind */
                strlcpy(lib, ICON_LIB "/", sizeof lib);
                str_cat(lib, n, sizeof lib);
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
    if (how && !apply && !same_word(how, "CHECK")) { PutStr((STRPTR)"Usage: OpenUpTool ICONS [CHECK | APPLY]\n"); return RETURN_FAIL; }
    manifest_load();
    icon_count = 0;
    icons_walk("SYS:", 0);
    if (exists("Work:")) icons_walk("Work:", 0);
    if (!icon_count) { PutStr((STRPTR)"OpenUp: every icon is in the house style already\n"); return RETURN_OK; }
    for (i = 0; i < icon_count; i++) {
        if (!apply) { Printf((STRPTR)"  old: %s  (new face: %s)\n", (LONG)icon_found[i], (LONG)icon_from[i]); continue; }
        if (face_icon(icon_from[i], icon_found[i]) > 0) { Printf((STRPTR)"  new face: %s\n", (LONG)icon_found[i]); done++; }
        else failed++;
    }
    if (!apply) { Printf((STRPTR)"OpenUp: %ld old icons; ICONS APPLY gives them our faces\n", (LONG)icon_count); return RETURN_OK; }
    if (!manifest_save()) return RETURN_ERROR;
    Printf((STRPTR)"OpenUp: %ld icons given our faces, %ld left as they were\n", (LONG)done, (LONG)failed);
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
    if (!nents) { PutStr((STRPTR)"OpenUp isn't installed here.\n"); return RETURN_WARN; }
    for (i = 0; i < nents; i++)
        if (!strcmp(ents[i].kind, "part")) Printf((STRPTR)"%s %s\n", (LONG)ents[i].a, (LONG)ents[i].b);
        else if (!strcmp(ents[i].kind, "new") || !strcmp(ents[i].kind, "replaced")) files++;
    Printf((STRPTR)"%ld files are OpenUp's (" MANIFEST ")\n", files);
    return RETURN_OK;
}

/* ---- UPDATE: a newer pack from Cradle (amigachrome docs/design/AutoDocs-OpenUp-Update.md) ---- */

#define CRADLE_DRAWER "DH1:OpenUp"
#define CRADLE_PACK "DH1:OpenUp/Cradle-Pack"
#define RELEASES "https://github.com/DalsinAI/openamigaup/releases"

/* Versions compare as numbers, dot by dot: 0.6.14 is newer than 0.6.9. */
static int version_cmp(const char *a, const char *b)
{
    while (*a || *b) {
        char *ea, *eb;
        long x = strtol(a, &ea, 10), y = strtol(b, &eb, 10);
        if (x != y) return x < y ? -1 : 1;
        a = *ea == '.' ? ea + 1 : ea;
        b = *eb == '.' ? eb + 1 : eb;
        if (a == ea && b == eb) break;          /* neither moved on: the rest isn't a number */
    }
    return 0;
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

/* UPDATE [CHECK|YES]: CHECK only says (WARN when a newer pack is waiting);
 * plain UPDATE shows what will change and asks; YES doesn't ask (scripts).
 * It never runs on its own, and it never changes anything until asked. */
static int update(const char *how)
{
    char newer[24], installed[24] = "";
    entry *e;
    int check = how && same_word(how, "CHECK"), yes = how && same_word(how, "YES");
    manifest_load();
    if ((e = find_entry("part", "OpenUp")) != NULL)
        strlcpy(installed, e->b, sizeof installed);
    if (!installed[0]) {
        say("OpenUp isn't installed here, so there is nothing to update. Run Install-OpenUp to install it.", NULL, NULL);
        return RETURN_WARN;
    }
    if (!cradle_version(newer, sizeof newer)) {
        say("OpenUp ", installed, " is installed. No newer OpenUp from Cradle: this Amiga isn't an AmigaChrome instance with OpenUp's drawer (DH1:OpenUp).");
        say("On a real Amiga, newer packs are at " RELEASES ": unpack one and run its Install-OpenUp.", NULL, NULL);
        return RETURN_OK;
    }
    if (version_cmp(newer, installed) <= 0) {
        say("OpenUp ", installed, " is installed, and it is the newest Cradle has. Nothing to do.");
        return RETURN_OK;
    }
    Printf((STRPTR)"OpenUp %s is waiting in " CRADLE_DRAWER " (%s is installed).\n", (LONG)newer, (LONG)installed);
    if (check) return RETURN_WARN;
    if (!show_changes()) say("  (the same parts, updated in place)", NULL, NULL);
    if (!yes) {
        struct EasyStruct es = { sizeof(struct EasyStruct), 0, (UBYTE *)"OpenUp Update",
            (UBYTE *)"OpenUp %s is ready to install (%s is installed now).\n\n"
                     "OpenUp's Installer runs with your answers from last time;\n"
                     "what you have changed yourself stays.",
            (UBYTE *)"Not now|Update" };
        LONG answer = EasyRequest(NULL, &es, NULL, (LONG)newer, (LONG)installed);
        if (answer != 0) {           /* "Not now" is 1, the left; "Update" is 0, the right */
            say("Not updated. OpenUpTool UPDATE asks again whenever you like.", NULL, NULL);
            return RETURN_OK;
        }
    }
    {
        const char *inst = exists("C:Installer") ? "C:Installer" : exists("SYS:System/Installer") ? "SYS:System/Installer" : "SYS:Utilities/Installer";
        char cmd[200];
        LONG rc;
        strcpy(cmd, "CD " CRADLE_DRAWER "\n");
        strcat(cmd, inst);
        strcat(cmd, " Install-OpenUp APPNAME OpenUp NOLOG\n");       /* no install_log_file left in SYS: */
        if (!write_file("T:OpenUp-update", cmd, strlen(cmd))) {
            say("OpenUp: can't write T:OpenUp-update. Is T: full?", NULL, NULL);
            return RETURN_ERROR;
        }
        rc = SystemTagList((STRPTR)"Execute T:OpenUp-update", NULL);
        DeleteFile((STRPTR)"T:OpenUp-update");
        return rc == 0 ? RETURN_OK : RETURN_WARN;
    }
}

static int real_main(void);

/* OpenUpTool needs a big stack: FIXTOOLS reads and writes icons through icon.library, which takes far more
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
        PutStr((STRPTR)"Usage: OpenUpTool DETECT | INSTALL <part drawer> | STARTUP | UNINSTALL | FIXTOOLS <table> [ALL] | LIST | SETTING <dest> <text> | BACKDROP <prefix> | FONT <font> <size> | TITLEBARS <screen> <window> | WBBACKDROP | WBGAUGE | RECONCILE [CHECK|APPLY [ONCE]|UNDO] | TIDY [ONCE] | ICONS [CHECK|APPLY] | UPDATE [CHECK|YES]\n");
        return RETURN_FAIL;
    }
    if (!(ExpansionBase = (struct ExpansionBase *)OpenLibrary((STRPTR)"expansion.library", 37))) {
        FreeArgs(rda);
        return RETURN_FAIL;
    }
    cmd = (const char *)args[0];
    if (same_word(cmd, "DETECT")) rc = detect();
    else if (same_word(cmd, "INSTALL") && args[1]) rc = install((const char *)args[1]);
    else if (same_word(cmd, "STARTUP")) rc = write_startup(0) ? RETURN_OK : (say("OpenUp: can't write S:User-Startup. Is SYS: write-protected?", NULL, NULL), RETURN_ERROR);
    else if (same_word(cmd, "UNINSTALL")) rc = uninstall();
    else if (same_word(cmd, "FIXTOOLS") && args[1]) rc = fix_tools((const char *)args[1], args[2] && same_word((const char *)args[2], "ALL"));
    else if (same_word(cmd, "LIST")) rc = list();
    else if (same_word(cmd, "BACKDROP") && args[1]) rc = backdrop((const char *)args[1]);
    else if (same_word(cmd, "FONT") && args[1] && args[2]) rc = set_font((const char *)args[1], atol((const char *)args[2]));
    else if (same_word(cmd, "TITLEBARS") && args[1] && args[2]) rc = set_titlebars(atol((const char *)args[1]), atol((const char *)args[2]));
    else if (same_word(cmd, "WBBACKDROP")) rc = set_wb_backdrop();
    else if (same_word(cmd, "WBGAUGE")) rc = set_wb_gauge();
    else if (same_word(cmd, "RECONCILE")) rc = reconcile((const char *)args[1], (const char *)args[2]);
    else if (same_word(cmd, "TIDY")) rc = tidy((const char *)args[1]);
    else if (same_word(cmd, "ICONS")) rc = icons((const char *)args[1]);
    else if (same_word(cmd, "SETTING") && args[1] && args[2]) {
        /* The text goes through T: so it is installed like any file. */
        char line[200];
        strncpy(line, (const char *)args[2], sizeof line - 2);
        line[sizeof line - 2] = 0;
        strcat(line, "\n");
        manifest_load();
        if (!write_file("T:OpenUp-setting", line, strlen(line))) rc = RETURN_ERROR;
        else {
            rc = install_file("T:OpenUp-setting", (const char *)args[1], 0) < 0 ? RETURN_ERROR : RETURN_OK;
            DeleteFile((STRPTR)"T:OpenUp-setting");
            if (rc == RETURN_OK && !manifest_save()) rc = RETURN_ERROR;
        }
    }
    else if (same_word(cmd, "P96LEFTOVER")) {
        /* Picasso96API.library in LIBS: with no Picasso96 board running (its
         * monitor driver would have opened rtg.library at boot) hangs SDL's
         * video start (6 Oct 2026). AmigaChrome's instances are never asked. */
        int rtg;
        Forbid();
        rtg = FindName(&SysBase->LibList, (STRPTR)"rtg.library") != NULL;
        Permit();
        rc = (!is_amigachrome() && !rtg && exists("LIBS:Picasso96API.library")) ? RETURN_WARN : RETURN_OK;
    }
    else if (same_word(cmd, "UPDATE")) rc = update((const char *)args[1]);
    else if (same_word(cmd, "RETIRE") && args[1]) {
        manifest_load();
        rc = retire_file((const char *)args[1]) < 0 ? RETURN_ERROR : RETURN_OK;
        if (rc == RETURN_OK && !manifest_save()) rc = RETURN_ERROR;
    }
    else { PutStr((STRPTR)"Usage: OpenUpTool DETECT | INSTALL <part drawer> | STARTUP | UNINSTALL | FIXTOOLS <table> [ALL] | LIST | SETTING <dest> <text> | BACKDROP <prefix> | FONT <font> <size> | TITLEBARS <screen> <window> | WBBACKDROP | WBGAUGE | RECONCILE [CHECK|APPLY [ONCE]|UNDO] | TIDY [ONCE] | ICONS [CHECK|APPLY] | UPDATE [CHECK|YES]\n"); rc = RETURN_FAIL; }
    CloseLibrary((struct Library *)ExpansionBase);
    FreeArgs(rda);
    (void)version;
    return rc;
}
