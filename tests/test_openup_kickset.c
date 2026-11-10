/* Host test of src/openup_kickset.h: which of a fake set's files KICKSET
 * copies for each host, and which leftover drawers it moves aside.
 *
 *   cc -std=c99 -Wall -Werror -Isrc tests/test_openup_kickset.c -o test_openup_kickset && ./test_openup_kickset
 *
 * tests/test_build_pack.py builds and runs it.
 *
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <stdio.h>
#include <string.h>
#include "openup_kickset.h"

static int fails;

static void is(const char *what, int got, int want)
{
    if (got == want) { printf("ok   %s\n", what); return; }
    printf("FAIL %s: got %d, want %d\n", what, got, want);
    fails++;
}

/* The pack holds 68k modules, bodies for 62, 183 and 20, and a file on demand. */
static const char *pack[] = {
    "SYS:Kickstart/default/icon.module",
    "SYS:Kickstart/default/Manifest",
    "SYS:Kickstart/default/.bodies/62/icon.module",
    "SYS:Kickstart/default/.bodies/183/icon.module",
    "SYS:Kickstart/default/.bodies/20/icon.module",
    "SYS:Libs/asl.library",
};
/* Already on the disk from an earlier install: two other CPUs' drawers. */
static const char *old[] = {
    "SYS:Kickstart/default/.bodies/183/old.module",
    "SYS:Kickstart/default/.bodies/20/ppc.module",
};

static int copied(unsigned host, const char *path)
{
    unsigned i;
    for (i = 0; i < sizeof pack / sizeof pack[0]; i++)
        if (!strcmp(pack[i], path) && ouk_copy(path, host)) return 1;
    return 0;
}

static int retired(unsigned host, const char *path)
{
    unsigned cpu = ouk_body_cpu(path), i;
    if (!cpu || !ouk_retire_drawer(cpu, host)) return 0;
    for (i = 0; i < sizeof old / sizeof old[0]; i++)
        if (!strcmp(old[i], path)) return 1;
    return 0;
}

static int landed(unsigned host, const char *path)
{
    if (retired(host, path)) return 0;
    if (copied(host, path)) return 1;
    return 0;
}

static const char *V2 =
    "# GFOpenKickLoad manifest 2\n"
    "manifest 2\n"
    "icon.module icon.library 47.5 4 aaaa 4\n"
    ".bodies/62/icon.module icon.library 47.5 8 bbbb 62\n"
    ".bodies/183/icon.module icon.library 47.5 8 cccc 183\n"
    ".bodies/20/icon.module icon.library 47.5 8 dddd 20\n";

static const char *V1 =
    "# GFOpenKickLoad manifest 1\n"
    "icon.module icon.library 47.5 4 aaaa\n"
    "workbench.module workbench.library 47.1 2 bbbb\n";

/* A stand-in disk: the 68k modules are there, and so is the x86-64 body only. */
static int present_host(const char *file, long size)
{
    (void)size;
    if (!strncmp(file, ".bodies/", 8)) return ouk_body_cpu(file) == 62;
    return 1;
}

static int missing_bodies(const char *text, unsigned host)
{
    char buf[600], *p, *w[8];
    int v2 = 0, missing = 0;
    snprintf(buf, sizeof buf, "%s", text);
    for (p = buf; *p; ) {
        char *line = p;
        int n = 0, kind;
        unsigned cpu = 0;
        const char *bytes = 0;
        while (*p && *p != '\n') p++;
        if (*p) *p++ = 0;
        while (*line == ' ' || *line == '\t') line++;
        if (!*line || *line == '#') continue;
        w[n++] = line;
        while (*line && n < 8) {
            while (*line && *line != ' ' && *line != '\t') line++;
            if (!*line) break;
            *line++ = 0;
            while (*line == ' ' || *line == '\t') line++;
            if (*line) w[n++] = line;
        }
        kind = ouk_manifest(n, w, v2, &cpu, &bytes);
        if (kind == OUK_VER) { v2 = 1; continue; }
        if (kind != OUK_FILE) continue;
        if (cpu == 4 || (host != 4 && cpu == host)) {
            if (!present_host(w[0], 0)) missing = 1;
        }
    }
    return missing;
}

