/* OpenUpTool: what OpenUp's Installer scripts call (DESIGN.md section 3).
 *
 *   OpenUpTool DETECT                 what this machine is; ENV:OpenUp/...
 *   OpenUpTool INSTALL <part drawer>  installs one part, as its Part file says
 *   OpenUpTool STARTUP                writes the marked block in S:User-Startup
 *   OpenUpTool UNINSTALL              puts everything back
 *   OpenUpTool LIST                   what is installed
 *   OpenUpTool SETTING <dest> <text>  a setting the Installer asked for, as a file
 *   OpenUpTool BACKDROP <prefix>      Workbench's backdrop: <prefix>-strip-<H>.png for the
 *                                     screen's height, tiled, when there is one; else <prefix>-<W>x<H>.png, the
 *                                     one nearest the screen's shape, scaled to fit
 *   OpenUpTool FONT <font> <size>     the screens' font (Font prefs, "Screen text")
 *   OpenUpTool TITLEBARS <screen> <window>  extra pixels in the title bars (IControl
 *                                     prefs, Appearance Settings, Title Bar Extra Height)
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
 *   ";OFF " in front, so AmigaDOS skips it.
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
 *   startup <the rest of the line>
 * Words with spaces are quoted.
 *
 * MIT, Copyright (c) 2026 Dalsin Limited. */
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <libraries/configvars.h>
#include <libraries/expansionbase.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/expansion.h>
#include <proto/intuition.h>
#include <intuition/screens.h>

#include <string.h>
#include <stdlib.h>

static const char version[] = "$VER: OpenUpTool 0.3 (6.10.2026) MIT, Copyright (c) 2026 Dalsin Limited";

struct ExpansionBase *ExpansionBase;

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

static void say(const char *a, const char *b, const char *c)
{
    PutStr((STRPTR)a);
    if (b) PutStr((STRPTR)b);
    if (c) PutStr((STRPTR)c);
    PutStr((STRPTR)"\n");
}

static int exists(const char *path)
{
    BPTR l = Lock((STRPTR)path, ACCESS_READ);
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
 *   newfile <path> (a file OpenUp made, such as S:User-Startup) */
#define MAXE 1024
typedef struct { char kind[10]; char a[160]; char b[200]; } entry;
static entry ents[MAXE];
static int nents;

static void manifest_load(void)
{
    char *text = read_file(MANIFEST, NULL), *p, *line;
    nents = 0;
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
        strncpy(ents[nents].kind, f[0], sizeof ents[0].kind - 1);
        strncpy(ents[nents].a, f[1] ? f[1] : "", sizeof ents[0].a - 1);
        strncpy(ents[nents].b, f[2] ? f[2] : "", sizeof ents[0].b - 1);
        nents++;
    }
    FreeVec(text);
}

static int ensure_dir(const char *path, int record);

static int manifest_save(void)
{
    LONG size = 32, len = 0;
    char *buf, *p;
    int i, ok;
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
    entry *e;
    if (nents >= MAXE)
        return NULL;
    e = &ents[nents++];
    memset(e, 0, sizeof *e);
    strncpy(e->kind, kind, sizeof e->kind - 1);
    strncpy(e->a, a, sizeof e->a - 1);
    if (b) strncpy(e->b, b, sizeof e->b - 1);
    return e;
}

/* ---- files ---------------------------------------------------------------------- */

