#ifndef KANA_HZCORRECT_H
#define KANA_HZCORRECT_H

#include "../include/jsonReader.h"
#include "../include/coefficients.h"

void processDataFolder(JSON_Tree* params, char* signalName);
void dataCorrection(JSON_Tree* params, char* filename, char* signalName);
double HzFunction(Coefficients* coeff, double time);

void writeSignal(float* signal, JSON_Tree* jsonParam, char* fName);
void updateJson(JSON_Tree* jsonParam, char* jsonPath);

#endif //KANA_HZCORRECT_H