int main(void)
{
    unsigned h62 = ouk_host_cpu(62), h183 = ouk_host_cpu(183), h20 = ouk_host_cpu(20);
    unsigned h68 = ouk_host_cpu(0), hplain = ouk_host_cpu(4), hunk = ouk_host_cpu(99);
    is("unset is 68k", (int)h68, 4);
    is("4 stays 4", (int)hplain, 4);
    is("62 stays 62", (int)h62, 62);

    is("x86-64: its body lands", landed(h62, "SYS:Kickstart/default/.bodies/62/icon.module"), 1);
    is("x86-64: the 68k module lands", landed(h62, "SYS:Kickstart/default/icon.module"), 1);
    is("x86-64: the Manifest lands", landed(h62, "SYS:Kickstart/default/Manifest"), 1);
    is("x86-64: a file on demand lands", landed(h62, "SYS:Libs/asl.library"), 1);
    is("x86-64: ARM64 is not copied", copied(h62, "SYS:Kickstart/default/.bodies/183/icon.module"), 0);
    is("x86-64: PowerPC is not copied", copied(h62, "SYS:Kickstart/default/.bodies/20/icon.module"), 0);
    is("x86-64: an old ARM64 drawer is retired", retired(h62, "SYS:Kickstart/default/.bodies/183/old.module"), 1);
    is("x86-64: an old PowerPC drawer is retired", retired(h62, "SYS:Kickstart/default/.bodies/20/ppc.module"), 1);

    is("ARM64: its body lands", landed(h183, "SYS:Kickstart/default/.bodies/183/icon.module"), 1);
    is("ARM64: x86-64 is not copied", copied(h183, "SYS:Kickstart/default/.bodies/62/icon.module"), 0);
    is("ARM64: the old PowerPC drawer is retired", retired(h183, "SYS:Kickstart/default/.bodies/20/ppc.module"), 1);
    is("ARM64: its own old file is not retired", retired(h183, "SYS:Kickstart/default/.bodies/183/old.module"), 0);

    is("PowerPC: its body lands", landed(h20, "SYS:Kickstart/default/.bodies/20/icon.module"), 1);
    is("PowerPC: the others are not copied",
       copied(h20, "SYS:Kickstart/default/.bodies/62/icon.module") ||
       copied(h20, "SYS:Kickstart/default/.bodies/183/icon.module"), 0);

    is("68k from unset: no body is copied",
       copied(h68, "SYS:Kickstart/default/.bodies/62/icon.module") ||
       copied(h68, "SYS:Kickstart/default/.bodies/183/icon.module") ||
       copied(h68, "SYS:Kickstart/default/.bodies/20/icon.module"), 0);
    is("68k: the modules still land", landed(h68, "SYS:Kickstart/default/icon.module"), 1);
    is("68k: every leftover drawer is retired",
       retired(h68, "SYS:Kickstart/default/.bodies/183/old.module") &&
       retired(h68, "SYS:Kickstart/default/.bodies/20/ppc.module"), 1);
    is("unknown host: no body is copied", copied(hunk, "SYS:Kickstart/default/.bodies/62/icon.module"), 0);

    /* Manifest 1: the same files as before, and no body appears. */
    is("version 1: the module is copied", ouk_copy("SYS:Kickstart/default/icon.module", h62), 1);
    is("version 1: a body path is still not copied for the wrong host",
       ouk_copy("SYS:Kickstart/default/.bodies/183/icon.module", h62), 0);
    is("version 1 set has no body to add",
       ouk_body_cpu("SYS:Kickstart/default/workbench.module"), 0);

    is("CHECK: x86-64's listed body is there", missing_bodies(V2, h62), 0);
    is("CHECK: ARM64's listed body is missing", missing_bodies(V2, h183), 1);
    is("CHECK: a 68k does not warn that other bodies are absent", missing_bodies(V2, h68), 0);
    is("CHECK: a version 1 set does not warn", missing_bodies(V1, h62), 0);

    printf(fails ? "%d failed\n" : "all passed\n", fails);
    return fails ? 1 : 0;
}
