/* GFOpenUpTool's unattended mode (Dale, 13 Nov 2026: amicradle-teams
 * teams/DECISION-UNATTENDED-SETUP-20261113-1959.md, every installer), the same
 * switches as GFOpenUp Setup 0.1.9 (gfopenprefs Setup/src/setup_auto.h):
 *   - AUTO (GFOpenUpTool UPDATE AUTO);
 *   - the answers file ENVARC:GFOpenUp/Answers;
 *   - the host flag ENV:AmiCradle/Unattended or ENVARC:AmiCradle/Unattended,
 *     which the Cradle or the card writes for labs, test runs and fresh cards.
 * UPDATE is the only command that waits for a person (its requester, then the
 * Installer, which waits for clicks even as a Novice). Unattended, UPDATE asks
 * nothing and opens no Installer: it reinstalls the parts that are installed
 * from the waiting pack (REINSTALL: the settings in place stay, no new part is
 * added), writes the startup block again (STARTUP, which keeps the lines
 * switched off), writes one line to ENVARC:GFOpenUp/GFOpenUpTool.log and
 * returns the run's code. CHECK still only says; YES still runs the Installer.
 * The parts here have no Amiga calls, for tests/test_openup_unattended.c.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef OPENUP_UNATTENDED_H
#define OPENUP_UNATTENDED_H

#include <string.h>

#define OU_ANSWERS "ENVARC:GFOpenUp/Answers"
#define OU_FLAG "ENV:AmiCradle/Unattended"
#define OU_FLAG_ARC "ENVARC:AmiCradle/Unattended"
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

/* UPDATE's argument (NULL, CHECK, YES or AUTO), whether the answers file and the host flag are there.
 * CHECK and YES say what they say whatever else is set (a script asked for them); AUTO, or a plain UPDATE
 * with the answers file or the host flag, is unattended; a plain UPDATE otherwise asks, as before. */
static int ou_update_mode(const char *how, int answers, int flag, int *by)
{
    *by = OU_BY_NONE;
    if (ou_word(how, "CHECK")) return OU_CHECK;
    if (ou_word(how, "YES")) return OU_INSTALLER;
    if (ou_word(how, "AUTO")) { *by = OU_BY_SWITCH; return OU_AUTO; }
    if (answers) { *by = OU_BY_ANSWERS; return OU_AUTO; }
    if (flag) { *by = OU_BY_FLAG; return OU_AUTO; }
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
 * what was found, and done < 0 when nothing was tried. rc is the run's return code. */
static void ou_log_line(char *buf, int size, int by, const char *installed, const char *newer, int done, int rc)
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
    }
    ou_cat(buf, size, ": ");
    ou_cat(buf, size, rc == 0 ? "ok" : rc < 10 ? "done, with warnings" : "failed");
}

#endif
