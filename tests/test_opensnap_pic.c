/* Host test of src/opensnap/opensnap_pic.h, GFOpenSnap's ILBM writer, HAM and Extra Half-Brite colours and file names.
 *
 *   cc -std=c99 -Wall -Werror -Isrc/opensnap tests/test_opensnap_pic.c -o test_opensnap_pic && ./test_opensnap_pic
 *
 * Every ILBM it writes is read back here by a reader of its own (FORM, BMHD, CMAP, CAMG, BODY, ByteRun1
 * unpacked) and compared pixel for pixel. tests/test_build_pack.py runs it.
 *
 * MIT, Copyright (c) 2026 AmigaChrome Team. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "opensnap_pic.h"

static int fails, checks;

static void truth(const char *what, int ok)
{
    checks++;
    if (ok) return;
    printf("FAIL %s\n", what);
    fails++;
}

static uint32_t be32(const uint8_t *p) { return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3]; }
static uint32_t be16(const uint8_t *p) { return (uint32_t)(p[0] << 8) | p[1]; }

static uint32_t seed = 12345;
static uint32_t rnd(void) { seed = seed * 1103515245u + 12345u; return seed >> 8; }

/* ByteRun1 unpacked: n bytes wanted; the packed bytes used, or 0 on a bad stream */
static uint32_t unpack(const uint8_t *src, uint32_t avail, uint8_t *dst, uint32_t n)
{
    uint32_t i = 0, o = 0;
    while (o < n) {
        int8_t c;
        if (i >= avail) return 0;
        c = (int8_t)src[i++];
        if (c >= 0) {
            uint32_t k = (uint32_t)c + 1;
            if (i + k > avail || o + k > n) return 0;
            memcpy(dst + o, src + i, k); i += k; o += k;
        } else if (c != -128) {
            uint32_t k = (uint32_t)(1 - c);
            if (i >= avail || o + k > n) return 0;
            memset(dst + o, src[i++], k); o += k;
        }
    }
    return i;
}

struct read_back {
    uint32_t w, h, planes, comp, masking, xa, ya, pw, ph, camg, cmap_n;
    uint8_t cmap[768];
    const uint8_t *body; uint32_t bodylen;
    int ok;
};

static struct read_back parse(const uint8_t *f, uint32_t len)
{
    struct read_back r;
    uint32_t off;
    memset(&r, 0, sizeof r);
    if (len < 12 || be32(f) != 0x464F524D || be32(f + 8) != 0x494C424D || be32(f + 4) != len - 8) return r;
    for (off = 12; off + 8 <= len; ) {
        uint32_t id = be32(f + off), n = be32(f + off + 4);
        const uint8_t *d = f + off + 8;
        if (off + 8 + n > len) return r;
        if (id == 0x424D4844) {
            r.w = be16(d); r.h = be16(d + 2); r.planes = d[8]; r.masking = d[9]; r.comp = d[10];
            r.xa = d[14]; r.ya = d[15]; r.pw = be16(d + 16); r.ph = be16(d + 18);
        } else if (id == 0x434D4150) {
            r.cmap_n = n / 3; memcpy(r.cmap, d, n > 768 ? 768 : n);
        } else if (id == 0x43414D47) {
            r.camg = be32(d);
        } else if (id == 0x424F4459) {
            r.body = d; r.bodylen = n;
        }
        off += 8 + n + (n & 1);
    }
    r.ok = off == len && r.body != NULL;
    return r;
}

/* the BODY back to pixels: pens (planes <= 8) or 0x00RRGGBB (24) */
static int decode(const struct read_back *r, uint32_t *px)
{
    uint32_t rb = osp_rowbytes(r->w), y, x, d, i = 0;
    uint8_t row[1024];
    memset(px, 0, r->w * r->h * sizeof *px);
    for (y = 0; y < r->h; y++)
        for (d = 0; d < r->planes; d++) {
            uint32_t used = unpack(r->body + i, r->bodylen - i, row, rb);
            if (!used) return 0;
            i += used;
            for (x = 0; x < r->w; x++)
                if (row[x >> 3] & (0x80 >> (x & 7))) {
                    if (r->planes <= 8) px[y * r->w + x] |= 1u << d;
                    else px[y * r->w + x] |= 1u << ((d < 8 ? 16 : d < 16 ? 8 : 0) + (d & 7));
                }
        }
    return i == r->bodylen;
}

