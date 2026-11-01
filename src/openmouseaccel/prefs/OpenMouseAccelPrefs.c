/*
 * GFOpenMouseAccelPrefs
 *
 * A MUI-oriented preferences UI for AmigaOS/AROS. The implementation below
 * keeps the UI logic isolated so it can be retargeted to Reaction if desired.
 * Define OMA_USE_MUI when building in an SDK that provides MUI headers.
 */

#include "openup/openmouseaccel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef OMA_USE_MUI
#include <libraries/mui.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

static Object *app, *win;
static Object *enable_check;
static Object *precision_slider;
static Object *strength_slider;
static Object *maxcycle;
static Object *speedcycle;
static Object *recovery_check;
static Object *status_text;

static const char *max_entries[] = { "3x", "4x", "5x", "6x", "8x", "10x", NULL };
static const float max_values[] = { 3.0f, 4.0f, 5.0f, 6.0f, 8.0f, 10.0f };
static const char *speed_entries[] = { "Automatic", "1000 px/s", "1500 px/s", "2000 px/s", "3000 px/s", "Unlimited", NULL };
static const float speed_values[] = { 3000.0f, 1000.0f, 1500.0f, 2000.0f, 3000.0f, 1000000.0f };

static void config_to_ui(const OMA_Config *cfg)
{
    ULONG i, max_idx = 3, speed_idx = 0;
    set(enable_check, MUIA_Selected, (cfg->flags & OMA_FLAG_ENABLED) != 0);
    set(precision_slider, MUIA_Numeric_Value, (LONG)(cfg->precision_velocity));
    set(strength_slider, MUIA_Numeric_Value, (LONG)(cfg->curve_exponent * 100.0f));
    for (i = 0; i < 6; ++i) if (max_values[i] == cfg->max_multiplier) max_idx = i;
    for (i = 0; i < 6; ++i) if (speed_values[i] == cfg->max_output_velocity) speed_idx = i;
    set(maxcycle, MUIA_Cycle_Active, max_idx);
    set(speedcycle, MUIA_Cycle_Active, speed_idx);
    set(recovery_check, MUIA_Selected, (cfg->flags & OMA_FLAG_RAPID_PRECISION_RECOVERY) != 0);
}

static void ui_to_config(OMA_Config *cfg)
{
    ULONG v, max_idx, speed_idx;
    get(enable_check, MUIA_Selected, &v);
    if (v) cfg->flags |= OMA_FLAG_ENABLED; else cfg->flags &= ~OMA_FLAG_ENABLED;
    get(precision_slider, MUIA_Numeric_Value, &v);
    cfg->precision_velocity = (float)v;
    get(strength_slider, MUIA_Numeric_Value, &v);
    cfg->curve_exponent = (float)v / 100.0f;
    get(maxcycle, MUIA_Cycle_Active, &max_idx);
    get(speedcycle, MUIA_Cycle_Active, &speed_idx);
    if (max_idx < 6) cfg->max_multiplier = max_values[max_idx];
    if (speed_idx < 6) cfg->max_output_velocity = speed_values[speed_idx];
    if (speed_idx == 5) cfg->flags &= ~OMA_FLAG_LIMIT_OUTPUT_SPEED;
    else cfg->flags |= OMA_FLAG_LIMIT_OUTPUT_SPEED;
    get(recovery_check, MUIA_Selected, &v);
    if (v) cfg->flags |= OMA_FLAG_RAPID_PRECISION_RECOVERY; else cfg->flags &= ~OMA_FLAG_RAPID_PRECISION_RECOVERY;
    OMA_ValidateConfig(cfg);
}

