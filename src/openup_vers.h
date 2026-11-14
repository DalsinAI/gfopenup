/* A file's version, from its "$VER: <name> <version> (<date>)" string, and INSTALL's rule that an older file never
 * goes over a newer one (Team 5, 14 Nov 2026: REINSTALL put older GFOpenBench, Write, Calc and GFOffice files over
 * newer ones on the lab, silently). Plain C with no OS calls, shared with tests/test_openup_vers.c.
 *
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef OPENUP_VERS_H
#define OPENUP_VERS_H

static int ouv_digit(char c) { return c >= '0' && c <= '9'; }

/* The version in the first "$VER: " of buf (len bytes): the first word after the name that starts with a digit and
 * holds a dot, before any "(" (the date), on the same line. Copied into out (size bytes): 1, else 0 (no $VER, or
 * none there that reads as a version). */
static int ouv_ver(const char *buf, long len, char *out, int size)
{
    long i, j;
    for (i = 0; i + 6 <= len; i++) {
        if (buf[i] != '$' || buf[i + 1] != 'V' || buf[i + 2] != 'E' || buf[i + 3] != 'R' || buf[i + 4] != ':') continue;
        j = i + 5;
        while (j < len && buf[j] == ' ') j++;
        while (j < len && buf[j] != ' ' && buf[j] && buf[j] != '\n') j++;            /* the name */
        while (j < len && buf[j] && buf[j] != '\n' && buf[j] != '(') {
            while (j < len && buf[j] == ' ') j++;
            if (j < len && ouv_digit(buf[j])) {
                long k = j;
                int n = 0, dot = 0;
                while (k < len && (ouv_digit(buf[k]) || buf[k] == '.')) dot |= buf[k++] == '.';
                if (dot && k - j < size) {
                    while (j < k) out[n++] = buf[j++];
                    while (n > 0 && out[n - 1] == '.') n--;
                    out[n] = 0;
                    return n > 0;
                }
                j = k;
            }
            while (j < len && buf[j] != ' ' && buf[j] && buf[j] != '\n' && buf[j] != '(') j++;
        }
        return 0;                                   /* the first $VER is the file's own; another is a borrowed one */
    }
    return 0;
}

/* Versions compare as numbers, dot by dot: 47.10 is newer than 47.9. -1, 0 or 1. */
static int ouv_cmp(const char *a, const char *b)
{
    for (;;) {
        unsigned long x = 0, y = 0;
        int ma = 0, mb = 0;
        while (ouv_digit(*a)) { x = x * 10 + (unsigned long)(*a++ - '0'); ma = 1; }
        while (ouv_digit(*b)) { y = y * 10 + (unsigned long)(*b++ - '0'); mb = 1; }
        if (x != y) return x < y ? -1 : 1;
        if (!ma && !mb) return 0;
        if (*a == '.') a++;
        if (*b == '.') b++;
    }
}

/* 1 when the file on the disk (its version there_v, "" none) is newer than the pack's (from_v) and FORCE wasn't
 * given: it stays, and INSTALL says so. A file without a version on either side is weighed as before (written). */
static int ouv_keep_newer(const char *from_v, const char *there_v, int force)
{
    if (force || !from_v || !*from_v || !there_v || !*there_v) return 0;
    return ouv_cmp(there_v, from_v) > 0;
}

#endif
