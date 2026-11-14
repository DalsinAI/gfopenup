/* Host test of src/openup_sysfont.h: Font prefs' system default text moves from DejaVuSansMono.font 11 to
 * GFOSMono.font 11, and nothing else does.
 *   cc -std=c99 -Wall -Werror -Isrc tests/test_openup_sysfont.c -o t && ./t
 * tests/test_build_pack.py builds and runs it.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <stdio.h>
#include <string.h>
#include "openup_sysfont.h"

static int fails, checks;
#define CHECK(c, msg) do { checks++; if (!(c)) { fails++; printf("FAIL %s\n", msg); } } while (0)

static void put32(unsigned char *p, unsigned long v) { p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v; }

/* a FONT chunk at o: fp_Type, pens, the font and its size */
static unsigned char *chunk(unsigned char *o, int type, const char *font, int size)
{
    memcpy(o, "FONT", 4); put32(o + 4, 156); o += 8;
    memset(o, 0, 156);
    o[15] = type; o[16] = 1; o[17] = 0; o[18] = 1; o[24] = size >> 8; o[25] = size & 255; o[26] = 0; o[27] = 0x42;
    strcpy((char *)o + 28, font);
    return o + 156;
}

static long prefs(unsigned char *buf, const char *wb, const char *sys, int syssize, const char *scr)
{
    unsigned char *o = buf + 12;
    memcpy(buf, "FORM", 4); memcpy(buf + 8, "PREF", 4);
    memcpy(o, "PRHD", 4); put32(o + 4, 6); memset(o + 8, 0, 6); o += 14;
    if (wb) o = chunk(o, 0, wb, 13);
    if (sys) o = chunk(o, 1, sys, syssize);
    if (scr) o = chunk(o, 2, scr, 13);
    put32(buf + 4, (unsigned long)(o - buf - 8));
    return o - buf;
}

int main(void)
{
    unsigned char a[1024], b[1024];
    long n;

    n = prefs(a, "GFOSSans.font", "DejaVuSansMono.font", 11, "GFOSSans.font");
    memcpy(b, a, n);
    CHECK(ous_move(a, n) == 1, "DejaVuSansMono.font 11 as the system font moves");
    CHECK(!strcmp((char *)ous_sysfont(a, n) + 28, "GFOSMono.font"), "to GFOSMono.font");
    {
        unsigned char *f = ous_sysfont(a, n), *g = ous_sysfont(b, n);
        int i, cleared = 1, rest_same;
        for (i = 13; i < 19; i++) if (f[28 + i]) cleared = 0;
        rest_same = !memcmp(f, g, 28) && !memcmp(f + 28 + 19, g + 28 + 19, 128 - 19) && cleared;
        CHECK(rest_same, "its pens, size, style and flags stay; the old name's last bytes are cleared");
        CHECK(((f[24] << 8) | f[25]) == 11, "size 11 still");
    }
    {
        int i, others = 1;
        for (i = 0; i < n; i++)
            if (a[i] != b[i] && !(i >= (ous_sysfont(a, n) - a) + 28 && i < (ous_sysfont(a, n) - a) + 28 + 128)) others = 0;
        CHECK(others, "only the system font's name changes in the file");
    }
    CHECK(ous_move(a, n) == 0, "a second run has nothing to move");

    n = prefs(a, NULL, "DejaVuSansMono.font", 13, NULL);
    CHECK(ous_move(a, n) == 0, "DejaVuSansMono.font 13 is the user's: stays");
    n = prefs(a, NULL, "topaz.font", 11, NULL);
    CHECK(ous_move(a, n) == 0, "another font at 11 stays");
    n = prefs(a, NULL, "DejaVuSansMono.fontx", 11, NULL);
    CHECK(ous_move(a, n) == 0, "only the exact name");
    n = prefs(a, "DejaVuSansMono.font", NULL, 0, "DejaVuSansMono.font");
    CHECK(ous_move(a, n) == 0 && ous_sysfont(a, n) == NULL, "no system font chunk: the screens' and icons' fonts are not touched");
    {
        unsigned char *f;
        n = prefs(a, NULL, "DejaVuSansMono.font", 11, NULL);
        f = ous_sysfont(a, n);
        f[24] = 0; f[25] = 11;
        CHECK(ous_move(a, n - 40) == 0, "a cut-off file is left alone");
    }
    memcpy(a, "FORM\0\0\0\4ILBM", 12);
    CHECK(ous_move(a, 12) == 0, "not a Font.prefs");
    CHECK(ous_move(NULL, 0) == 0, "no file");

    printf("%s test_openup_sysfont: %d checks\n", fails ? "FAIL" : "ok  ", checks);
    return fails != 0;
}
