#include "opencalc.h"
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *p;
    OC_Context *ctx;
    OC_Status st;
} Parser;

static void ws(Parser *ps) { while (isspace((unsigned char)*ps->p)) ps->p++; }

static double expr(Parser *ps);

static int name(Parser *ps, char *buf, size_t cap) {
    ws(ps);
    size_t n = 0;
    if (!isalpha((unsigned char)*ps->p)) return 0;
    while (isalpha((unsigned char)*ps->p) && n + 1 < cap)
        buf[n++] = (char)tolower((unsigned char)*ps->p++);
    buf[n] = 0;
    return 1;
}

static double primary(Parser *ps) {
    ws(ps);
    if (ps->st != OC_OK) return 0.0;

    if (*ps->p == '(') {
        ps->p++;
        double v = expr(ps);
        ws(ps);
        if (*ps->p != ')') { ps->st = OC_ERR_PARSE; return 0.0; }
        ps->p++;
        return v;
    }

    if (*ps->p == '+') { ps->p++; return primary(ps); }
    if (*ps->p == '-') { ps->p++; return -primary(ps); }

    if (isalpha((unsigned char)*ps->p)) {
        char n[24];
        if (!name(ps, n, sizeof(n))) { ps->st = OC_ERR_PARSE; return 0.0; }

        if (!strcmp(n, "pi")) return 3.14159265358979323846;
        if (!strcmp(n, "e")) return 2.71828182845904523536;
        if (!strcmp(n, "ans")) return ps->ctx->ans;
        if (!strcmp(n, "mem")) return ps->ctx->memory;

        ws(ps);
        if (*ps->p != '(') { ps->st = OC_ERR_PARSE; return 0.0; }
        ps->p++;
        double a = expr(ps);
        ws(ps);
        if (*ps->p != ')') { ps->st = OC_ERR_PARSE; return 0.0; }
        ps->p++;

        double rad = ps->ctx->degrees ? a * 3.14159265358979323846 / 180.0 : a;
        if (!strcmp(n, "sin")) return sin(rad);
        if (!strcmp(n, "cos")) return cos(rad);
        if (!strcmp(n, "tan")) return tan(rad);
        if (!strcmp(n, "sqrt")) {
            if (a < 0.0) { ps->st = OC_ERR_DOMAIN; return 0.0; }
            return sqrt(a);
        }
        if (!strcmp(n, "ln")) {
            if (a <= 0.0) { ps->st = OC_ERR_DOMAIN; return 0.0; }
            return log(a);
        }
        if (!strcmp(n, "log")) {
            if (a <= 0.0) { ps->st = OC_ERR_DOMAIN; return 0.0; }
            return log10(a);
        }
        if (!strcmp(n, "abs")) return fabs(a);

        ps->st = OC_ERR_PARSE;
        return 0.0;
    }

    errno = 0;
    char *end = NULL;
    double v = strtod(ps->p, &end);
    if (end == ps->p) { ps->st = OC_ERR_PARSE; return 0.0; }
    if (errno == ERANGE) ps->st = OC_ERR_RANGE;
    ps->p = end;
    return v;
}

static double power(Parser *ps) {
    double lhs = primary(ps);
    ws(ps);
    if (*ps->p == '^') {
        ps->p++;
        double rhs = power(ps);
        lhs = pow(lhs, rhs);
        if (!isfinite(lhs)) ps->st = OC_ERR_RANGE;
    }
    return lhs;
}

static double term(Parser *ps) {
    double v = power(ps);
    for (;;) {
        ws(ps);
        char op = *ps->p;
        if (op != '*' && op != '/') break;
        ps->p++;
        double rhs = power(ps);
        if (op == '*') v *= rhs;
        else {
            if (rhs == 0.0) { ps->st = OC_ERR_DIVZERO; return 0.0; }
            v /= rhs;
        }
    }
    return v;
}

static double expr(Parser *ps) {
    double v = term(ps);
    for (;;) {
        ws(ps);
        char op = *ps->p;
        if (op != '+' && op != '-') break;
        ps->p++;
        double rhs = term(ps);
        v = (op == '+') ? v + rhs : v - rhs;
    }
    return v;
}

void OC_Init(OC_Context *ctx) {
    ctx->memory = 0.0;
    ctx->ans = 0.0;
    ctx->degrees = 1;
}

OC_Status OC_Eval(OC_Context *ctx, const char *s, double *out) {
    Parser ps = { s, ctx, OC_OK };
    double v = expr(&ps);
    ws(&ps);
    if (ps.st == OC_OK && *ps.p != '\0') ps.st = OC_ERR_PARSE;
    if (ps.st == OC_OK && !isfinite(v)) ps.st = OC_ERR_RANGE;
    if (ps.st == OC_OK) {
        ctx->ans = v;
        *out = v;
    }
    return ps.st;
}

const char *OC_StatusText(OC_Status s) {
    switch (s) {
        case OC_OK: return "OK";
        case OC_ERR_PARSE: return "Parse error";
        case OC_ERR_DIVZERO: return "Division by zero";
        case OC_ERR_DOMAIN: return "Domain error";
        case OC_ERR_RANGE: return "Range error";
        default: return "Unknown error";
    }
}
