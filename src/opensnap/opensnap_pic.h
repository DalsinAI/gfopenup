/* OpenSnap's pictures in memory: the IFF ILBM it puts on the clipboard (and saves when there is no PNG
 * writer), HAM and Extra Half-Brite colours, and the file's name. Plain C with no OS calls, so
 * tests/test_opensnap_pic.c runs it on the build PC.
 *
 * The ILBM is written from the IFF and ILBM specifications (EA IFF 85, "ILBM" IFF Interleaved Bitmap,
 * and the Deep ILBM convention for 24 planes):
 *   - a screen of 256 colours or fewer: 1 to 8 planes and a CMAP of 1 << planes colours, and a CAMG with
 *     the screen's mode for the Amiga's own screens;
 *   - truecolour (RTG screens of 15 to 32 bits, and HAM, which is turned into its colours): 24 planes,
 *     red's eight bits first (bit 0 to bit 7), then green's, then blue's, and no CMAP;
 *   - BODY packed with ByteRun1, a row of each plane at a time.
 *
 * MIT, Copyright (c) 2026 AmigaChrome Team. */
#ifndef OPENSNAP_PIC_H
#define OPENSNAP_PIC_H

#include <stdint.h>
#include <string.h>

struct osp_pic {
    uint32_t w, h;                  /* the picture's size */
    unsigned planes;                /* indexed: 1 to 8; 0: truecolour from argb */
    const uint8_t *pens;            /* indexed: w * h pens, a byte each */
    const uint32_t *argb;           /* truecolour: w * h pixels, 0xAARRGGBB */
    uint32_t palette[256];          /* indexed: 0x00RRGGBB */
    uint32_t camg;                  /* the Amiga display mode for a CAMG chunk, or 0 for none */
    uint8_t xaspect, yaspect;       /* a pixel's width to its height */
    uint16_t pagew, pageh;          /* the screen's size */
};

static inline uint8_t *osp_be32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16); p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v;
    return p + 4;
}

static inline uint8_t *osp_be16(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v;
    return p + 2;
}

/* ByteRun1 (PackBits): n bytes from src into dst; the packed length. At most n + (n + 127) / 128. A run of
 * three or more equal bytes is a replicate run (one byte -(count - 1) and the byte); the rest go as literal
 * runs of up to 128 (count - 1 and the bytes). */
static inline uint32_t osp_pack(const uint8_t *src, uint32_t n, uint8_t *dst)
{
    uint32_t i = 0, o = 0;
    while (i < n) {
        uint32_t run = 1;
        while (i + run < n && run < 128 && src[i + run] == src[i]) run++;
        if (run >= 3) {
            dst[o++] = (uint8_t)(257 - run);   /* -(run - 1) */
            dst[o++] = src[i];
            i += run;
            continue;
        }
        {   /* a literal run: up to the next run of three equal bytes */
            uint32_t start = i, len = 0;
            while (i < n && len < 128) {
                if (i + 2 < n && src[i] == src[i + 1] && src[i] == src[i + 2]) break;
                i++; len++;
            }
            dst[o++] = (uint8_t)(len - 1);
            memcpy(dst + o, src + start, len);
            o += len;
        }
    }
    return o;
}

static inline uint32_t osp_rowbytes(uint32_t w) { return ((w + 15) >> 4) << 1; }

static inline unsigned osp_planes(const struct osp_pic *p) { return p->planes ? p->planes : 24; }

/* The most bytes osp_ilbm() writes for the picture. */
static inline uint32_t osp_ilbm_max(const struct osp_pic *p)
{
    uint32_t rb = osp_rowbytes(p->w), packed = rb + (rb + 127) / 128;
    uint32_t body = packed * osp_planes(p) * p->h;
    uint32_t cmap = p->planes ? 3u << p->planes : 0;
    return 12 + 8 + 20 + (cmap ? 8 + cmap : 0) + (p->camg ? 8 + 4 : 0) + 8 + body + 1;
}

