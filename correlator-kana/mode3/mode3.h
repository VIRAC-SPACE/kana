#ifndef KANA_MODE3_H
#define KANA_MODE3_H

#include <dirent.h>
#include <fftw3.h>
#include "../include/jsonReader.h"

void processData(JSON_Tree* params, int u, char* sig1, char* sig2, int ch);
fftwf_complex* spectraOfSignal(float* sig, long unsigned dataSize, long unsigned WN);
fftwf_complex* multiplyOfSignals(float* sig1, float* sig2, long unsigned dataSize, int k);
void createPSfile(fftwf_complex* spectra2, long unsigned dataSize, long double dw, char* psName, int plot);
void meanOfCorrGraph(float* cor, long unsigned dataSize, long double dt, char* psName);

char* getSignalFileName(char* signalName, int channel);

#endif //KANA_MODE3_H