/* Makes the directories up to path (not a volume), noting those it made. */
static int ensure_dir(const char *path, int record)
{
    char buf[200];
    char *p;
    strncpy(buf, path, sizeof buf - 1);
    buf[sizeof buf - 1] = 0;
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
        if (e) { strcpy(e->kind, "replaced"); strncpy(e->b, backup, sizeof e->b - 1); }
        else add_entry("replaced", dest, backup);
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
                if (pass == 1 && !find_entry("startup", line + 8))
                    add_entry("startup", line + 8, partname);
                continue;
            }
            n = words(line, w, 12);
            if (!n) continue;
            if (!strcmp(w[0], "part") && n >= 3) {
                strncpy(partname, w[1], sizeof partname - 1);
                strncpy(partver, w[2], sizeof partver - 1);
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
    if (refused)
        return RETURN_WARN;
    {
        entry *pe = find_entry("part", partname);
        if (pe) strncpy(pe->b, partver, sizeof pe->b - 1);
        else add_entry("part", partname, partver);
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
 * program's block whose name starts the same way. */
static int is_marker_line(const char *p, const char *e, const char *marker)
{
    size_t ml = strlen(marker);
    while (e > p && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r')) e--;
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

/* ---- UNINSTALL ----------------------------------------------------------------------------- */

static int uninstall(void)
{
    int i, problems = 0, restored = 0, removed = 0;
    manifest_load();
    if (!nents && !exists(MANIFEST)) {
        say("OpenUp isn't installed here: there is no list of its files (" MANIFEST ").", NULL, NULL);
        return RETURN_WARN;
    }
    if (!write_startup(1)) {
        say("OpenUp: can't take its block out of S:User-Startup. Remove the lines from \"" BLOCK_BEGIN "\" to \"" BLOCK_END "\" by hand.", NULL, NULL);
        problems++;
    }
    for (i = nents - 1; i >= 0; i--) {
        entry *e = &ents[i];
        if (!strcmp(e->kind, "new") || !strcmp(e->kind, "newfile")) {
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
            char dir[260];
            strcpy(dir, ents[i].b);
            for (;;) {
                char *s = strrchr(dir, '/');
                if (!s || strlen(dir) <= strlen(SUPERSEDED)) break;
                *s = 0;
                if (!DeleteFile((STRPTR)dir)) break;
            }
        }
    DeleteFile((STRPTR)SUPERSEDED);
    if (!problems) {
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

/* Puts a prefs file built in memory in place in ENVARC: and ENV:, the
 * originals kept as any file OpenUp replaces. */
static int install_prefs(const char *name, const UBYTE *data, LONG len)
{
    char envarc[64], env[64];
    int ok;
    strcpy(envarc, "ENVARC:Sys/"); strcat(envarc, name);
    strcpy(env, "ENV:Sys/"); strcat(env, name);
    manifest_load();
    ensure_dir("ENVARC:Sys", 0);
    ensure_dir("ENV:Sys", 0);
    ok = write_file("T:OpenUp-prefs", (const char *)data, len)
         && install_file("T:OpenUp-prefs", envarc, 0) >= 0
         && install_file("T:OpenUp-prefs", env, 0) >= 0
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

/* The screens' font: the FONT chunk whose fp_Type is 2 (FP_SCREENFONT). */
static int set_font(const char *font, LONG size)
{
    LONG oldlen = 0, k, len;
    UBYTE *old = (UBYTE *)read_file("ENVARC:Sys/Font.prefs", &oldlen), *buf, *o, *mine;
    int ok;
    if (size < 6 || size > 64 || strlen(font) > 120) { say("OpenUp: FONT wants a font name and a size from 6 to 64", NULL, NULL); if (old) FreeVec(old); return RETURN_FAIL; }
    if (!(buf = prefs_start(old, oldlen, FONT_CHUNK + 8, &o))) { if (old) FreeVec(old); return RETURN_FAIL; }
    if (old && oldlen > 12 && !memcmp(old, "FORM", 4)) {
        for (k = 12; k + 8 <= oldlen; ) {
            ULONG n = get32(old + k + 4), whole = 8 + n + (n & 1);
            if (k + (LONG)whole > oldlen) break;
            if (!memcmp(old + k, "FONT", 4) && !(n >= 16 && old[k + 8 + 14] == 0 && old[k + 8 + 15] == 2)) {
                memcpy(o, old + k, whole); o += whole;               /* the icons' and the system font stay */
            }
            k += whole;
        }
    }
    memcpy(o, "FONT", 4); put32(o + 4, FONT_CHUNK); o += 8;
    mine = o;
    memset(mine, 0, FONT_CHUNK);
    mine[15] = 2;                                   /* fp_Type: the screens' font */
    mine[16] = 1; mine[17] = 0; mine[18] = 1;       /* fp_FrontPen, fp_BackPen, fp_DrawMode (JAM2) */
    mine[24] = size >> 8; mine[25] = size & 255;    /* fp_TextAttr.ta_YSize */
    mine[27] = 0x02;                                /* ta_Flags: FPF_DISKFONT */
    strcpy((char *)mine + 28, font);                /* fp_Name */
    o += FONT_CHUNK;
    len = o - buf;
    put32(buf + 4, len - 8);
    if (old) FreeVec(old);
    ok = install_prefs("Font.prefs", buf, len);
    FreeVec(buf);
    if (!ok) return RETURN_ERROR;
    say("OpenUp: the screens' font is ", font, " (from the next reboot)");
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

int main(void)
{
    LONG args[3] = { 0, 0, 0 };
    struct RDArgs *rda = ReadArgs((STRPTR)"COMMAND/A,DRAWER,TEXT", args, NULL);
    const char *cmd;
    int rc;
    if (!rda) {
        PutStr((STRPTR)"Usage: OpenUpTool DETECT | INSTALL <part drawer> | STARTUP | UNINSTALL | LIST | SETTING <dest> <text> | BACKDROP <prefix> | FONT <font> <size> | TITLEBARS <screen> <window>\n");
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
    else if (same_word(cmd, "LIST")) rc = list();
    else if (same_word(cmd, "BACKDROP") && args[1]) rc = backdrop((const char *)args[1]);
    else if (same_word(cmd, "FONT") && args[1] && args[2]) rc = set_font((const char *)args[1], atol((const char *)args[2]));
    else if (same_word(cmd, "TITLEBARS") && args[1] && args[2]) rc = set_titlebars(atol((const char *)args[1]), atol((const char *)args[2]));
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
    else { PutStr((STRPTR)"Usage: OpenUpTool DETECT | INSTALL <part drawer> | STARTUP | UNINSTALL | LIST | SETTING <dest> <text> | BACKDROP <prefix> | FONT <font> <size> | TITLEBARS <screen> <window>\n"); rc = RETURN_FAIL; }
    CloseLibrary((struct Library *)ExpansionBase);
    FreeArgs(rda);
    (void)version;
    return rc;
}
