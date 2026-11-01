#include "opencalc.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static void near(double a, double b) { assert(fabs(a-b) < 1e-9); }

int main(void) {
    OC_Context c; OC_Init(&c);
    double v;
    assert(OC_Eval(&c, "2+2*5", &v) == OC_OK); near(v,12);
    assert(OC_Eval(&c, "sqrt(81)+12", &v) == OC_OK); near(v,21);
    assert(OC_Eval(&c, "sin(30)", &v) == OC_OK); near(v,0.5);
    assert(OC_Eval(&c, "2^3^2", &v) == OC_OK); near(v,512);
    assert(OC_Eval(&c, "1/0", &v) == OC_ERR_DIVZERO);
    puts("GFOpenCalc core tests: PASS");
    return 0;
}
