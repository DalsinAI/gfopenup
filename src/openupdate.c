/* GFOpenUpdate 0.1: GFOS's updates offered on the Amiga (docs/design/GFOPENUPDATE.md, step 3).
 *
 * Dale, 17 November 2026: "a GFOS updater that knows what versions of things are installed and checks dalsinai etc
 * for updates and offers them to the user to update"; "make it a wbstartup commodity"; "once at start up when the
 * network is up, then periodically that can be set via TOOLTYPE"; "store its data in envarc:".
 *
 * A commodity in SYS:WBStartup. Once a minute it looks whether a check is due (src/openupdate.h, oud_due): the
 * first once bsdsocket.library opens and the index comes, then every INTERVAL seconds. A check fetches
 * GFOpenUpdate.index from the release with GFOpenSocket's httpget, weighs it against GFOpenUpTool's manifest and,
 * when an installed part has a newer version, asks: Update, Not now, or Skip these versions. Update fetches each
 * part's archive, checks its size and SHA-256, unpacks it with GFOpenCompress and installs it with
 * GFOpenUpTool INSTALL. Nothing is installed unasked.
 *
 * Exchange's Show (or GFOpenUpdate started again) checks now, and says so when there is nothing newer.
 * Tool types (its icon, or KEY=VALUE on the command line):
 *   INTERVAL=86400      seconds between checks after the one at startup; never under 600; 0 = only at startup
 *   URL=<address>       where the index and archives are (default gfopenup's release "updates")
 *   CX_PRIORITY=0       the broker's priority
 * Its data: ENVARC:GFOpenUpdate/LastCheck (seconds) and ENVARC:GFOpenUpdate/Skipped ("skip <Name> <version>").
 *
 * MIT, Copyright (c) 2026 AmiCradle Team. */

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <devices/timer.h>
#include <intuition/intuition.h>
#include <libraries/commodities.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/commodities.h>
#include <proto/icon.h>
#include <proto/utility.h>
#include <string.h>

/* the shared headers carry helpers only the host tests and GFOpenUpTool use */
#pragma GCC diagnostic ignored "-Wunused-function"
#include "openupdate.h"
#include "openupdate_sha256.h"

#define NAME "GFOpenUpdate"
#define VERSION_TEXT "0.1"
static const char version[] = "$VER: " NAME " " VERSION_TEXT " (17.11.2026)";

struct Library *CxBase, *IconBase;
struct IntuitionBase *IntuitionBase;
struct Library *UtilityBase;

#define MANIFEST "ENVARC:GFOpenAmiga/GFOpenUp.manifest"
#define INDEX_FILE OUD_WORK "/GFOpenUpdate.index"
#define UNPACKED OUD_WORK "/x"
#define MAX_PARTS 96
#define MAX_TEXT 65536L

static struct {
    long interval;
    char url[200];
    LONG cxpri;
} set = { OUD_INTERVAL_DEFAULT, OUD_URL_DEFAULT, 0 };

static struct oud_part parts[MAX_PARTS];

/* ---- small things ---------------------------------------------------------------------------------------- */

static long now_seconds(void)
{
    struct DateStamp ds;
    DateStamp(&ds);
    return ds.ds_Days * 86400L + ds.ds_Minute * 60L + ds.ds_Tick / TICKS_PER_SECOND;
}

/* A whole file into a new buffer (NUL after it), its length in *len; NULL when there is none or it is too big. */
static char *read_file(const char *path, long *len)
{
    BPTR fh = Open((STRPTR)path, MODE_OLDFILE);
    char *buf;
    long n;
    *len = 0;
    if (!fh) return NULL;
    if (!(buf = AllocVec(MAX_TEXT + 1, MEMF_ANY))) { Close(fh); return NULL; }
    n = Read(fh, buf, MAX_TEXT);
    Close(fh);
    if (n < 0 || n == MAX_TEXT) { FreeVec(buf); return NULL; }
    buf[n] = 0;
    *len = n;
    return buf;
}

static int write_file(const char *path, const char *text, long len)
{
    BPTR fh;
    BPTR l = CreateDir((STRPTR)OUD_DATA);
    if (l) UnLock(l);
    if (!(fh = Open((STRPTR)path, MODE_NEWFILE))) return 0;
    if (Write(fh, (APTR)text, len) != len) { Close(fh); return 0; }
    return Close(fh) ? 1 : 0;
}

