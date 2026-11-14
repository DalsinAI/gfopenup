/* GFOpenUpTool's unattended mode (Dale, 13 Nov 2026: amicradle-teams
 * teams/DECISION-UNATTENDED-SETUP-20261113-1959.md, every installer), the same
 * switch as GFOpenUp Setup 0.1.9 (gfopenprefs Setup/src/setup_auto.h): AUTO
 * (GFOpenUpTool UPDATE AUTO). Only AUTO: the answers file and the host flag
 * stand for Setup's first start, but a plain UPDATE is a person's choice (the
 * menu's "GFOpenUp Update..."), so it still asks, flag or not (rule 5; Legion's
 * review of gfopenup #244).
 * UPDATE is the only command that waits for a person (its requester, then the
 * Installer, which waits for clicks even as a Novice). With AUTO, UPDATE asks
 * nothing and opens no Installer: it reinstalls the parts that are installed
 * from the waiting pack (REINSTALL: the settings in place stay), installs the
 * pack's KickstartSet on the slim ROM as the Installer does (ou_auto_kickset;
 * no other new part is added), writes the startup block again (STARTUP, which keeps the lines
 * switched off), writes one line to ENVARC:GFOpenUp/GFOpenUpTool.log and
 * returns the run's code. CHECK still only says; YES still runs the Installer.
 * The parts here have no Amiga calls, for tests/test_openup_unattended.c.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef OPENUP_UNATTENDED_H
#define OPENUP_UNATTENDED_H

#include <string.h>

#define OU_LOG "ENVARC:GFOpenUp/GFOpenUpTool.log"

/* how UPDATE goes */
enum { OU_CHECK, OU_ASK, OU_INSTALLER, OU_AUTO };
/* what turned unattended on */
enum { OU_BY_NONE, OU_BY_SWITCH, OU_BY_ANSWERS, OU_BY_FLAG };

static int ou_lower(int c) { return c >= 'A' && c <= 'Z' ? c + 32 : c; }

static int ou_word(const char *a, const char *b)
{
    if (!a) return 0;
    while (*a && *b && ou_lower((unsigned char)*a) == ou_lower((unsigned char)*b)) { a++; b++; }
    return !*a && !*b;
}

/* UPDATE's argument (NULL, CHECK, YES or AUTO). CHECK only says, YES runs the Installer without asking,
 * AUTO is unattended, and a plain UPDATE asks, as before, whatever files are there. */
static int ou_update_mode(const char *how, int *by)
{
    *by = OU_BY_NONE;
    if (ou_word(how, "CHECK")) return OU_CHECK;
    if (ou_word(how, "YES")) return OU_INSTALLER;
    if (ou_word(how, "AUTO")) { *by = OU_BY_SWITCH; return OU_AUTO; }
    return OU_ASK;
}

static void ou_cat(char *buf, int size, const char *s)
{
    int n = (int)strlen(buf);
    while (s && *s && n < size - 1) buf[n++] = *s++;
    buf[n] = 0;
}

static void ou_catn(char *buf, int size, int v)
{
    char d[12];
    int i = sizeof d - 1;
    unsigned u = v < 0 ? 0u - (unsigned)v : (unsigned)v;
    d[i] = 0;
    do d[--i] = (char)('0' + u % 10); while ((u /= 10) && i > 1);
    if (v < 0) d[--i] = '-';
    ou_cat(buf, size, d + i);
}

/* The log's one line, without its newline:
 *   GFOpenUpTool UPDATE: unattended (host flag) 0.8.28 to 0.8.29, 14 parts reinstalled: ok
 * installed "" (not installed), newer "" (no newer pack), newer not above installed (nothing to do):
 * what was found, and done < 0 when nothing was tried. rc is the run's return code. kick: 1 the Kickstart set
 * installed, -1 it failed, 0 not tried (ou_auto_kickset). */
static void ou_log_line(char *buf, int size, int kick, int by, const char *installed, const char *newer, int done, int rc)
{
    static const char *const why[] = { "?", "AUTO", "answers", "host flag" };
    if (size <= 0) return;
    buf[0] = 0;
    ou_cat(buf, size, "GFOpenUpTool UPDATE: unattended (");
    ou_cat(buf, size, why[by >= 0 && by <= OU_BY_FLAG ? by : 0]);
    ou_cat(buf, size, ") ");
    if (!installed || !*installed) ou_cat(buf, size, "GFOpenUp not installed, nothing to update");
    else if (!newer || !*newer) { ou_cat(buf, size, installed); ou_cat(buf, size, " installed, no newer pack from Cradle"); }
    else if (done < 0) { ou_cat(buf, size, installed); ou_cat(buf, size, " installed, the newest Cradle has"); }
    else {
        ou_cat(buf, size, installed); ou_cat(buf, size, " to "); ou_cat(buf, size, newer);
        ou_cat(buf, size, ", "); ou_catn(buf, size, done);
        ou_cat(buf, size, done == 1 ? " part reinstalled" : " parts reinstalled");
        if (kick) ou_cat(buf, size, kick > 0 ? ", the Kickstart set installed" : ", the Kickstart set failed");
    }
    ou_cat(buf, size, ": ");
    ou_cat(buf, size, rc == 0 ? "ok" : rc < 10 ? "done, with warnings" : "failed");
}

/* The part AUTO installs though the manifest may not list it: the one the Installer installs on this machine
 * whatever was chosen. KickstartSet on the slim ROM (Install-GFOpenUp: without its module set the slim ROM boots
 * to its rescue Shell), through KICKSET, as the Installer does. 1: install it. */
static int ou_auto_kickset(int slim_rom, int in_pack)
{
    return slim_rom && in_pack;
}

#endif
