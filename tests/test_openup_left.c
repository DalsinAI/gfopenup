/* Host test of src/openup_left.h: REINSTALL's list of the installed parts the pack hasn't got.
 *   cc -std=c99 -Wall -Werror -Isrc tests/test_openup_left.c -o t && ./t
 * tests/test_build_pack.py builds and runs it.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <stdio.h>
#include "openup_left.h"

static int fails, checks;
#define CHECK(c, msg) do { checks++; if (!(c)) { fails++; printf("FAIL %s: [%s]\n", msg, buf); } } while (0)

int main(void)
{
    char buf[40] = "";
    CHECK(!buf[0], "empty to start");
    oul_left_add(buf, sizeof buf, "GFOpenMUI");
    CHECK(!strcmp(buf, "GFOpenMUI"), "one name");
    oul_left_add(buf, sizeof buf, "GFOpenBase");
    CHECK(!strcmp(buf, "GFOpenMUI, GFOpenBase"), "two, after a comma");
    oul_left_add(buf, sizeof buf, "AVeryLongPartName");
    CHECK(!strcmp(buf, "GFOpenMUI, GFOpenBase, and more"), "one that doesn't fit: and more");
    oul_left_add(buf, sizeof buf, "X");
    CHECK(!strcmp(buf, "GFOpenMUI, GFOpenBase, and more"), "and more only once, later names dropped");
    CHECK(strlen(buf) < sizeof buf, "within the buffer");
    {
        char small[16] = "";
        oul_left_add(small, sizeof small, "ABCDEFGHIJKLMNOPQRST");
        CHECK(!strcmp(small, ", and more") || strlen(small) < sizeof small, "a first name too long: within the buffer");
    }
    printf("%d checks, %d failed\n", checks, fails);
    return fails != 0;
}
