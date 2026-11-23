/* GFOpenUpdate's Amiga side that its two faces share: the commodity's window (src/openupdate/openupdate.c) and the
 * Shell command C:apt (src/openapt/openapt.c, docs/design/GFOPENAPT.md). One way to read a file, run a command,
 * fetch an archive and check it, unpack it and hand its part to GFOpenUpTool, so the two can't come to differ.
 * These were openupdate.c's own until 23 November 2026; they moved here as they were.
 *
 * Include after <proto/exec.h>, <proto/dos.h>, <dos/dostags.h>, openupdate.h and openupdate_sha256.h.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef OPENUPDATE_AMIGA_H
#define OPENUPDATE_AMIGA_H

/* A whole file into a new buffer (NUL after it), its length in *len; NULL when there is none or it is over max
 * (GFOpenUpTool's manifest is over 120 KB on a full install). FreeVec it. */
static char *oud_read_file(const char *path, long *len, long max)
{
    BPTR fh = Open((STRPTR)path, MODE_OLDFILE);
    struct FileInfoBlock *fib;
    char *buf = NULL;
    long size = -1, n;
    *len = 0;
    if (!fh) return NULL;
    if ((fib = AllocDosObject(DOS_FIB, NULL))) {
        if (ExamineFH(fh, fib)) size = fib->fib_Size;
        FreeDosObject(DOS_FIB, fib);
    }
    if (size < 0 || size > max || !(buf = AllocVec(size + 1, MEMF_ANY))) { Close(fh); return NULL; }
    n = Read(fh, buf, size);
    Close(fh);
    if (n != size) { FreeVec(buf); return NULL; }
    buf[n] = 0;
    *len = n;
    return buf;
}

/* One command run and waited for: its return code, or -1 when it couldn't be started. What it prints goes to out,
 * or nowhere when out is 0 (the commodity has no Shell). */
static LONG oud_run(const char *cmd, BPTR out)
{
    BPTR nil = out ? 0 : Open((STRPTR)"NIL:", MODE_NEWFILE);
    LONG rc;
    if (!out && !nil) return -1;
    rc = SystemTags((STRPTR)cmd, SYS_Input, 0, SYS_Output, out ? out : nil, NP_StackSize, 32768, TAG_DONE);
    if (nil) Close(nil);
    return rc;
}

/* 1 when a TCP/IP stack is running: bsdsocket.library opens. */
static int oud_network_up(void)
{
    struct Library *b = OpenLibrary((STRPTR)"bsdsocket.library", 4);
    if (!b) return 0;
    CloseLibrary(b);
    return 1;
}

/* The drawer (and all in it) deleted, quietly. */
static void oud_delete_all(const char *drawer)
{
    char cmd[300];
    const char *bits[] = { "C:Delete >NIL: \"", drawer, "\" ALL QUIET FORCE", NULL };
    if (oud_quotable(drawer) && oud_cat(cmd, sizeof cmd, bits)) oud_run(cmd, 0);
}

/* The part's drawer in what was unpacked: the one drawer there ("<drawer>/..." in the archive, by the part's own
 * drawer name, which for a part inside another's, rexx in GFOpenLibs, isn't the part's name). 1, else 0. */
static int oud_part_drawer(const char *where, char *out, int size)
{
    BPTR lock = Lock((STRPTR)where, ACCESS_READ);
    struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
    int found = 0, many = 0;
    if (lock && fib && Examine(lock, fib)) {
        while (ExNext(lock, fib)) {
            if (fib->fib_DirEntryType <= 0) continue;
            if (found) { many = 1; continue; }
            if ((int)strlen((const char *)fib->fib_FileName) < size) { strcpy(out, (const char *)fib->fib_FileName); found = 1; }
        }
    }
    if (fib) FreeDosObject(DOS_FIB, fib);
    if (lock) UnLock(lock);
    return found && !many;
}

/* url fetched as the file path with GFOpenSocket's httpget, then weighed: its length must be bytes and its SHA-256
 * sha256. NULL when it is right, else why not, in a few words. With sha256 NULL nothing is weighed (the index
 * itself, which has its own end line). A file that is wrong stays where it is for the caller to delete. */
static const char *oud_fetch_checked(const char *url, const char *path, long bytes, const char *sha256)
{
    char cmd[600], got[65];
    struct oud_sha s;
    unsigned char *buf;
    long total = 0, n;
    BPTR fh;

    if (!oud_fetch_cmd(url, path, cmd, sizeof cmd)) return "its address won't fit";
    if (oud_run(cmd, 0) != 0) return "it couldn't be fetched";
    if (!sha256) return NULL;
    if (!(fh = Open((STRPTR)path, MODE_OLDFILE))) return "it couldn't be fetched";
    if (!(buf = AllocVec(8192, MEMF_ANY))) { Close(fh); return "no memory"; }
    oud_sha_start(&s);
    while ((n = Read(fh, buf, 8192)) > 0) { oud_sha_add(&s, buf, n); total += n; }
    Close(fh);
    FreeVec(buf);
    oud_sha_end(&s, got);
    if (n < 0 || total != bytes) return "its size is wrong";
    if (!oud_sha_same(got, sha256)) return "its SHA-256 is wrong";
    return NULL;
}

/* The archive unpacked into the drawer where (emptied first) with GFOpenCompress; the one drawer it holds, the
 * part's, into drawer. NULL, else why not. */
static const char *oud_unpack_part(const char *archive, const char *where, char *drawer, int size)
{
    char cmd[600];
    oud_delete_all(where);
    if (!oud_unpack_cmd(archive, where, cmd, sizeof cmd) || oud_run(cmd, 0) != 0) return "it couldn't be unpacked";
    if (!oud_part_drawer(where, drawer, size)) return "its archive holds no part";
    return NULL;
}

/* GFOpenUpTool INSTALL on <where>/<drawer>, what it prints to out (0: nowhere). NULL, else why not; its return
 * code in *rc when rc isn't NULL (5, WARN, is GFOpenUpTool leaving the part out and saying why). */
static const char *oud_install_part(const char *where, const char *drawer, BPTR out, LONG *rc)
{
    char cmd[600];
    LONG r;
    if (!oud_install_cmd(where, drawer, cmd, sizeof cmd)) return "GFOpenUpTool INSTALL failed (T:GFOpenUp.log)";
    r = oud_run(cmd, out);
    if (rc) *rc = r;
    if (r > RETURN_WARN) return "GFOpenUpTool INSTALL failed (T:GFOpenUp.log)";
    return NULL;
}

#endif
