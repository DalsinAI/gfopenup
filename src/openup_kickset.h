/* GFOpenUpTool KICKSET's plain C (no OS calls), shared with its host test
 * (tests/test_openup_kickset.c).
 *
 * A set may list a body for every CPU. KICKSET copies the 68k modules and
 * only .bodies/<this host>/. A real 68k, and a host that has not said
 * (od_HostCPU: 0 is 4), gets no bodies. Another CPU's drawer already on
 * the disk is moved aside, not deleted.
 *
 * The host number is elf.library's ElfHostCPU(0), which is od_HostCPU's
 * rule (amicradle-guest #666).
 *
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef OPENUP_KICKSET_H
#define OPENUP_KICKSET_H

#define OUK_SKIP 0
#define OUK_VER  1
#define OUK_FILE 2
#define OUK_BAD  (-1)

/* 0 or unset is a real 680x0, which is 4. Anything else is answered back. */
static unsigned ouk_host_cpu(unsigned raw)
{
    return raw ? raw : 4u;
}

/* The number in a ".bodies/<n>/" path component, or 0 when dest is not one.
 * A leading 0 is not a CPU's drawer. */
static unsigned ouk_body_cpu(const char *dest)
{
    const char *p;
    unsigned n;
    if (!dest) return 0;
    for (p = dest; *p; p++) {
        const char *s;
        if (p != dest && p[-1] != '/' && p[-1] != ':') continue;
        s = p;
        if (s[0] != '.' || s[1] != 'b' || s[2] != 'o' || s[3] != 'd' || s[4] != 'i' ||
            s[5] != 'e' || s[6] != 's' || s[7] != '/') continue;
        s += 8;
        if (*s < '1' || *s > '9') return 0;
        n = 0;
        while (*s >= '0' && *s <= '9') { n = n * 10u + (unsigned)(*s - '0'); s++; }
        if (*s != '/' && *s != 0) return 0;
        return n;
    }
    return 0;
}

/* 1 when KICKSET copies this destination. A body only for the host, and
 * never on a 68k. */
static int ouk_copy(const char *dest, unsigned host)
{
    unsigned cpu = ouk_body_cpu(dest);
    if (!cpu) return 1;
    if (!host || host == 4) return 0;
    return cpu == host;
}

/* 1 when a .bodies/<drawer> left from an earlier install is moved to
 * SYS:Kickstart/.retired/. The host's own drawer stays. A 68k keeps none. */
static int ouk_retire_drawer(unsigned drawer, unsigned host)
{
    if (!drawer) return 0;
    if (!host || host == 4) return 1;
    return drawer != host;
}

/* A decimal field with no leading zero. "0" is 0 and counts. */
static int ouk_u(const char *s, unsigned *out)
{
    unsigned v = 0;
    if (!s || !*s || (s[0] == '0' && s[1])) return 0;
    for (; *s; s++) {
        if (*s < '0' || *s > '9') return 0;
        v = v * 10u + (unsigned)(*s - '0');
    }
    *out = v;
    return 1;
}

/* One Manifest line, already split. v2 is 1 after a "manifest 2" line.
 * On OUK_FILE, *cpu is the e_machine (4 for a version-1 line) and *bytes
 * is the size field. */
static int ouk_manifest(int n, char **w, int v2, unsigned *cpu, const char **bytes)
{
    unsigned c;
    if (n <= 0) return OUK_SKIP;
    if (n == 2 && w[0][0] == 'm' && w[0][1] == 'a' && w[0][2] == 'n' && w[0][3] == 'i' &&
        w[0][4] == 'f' && w[0][5] == 'e' && w[0][6] == 's' && w[0][7] == 't' && !w[0][8]) {
        if (w[1][0] != '2' || w[1][1]) return OUK_BAD;
        return OUK_VER;
    }
    if (v2) {
        if (n != 6 || !ouk_u(w[5], &c) || !c) return OUK_BAD;
        *cpu = c;
        *bytes = w[3];
        return OUK_FILE;
    }
    if (n != 5) return OUK_BAD;
    *cpu = 4;
    *bytes = w[3];
    return OUK_FILE;
}

/* KICKSET's prune (14 Nov 2026, Team 5: a gfopenfiles.module left in Instance-32's SYS:Kickstart/default/ that the
 * new Manifest doesn't list made kickload refuse the whole set, so the update never took). A file there is pruned
 * (moved to SYS:Kickstart/.retired/) when it isn't the Manifest, an icon, a dot name (kickload passes over those),
 * and no Manifest line starts with its name and a blank. Names compare without case, as AmigaDOS does. */
static int ouk_prunable(const char *fn)
{
    unsigned k = 0;
    if (!fn || !fn[0] || fn[0] == '.') return 0;
    while (fn[k]) k++;
    if (k == 8 && (fn[0] | 0x20) == 'm' && (fn[1] | 0x20) == 'a' && (fn[2] | 0x20) == 'n' && (fn[3] | 0x20) == 'i' &&
        (fn[4] | 0x20) == 'f' && (fn[5] | 0x20) == 'e' && (fn[6] | 0x20) == 's' && (fn[7] | 0x20) == 't') return 0;
    if (k > 5 && fn[k - 5] == '.' && (fn[k - 4] | 0x20) == 'i' && (fn[k - 3] | 0x20) == 'n' && (fn[k - 2] | 0x20) == 'f' &&
        (fn[k - 1] | 0x20) == 'o') return 0;
    return 1;
}

static int ouk_listed(const char *manifest, const char *fn)
{
    const char *p = manifest;
    unsigned k = 0;
    if (!p || !fn) return 0;
    while (fn[k]) k++;
    while (*p) {
        unsigned i;
        for (i = 0; i < k && p[i] && (p[i] | 0x20) == (fn[i] | 0x20); i++) ;
        if (i == k && (p[k] == ' ' || p[k] == '\t')) return 1;
        while (*p && *p != '\n') p++;
        if (*p) p++;
    }
    return 0;
}

#endif
