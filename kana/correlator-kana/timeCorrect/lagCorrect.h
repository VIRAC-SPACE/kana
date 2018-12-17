#ifndef LAGCORRECT_H
#define LAGCORRECT_H

#include "../jsonReader.h"
#include "../coefficients.h"

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

#endif /* LAGCORRECT_H */
