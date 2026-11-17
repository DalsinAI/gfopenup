/* SHA-256 (FIPS 180-4), written for GFOpenUpdate: a downloaded part's archive is installed only when its bytes
 * give the SHA-256 the index names (docs/design/GFOPENUPDATE.md). Plain C with no OS calls, the same on a
 * big-endian 68k and a little-endian host; tested by tests/test_openupdate_sha256.c against the standard's vectors.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef OPENUPDATE_SHA256_H
#define OPENUPDATE_SHA256_H

struct oud_sha {
    unsigned long h[8];                     /* 32 bits used of each */
    unsigned long bits_hi, bits_lo;         /* the length in bits */
    unsigned char block[64];
    int have;
};

#define OUD_M32(x) ((x) & 0xFFFFFFFFUL)
#define OUD_ROR(x, n) OUD_M32(((x) >> (n)) | ((x) << (32 - (n))))

static void oud_sha_block(struct oud_sha *s, const unsigned char *p)
{
    static const unsigned long k[64] = {
        0x428a2f98UL, 0x71374491UL, 0xb5c0fbcfUL, 0xe9b5dba5UL, 0x3956c25bUL, 0x59f111f1UL, 0x923f82a4UL, 0xab1c5ed5UL,
        0xd807aa98UL, 0x12835b01UL, 0x243185beUL, 0x550c7dc3UL, 0x72be5d74UL, 0x80deb1feUL, 0x9bdc06a7UL, 0xc19bf174UL,
        0xe49b69c1UL, 0xefbe4786UL, 0x0fc19dc6UL, 0x240ca1ccUL, 0x2de92c6fUL, 0x4a7484aaUL, 0x5cb0a9dcUL, 0x76f988daUL,
        0x983e5152UL, 0xa831c66dUL, 0xb00327c8UL, 0xbf597fc7UL, 0xc6e00bf3UL, 0xd5a79147UL, 0x06ca6351UL, 0x14292967UL,
        0x27b70a85UL, 0x2e1b2138UL, 0x4d2c6dfcUL, 0x53380d13UL, 0x650a7354UL, 0x766a0abbUL, 0x81c2c92eUL, 0x92722c85UL,
        0xa2bfe8a1UL, 0xa81a664bUL, 0xc24b8b70UL, 0xc76c51a3UL, 0xd192e819UL, 0xd6990624UL, 0xf40e3585UL, 0x106aa070UL,
        0x19a4c116UL, 0x1e376c08UL, 0x2748774cUL, 0x34b0bcb5UL, 0x391c0cb3UL, 0x4ed8aa4aUL, 0x5b9cca4fUL, 0x682e6ff3UL,
        0x748f82eeUL, 0x78a5636fUL, 0x84c87814UL, 0x8cc70208UL, 0x90befffaUL, 0xa4506cebUL, 0xbef9a3f7UL, 0xc67178f2UL };
    unsigned long w[64], a, b, c, d, e, f, g, h;
    int i;
    for (i = 0; i < 16; i++)
        w[i] = ((unsigned long)p[i * 4] << 24) | ((unsigned long)p[i * 4 + 1] << 16)
             | ((unsigned long)p[i * 4 + 2] << 8) | (unsigned long)p[i * 4 + 3];
    for (; i < 64; i++) {
        unsigned long s0 = OUD_ROR(w[i - 15], 7) ^ OUD_ROR(w[i - 15], 18) ^ (w[i - 15] >> 3);
        unsigned long s1 = OUD_ROR(w[i - 2], 17) ^ OUD_ROR(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = OUD_M32(w[i - 16] + s0 + w[i - 7] + s1);
    }
    a = s->h[0]; b = s->h[1]; c = s->h[2]; d = s->h[3]; e = s->h[4]; f = s->h[5]; g = s->h[6]; h = s->h[7];
    for (i = 0; i < 64; i++) {
        unsigned long s1 = OUD_ROR(e, 6) ^ OUD_ROR(e, 11) ^ OUD_ROR(e, 25);
        unsigned long t1 = OUD_M32(h + s1 + ((e & f) ^ (OUD_M32(~e) & g)) + k[i] + w[i]);
        unsigned long s0 = OUD_ROR(a, 2) ^ OUD_ROR(a, 13) ^ OUD_ROR(a, 22);
        unsigned long t2 = OUD_M32(s0 + ((a & b) ^ (a & c) ^ (b & c)));
        h = g; g = f; f = e; e = OUD_M32(d + t1); d = c; c = b; b = a; a = OUD_M32(t1 + t2);
    }
    s->h[0] = OUD_M32(s->h[0] + a); s->h[1] = OUD_M32(s->h[1] + b); s->h[2] = OUD_M32(s->h[2] + c);
    s->h[3] = OUD_M32(s->h[3] + d); s->h[4] = OUD_M32(s->h[4] + e); s->h[5] = OUD_M32(s->h[5] + f);
    s->h[6] = OUD_M32(s->h[6] + g); s->h[7] = OUD_M32(s->h[7] + h);
}

static void oud_sha_start(struct oud_sha *s)
{
    s->h[0] = 0x6a09e667UL; s->h[1] = 0xbb67ae85UL; s->h[2] = 0x3c6ef372UL; s->h[3] = 0xa54ff53aUL;
    s->h[4] = 0x510e527fUL; s->h[5] = 0x9b05688cUL; s->h[6] = 0x1f83d9abUL; s->h[7] = 0x5be0cd19UL;
    s->bits_hi = s->bits_lo = 0;
    s->have = 0;
}

static void oud_sha_add(struct oud_sha *s, const unsigned char *p, long len)
{
    while (len > 0) {
        unsigned long before = s->bits_lo;
        s->block[s->have++] = *p++;
        len--;
        s->bits_lo = OUD_M32(s->bits_lo + 8);
        if (s->bits_lo < before) s->bits_hi = OUD_M32(s->bits_hi + 1);
        if (s->have == 64) { oud_sha_block(s, s->block); s->have = 0; }
    }
}

/* The digest as 64 lower-case hex letters and a NUL into out (65 bytes). */
static void oud_sha_end(struct oud_sha *s, char *out)
{
    static const char hex[] = "0123456789abcdef";
    unsigned long hi = s->bits_hi, lo = s->bits_lo;
    int i;
    s->block[s->have++] = 0x80;
    if (s->have > 56) {
        while (s->have < 64) s->block[s->have++] = 0;
        oud_sha_block(s, s->block);
        s->have = 0;
    }
    while (s->have < 56) s->block[s->have++] = 0;
    for (i = 0; i < 4; i++) {
        s->block[56 + i] = (unsigned char)(hi >> (24 - 8 * i));
        s->block[60 + i] = (unsigned char)(lo >> (24 - 8 * i));
    }
    oud_sha_block(s, s->block);
    for (i = 0; i < 32; i++) {
        unsigned char b = (unsigned char)(s->h[i / 4] >> (24 - 8 * (i % 4)));
        out[i * 2] = hex[b >> 4];
        out[i * 2 + 1] = hex[b & 15];
    }
    out[64] = 0;
}

/* 1 when two SHA-256 in hex are the same, whatever the case of their letters. */
static int oud_sha_same(const char *a, const char *b)
{
    int i;
    for (i = 0; i < 64; i++) {
        char x = a[i], y = b[i];
        if (x >= 'A' && x <= 'F') x += 32;
        if (y >= 'A' && y <= 'F') y += 32;
        if (!x || x != y) return 0;
    }
    return !a[64] && !b[64];
}

#endif