static LONG run(const char *cmd)
{
    BPTR nil = Open((STRPTR)"NIL:", MODE_NEWFILE);
    LONG rc;
    if (!nil) return -1;
    rc = SystemTags((STRPTR)cmd, SYS_Input, 0, SYS_Output, nil, NP_StackSize, 32768, TAG_DONE);
    Close(nil);
    return rc;
}

static void clear_work(void)
{
    run("C:Delete >NIL: \"" OUD_WORK "\" ALL QUIET FORCE");
}

static LONG ask(const char *body, const char *gadgets, ULONG arg)
{
    struct EasyStruct es = { sizeof(struct EasyStruct), 0, (STRPTR)NAME, (STRPTR)body, (STRPTR)gadgets };
    return EasyRequest(NULL, &es, NULL, arg);
}

static int network_up(void)
{
    struct Library *b = OpenLibrary((STRPTR)"bsdsocket.library", 4);
    if (!b) return 0;
    CloseLibrary(b);
    return 1;
}

/* ---- one part: fetch, check, unpack, install ------------------------------------------------------------- */

/* 0 when installed; else why not into why. */
static int update_part(const struct oud_part *p, char *why, int size)
{
    char url[300], path[160], cmd[600];
    struct oud_sha s;
    char got[65];
    unsigned char *buf;
    long total = 0, n;
    BPTR fh;

    strcpy(path, OUD_WORK "/");
    strcat(path, p->file);
    if (!oud_url(set.url, p->file, url, sizeof url) || !oud_fetch_cmd(url, path, cmd, sizeof cmd)) {
        strncpy(why, "its address won't fit", size); return 1;
    }
    if (run(cmd) != 0) { strncpy(why, "it couldn't be fetched", size); return 1; }
    if (!(fh = Open((STRPTR)path, MODE_OLDFILE))) { strncpy(why, "it couldn't be fetched", size); return 1; }
    if (!(buf = AllocVec(8192, MEMF_ANY))) { Close(fh); strncpy(why, "no memory", size); return 1; }
    oud_sha_start(&s);
    while ((n = Read(fh, buf, 8192)) > 0) { oud_sha_add(&s, buf, n); total += n; }
    Close(fh);
    FreeVec(buf);
    oud_sha_end(&s, got);
    if (n < 0 || total != p->bytes) { strncpy(why, "its size is wrong", size); return 1; }
    if (!oud_sha_same(got, p->sha256)) { strncpy(why, "its SHA-256 is wrong", size); return 1; }

    run("C:Delete >NIL: \"" UNPACKED "\" ALL QUIET FORCE");
    if (!oud_unpack_cmd(path, UNPACKED, cmd, sizeof cmd) || run(cmd) != 0) { strncpy(why, "it couldn't be unpacked", size); return 1; }
    if (!oud_install_cmd(UNPACKED, p->name, cmd, sizeof cmd) || run(cmd) > RETURN_WARN) {
        strncpy(why, "GFOpenUpTool INSTALL failed (T:GFOpenUp.log)", size); return 1;
    }
    return 0;
}

/* ---- a check --------------------------------------------------------------------------------------------- */

/* -1 when the index couldn't be had (try again later), else 0. asked: the user asked for it (Show), so the
 * versions said no to are offered again and "nothing newer" is said. */
