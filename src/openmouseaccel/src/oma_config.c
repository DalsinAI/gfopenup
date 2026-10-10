#include "openup/openmouseaccel.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void trim(char *s)
{
    size_t n;
    while (*s == ' ' || *s == '\t') memmove(s, s + 1, strlen(s));
    n = strlen(s);
    while (n && (s[n-1] == '\r' || s[n-1] == '\n' || s[n-1] == ' ' || s[n-1] == '\t')) s[--n] = 0;
}

int OMA_SaveConfigFile(const OMA_Config *cfg, const char *path)
{
    FILE *f;
    if (!cfg || !path) return -1;
    f = fopen(path, "w");
    if (!f) return -1;
    fprintf(f, "enabled=%u\n", (cfg->flags & OMA_FLAG_ENABLED) ? 1u : 0u);
    fprintf(f, "precision_velocity=%.3f\n", cfg->precision_velocity);
    fprintf(f, "full_accel_velocity=%.3f\n", cfg->full_accel_velocity);
    fprintf(f, "max_multiplier=%.3f\n", cfg->max_multiplier);
    fprintf(f, "curve_exponent=%.3f\n", cfg->curve_exponent);
    fprintf(f, "smoothing_up=%.3f\n", cfg->smoothing_up);
    fprintf(f, "smoothing_down=%.3f\n", cfg->smoothing_down);
    fprintf(f, "direction_reset_angle=%.3f\n", cfg->direction_reset_angle);
    fprintf(f, "max_output_velocity=%.3f\n", cfg->max_output_velocity);
    fprintf(f, "rapid_precision_recovery=%u\n", (cfg->flags & OMA_FLAG_RAPID_PRECISION_RECOVERY) ? 1u : 0u);
    fprintf(f, "direction_reset=%u\n", (cfg->flags & OMA_FLAG_DIRECTION_RESET) ? 1u : 0u);
    fprintf(f, "limit_output_speed=%u\n", (cfg->flags & OMA_FLAG_LIMIT_OUTPUT_SPEED) ? 1u : 0u);
    fclose(f);
    return 0;
}

static void set_flag(OMA_Config *cfg, uint32_t flag, int on)
{
    if (on) cfg->flags |= flag;
    else cfg->flags &= ~flag;
}

int OMA_LoadConfigFile(OMA_Config *cfg, const char *path)
{
    FILE *f;
    char line[256];
    char key[128], value[128];
    OMA_Config temp;

    if (!cfg || !path) return -1;
    OMA_DefaultConfig(&temp);
    f = fopen(path, "r");
    if (!f) return -1;

    while (fgets(line, sizeof(line), f)) {
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = 0;
        strncpy(key, line, sizeof(key)-1); key[sizeof(key)-1] = 0;
        strncpy(value, eq+1, sizeof(value)-1); value[sizeof(value)-1] = 0;
        trim(key); trim(value);

        if (!strcmp(key, "enabled")) set_flag(&temp, OMA_FLAG_ENABLED, atoi(value) != 0);
        else if (!strcmp(key, "precision_velocity")) temp.precision_velocity = (float)atof(value);
        else if (!strcmp(key, "full_accel_velocity")) temp.full_accel_velocity = (float)atof(value);
        else if (!strcmp(key, "max_multiplier")) temp.max_multiplier = (float)atof(value);
        else if (!strcmp(key, "curve_exponent")) temp.curve_exponent = (float)atof(value);
        else if (!strcmp(key, "smoothing_up")) temp.smoothing_up = (float)atof(value);
        else if (!strcmp(key, "smoothing_down")) temp.smoothing_down = (float)atof(value);
        else if (!strcmp(key, "direction_reset_angle")) temp.direction_reset_angle = (float)atof(value);
        else if (!strcmp(key, "max_output_velocity")) temp.max_output_velocity = (float)atof(value);
        else if (!strcmp(key, "rapid_precision_recovery")) set_flag(&temp, OMA_FLAG_RAPID_PRECISION_RECOVERY, atoi(value) != 0);
        else if (!strcmp(key, "direction_reset")) set_flag(&temp, OMA_FLAG_DIRECTION_RESET, atoi(value) != 0);
        else if (!strcmp(key, "limit_output_speed")) set_flag(&temp, OMA_FLAG_LIMIT_OUTPUT_SPEED, atoi(value) != 0);
    }
    fclose(f);
    OMA_ValidateConfig(&temp);
    *cfg = temp;
    return 0;
}
