#ifndef MODE2_H
#define MODE2_H

#include "../jsonReader.h"

void processDataFolder(JSON_Tree* params, char* signalName);
void mode2(JSON_Tree* params, char* filename, char* signalName);
void correlate(float* signal, JSON_Tree* jsonParam, JSON_Tree* params, char* fName, char* signalName);
float getDopplerHz(float* spectraABS, long int size, long double dw);
int isDopHz(float* spectraABS, long int size, JSON_Tree* jsonParam, JSON_Tree* params, float* hz);

void writeSpectra(float* spectraABS, JSON_Tree* jsonParam, JSON_Tree* params, char* fName, long int size);

#endif /* MODE2_H */
