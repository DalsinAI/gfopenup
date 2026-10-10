#ifndef OPENUP_OPENMOUSEACCEL_H
#define OPENUP_OPENMOUSEACCEL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OMA_VERSION_MAJOR 0
#define OMA_VERSION_MINOR 1
#define OMA_VERSION_PATCH 0

#define OMA_MIN_MULTIPLIER 1.0f
#define OMA_MAX_MULTIPLIER 10.0f

#define OMA_FLAG_ENABLED                  (1u << 0)
#define OMA_FLAG_RAPID_PRECISION_RECOVERY (1u << 1)
#define OMA_FLAG_DIRECTION_RESET          (1u << 2)
#define OMA_FLAG_LIMIT_OUTPUT_SPEED       (1u << 3)

typedef struct OMA_Config {
    float precision_velocity;      /* counts / second; <= this is always 1:1 */
    float full_accel_velocity;     /* counts / second; >= this reaches max multiplier */
    float max_multiplier;          /* clamped to [1, 10] */
    float curve_exponent;          /* > 0; 2.0 is quadratic */
    float smoothing_up;            /* 0..1, higher reacts faster */
    float smoothing_down;          /* 0..1, higher reacts faster */
    float direction_reset_angle;   /* degrees; default 90 */
    float max_output_velocity;     /* pixels / second; used when limit flag set */
    uint32_t flags;
} OMA_Config;

typedef struct OMA_InputEvent {
    int32_t dx;
    int32_t dy;
    uint64_t timestamp_us;
} OMA_InputEvent;

typedef struct OMA_OutputEvent {
    int32_t dx;
    int32_t dy;
    float instantaneous_velocity;
    float smoothed_velocity;
    float multiplier;
} OMA_OutputEvent;

typedef struct OMA_Context {
    OMA_Config config;
    uint64_t last_timestamp_us;
    float smoothed_velocity;
    float residual_x;
    float residual_y;
    float last_dir_x;
    float last_dir_y;
    int has_last_direction;
} OMA_Context;

void OMA_DefaultConfig(OMA_Config *cfg);
void OMA_PresetPrecision(OMA_Config *cfg);
void OMA_PresetBalanced(OMA_Config *cfg);
void OMA_PresetFast(OMA_Config *cfg);
void OMA_PresetExtreme(OMA_Config *cfg);
int  OMA_ValidateConfig(OMA_Config *cfg);

void OMA_Init(OMA_Context *ctx, const OMA_Config *cfg);
void OMA_Reset(OMA_Context *ctx);
void OMA_SetConfig(OMA_Context *ctx, const OMA_Config *cfg);
void OMA_GetConfig(const OMA_Context *ctx, OMA_Config *cfg);
void OMA_Process(OMA_Context *ctx, const OMA_InputEvent *in, OMA_OutputEvent *out);

/* Simple text prefs format suitable for ENV:/ENVARC: wrappers. */
int OMA_SaveConfigFile(const OMA_Config *cfg, const char *path);
int OMA_LoadConfigFile(OMA_Config *cfg, const char *path);

#ifdef __cplusplus
}
#endif

#endif