static int check(int asked)
{
    char url[300], cmd[600], *index, *manifest, *skipped, *text = NULL, *res = NULL;
    long ilen, mlen, slen = 0;
    int count, offers, i;
    LONG choice;
    BPTR l;

    clear_work();
    if ((l = CreateDir((STRPTR)OUD_WORK))) UnLock(l);
    if (!oud_url(set.url, OUD_INDEX_NAME, url, sizeof url) || !oud_fetch_cmd(url, INDEX_FILE, cmd, sizeof cmd)) return -1;
    if (run(cmd) != 0 || !(index = read_file(INDEX_FILE, &ilen))) {
        if (asked) ask("The list of updates couldn't be fetched.\nIs the network up?", "OK", 0);
        return -1;
    }
    count = oud_index_parts(index, ilen, parts, MAX_PARTS);
    FreeVec(index);
    if (count < 0) {
        if (asked) ask("The list of updates came incomplete.\nTry again later.", "OK", 0);
        return -1;
    }
    if (!(manifest = read_file(MANIFEST, &mlen))) {
        if (asked) ask("GFOpenUp's list of what is installed\n(" MANIFEST ") couldn't be read.", "OK", 0);
        clear_work();
        return 0;
    }
    skipped = asked ? NULL : read_file(OUD_SKIPPED, &slen);
    offers = oud_offers(parts, count, manifest, mlen, skipped, slen);
    FreeVec(manifest);
    if (skipped) FreeVec(skipped);
    if (!offers) {
        if (asked) ask("GFOS is up to date.", "OK", 0);
        clear_work();
        return 0;
    }

    if (!(text = AllocVec(4096, MEMF_ANY)) || !(res = AllocVec(4096, MEMF_CLEAR))) goto done;
    oud_offer_text(parts, offers, 12, text, 4096);
    choice = ask("Updates for GFOS:\n\n%s\nUpdate them now?", "Update|Not now|Skip these versions", (ULONG)text);
    if (choice == 0) {                       /* skip: not offered again until a newer one comes */
        char *old = read_file(OUD_SKIPPED, &slen);
        int n = oud_skip_text(parts, offers, old, slen, text, 4096);
        if (n >= 0) write_file(OUD_SKIPPED, text, n);
        if (old) FreeVec(old);
    } else if (choice == 1) {
        int bad = 0, len = 0;
        for (i = 0; i < offers; i++) {
            char why[80];
            why[0] = 0;
            if (update_part(&parts[i], why, sizeof why - 1)) {
                why[sizeof why - 1] = 0;
                bad++;
                len = oud_put(res, 4096, len, parts[i].name);
                len = oud_put(res, 4096, len, ": ");
                len = oud_put(res, 4096, len, why);
                len = oud_put(res, 4096, len, "\n");
            }
        }
        if (bad) ask("Not updated:\n\n%s", "OK", (ULONG)res);
        else ask("GFOS is updated.\nSome changes need a reboot to start.", "OK", 0);
    }
done:
    if (text) FreeVec(text);
    if (res) FreeVec(res);
    clear_work();
    return 0;
}

/* ---- settings -------------------------------------------------------------------------------------------- */

static void take(const char *key, const char *value)
{
    if (!value) return;
    if (!Stricmp((STRPTR)key, (STRPTR)"INTERVAL")) set.interval = oud_interval(value);
    else if (!Stricmp((STRPTR)key, (STRPTR)"URL")) {
        char t[300];
        if (oud_url(value, "x", t, sizeof t) && strlen(value) < sizeof set.url) strcpy(set.url, value);
    } else if (!Stricmp((STRPTR)key, (STRPTR)"CX_PRIORITY")) { LONG v; if (StrToLong((STRPTR)value, &v) > 0) set.cxpri = v; }
}

static const char *const KEYS[] = { "INTERVAL", "URL", "CX_PRIORITY" };
#define NKEYS (sizeof KEYS / sizeof KEYS[0])

static void from_icon(struct DiskObject *d)
{
    unsigned i;
    if (!d || !d->do_ToolTypes) return;
    for (i = 0; i < NKEYS; i++)
        take(KEYS[i], (const char *)FindToolType((CONST_STRPTR *)d->do_ToolTypes, (STRPTR)KEYS[i]));
}

static void settings(int argc, char **argv)
{
    if (argc == 0) {                        /* from Workbench: its icon */
        struct WBStartup *wbs = (struct WBStartup *)argv;
        if (IconBase && wbs && wbs->sm_NumArgs > 0) {
            BPTR old = CurrentDir(wbs->sm_ArgList[0].wa_Lock);
            struct DiskObject *d = GetDiskObject((STRPTR)wbs->sm_ArgList[0].wa_Name);
            from_icon(d);
            if (d) FreeDiskObject(d);
            CurrentDir(old);
        }
    } else {                                /* from a Shell: the icon beside it, then the line */
        char prog[256], icon[300];
        LONG a[NKEYS] = { 0, 0, 0 };
        struct RDArgs *rd;
        unsigned i;
        if (IconBase && GetProgramName((STRPTR)prog, sizeof prog)) {
            struct DiskObject *d;
            strcpy(icon, "PROGDIR:");
            strncat(icon, (const char *)FilePart((STRPTR)prog), sizeof icon - 10);
            if ((d = GetDiskObject((STRPTR)icon))) { from_icon(d); FreeDiskObject(d); }
        }
        if ((rd = ReadArgs((STRPTR)"INTERVAL/K,URL/K,CX_PRIORITY/K", a, NULL))) {
            for (i = 0; i < NKEYS; i++) take(KEYS[i], (const char *)a[i]);
            FreeArgs(rd);
        }
    }
}

/* ---- the commodity --------------------------------------------------------------------------------------- */

static struct NewBroker nb = {
    NB_VERSION, (STRPTR)NAME, (STRPTR)NAME " " VERSION_TEXT, (STRPTR)"Offers GFOS's updates (Show: check now)",
    NBU_UNIQUE | NBU_NOTIFY, COF_SHOW_HIDE, 0, NULL, 0
};

