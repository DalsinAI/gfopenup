/*
 * OpenInput glue.
 *
 * The actual OpenInput ABI is project-owned and may evolve, so this module
 * deliberately exposes a tiny shim instead of inventing private OS hooks.
 * Wire OMA_FilterProcess() immediately after raw relative-device
 * normalisation and before pointer-position clamping/delivery.
 */
#include "openup/openmouseaccel.h"

void OMA_FilterProcess(OMA_Context *ctx,
                       int32_t dx,
                       int32_t dy,
                       uint64_t timestamp_us,
                       int32_t *out_dx,
                       int32_t *out_dy,
                       float *out_multiplier)
{
    OMA_InputEvent in;
    OMA_OutputEvent out;
    in.dx = dx;
    in.dy = dy;
    in.timestamp_us = timestamp_us;
    OMA_Process(ctx, &in, &out);
    if (out_dx) *out_dx = out.dx;
    if (out_dy) *out_dy = out.dy;
    if (out_multiplier) *out_multiplier = out.multiplier;
}
