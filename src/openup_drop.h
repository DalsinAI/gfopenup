/* What taking one listed file out does: REMOVE for each file of a part, and INSTALL's "drop <dest>" line, a file an
 * earlier version of the part put there that the part no longer has (GFOpenMUI 0.2.1's zunemaster.library and
 * ZuneTest, 14 Nov 2026). Only a file the manifest lists is touched: one GFOpenUp never put there stays.
 * Shared by src/openuptool.c and tests/test_openup_drop.c.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef OPENUP_DROP_H
#define OPENUP_DROP_H

#include <string.h>

enum {
    OUD_NONE,       /* not listed (the user's own, or never there): nothing changes */
    OUD_DELETE,     /* GFOpenUp put it there and it is still GFOpenUp's bytes: deleted, and leaves the list */
    OUD_STAYS,      /* GFOpenUp put it there but it has changed since: someone else's now, it stays, leaves the list */
    OUD_RESTORE,    /* GFOpenUp put it over a file it kept: that original comes back from SYS:Storage */
    OUD_FORGET      /* GFOpenUp found it already the same: it stays, and leaves the list */
};

/* kind: the manifest line's kind for the file ("new", "replaced", "same"...), NULL when it isn't listed; ours:
 * 1 when a "new" file still holds what GFOpenUp wrote (still_ours). */
static int oud_action(const char *kind, int ours)
{
    if (!kind || !*kind) return OUD_NONE;
    if (!strcmp(kind, "new")) return ours ? OUD_DELETE : OUD_STAYS;
    if (!strcmp(kind, "replaced")) return OUD_RESTORE;
    if (!strcmp(kind, "same")) return OUD_FORGET;
    return OUD_NONE;                        /* dir, startup, part...: not a file this takes out */
}

/* "drop <dest> <text>": 1 when the file's bytes (n of them, NULs and all) hold text, so only that file goes (GFOpenMUI
 * 0.2.1's muimaster.library, "a face over zunemaster.library", 19.1, which INSTALL's newer-file rule kept over the
 * clean-room 19.0 and which needs the zunemaster.library the same part drops: Ivanova's lab, 14 Nov 2026). No text:
 * any file; nothing read: no. */
static inline int oud_has_text(const char *buf, long n, const char *text)
{
    long i, t = (long)strlen(text);
    if (!t) return 1;
    if (!buf || n < t) return 0;
    for (i = 0; i + t <= n; i++)
        if (buf[i] == text[0] && !memcmp(buf + i, text, (size_t)t)) return 1;
    return 0;
}

/* A manifest entry another step cleared (kind[0] = 0: taken out, dropped, retired) has left the list, as
 * manifest_save leaves it out, so a lookup passes it by. Before, find_entry still returned it: after drop took 0.2.1's
 * muimaster.library face out, the clean-room 19.0's add found that cleared entry, wrote the file and listed nothing
 * (Ivanova's lab, bundle C, row 11). kind NULL: any kind. */
static inline int oud_match(const char *ekind, const char *ea, const char *kind, const char *a)
{
    return ekind[0] && !strcmp(ea, a) && (!kind || !strcmp(ekind, kind));
}

/* A drop line's words after "drop <dest>": [<text>] [unless <path> <text>]. The unless guard keeps dest while the
 * file at path still holds that text: GFOpenMUI's zunemaster.library stays while a muimaster.library face over it is
 * still there (one the user changed, which the face drop leaves: Flibble's note, Ivanova's lab bundle C row 13). */
static inline void oud_words(char **w, int n, const char **text, const char **upath, const char **utext)
{
    int i = 2;
    *text = *upath = *utext = NULL;
    if (i < n && strcmp(w[i], "unless")) *text = w[i++];
    if (i + 2 < n && !strcmp(w[i], "unless")) { *upath = w[i + 1]; *utext = w[i + 2]; }
}

#endif
