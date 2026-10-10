#ifndef OPENCALC_H
#define OPENCALC_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    OC_OK = 0,
    OC_ERR_PARSE,
    OC_ERR_DIVZERO,
    OC_ERR_DOMAIN,
    OC_ERR_RANGE
} OC_Status;

typedef struct {
    double memory;
    double ans;
    int degrees;
} OC_Context;

void OC_Init(OC_Context *ctx);
OC_Status OC_Eval(OC_Context *ctx, const char *expr, double *out);
const char *OC_StatusText(OC_Status s);

#ifdef __cplusplus
}
#endif
#endif