static void test_pack(void)
{
    static uint8_t src[1000], packed[1100], back[1000];
    uint32_t n, k, round;
    for (round = 0; round < 400; round++) {
        n = 1 + rnd() % 999;
        for (k = 0; k < n; k++)
            src[k] = (uint8_t)(round % 4 == 0 ? rnd() : round % 4 == 1 ? (k / (1 + round % 7)) : round % 4 == 2 ? 0 : (rnd() % 3));
        k = osp_pack(src, n, packed);
        if (k > n + (n + 127) / 128 || unpack(packed, k, back, n) != k || memcmp(src, back, n)) {
            truth("ByteRun1: packed and unpacked again, the same bytes, within its bound", 0);
            return;
        }
    }
    truth("ByteRun1: 400 rows of noise, runs and both, packed and unpacked again", 1);
    memset(src, 7, 300);
    k = osp_pack(src, 300, packed);
    truth("ByteRun1: 300 equal bytes are three replicate runs (6 bytes)", k == 6 && unpack(packed, k, back, 300) == 6);
    truth("ByteRun1: one byte is a literal run of one", osp_pack((const uint8_t *)"x", 1, packed) == 2 && packed[0] == 0);
}

static void test_indexed(void)
{
    static struct osp_pic p;
    static uint8_t pens[37 * 5], out[8192], row[64];
    static uint32_t px[37 * 5];
    struct read_back r;
    uint32_t i, len;
    int same = 1;
    memset(&p, 0, sizeof p);
    p.w = 37; p.h = 5; p.planes = 3; p.pens = pens; p.camg = 0x8004; p.xaspect = 22; p.yaspect = 44; p.pagew = 640; p.pageh = 256;
    for (i = 0; i < 8; i++) p.palette[i] = 0x102030u * i;
    for (i = 0; i < 37 * 5; i++) pens[i] = (uint8_t)(rnd() % 8);
    len = osp_ilbm(&p, out, row);
    truth("indexed: within osp_ilbm_max", len <= osp_ilbm_max(&p));
    r = parse(out, len);
    truth("indexed: FORM ILBM, its chunks inside it", r.ok);
    truth("indexed: BMHD 37 x 5, 3 planes, ByteRun1, no mask", r.w == 37 && r.h == 5 && r.planes == 3 && r.comp == 1 && r.masking == 0);
    truth("indexed: BMHD aspect 22:44 and the page 640 x 256", r.xa == 22 && r.ya == 44 && r.pw == 640 && r.ph == 256);
    truth("indexed: CMAP of 8 colours, as the palette", r.cmap_n == 8 && r.cmap[3] == 0x10 && r.cmap[4] == 0x20 && r.cmap[5] == 0x30 && r.cmap[21] == 0x70);
    truth("indexed: CAMG the mode", r.camg == 0x8004);
    truth("indexed: BODY unpacks to every row of every plane", decode(&r, px));
    for (i = 0; i < 37 * 5; i++) if (px[i] != pens[i]) same = 0;
    truth("indexed: each pixel's pen comes back", same);
}

static void test_eight_planes(void)
{
    static struct osp_pic p;
    static uint8_t pens[320 * 3], out[64000], row[64];
    static uint32_t px[320 * 3];
    struct read_back r;
    uint32_t i, len;
    int same = 1;
    memset(&p, 0, sizeof p);
    p.w = 320; p.h = 3; p.planes = 8; p.pens = pens;
    for (i = 0; i < 256; i++) p.palette[i] = i * 0x010101u;
    for (i = 0; i < 320 * 3; i++) pens[i] = (uint8_t)(i < 320 ? i : rnd());
    len = osp_ilbm(&p, out, row);
    r = parse(out, len);
    truth("8 planes: CMAP of 256 colours, no CAMG", r.ok && r.cmap_n == 256 && r.camg == 0 && r.cmap[3 * 200] == 200);
    truth("8 planes: decodes", decode(&r, px));
    for (i = 0; i < 320 * 3; i++) if (px[i] != pens[i]) same = 0;
    truth("8 planes: each pen comes back", same);
}

