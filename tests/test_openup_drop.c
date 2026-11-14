/* Host test of src/openup_drop.h: what REMOVE and INSTALL's "drop" line do to one listed file.
 *   cc -std=c99 -Wall -Werror -Isrc tests/test_openup_drop.c -o t && ./t
 * tests/test_build_pack.py builds and runs it.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <stdio.h>
#include "openup_drop.h"

static int fails, checks;
#define CHECK(c, msg) do { checks++; if (!(c)) { fails++; printf("FAIL %s\n", msg); } } while (0)

int main(void)
{
    CHECK(oud_action("new", 1) == OUD_DELETE, "a file GFOpenUp put there, still its bytes: deleted");
    CHECK(oud_action("new", 0) == OUD_STAYS, "a file GFOpenUp put there, changed since: stays");
    CHECK(oud_action("replaced", 1) == OUD_RESTORE && oud_action("replaced", 0) == OUD_RESTORE,
          "a file GFOpenUp put over another: the original back");
    CHECK(oud_action("same", 1) == OUD_FORGET, "a file that was already the same: stays, off the list");
    CHECK(oud_action(NULL, 1) == OUD_NONE, "not listed: the user's own, untouched");
    CHECK(oud_action("", 1) == OUD_NONE, "a line already taken out: untouched");
    CHECK(oud_action("dir", 1) == OUD_NONE && oud_action("startup", 1) == OUD_NONE && oud_action("part", 1) == OUD_NONE,
          "a drawer, a startup line or a part line: not a file to take out");
    /* drop <dest> <text>: only the file holding that text (GFOpenMUI 0.2.1's muimaster.library face, 19.1, which
     * the newer-file rule kept over the clean-room 19.0: Ivanova's lab, 14 Nov) */
    {
        static const char face[] = "\0\0\x03\xf3...\0$VER: muimaster.library 19.1 (9.10.2026) GFOpenMUI: a face over zunemaster.library";
        static const char clean[] = "\0\0\x03\xf3...\0$VER: muimaster.library 19.0 (10.11.2026) GFOpenMUI, AmiCradle Team";
        CHECK(oud_has_text(face, (long)sizeof face, "a face over zunemaster.library") == 1, "the face holds its text, past NULs");
        CHECK(oud_has_text(clean, (long)sizeof clean, "a face over zunemaster.library") == 0, "the clean-room library doesn't");
        CHECK(oud_has_text(face, 0, "a face") == 0 && oud_has_text(NULL, 9, "a") == 0, "nothing read: no");
        CHECK(oud_has_text(face, (long)sizeof face, "") == 1, "no text asked: any file");
    }
    /* a cleared entry has left the list (Ivanova's bundle C, row 11): the 19.0's add after the face drop must find
     * none, so it lists its own file as "new" */
    {
        struct { char kind[12]; const char *a; } m[] = { { "", "LIBS:muimaster.library" }, { "new", "LIBS:muimaster.library" },
                                                         { "dir", "SYS:Tools/GFOpenMUI" } };
        CHECK(!oud_match(m[0].kind, m[0].a, NULL, "LIBS:muimaster.library"), "a cleared entry isn't found");
        CHECK(oud_match(m[1].kind, m[1].a, NULL, "LIBS:muimaster.library"), "the live one after it is");
        CHECK(oud_match(m[2].kind, m[2].a, "dir", "SYS:Tools/GFOpenMUI") && !oud_match(m[2].kind, m[2].a, "new", m[2].a),
              "a kind asked for: only that kind");
    }
    /* drop <dest> [<text>] [unless <path> <text>] (Flibble; bundle C row 13) */
    {
        char *a[] = { "drop", "LIBS:zunemaster.library", "unless", "LIBS:muimaster.library", "a face over zunemaster.library" };
        char *b[] = { "drop", "LIBS:muimaster.library", "a face over zunemaster.library" };
        char *c[] = { "drop", "SYS:Tools/ZuneTest" };
        char *d[] = { "drop", "X", "t", "unless", "P", "u" };
        char *e[] = { "drop", "X", "unless", "P" };
        const char *t, *up, *ut;
        oud_words(a, 5, &t, &up, &ut);
        CHECK(!t && up && !strcmp(up, "LIBS:muimaster.library") && !strcmp(ut, "a face over zunemaster.library"), "unless alone");
        oud_words(b, 3, &t, &up, &ut);
        CHECK(t && !strcmp(t, "a face over zunemaster.library") && !up && !ut, "a text alone");
        oud_words(c, 2, &t, &up, &ut);
        CHECK(!t && !up && !ut, "the plain drop");
        oud_words(d, 6, &t, &up, &ut);
        CHECK(!strcmp(t, "t") && !strcmp(up, "P") && !strcmp(ut, "u"), "a text and unless");
        oud_words(e, 4, &t, &up, &ut);
        CHECK(!t && !up && !ut, "unless without its text: no guard read past the line");
    }
    printf("%d checks, %d failed\n", checks, fails);
    return fails != 0;
}