/* One plane's row: bit `bit` of each pixel's byte (pens, or a channel of argb), as an ILBM plane row. */
static inline void osp_plane_row(const struct osp_pic *p, uint32_t y, unsigned plane, uint8_t *row)
{
    uint32_t rb = osp_rowbytes(p->w), x;
    memset(row, 0, rb);
    if (p->planes) {
        const uint8_t *s = p->pens + y * p->w;
        for (x = 0; x < p->w; x++)
            if (s[x] & (1u << plane)) row[x >> 3] |= (uint8_t)(0x80 >> (x & 7));
    } else {
        const uint32_t *s = p->argb + y * p->w;
        unsigned shift = (plane < 8 ? 16 : plane < 16 ? 8 : 0) + (plane & 7);   /* red's bits, green's, blue's */
        for (x = 0; x < p->w; x++)
            if ((s[x] >> shift) & 1) row[x >> 3] |= (uint8_t)(0x80 >> (x & 7));
    }
}

/* Writes the IFF ILBM into out (osp_ilbm_max(p) bytes); row is scratch of osp_rowbytes(p->w) bytes. The
 * file's length. */
static inline uint32_t osp_ilbm(const struct osp_pic *p, uint8_t *out, uint8_t *row)
{
    uint8_t *o = out, *bodylen;
    unsigned planes = osp_planes(p), d;
    uint32_t y, rb = osp_rowbytes(p->w), n, total;

    o = osp_be32(o, 0x464F524D);              /* FORM, its length at the end */
    o = osp_be32(o, 0);
    o = osp_be32(o, 0x494C424D);              /* ILBM */
    o = osp_be32(o, 0x424D4844);              /* BMHD */
    o = osp_be32(o, 20);
    o = osp_be16(o, p->w); o = osp_be16(o, p->h);
    o = osp_be16(o, 0); o = osp_be16(o, 0);   /* x, y */
    *o++ = (uint8_t)planes;
    *o++ = 0;                                 /* masking: none */
    *o++ = 1;                                 /* compression: ByteRun1 */
    *o++ = 0;                                 /* pad */
    o = osp_be16(o, 0);                       /* transparent colour */
    *o++ = p->xaspect ? p->xaspect : 1;
    *o++ = p->yaspect ? p->yaspect : 1;
    o = osp_be16(o, p->pagew ? p->pagew : p->w);
    o = osp_be16(o, p->pageh ? p->pageh : p->h);
    if (p->planes) {
        uint32_t i, colours = 1u << p->planes;
        o = osp_be32(o, 0x434D4150);          /* CMAP */
        o = osp_be32(o, colours * 3);
        for (i = 0; i < colours; i++) {
            *o++ = (uint8_t)(p->palette[i] >> 16);
            *o++ = (uint8_t)(p->palette[i] >> 8);
            *o++ = (uint8_t)p->palette[i];
        }
        if ((colours * 3) & 1) *o++ = 0;
    }
    if (p->camg) {
        o = osp_be32(o, 0x43414D47);          /* CAMG */
        o = osp_be32(o, 4);
        o = osp_be32(o, p->camg);
    }
    o = osp_be32(o, 0x424F4459);              /* BODY */
    bodylen = o;
    o = osp_be32(o, 0);
    n = 0;
    for (y = 0; y < p->h; y++)
        for (d = 0; d < planes; d++) {
            uint32_t k;
            osp_plane_row(p, y, d, row);
            k = osp_pack(row, rb, o);
            o += k; n += k;
        }
    osp_be32(bodylen, n);
    if (n & 1) *o++ = 0;
    total = (uint32_t)(o - out);
    osp_be32(out + 4, total - 8);
    return total;
}

/* Extra Half-Brite: colours 32 to 63 are 0 to 31 at half brightness. */
static inline void osp_ehb(uint32_t *palette)
{
    unsigned i;
    for (i = 0; i < 32; i++) palette[32 + i] = (palette[i] >> 1) & 0x7F7F7Fu;
}

