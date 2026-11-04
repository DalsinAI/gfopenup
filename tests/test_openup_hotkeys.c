/* Host test of src/openup_hotkeys.h, GFOpenUpMenu's hotkeys (0.1.7): which key events open GFOpenMonitor, through
 * the filter's InputXpression and commodities' MatchIX, and the words of ENV:GFOpenPrefs/Hotkeys.
 *
 *   cc -std=c99 -Wall -Werror -Isrc tests/test_openup_hotkeys.c -o test_openup_hotkeys && ./test_openup_hotkeys
 *
 * tests/test_build_pack.py runs it.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <stdio.h>
#include "openup_hotkeys.h"

static int fails;

static void truth(const char *what, int got, int want)
{
    if (!got == !want) { printf("ok   %s\n", what); return; }
    printf("FAIL %s: got %d, wanted %d\n", what, got, want);
    fails++;
}

#define CTRL OUH_Q_CONTROL
#define LALT OUH_Q_LALT
#define RALT OUH_Q_RALT

int main(void)
{
    struct ouh_ix help, f12;
    ouh_ix_for(&help, OUH_RAW_HELP);
    ouh_ix_for(&f12, OUH_RAW_F12);

    /* the keys */
    truth("Help is raw key 0x5F", OUH_RAW_HELP == 0x5F, 1);
    truth("F12 is raw key 0x6F", OUH_RAW_F12 == 0x6F, 1);
    truth("the expression is a version 2 InputXpression for raw keys", help.version == 2 && help.cls == 1, 1);

    /* what opens GFOpenMonitor */
    truth("Ctrl+Left Alt+Help", ouh_match(&help, 1, 0x5F, CTRL | LALT), 1);
    truth("Ctrl+Right Alt+Help", ouh_match(&help, 1, 0x5F, CTRL | RALT), 1);
    truth("Ctrl+both Alts+Help", ouh_match(&help, 1, 0x5F, CTRL | LALT | RALT), 1);
    truth("Ctrl+Left Alt+F12", ouh_match(&f12, 1, 0x6F, CTRL | LALT), 1);
    truth("Ctrl+AltGr+F12 (a PC's AltGr is Ctrl and Right Alt)", ouh_match(&f12, 1, 0x6F, CTRL | RALT), 1);
    truth("with Caps Lock on", ouh_match(&help, 1, 0x5F, CTRL | LALT | OUH_Q_CAPSLOCK), 1);
    truth("with a mouse button held (0x2000)", ouh_match(&f12, 1, 0x6F, CTRL | LALT | 0x2000), 1);
    truth("with the numeric pad bit (0x0100)", ouh_match(&help, 1, 0x5F, CTRL | LALT | 0x0100), 1);

    /* what doesn't */
    truth("not Help alone", ouh_match(&help, 1, 0x5F, 0), 0);
    truth("not Ctrl+Help", ouh_match(&help, 1, 0x5F, CTRL), 0);
    truth("not Alt+Help", ouh_match(&help, 1, 0x5F, LALT), 0);
    truth("not Ctrl+Alt+Shift+Help", ouh_match(&help, 1, 0x5F, CTRL | LALT | OUH_Q_LSHIFT), 0);
    truth("not Ctrl+Alt+Amiga+Help", ouh_match(&help, 1, 0x5F, CTRL | LALT | OUH_Q_LCOMMAND), 0);
    truth("not Ctrl+Alt+Right Amiga+F12", ouh_match(&f12, 1, 0x6F, CTRL | LALT | OUH_Q_RCOMMAND), 0);
    truth("not the key coming up", ouh_match(&help, 1, 0x5F | 0x80, CTRL | LALT), 0);
    truth("not a key repeat", ouh_match(&f12, 1, 0x6F, CTRL | LALT | OUH_Q_REPEAT), 0);
    truth("not another key (Ctrl+Alt+Del, 0x46)", ouh_match(&help, 1, 0x46, CTRL | LALT), 0);
    truth("not F10 (0x59)", ouh_match(&f12, 1, 0x59, CTRL | LALT), 0);
    truth("not F11 (0x4B)", ouh_match(&f12, 1, 0x4B, CTRL | LALT), 0);
    truth("not a mouse event (class 2)", ouh_match(&help, 2, 0x5F, CTRL | LALT), 0);
    truth("Help's filter doesn't take F12", ouh_match(&help, 1, 0x6F, CTRL | LALT), 0);

    /* ENV:GFOpenPrefs/Hotkeys */
    truth("no file: on", ouh_monitor_on(NULL), 1);
    truth("an empty file: on", ouh_monitor_on(""), 1);
    truth("monitor on", ouh_monitor_on("monitor on\n"), 1);
    truth("monitor off", ouh_monitor_on("monitor off\n"), 0);
    truth("monitor off with no newline", ouh_monitor_on("monitor off"), 0);
    truth("monitor no", ouh_monitor_on("monitor no\n"), 0);
    truth("monitor 0", ouh_monitor_on("monitor 0\n"), 0);
    truth("monitor yes", ouh_monitor_on("monitor yes\n"), 1);
    truth("a comment line doesn't count", ouh_monitor_on("; monitor off\n"), 1);
    truth("among other lines", ouh_monitor_on("; GFOpenPrefs' hotkeys\nsomething else\nmonitor off\nmore 1\n"), 0);
    truth("the last monitor line wins", ouh_monitor_on("monitor off\nmonitor on\n"), 1);
    truth("monitorx off is another word", ouh_monitor_on("monitorx off\n"), 1);
    truth("a tab after the word", ouh_monitor_on("monitor\toff\n"), 0);
    truth("the word alone", ouh_monitor_on("monitor\n"), 1);

    printf(fails ? "FAIL\n" : "PASS\n");
    return fails ? 1 : 0;
}
