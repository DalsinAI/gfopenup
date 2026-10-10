#ifndef OPENSHELL_MODEL_H
#define OPENSHELL_MODEL_H

#include <stddef.h>

typedef struct {
    char cwd[512];
    char title[128];
    int busy;
    int exit_code;
    unsigned long command_count;
} OSH_Tab;

typedef struct {
    OSH_Tab *tabs;
    size_t count;
    size_t active;
} OSH_Session;

#endif
