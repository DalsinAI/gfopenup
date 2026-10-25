/*
 * OpenCalc UI front-end.
 *
 * The calculator engine in opencalc.c is toolkit-independent.
 * This file intentionally keeps the UI adapter thin so the OpenUp tree can
 * bind it to the project's preferred MUI/OpenGUI wrapper without touching
 * the maths engine.
 *
 * Intended layout:
 *
 * +--------------------------------------------------+
 * | [ DEG ]                         OpenCalc          |
 * |--------------------------------------------------|
 * |  12 + sqrt(81)                                    |
 * |                                      = 21        |
 * |--------------------------------------------------|
 * | MC MR M+ M- | ( ) % /                            |
 * | 7  8  9     | sin cos tan                        |
 * | 4  5  6     | ln  log sqrt                       |
 * | 1  2  3     | x^y  pi  e                         |
 * | 0  .  +/-   |       =                            |
 * +--------------------------------------------------+
 *
 * Keyboard:
 *   digits/operators type directly
 *   Return evaluates
 *   Escape clears entry
 *   Ctrl+C copies result
 *   Ctrl+V pastes an expression
 *   Up/Down browse calculation history
 */

#include "opencalc.h"
#include <stdio.h>
#include <string.h>

static void print_result(OC_Context *ctx, const char *line) {
    double result = 0.0;
    OC_Status st = OC_Eval(ctx, line, &result);
    if (st == OC_OK)
        printf("= %.15g\n", result);
    else
        printf("%s\n", OC_StatusText(st));
}

int main(int argc, char **argv) {
    OC_Context ctx;
    OC_Init(&ctx);

    if (argc > 1) {
        print_result(&ctx, argv[1]);
        return 0;
    }

    puts("OpenCalc 0.1 - console development front-end");
    puts("The OpenUp GUI adapter should call the same OC_Eval() engine.");
    puts("Type q to quit.");

    char line[512];
    while (fputs("> ", stdout), fflush(stdout), fgets(line, sizeof(line), stdin)) {
        line[strcspn(line, "\r\n")] = 0;
        if (!strcmp(line, "q") || !strcmp(line, "quit")) break;
        if (!strcmp(line, "deg")) { ctx.degrees = 1; puts("Degrees"); continue; }
        if (!strcmp(line, "rad")) { ctx.degrees = 0; puts("Radians"); continue; }
        print_result(&ctx, line);
    }
    return 0;
}
