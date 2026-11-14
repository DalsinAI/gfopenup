/* REINSTALL's list of the installed parts the pack hasn't got ("GFOpenMUI, GFOpenBase"), which it leaves as they
 * are: GFOpenUpTool sets it in ENV:GFOpenUp/Left, so the Installer's last screen can name them (Ivanova's pack lab b,
 * 14 Nov 2026: the warning was only in T:GFOpenUp.log). Shared by src/openuptool.c and tests/test_openup_left.c.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef OPENUP_LEFT_H
#define OPENUP_LEFT_H

#include <string.h>

#define OUL_LEFT_MORE ", and more"

/* name added to the list in buf (size bytes, at least 16), after ", "; when it doesn't fit, ", and more" ends the
 * list once and later names are dropped. buf is always a whole string. */
static void oul_left_add(char *buf, size_t size, const char *name)
{
    size_t have = strlen(buf), more = sizeof OUL_LEFT_MORE - 1, need = strlen(name) + (have ? 2 : 0);
    if (have >= more && !strcmp(buf + have - more, OUL_LEFT_MORE)) return;          /* full already */
    if (have + need + more + 1 <= size) {
        if (have) strcat(buf, ", ");
        strcat(buf, name);
    } else if (have + more + 1 <= size) {
        strcat(buf, OUL_LEFT_MORE);
    }
}

#endif
