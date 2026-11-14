/* openup_sysfont.h: Font prefs' system default text from DejaVu Sans Mono 11 to GFOS Mono 11 (Dale, 14 November 2026,
 * 16:23: "Move it at update"). OS 3.2.3's listbrowser.gadget drops a right-justified cell's last pixel column, so
 * a glyph inked there loses it ("MB" reads "ME" in ShowConfig); GFOS Mono keeps a blank last column in every glyph
 * (gfosfonts #10) and is the ROM's default (amicradle-guest #771), but a Font.prefs that names DejaVuSansMono.font
 * 11 wins over the ROM. GFOpenUpTool moves exactly that one setting when it installs GFOSFonts (on UPDATE, and on
 * a fresh install alike), and says so; any other font or size is the user's and stays.
 * Plain C on the bytes of a Font.prefs file (FORM PREF, FONT chunks of struct FontPrefs: fp_Type at 15,
 * ta_YSize at 24, fp_Name at 28, 128 bytes): tests/test_openup_sysfont.c.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef OPENUP_SYSFONT_H
#define OPENUP_SYSFONT_H

#include <string.h>

#define OUS_OLD_FONT "DejaVuSansMono.font"
#define OUS_NEW_FONT "GFOSMono.font"
#define OUS_SIZE 11
#define OUS_FP_SYSFONT 1             /* fp_Type: the system default text (the Shell's, fixed width) */
#define OUS_FONT_MIN 156             /* struct FontPrefs */

static unsigned long ous_get32(const unsigned char *p)
{
    return (unsigned long)p[0] << 24 | (unsigned long)p[1] << 16 | (unsigned long)p[2] << 8 | p[3];
}

/* The system font's FONT chunk data in a Font.prefs file of len bytes, or NULL */
static unsigned char *ous_sysfont(unsigned char *d, long len)
{
    long k;
    if (!d || len < 12 || memcmp(d, "FORM", 4) || memcmp(d + 8, "PREF", 4)) return NULL;
    for (k = 12; k + 8 <= len; ) {
        unsigned long n = ous_get32(d + k + 4);
        if ((unsigned long)(len - k - 8) < n) break;
        if (!memcmp(d + k, "FONT", 4) && n >= OUS_FONT_MIN && d[k + 8 + 14] == 0 && d[k + 8 + 15] == OUS_FP_SYSFONT)
            return d + k + 8;
        k += 8 + (long)n + (long)(n & 1);
    }
    return NULL;
}

/* 1: the file's system font was DejaVuSansMono.font 11 and is now GFOSMono.font 11 (the rest of the chunk, its
 * pens, style and flags, as they were); 0: nothing to move (another font or size, no system font, not a Font.prefs) */
static int ous_move(unsigned char *d, long len)
{
    unsigned char *f = ous_sysfont(d, len);
    if (!f || ((f[24] << 8) | f[25]) != OUS_SIZE) return 0;
    if (strncmp((const char *)f + 28, OUS_OLD_FONT, 128) != 0) return 0;
    memset(f + 28, 0, 128);
    memcpy(f + 28, OUS_NEW_FONT, sizeof OUS_NEW_FONT - 1);
    return 1;
}

#endif
