#include "openup/openmouseaccel.h"
#include <assert.h>
#include <stdio.h>

static void feed(OMA_Context *ctx, int dx, int dy, unsigned long long t, OMA_OutputEvent *out)
{
    OMA_InputEvent in;
    in.dx = dx; in.dy = dy; in.timestamp_us = (uint64_t)t;
    OMA_Process(ctx, &in, out);
}

int main(void)
{
    OMA_Config cfg;
    OMA_Context ctx;
    OMA_OutputEvent out;

    OMA_DefaultConfig(&cfg);
    cfg.flags &= ~OMA_FLAG_LIMIT_OUTPUT_SPEED;
    OMA_Init(&ctx, &cfg);

    /* Prime timestamp, then verify slow movement remains 1:1. */
    feed(&ctx, 0, 0, 1000000ULL, &out);
    feed(&ctx, 1, 0, 1020000ULL, &out); /* 50 counts/s */
    assert(out.dx == 1);
    assert(out.multiplier == 1.0f);

    /* Fast movement accelerates. */
    feed(&ctx, 10, 0, 1025000ULL, &out); /* 2000 counts/s */
    assert(out.multiplier > 1.0f);
    assert(out.multiplier <= 10.0f);

    /* Rapid precision recovery: slow again returns to 1x. */
    feed(&ctx, 1, 0, 1045000ULL, &out);
    assert(out.multiplier == 1.0f);

    /* Disabled is exact pass-through. */
    cfg.flags &= ~OMA_FLAG_ENABLED;
    OMA_SetConfig(&ctx, &cfg);
    feed(&ctx, -7, 3, 1050000ULL, &out);
    assert(out.dx == -7 && out.dy == 3);

    /* 10x hard ceiling. */
    OMA_PresetExtreme(&cfg);
    cfg.flags &= ~OMA_FLAG_LIMIT_OUTPUT_SPEED;
    OMA_SetConfig(&ctx, &cfg);
    OMA_Reset(&ctx);
    feed(&ctx, 0, 0, 2000000ULL, &out);
    feed(&ctx, 100, 0, 2001000ULL, &out);
    assert(out.multiplier <= 10.0f);

    puts("GFOpenMouseAccel core tests: PASS");
    return 0;
}
