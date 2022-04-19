#ifndef KANA_LAGCORRECT_H
#define KANA_LAGCORRECT_H


#include "../include/jsonReader.h"
#include "../include/coefficients.h"

struct lagCorrect {
    long long int index;
    int	remove;
};

void processDataFolder(JSON_Tree* params, char* signalName);
void dataCorrection(JSON_Tree* params, char* filename, char* signalName);
long double lagFunction(Coefficients* coeff, double time);
void LagCorect(float* signal, struct lagCorrect* lc, long unsigned dataSize, int indexLC);

void writeSignal(float* signal, JSON_Tree* jsonParam, char* fName);
void updateJson(JSON_Tree* jsonParam, char* jsonPath, double sL, double eL);


#endif //KANA_LAGCORRECT_H