/* A HAM row's colours (AGA's rule): planes 6 (HAM6) or 8 (HAM8); the two top bits of a pen say what it
 * does: 0 a base colour (its low 4 or 6 bits), 1 blue, 2 red, 3 green changed to the low bits, which become
 * the channel's top bits (the rest kept from the pixel before). A row starts from colour 0. */
static inline void osp_ham_row(const uint8_t *pens, uint32_t w, unsigned planes, const uint32_t *palette, uint32_t *argb)
{
    unsigned bits = planes == 8 ? 6 : 4, keep = (1u << (8 - bits)) - 1;
    uint32_t prev = palette[0] & 0xFFFFFFu, x;
    for (x = 0; x < w; x++) {
        unsigned pen = pens[x], ctl = (pen >> bits) & 3, val = pen & ((1u << bits) - 1);
        uint32_t c = prev;
        unsigned top = val << (8 - bits);
        if (ctl == 0) c = palette[val] & 0xFFFFFFu;
        else if (ctl == 1) c = (prev & 0xFFFF00u) | top | (prev & keep);
        else if (ctl == 2) c = (prev & 0x00FFFFu) | ((uint32_t)(top | ((prev >> 16) & keep)) << 16);
        else c = (prev & 0xFF00FFu) | ((uint32_t)(top | ((prev >> 8) & keep)) << 8);
        argb[x] = 0xFF000000u | c;
        prev = c;
    }
}

/* A window's title as part of a file name: letters and digits kept, anything else one '-', none at either
 * end, at most max - 1 characters. ASCII only, as the drive may be a folder on a PC. Empty when nothing
 * is left. */
static inline void osp_safe_title(const char *title, char *out, unsigned max)
{
    unsigned o = 0;
    int dash = 0;
    if (!max) return;
    for (; title && *title && o + 1 < max; title++) {
        unsigned char c = (unsigned char)*title;
        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
            if (dash && o) { out[o++] = '-'; if (o + 1 >= max) break; }
            dash = 0;
            out[o++] = (char)c;
        } else dash = 1;
    }
    out[o] = 0;
}

/* Snapshot-2026-10-24-15.42.07.png, with -Title for a window, and -2, -3... when the name is taken. */
static inline void osp_name(char *out, unsigned max, int year, int month, int day, int hour, int min, int sec,
                            const char *safe_title, unsigned n, const char *ext)
{
    char num[12] = "";
    unsigned i = 0;
    if (n > 1) {
        char t[11];
        unsigned k = 0;
        while (n && k < sizeof t) { t[k++] = (char)('0' + n % 10); n /= 10; }
        num[i++] = '-';
        while (k) num[i++] = t[--k];
        num[i] = 0;
    }
    /* no snprintf: libnix's pulls in its stdio */
    {
        char b[96];
        unsigned o = 0;
        const char *s;
        int v[6] = { year, month, day, hour, min, sec };
        const char seps[6] = { '-', '-', '-', '.', '.', 0 };
        for (s = "Snapshot-"; *s; s++) b[o++] = *s;
        for (i = 0; i < 6; i++) {
            if (i == 0) { b[o++] = (char)('0' + v[0] / 1000 % 10); b[o++] = (char)('0' + v[0] / 100 % 10); }
            b[o++] = (char)('0' + v[i] / 10 % 10); b[o++] = (char)('0' + v[i] % 10);
            if (seps[i]) b[o++] = seps[i];
        }
        if (safe_title && *safe_title) { b[o++] = '-'; for (s = safe_title; *s && o < 80; s++) b[o++] = *s; }
        for (s = num; *s; s++) b[o++] = *s;
        b[o++] = '.';
        for (s = ext; *s && o < 94; s++) b[o++] = *s;
        b[o] = 0;
        if (max) { strncpy(out, b, max - 1); out[max - 1] = 0; }
    }
}

#endif
