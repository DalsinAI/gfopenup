/* Host test of src/openup_unattended.h: how GFOpenUpTool UPDATE goes (asks, Installer, unattended) and the
 * unattended run's log line (0.8.20, Dale's 13 Nov 2026 rule: every installer can run unattended).
 *   cc -std=c99 -Wall -Werror -Isrc tests/test_openup_unattended.c -o t && ./t
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <stdio.h>
#include <string.h>
#include "openup_unattended.h"

static int fails, checks;
#define CHECK(c, msg) do { checks++; if (!(c)) { fails++; printf("FAIL %s\n", msg); } } while (0)

int main(void)
{
    int by;
    char buf[200];
    CHECK(ou_update_mode(NULL, &by) == OU_ASK && by == OU_BY_NONE, "plain UPDATE asks, as before (the menu's Update, flag or not)");
    CHECK(ou_update_mode("CHECK", &by) == OU_CHECK, "CHECK only says");
    CHECK(ou_update_mode("check", &by) == OU_CHECK, "CHECK any case");
    CHECK(ou_update_mode("YES", &by) == OU_INSTALLER, "YES runs the Installer, as before");
    CHECK(ou_update_mode("AUTO", &by) == OU_AUTO && by == OU_BY_SWITCH, "AUTO: unattended");
    CHECK(ou_update_mode("Auto", &by) == OU_AUTO, "AUTO any case");
    CHECK(ou_update_mode("AUTOX", &by) == OU_ASK, "a word that only starts AUTO isn't AUTO");
    CHECK(ou_update_mode("", &by) == OU_ASK, "empty word: asks");

    ou_log_line(buf, sizeof buf, 0, OU_BY_FLAG, "0.8.28", "0.8.29", 14, 0);
    CHECK(!strcmp(buf, "GFOpenUpTool UPDATE: unattended (host flag) 0.8.28 to 0.8.29, 14 parts reinstalled: ok"), buf);
    ou_log_line(buf, sizeof buf, 0, OU_BY_SWITCH, "0.8.28", "0.8.29", 1, 5);
    CHECK(!strcmp(buf, "GFOpenUpTool UPDATE: unattended (AUTO) 0.8.28 to 0.8.29, 1 part reinstalled: done, with warnings"), buf);
    ou_log_line(buf, sizeof buf, 0, OU_BY_ANSWERS, "", "", -1, 5);
    CHECK(!strcmp(buf, "GFOpenUpTool UPDATE: unattended (answers) GFOpenUp not installed, nothing to update: done, with warnings"), buf);
    ou_log_line(buf, sizeof buf, 0, OU_BY_FLAG, "0.8.28", "", -1, 0);
    CHECK(!strcmp(buf, "GFOpenUpTool UPDATE: unattended (host flag) 0.8.28 installed, no newer pack from Cradle: ok"), buf);
    ou_log_line(buf, sizeof buf, 0, OU_BY_FLAG, "0.8.29", "0.8.29", -1, 0);
    CHECK(!strcmp(buf, "GFOpenUpTool UPDATE: unattended (host flag) 0.8.29 installed, the newest Cradle has: ok"), buf);
    ou_log_line(buf, sizeof buf, 0, OU_BY_SWITCH, "0.8.28", "0.8.29", 3, 10);
    CHECK(strstr(buf, ": failed") && !strchr(buf, '\n'), "failed, one line");
    ou_log_line(buf, 16, 0, OU_BY_SWITCH, "0.8.28", "0.8.29", 3, 0);
    CHECK(strlen(buf) == 15, "cut to the buffer");
    ou_log_line(buf, sizeof buf, 0, 9, "0.8.28", "0.8.29", 120, 0);
    CHECK(strstr(buf, "(?)") && strstr(buf, "120 parts"), "an unknown reason, a three-digit count");
    /* the Kickstart set: AUTO installs it on the slim ROM whether or not the manifest lists it, as the Installer does
     * (Ivanova's unattended run, 14 Nov: KickstartSet is new on Instance-32, so a reinstall alone never added it) */
    CHECK(ou_auto_kickset(1, 1) == 1, "slim ROM, the pack has KickstartSet: AUTO installs it");
    CHECK(ou_auto_kickset(1, 0) == 0, "slim ROM, no KickstartSet in the pack: nothing");
    CHECK(ou_auto_kickset(0, 1) == 0, "another ROM: not installed (the Installer's rule)");
    ou_log_line(buf, sizeof buf, 1, OU_BY_FLAG, "0.8.28", "0.8.29", 14, 0);
    CHECK(!strcmp(buf, "GFOpenUpTool UPDATE: unattended (host flag) 0.8.28 to 0.8.29, 14 parts reinstalled, "
                       "the Kickstart set installed: ok"), buf);
    ou_log_line(buf, sizeof buf, -1, OU_BY_SWITCH, "0.8.28", "0.8.29", 3, 10);
    CHECK(strstr(buf, "3 parts reinstalled, the Kickstart set failed: failed") != NULL, buf);
    printf("%s   %d checks (GFOpenUpTool UPDATE unattended)\n", fails ? "FAIL" : "ok", checks);
    return fails != 0;
}
