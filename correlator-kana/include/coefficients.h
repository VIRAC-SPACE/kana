#ifndef KANA_COEFFICIENTS_H
#define KANA_COEFFICIENTS_H

#include "jsonReader.h"

typedef struct {
    long double	dt;
    long double F;
    long double	F0;
    long double	Fsn;
    long double A0;
    long double A1;
    long double	A2;
    long double	A3;
    long double	sTime;
} Coefficients;

void getCoefficients(Coefficients* coeff, JSON_Tree* coeffTree, JSON_Tree* jsonParam, char* signalName);

#endif //KANA_COEFFICIENTS_H
