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

#endif
