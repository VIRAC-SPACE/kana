#ifndef HZCORRECT_H
#define HZCORRECT_H

#include "../jsonReader.h"
#include "../coefficients.h"

void processDataFolder(JSON_Tree* params, char* signalName);
void dataCorrection(JSON_Tree* params, char* filename, char* signalName);
double HzFunction(Coefficients* coeff, double time);

void writeSignal(float* signal, JSON_Tree* jsonParam, char* fName);
void updateJson(JSON_Tree* jsonParam, char* jsonPath);

#endif /* HZCORRECT_H */
