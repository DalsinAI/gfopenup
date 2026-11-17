/* Where an application lives: on Work: where a volume named Work is mounted, in SYS:Utilities elsewhere (Dale,
 * 17 Nov 2026: "work is for applications", and of the applications GFOpenUp had in SYS:Utilities: "Yes, move
 * them"). The pack's parts give each of these two homes ("when WorkDrive" and "unless WorkDrive",
 * tools/build_pack.py's WORK_APPS, the same names as here); the menu and the dock look in Work: first.
 * Plain C with no OS calls, shared with tests/test_openup_home.c.
 *
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef OPENUP_HOME_H
#define OPENUP_HOME_H

#include <string.h>

#define OUH_OLD "SYS:Utilities/"
#define OUH_NEW "Work:"

static const char *const ouh_apps[] = {
    "GFOpenWrite", "GFOpenCalc", "GFOpenPresentation", "GFOpenPuTTY", "GFOpenFTP", "GFOpenMail",
    "UnofficialTelegram", "OpenBrowser", "GFOpenBrowserLite", NULL
};

/* 1 with path's place on Work: in out (size bytes) when path is one of the applications' in SYS:Utilities (the
 * drawer or file itself, a file in the drawer, or its .info); 0 when it is anything else or doesn't fit. */
static int ouh_work(const char *path, char *out, int size)
{
    size_t old = strlen(OUH_OLD), n;
    const char *rest;
    int i;
    if (!path || strncmp(path, OUH_OLD, old)) return 0;
    rest = path + old;
    for (i = 0; ouh_apps[i]; i++) {
        n = strlen(ouh_apps[i]);
        if (!strncmp(rest, ouh_apps[i], n) && (!rest[n] || rest[n] == '/' || !strcmp(rest + n, ".info"))) {
            if (strlen(OUH_NEW) + strlen(rest) + 1 > (size_t)size) return 0;
            strcpy(out, OUH_NEW);
            strcat(out, rest);
            return 1;
        }
    }
    return 0;
}

#endif