int main(void)
{
    OMA_Config cfg;
    ULONG sigs = 0;
    LONG id;
    OMA_DefaultConfig(&cfg);
    OMA_LoadConfigFile(&cfg, "ENV:GFOpenUp/GFOpenMouseAccel.prefs");

    enable_check = MUI_MakeObject(MUIO_Checkmark, "Enable variable mouse acceleration");
    precision_slider = SliderObject,
        MUIA_Numeric_Min, 20,
        MUIA_Numeric_Max, 250,
        MUIA_Numeric_Format, "%ld counts/s",
    End;
    strength_slider = SliderObject,
        MUIA_Numeric_Min, 100,
        MUIA_Numeric_Max, 400,
        MUIA_Numeric_Format, "%ld",
    End;
    maxcycle = CycleObject, MUIA_Cycle_Entries, max_entries, End;
    speedcycle = CycleObject, MUIA_Cycle_Entries, speed_entries, End;
    recovery_check = MUI_MakeObject(MUIO_Checkmark, "Rapid precision recovery");
    status_text = TextObject, MUIA_Text_Contents, "Move the pointer to feel changes live.", End;

    app = ApplicationObject,
        MUIA_Application_Title, "GFOpenMouseAccel",
        MUIA_Application_Version, "$VER: GFOpenMouseAccelPrefs 0.1",
        SubWindow, win = WindowObject,
            MUIA_Window_Title, "GFOpenMouseAccel",
            WindowContents, VGroup,
                Child, TextObject, MUIA_Text_Contents, "\033bVariable Pointer Acceleration", End,
                Child, HGroup, Child, enable_check, Child, TextObject, MUIA_Text_Contents, "Enable", End, End,
                Child, TextObject, MUIA_Text_Contents, "Precision zone", End,
                Child, precision_slider,
                Child, TextObject, MUIA_Text_Contents, "Acceleration strength", End,
                Child, strength_slider,
                Child, HGroup,
                    Child, TextObject, MUIA_Text_Contents, "Maximum acceleration", End,
                    Child, maxcycle,
                End,
                Child, HGroup,
                    Child, TextObject, MUIA_Text_Contents, "Maximum pointer speed", End,
                    Child, speedcycle,
                End,
                Child, HGroup, Child, recovery_check, Child, TextObject, MUIA_Text_Contents, "Rapid precision recovery", End, End,
                Child, RectangleObject, MUIA_FixHeight, 70, End,
                Child, status_text,
                Child, HGroup,
                    Child, SimpleButton("Precision"),
                    Child, SimpleButton("Balanced"),
                    Child, SimpleButton("Fast"),
                    Child, SimpleButton("Extreme"),
                End,
                Child, HGroup,
                    Child, SimpleButton("Reset"),
                    Child, SimpleButton("Save"),
                    Child, SimpleButton("Close"),
                End,
            End,
        End,
    End;

    if (!app) return 20;
    config_to_ui(&cfg);
    DoMethod(win, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    set(win, MUIA_Window_Open, TRUE);

    while ((id = DoMethod(app, MUIM_Application_NewInput, &sigs)) != MUIV_Application_ReturnID_Quit) {
        if (sigs) { sigs = Wait(sigs | SIGBREAKF_CTRL_C); if (sigs & SIGBREAKF_CTRL_C) break; }
    }

    ui_to_config(&cfg);
    OMA_SaveConfigFile(&cfg, "ENV:GFOpenUp/GFOpenMouseAccel.prefs");
    OMA_SaveConfigFile(&cfg, "ENVARC:GFOpenUp/GFOpenMouseAccel.prefs");
    MUI_DisposeObject(app);
    return 0;
}

#else

/* Host/dev fallback so the component can still be built and exercised without MUI. */
int main(int argc, char **argv)
{
    OMA_Config cfg;
    OMA_DefaultConfig(&cfg);
    if (argc > 1 && !strcmp(argv[1], "precision")) OMA_PresetPrecision(&cfg);
    else if (argc > 1 && !strcmp(argv[1], "fast")) OMA_PresetFast(&cfg);
    else if (argc > 1 && !strcmp(argv[1], "extreme")) OMA_PresetExtreme(&cfg);
    printf("GFOpenMouseAccelPrefs host fallback\n");
    printf("precision_velocity=%.1f\n", cfg.precision_velocity);
    printf("max_multiplier=%.1f\n", cfg.max_multiplier);
    printf("curve_exponent=%.2f\n", cfg.curve_exponent);
    printf("max_output_velocity=%.1f\n", cfg.max_output_velocity);
    return 0;
}
#endif
