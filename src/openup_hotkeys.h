/* openup_hotkeys.h: GFOpenUpMenu's hotkeys (Dale, 3 November 2026: "an OS wide option to bring up monitor by
 * pressing ctrl alt help on an amiga and ctrl alt f12 on a pc keyboard").
 *
 *   Ctrl+Alt+Help   raw key 0x5F, every Amiga keyboard; on AmiCradle a PC's Insert sends it
 *   Ctrl+Alt+F12    raw key 0x6F, the code PC keyboards' F12 has on the Amiga (AROS rawkeycodes.h, OS 4, the
 *                   PS/2 and USB adapters); AmiCradle's window and the appliance send it for Ctrl+Alt+F12
 *
 * Either opens GFOpenMonitor, or brings its window to the front when it is open. Either Alt counts, so AltGr
 * (Ctrl+Alt on a PC) does too. Caps Lock, the mouse buttons and the numeric pad don't matter; Shift or an
 * Amiga key held as well, or a key repeat, is not the hotkey.
 *
 * The switch: ENV:GFOpenPrefs/Hotkeys (GFOpenPrefs' Input page writes it, and ENVARC: on Save, then sends
 * Ctrl-F to GFOpenUpMenu's task to read it again), "key value" lines. "monitor off" turns the hotkeys off;
 * with no file, or no monitor line, they are on.
 *
 * Plain C with no Amiga headers, so tests/test_openup_hotkeys.c builds it on the host as it is: the values are
 * those of devices/inputevent.h and libraries/commodities.h, and ouh_match() is commodities' MatchIX.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef OPENUP_HOTKEYS_H
#define OPENUP_HOTKEYS_H

#define OUH_SETTINGS "ENV:GFOpenPrefs/Hotkeys"

#define OUH_RAW_HELP 0x5F
#define OUH_RAW_F12 0x6F

/* devices/inputevent.h */
#define OUH_CLASS_RAWKEY 0x01
#define OUH_Q_LSHIFT 0x0001
#define OUH_Q_RSHIFT 0x0002
#define OUH_Q_CAPSLOCK 0x0004
#define OUH_Q_CONTROL 0x0008
#define OUH_Q_LALT 0x0010
#define OUH_Q_RALT 0x0020
#define OUH_Q_LCOMMAND 0x0040
#define OUH_Q_RCOMMAND 0x0080
#define OUH_Q_REPEAT 0x0200
/* libraries/commodities.h */
#define OUH_IX_VERSION 2
#define OUH_IXSYM_SHIFT 1
#define OUH_IXSYM_CAPS 2
#define OUH_IXSYM_ALT 4

/* struct InputXpression's fields, in its order */
struct ouh_ix {
    unsigned char version, cls;
    unsigned short code, codemask, qual, qualmask, qualsame;
};

/* The hotkey for one raw key: that key going down with Ctrl and either Alt, and no other key that counts. */
static inline void ouh_ix_for(struct ouh_ix *ix, unsigned short raw)
{
    ix->version = OUH_IX_VERSION;
    ix->cls = OUH_CLASS_RAWKEY;
    ix->code = raw;
    ix->codemask = 0xFF;                    /* the key going down only: its up code has bit 7 */
    ix->qual = OUH_Q_CONTROL | OUH_Q_LALT | OUH_Q_RALT;
    ix->qualmask = OUH_Q_LSHIFT | OUH_Q_RSHIFT | OUH_Q_CONTROL | OUH_Q_LALT | OUH_Q_RALT | OUH_Q_LCOMMAND | OUH_Q_RCOMMAND |
                   OUH_Q_REPEAT;
    ix->qualsame = OUH_IXSYM_ALT;
}

/* commodities.library's MatchIX, for the host test: 1 when the event is the hotkey */
static inline int ouh_match(const struct ouh_ix *ix, unsigned char cls, unsigned short code, unsigned short qual)
{
    if (ix->cls != cls) return 0;
    if ((ix->code ^ code) & ix->codemask) return 0;
    if ((ix->qualsame & OUH_IXSYM_SHIFT) && (qual & (OUH_Q_LSHIFT | OUH_Q_RSHIFT))) qual |= OUH_Q_LSHIFT | OUH_Q_RSHIFT;
    if ((ix->qualsame & OUH_IXSYM_CAPS) && (qual & (OUH_Q_LSHIFT | OUH_Q_RSHIFT | OUH_Q_CAPSLOCK)))
        qual |= OUH_Q_LSHIFT | OUH_Q_RSHIFT | OUH_Q_CAPSLOCK;
    if ((ix->qualsame & OUH_IXSYM_ALT) && (qual & (OUH_Q_LALT | OUH_Q_RALT))) qual |= OUH_Q_LALT | OUH_Q_RALT;
    return ((qual ^ ix->qual) & ix->qualmask) == 0;
}

/* The settings file's text (or NULL, no file): 1 the monitor hotkeys are on, 0 off. A "monitor" line whose value
 * starts with "off" (or "no", or "0") turns them off; anything else, or no line, leaves them on. The last line
 * wins. */
static inline int ouh_monitor_on(const char *text)
{
    int on = 1;
    const char *p = text;
    while (p && *p) {
        const char *line = p, *v;
        while (*p && *p != '\n') p++;
        if (*p) p++;
        if (*line == ';') continue;
        if (line[0] != 'm' || line[1] != 'o' || line[2] != 'n' || line[3] != 'i' || line[4] != 't' || line[5] != 'o' ||
            line[6] != 'r' || (line[7] != ' ' && line[7] != '\t'))
            continue;
        for (v = line + 7; *v == ' ' || *v == '\t'; v++) ;
        on = !((v[0] == 'o' && v[1] == 'f' && v[2] == 'f') || (v[0] == 'n' && v[1] == 'o') || v[0] == '0');
    }
    return on;
}

#endif /* OPENUP_HOTKEYS_H */