int main(int argc, char **argv)
{
    struct MsgPort *port = NULL, *tport = NULL;
    struct timerequest *tr = NULL;
    CxObj *broker = NULL;
    LONG err = 0;
    int timer = 0, tpending = 0, quit = 0, active = 1, checked = 0, first = 1, rc = RETURN_FAIL;
    long last = 0;
    (void)version;

    CxBase = OpenLibrary((STRPTR)"commodities.library", 37);
    IconBase = OpenLibrary((STRPTR)"icon.library", 37);
    IntuitionBase = (struct IntuitionBase *)OpenLibrary((STRPTR)"intuition.library", 37);
    UtilityBase = OpenLibrary((STRPTR)"utility.library", 37);
    if (!CxBase || !IntuitionBase || !UtilityBase) goto out;
    settings(argc, argv);
    {
        long len;
        char *t = read_file(OUD_LAST, &len);
        if (t) { last = oud_last(t, len); FreeVec(t); }
    }

    if (!(port = CreateMsgPort())) goto out;
    nb.nb_Port = port;
    nb.nb_Pri = (BYTE)set.cxpri;
    if (!(broker = CxBroker(&nb, &err))) {     /* running already: that one checks now (CXCMD_UNIQUE) */
        if (err == CBERR_DUP) rc = RETURN_OK;
        goto out;
    }
    ActivateCxObj(broker, 1);
    if (!(tport = CreateMsgPort()) || !(tr = (struct timerequest *)CreateIORequest(tport, sizeof *tr))) goto out;
    if (OpenDevice((STRPTR)TIMERNAME, UNIT_VBLANK, (struct IORequest *)tr, 0)) goto out;
    timer = 1;
    rc = RETURN_OK;

    while (!quit) {
        ULONG got;
        CxMsg *m;
        int now_asked = 0;
        if (!tpending) {                       /* a look each minute (OUD_RETRY): due, and the network up? */
            tr->tr_node.io_Command = TR_ADDREQUEST;
            tr->tr_time.tv_secs = first ? 5 : OUD_RETRY;     /* the first look soon after startup */
            first = 0;
            tr->tr_time.tv_micro = 0;
            SendIO((struct IORequest *)tr);
            tpending = 1;
        }
        got = Wait((1UL << port->mp_SigBit) | (1UL << tport->mp_SigBit) | SIGBREAKF_CTRL_C);
        if (got & SIGBREAKF_CTRL_C) quit = 1;
        while ((m = (CxMsg *)GetMsg(port))) {
            ULONG type = CxMsgType(m), id = CxMsgID(m);
            ReplyMsg((struct Message *)m);
            if (type == CXM_COMMAND) switch (id) {
            case CXCMD_DISABLE: ActivateCxObj(broker, 0); active = 0; break;
            case CXCMD_ENABLE: ActivateCxObj(broker, 1); active = 1; break;
            case CXCMD_APPEAR: case CXCMD_UNIQUE: now_asked = 1; break;
            case CXCMD_KILL: quit = 1; break;
            }
        }
        if (quit) break;
        if (tpending && CheckIO((struct IORequest *)tr)) {
            long now = now_seconds();
            WaitIO((struct IORequest *)tr);
            tpending = 0;
            if (active && oud_due(network_up(), checked, now, last, set.interval) && check(0) == 0) {
                char t[16];
                int k = 0, j;
                long v = now;
                char rev[16];
                do { rev[k++] = (char)('0' + v % 10); v /= 10; } while (v > 0 && k < 15);
                for (j = 0; j < k; j++) t[j] = rev[k - 1 - j];
                t[k++] = '\n';
                write_file(OUD_LAST, t, k);
                checked = 1;
                last = now;
            }
        }
        if (now_asked && check(1) == 0) { checked = 1; last = now_seconds(); }
    }
out:
    if (tpending) { AbortIO((struct IORequest *)tr); WaitIO((struct IORequest *)tr); }
    if (timer) CloseDevice((struct IORequest *)tr);
    if (tr) DeleteIORequest((struct IORequest *)tr);
    if (tport) DeleteMsgPort(tport);
    if (broker) DeleteCxObjAll(broker);
    if (port) { struct Message *msg; while ((msg = GetMsg(port))) ReplyMsg(msg); DeleteMsgPort(port); }
    if (UtilityBase) CloseLibrary(UtilityBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    if (IconBase) CloseLibrary(IconBase);
    if (CxBase) CloseLibrary(CxBase);
    return rc;
}
