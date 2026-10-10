#include "openup/openmouseaccel.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

static float clampf_local(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static float angle_degrees(float ax, float ay, float bx, float by)
{
    float amag = sqrtf(ax * ax + ay * ay);
    float bmag = sqrtf(bx * bx + by * by);
    float c;

    if (amag <= 0.0f || bmag <= 0.0f) return 0.0f;
    c = (ax * bx + ay * by) / (amag * bmag);
    c = clampf_local(c, -1.0f, 1.0f);
    return acosf(c) * 57.29577951308232f;
}

void OMA_DefaultConfig(OMA_Config *cfg)
{
    if (!cfg) return;
    cfg->precision_velocity = 80.0f;
    cfg->full_accel_velocity = 1200.0f;
    cfg->max_multiplier = 6.0f;
    cfg->curve_exponent = 2.0f;
    cfg->smoothing_up = 0.55f;
    cfg->smoothing_down = 0.85f;
    cfg->direction_reset_angle = 90.0f;
    cfg->max_output_velocity = 3000.0f;
    cfg->flags = OMA_FLAG_ENABLED |
                 OMA_FLAG_RAPID_PRECISION_RECOVERY |
                 OMA_FLAG_DIRECTION_RESET |
                 OMA_FLAG_LIMIT_OUTPUT_SPEED;
}

void OMA_PresetPrecision(OMA_Config *cfg)
{
    OMA_DefaultConfig(cfg);
    cfg->precision_velocity = 120.0f;
    cfg->full_accel_velocity = 1500.0f;
    cfg->max_multiplier = 3.0f;
    cfg->curve_exponent = 2.4f;
    cfg->max_output_velocity = 1800.0f;
}

void OMA_PresetBalanced(OMA_Config *cfg)
{
    OMA_DefaultConfig(cfg);
}

void OMA_PresetFast(OMA_Config *cfg)
{
    OMA_DefaultConfig(cfg);
    cfg->precision_velocity = 65.0f;
    cfg->full_accel_velocity = 1000.0f;
    cfg->max_multiplier = 8.0f;
    cfg->curve_exponent = 1.8f;
    cfg->max_output_velocity = 4200.0f;
}

void OMA_PresetExtreme(OMA_Config *cfg)
{
    OMA_DefaultConfig(cfg);
    cfg->precision_velocity = 50.0f;
    cfg->full_accel_velocity = 800.0f;
    cfg->max_multiplier = 10.0f;
    cfg->curve_exponent = 1.5f;
    cfg->max_output_velocity = 6000.0f;
}

int OMA_ValidateConfig(OMA_Config *cfg)
{
    int changed = 0;
    if (!cfg) return -1;

    if (cfg->precision_velocity < 1.0f) { cfg->precision_velocity = 1.0f; changed = 1; }
    if (cfg->full_accel_velocity <= cfg->precision_velocity) {
        cfg->full_accel_velocity = cfg->precision_velocity + 1.0f;
        changed = 1;
    }
    if (cfg->max_multiplier < OMA_MIN_MULTIPLIER) { cfg->max_multiplier = OMA_MIN_MULTIPLIER; changed = 1; }
    if (cfg->max_multiplier > OMA_MAX_MULTIPLIER) { cfg->max_multiplier = OMA_MAX_MULTIPLIER; changed = 1; }
    if (cfg->curve_exponent < 0.1f) { cfg->curve_exponent = 0.1f; changed = 1; }
    if (cfg->curve_exponent > 8.0f) { cfg->curve_exponent = 8.0f; changed = 1; }
    cfg->smoothing_up = clampf_local(cfg->smoothing_up, 0.0f, 1.0f);
    cfg->smoothing_down = clampf_local(cfg->smoothing_down, 0.0f, 1.0f);
    cfg->direction_reset_angle = clampf_local(cfg->direction_reset_angle, 0.0f, 180.0f);
    if (cfg->max_output_velocity < 1.0f) { cfg->max_output_velocity = 1.0f; changed = 1; }
    return changed;
}

void OMA_Init(OMA_Context *ctx, const OMA_Config *cfg)
{
    OMA_Config temp;
    if (!ctx) return;
    memset(ctx, 0, sizeof(*ctx));
    if (cfg) temp = *cfg; else OMA_DefaultConfig(&temp);
    OMA_ValidateConfig(&temp);
    ctx->config = temp;
}

void OMA_Reset(OMA_Context *ctx)
{
    if (!ctx) return;
    ctx->last_timestamp_us = 0;
    ctx->smoothed_velocity = 0.0f;
    ctx->residual_x = 0.0f;
    ctx->residual_y = 0.0f;
    ctx->last_dir_x = 0.0f;
    ctx->last_dir_y = 0.0f;
    ctx->has_last_direction = 0;
}

void OMA_SetConfig(OMA_Context *ctx, const OMA_Config *cfg)
{
    OMA_Config temp;
    if (!ctx || !cfg) return;
    temp = *cfg;
    OMA_ValidateConfig(&temp);
    ctx->config = temp;
}

void OMA_GetConfig(const OMA_Context *ctx, OMA_Config *cfg)
{
    if (!ctx || !cfg) return;
    *cfg = ctx->config;
}

static float compute_multiplier(const OMA_Config *cfg, float velocity)
{
    float t, curve;
    if (velocity <= cfg->precision_velocity) return 1.0f;
    if (velocity >= cfg->full_accel_velocity) return cfg->max_multiplier;

    t = (velocity - cfg->precision_velocity) /
        (cfg->full_accel_velocity - cfg->precision_velocity);
    t = clampf_local(t, 0.0f, 1.0f);
    curve = powf(t, cfg->curve_exponent);
    return 1.0f + curve * (cfg->max_multiplier - 1.0f);
}

void OMA_Process(OMA_Context *ctx, const OMA_InputEvent *in, OMA_OutputEvent *out)
{
    float dt_s = 0.0f;
    float fx, fy, distance;
    float inst_velocity = 0.0f;
    float alpha;
    float multiplier = 1.0f;
    float outx, outy;
    float dir_mag;

    if (!ctx || !in || !out) return;

    out->dx = in->dx;
    out->dy = in->dy;
    out->instantaneous_velocity = 0.0f;
    out->smoothed_velocity = ctx->smoothed_velocity;
    out->multiplier = 1.0f;

    if (!(ctx->config.flags & OMA_FLAG_ENABLED)) {
        ctx->last_timestamp_us = in->timestamp_us;
        return;
    }

    fx = (float)in->dx;
    fy = (float)in->dy;
    distance = sqrtf(fx * fx + fy * fy);

    if (ctx->last_timestamp_us != 0 && in->timestamp_us > ctx->last_timestamp_us) {
        uint64_t delta_us = in->timestamp_us - ctx->last_timestamp_us;
        /* Guard against zero/near-zero timing spikes and stale events. */
        if (delta_us < 250u) delta_us = 250u;
        if (delta_us > 100000u) delta_us = 100000u;
        dt_s = (float)delta_us / 1000000.0f;
        inst_velocity = distance / dt_s;
    }
    ctx->last_timestamp_us = in->timestamp_us;

    if (distance > 0.0f) {
        dir_mag = distance;
        fx /= dir_mag;
        fy /= dir_mag;

        if ((ctx->config.flags & OMA_FLAG_DIRECTION_RESET) && ctx->has_last_direction) {
            float angle = angle_degrees(ctx->last_dir_x, ctx->last_dir_y, fx, fy);
            if (angle >= ctx->config.direction_reset_angle) {
                ctx->smoothed_velocity = inst_velocity;
            }
        }
        ctx->last_dir_x = fx;
        ctx->last_dir_y = fy;
        ctx->has_last_direction = 1;
    }

    if ((ctx->config.flags & OMA_FLAG_RAPID_PRECISION_RECOVERY) &&
        inst_velocity <= ctx->config.precision_velocity) {
        ctx->smoothed_velocity = inst_velocity;
    } else {
        alpha = (inst_velocity >= ctx->smoothed_velocity)
              ? ctx->config.smoothing_up
              : ctx->config.smoothing_down;
        ctx->smoothed_velocity =
            ctx->smoothed_velocity * (1.0f - alpha) + inst_velocity * alpha;
    }

    multiplier = compute_multiplier(&ctx->config, ctx->smoothed_velocity);

    outx = (float)in->dx * multiplier;
    outy = (float)in->dy * multiplier;

    if ((ctx->config.flags & OMA_FLAG_LIMIT_OUTPUT_SPEED) && dt_s > 0.0f) {
        float max_distance = ctx->config.max_output_velocity * dt_s;
        float actual_distance = sqrtf(outx * outx + outy * outy);
        if (actual_distance > max_distance && actual_distance > 0.0f) {
            float s = max_distance / actual_distance;
            outx *= s;
            outy *= s;
        }
    }

    outx += ctx->residual_x;
    outy += ctx->residual_y;

    if (outx >= 0.0f) out->dx = (int32_t)floorf(outx);
    else out->dx = (int32_t)ceilf(outx);
    if (outy >= 0.0f) out->dy = (int32_t)floorf(outy);
    else out->dy = (int32_t)ceilf(outy);

    ctx->residual_x = outx - (float)out->dx;
    ctx->residual_y = outy - (float)out->dy;

    out->instantaneous_velocity = inst_velocity;
    out->smoothed_velocity = ctx->smoothed_velocity;
    out->multiplier = multiplier;
}
