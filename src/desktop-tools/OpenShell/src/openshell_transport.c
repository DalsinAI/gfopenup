/*
 * openshell_transport.c
 *
 * Integration boundary for the OpenShell UI and the real Amiga Shell.
 *
 * This intentionally contains no invented private OpenUp ABI. The repository
 * adapter should implement these operations using the project's selected
 * AmigaOS/AROS process, pipe and message primitives.
 */

#include "openshell_model.h"
#include <stddef.h>

typedef struct OSH_Transport OSH_Transport;

typedef void (*OSH_OutputFn)(void *userdata, const void *bytes, size_t n);

struct OSH_TransportOps {
    OSH_Transport *(*create)(const char *initial_path,
                             OSH_OutputFn output,
                             void *userdata);
    int (*submit)(OSH_Transport *t, const char *command);
    int (*send_break)(OSH_Transport *t);
    int (*request_cwd)(OSH_Transport *t, char *buffer, size_t size);
    int (*last_rc)(OSH_Transport *t);
    int (*busy)(OSH_Transport *t);
    void (*destroy)(OSH_Transport *t);
};

/*
 * The GUI layer depends only on this table. That lets us qualify the Shell
 * process transport independently from transcript rendering and tab UI.
 */
