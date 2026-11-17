/* Host checks of src/openup_home.h: an application's place on Work:.
 *   cc -Wall -Isrc -o /tmp/test_openup_home tests/test_openup_home.c && /tmp/test_openup_home
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <stdio.h>
#include "openup_home.h"

static int fails;

static void is(const char *what, const char *path, const char *want)
{
    char out[64];
    int got = ouh_work(path, out, sizeof out);
    int ok = want ? got && !strcmp(out, want) : !got;
    printf("%s %s\n", ok ? "ok  " : "FAIL", what);
    fails += !ok;
}

int main(void)
{
    char tiny[12];
    is("a program in its drawer", "SYS:Utilities/GFOpenPuTTY/GFOpenPuTTY", "Work:GFOpenPuTTY/GFOpenPuTTY");
    is("the drawer itself", "SYS:Utilities/GFOpenWrite", "Work:GFOpenWrite");
    is("the drawer's icon", "SYS:Utilities/GFOpenMail.info", "Work:GFOpenMail.info");
    is("an application that is one file", "SYS:Utilities/UnofficialTelegram", "Work:UnofficialTelegram");
    is("the browser", "SYS:Utilities/OpenBrowser/OpenBrowser", "Work:OpenBrowser/OpenBrowser");
    is("the lite browser is not taken for the browser", "SYS:Utilities/GFOpenBrowserLite/GFOpenBrowserLite", "Work:GFOpenBrowserLite/GFOpenBrowserLite");
    is("the desktop's own tools stay", "SYS:Utilities/GFOpenFiles", NULL);
    is("GFOpenPlay stays", "SYS:Utilities/GFOpenPlay", NULL);
    is("GFOpenPrint's viewer stays", "SYS:Utilities/GFOpenPrint/GFOpenView", NULL);
    is("a longer name is another program", "SYS:Utilities/GFOpenWriter", NULL);
    is("another drawer of SYS:", "SYS:Tools/GFOpenEdit", NULL);
    is("nothing", NULL, NULL);
    {
        int got = ouh_work("SYS:Utilities/GFOpenPuTTY/GFOpenPuTTY", tiny, sizeof tiny);
        printf("%s a place that doesn't fit is not written\n", got ? "FAIL" : "ok  ");
        fails += got;
    }
    {   /* the names are the pack's: tools/build_pack.py WORK_APPS (tests/test_build_pack.py compares the two lists) */
        int n = 0;
        while (ouh_apps[n]) n++;
        printf("%s nine applications\n", n == 9 ? "ok  " : "FAIL");
        fails += n != 9;
    }
    printf(fails ? "%d failed\n" : "all passed\n", fails);
    return fails != 0;
}