static void test_truecolour(void)
{
    static struct osp_pic p;
    static uint32_t argb[19 * 4], px[19 * 4];
    static uint8_t out[8192], row[64];
    struct read_back r;
    uint32_t i, len;
    int same = 1;
    memset(&p, 0, sizeof p);
    p.w = 19; p.h = 4; p.argb = argb;
    for (i = 0; i < 19 * 4; i++) argb[i] = 0xFF000000u | (rnd() & 0xFFFFFFu);
    argb[0] = 0xFF123456u;
    len = osp_ilbm(&p, out, row);
    truth("24-bit: within osp_ilbm_max", len <= osp_ilbm_max(&p));
    r = parse(out, len);
    truth("24-bit: 24 planes, no CMAP, no CAMG", r.ok && r.planes == 24 && r.cmap_n == 0 && r.camg == 0);
    truth("24-bit: the page is the picture's size when not given", r.pw == 19 && r.ph == 4 && r.xa == 1 && r.ya == 1);
    truth("24-bit: decodes", decode(&r, px));
    for (i = 0; i < 19 * 4; i++) if (px[i] != (argb[i] & 0xFFFFFFu)) same = 0;
    truth("24-bit: red's planes first, then green's, then blue's: each colour comes back", same);
}

static void test_colours(void)
{
    uint32_t pal[256], argb[8];
    uint8_t ham6[5] = { 0x01, 0x2F, 0x10 | 0x0A, 0x30 | 0x03, 0x00 };
    uint8_t ham8[3] = { 0x02, 0x80 | 0x3F, 0x40 | 0x01 };
    memset(pal, 0, sizeof pal);
    pal[0] = 0x000000; pal[1] = 0x112233; pal[2] = 0x445566; pal[31] = 0xFEFCFA;
    osp_ehb(pal);
    truth("Extra Half-Brite: 33 is 1 at half brightness", pal[33] == 0x081119);
    truth("Extra Half-Brite: 63 is 31 at half brightness", pal[63] == 0x7F7E7D);
    osp_ham_row(ham6, 5, 6, pal, argb);
    truth("HAM6: a base colour", argb[0] == 0xFF112233u);
    truth("HAM6: red to F, its low nibble kept", argb[1] == 0xFFF12233u);
    truth("HAM6: blue to A", argb[2] == 0xFFF122A3u);
    truth("HAM6: green to 3", argb[3] == 0xFFF132A3u);
    truth("HAM6: back to colour 0", argb[4] == 0xFF000000u);
    osp_ham_row(ham8, 3, 8, pal, argb);
    truth("HAM8: a base colour", argb[0] == 0xFF445566u);
    truth("HAM8: red's top six bits all set, its low two kept", argb[1] == 0xFFFC5566u);
    truth("HAM8: blue's top six bits 000001", argb[2] == 0xFFFC5506u);
}

static void test_names(void)
{
    char t[24], n[96];
    osp_safe_title("Shell: 1 / Work", t, sizeof t);
    truth("a title: letters and digits kept, the rest one dash each", !strcmp(t, "Shell-1-Work"));
    osp_safe_title("  ***Caf\xe9 au lait!  ", t, sizeof t);
    truth("a title: no dash at either end; Latin-1 letters are left out", !strcmp(t, "Caf-au-lait"));
    osp_safe_title("***", t, sizeof t);
    truth("a title of no letters: empty", t[0] == 0);
    osp_safe_title(NULL, t, sizeof t);
    truth("no title: empty", t[0] == 0);
    osp_safe_title("A very long window title that goes on and on", t, sizeof t);
    truth("a long title: cut to fit, no dash at the end", strlen(t) <= sizeof t - 1 && t[strlen(t) - 1] != '-');
    osp_name(n, sizeof n, 2026, 10, 24, 15, 42, 7, "", 0, "png");
    truth("the screen's name", !strcmp(n, "Snapshot-2026-10-24-15.42.07.png"));
    osp_name(n, sizeof n, 2026, 1, 2, 3, 4, 5, "Shell-1", 0, "png");
    truth("a window's name", !strcmp(n, "Snapshot-2026-01-02-03.04.05-Shell-1.png"));
    osp_name(n, sizeof n, 2026, 10, 24, 15, 42, 7, "", 2, "iff");
    truth("the second in the same second", !strcmp(n, "Snapshot-2026-10-24-15.42.07-2.iff"));
    osp_name(n, sizeof n, 2026, 10, 24, 15, 42, 7, "Ed", 12, "png");
    truth("the twelfth", !strcmp(n, "Snapshot-2026-10-24-15.42.07-Ed-12.png"));
}

int main(void)
{
    test_pack();
    test_indexed();
    test_eight_planes();
    test_truecolour();
    test_colours();
    test_names();
    printf("%s: %d checks, %d failed\n", fails ? "FAIL" : "ok  ", checks, fails);
    return fails ? 1 : 0;
}
