#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <dirent.h>
#include <fftw3.h>
#include "../include/jsonReader.h"
#include "../include/directories.h"
#include "../include/panic.h"
#include "../include/utils.h"
#include "../include/coefficients.h"
#include "mode2.h"

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

void processDataFolder(JSON_Tree* params, char* station) {
    DIR* directory;
    if ((directory = opendir(SIGNAL_DIR)) == NULL) {
        panic("cannot open signal dir");
    }

    struct dirent* entry;
    while ((entry = readdir(directory)) != NULL) {
        if (isKanaFile(entry, SIGNAL_DIR, station)) {
            printf("\t%s : ", entry->d_name);
            fflush(stdout);
            mode2(params, entry->d_name, station);
            printf("\033[32mOk!\033[0m\n");
        }
    }
}

void mode2(JSON_Tree* params, char* filename, char* station) {
    char* jsonFile = malloc(strlen(JSON_DIR) + strlen(filename) + 4);
    // if(!jsonFile) ALLOC_ERROR;

    sprintf(jsonFile,"%s%s", JSON_DIR, filename);
    changeFileExt(jsonFile, ".json");

    JSON_Tree* jsonParam = malloc(sizeof(JSON_Tree));
    // if (jsonParam == NULL) ALLOC_ERROR;

    if (JSON_read(jsonParam, jsonFile) == 0) {
        panic("cannot read json file");
    }

    long floatCount = JSON_getInt(jsonParam->tree, "root:float_count");

    ////////	Read in array hz corrected signal	//////

    float* signal = malloc(2 * floatCount * sizeof(float));
    // if(signal == NULL) ALLOC_ERROR;

    char* signalPath = malloc(strlen(SIGNAL_DIR) + strlen(filename) + 1);
    sprintf(signalPath, "%s%s", SIGNAL_DIR, filename);

    FILE* file = fopen(signalPath, "r");
    if(!file) {
        char message[32];
        sprintf(message, "cannot open '%s'", signalPath);
        panic(message);
    }

    if(fread(signal, sizeof(float), 2 * floatCount, file) != 2 * floatCount) {
        panic("cannot read file");
    }
    fclose(file);
    ///////////////////////////////////////////////////

    correlate(signal, jsonParam, params, filename, station);

    free(signal);
    free(signalPath);
    free(jsonFile);
    free(jsonParam->tree);
}

void correlate(float* signal, JSON_Tree* jsonParam, JSON_Tree* params, char* fName, char* station){
    Coefficients* coeff = malloc(sizeof(Coefficients));
    getCoefficients(coeff, params, jsonParam, station);

    long double F = coeff->F;

    long discret = JSON_getInt(params->tree, "root:discretization(Hz)");
    long floatCount = JSON_getInt(jsonParam->tree, "root:float_count");
    double Tu_sec = JSON_getFloat(params->tree, "root:m2_Tu");

    long double dt = (long double)1 / discret;
    long int Tu = (long int)round(Tu_sec / dt);
    long int size = (long int)floor(floatCount / Tu);

    fftw_complex* inputData = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * size);
    // if (inputData == NULL) ALLOC_ERROR;

    for(long i = 0; i < size; ++i) {
        inputData[i][0] = 0;
        inputData[i][1] = 0;
        for(long j = 0; j < Tu; ++j) {
            //Transmitted signal:
            long double time = (Tu * i + j) * dt;
            float Re1 = (float)cos(2 * M_PI * F * time);
            float Im1 = (float)0.0;

            //Received signal:
            float Re2 = signal[2 * Tu * i + 2 * j];
            float Im2 = signal[2 * Tu * i + 2 * j + 1];

            inputData[i][0] = inputData[i][0] + Re1 * Re2 + Im1 * Im2;
            inputData[i][1] = inputData[i][1] + Re2 * Im1 - Re1 * Im2;
        }
        inputData[i][0] = (float)(inputData[i][0] / Tu);
        inputData[i][1] = (float)(inputData[i][1] / Tu);
    }

    fftw_complex* spectra = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * size);
    // if (spectra == NULL) ALLOC_ERROR;

    fftw_plan plan_forward = fftw_plan_dft_1d(size, inputData, spectra, FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_execute(plan_forward);
    fftw_free(inputData);

    float* spectraABS = malloc(size * sizeof(float));
    // if(spectraABS == NULL) ALLOC_ERROR;

    for(long i = 0; i < size; ++i) {
        spectraABS[i] = sqrt(pow(spectra[i][0], 2) + pow(spectra[i][1], 2));
    }

    fftw_free(spectra);

    float hz;
    if(isDopHz(spectraABS, size, jsonParam, params, &hz)==1){
        writeSpectra(spectraABS, jsonParam, params, fName, size);
    }
}

void writeSpectra(float* spectraABS, JSON_Tree* jsonParam, JSON_Tree* params, char* fName, long int size) {
    float* spectraABS_shift = malloc(size * sizeof(float));
    // if(spectraABS_shift == NULL) ALLOC_ERROR;

    for(long i = 0; i < size / 2; ++i) {
        spectraABS_shift[i] = spectraABS[size/2+i];
    }
    for(long i = 0; i < size / 2; ++i) {
        spectraABS_shift[size / 2 + i] = spectraABS[i];
    }

    long discret = JSON_getInt(params->tree, "root:discretization(Hz)");
    double Tu_sec = JSON_getFloat(params->tree, "root:m2_Tu");

    long double dt = (long double)1 / discret;
    long int Tu = (long int)round(Tu_sec / dt);
    dt = Tu * dt;
    long double dw = (long double)1 / (dt * size);

    float dwMax = (float)dw * size;
    float halfHz = (float)dwMax / 2;

    char* spectraFile = malloc(strlen(M2_SPECTRA_DIR) + strlen(fName) + 5);
    sprintf(spectraFile, "%s%s", M2_SPECTRA_DIR, fName);
    changeFileExt(spectraFile, ".spectra");

    FILE* file = fopen(spectraFile, "w");
    if(!file) {
        char message[32];
        sprintf(message, "cannot open '%s'", spectraFile);
        panic(message);
    }

    for(long i = 0; i < size; ++i) {
        fprintf(file, "%f %lf\n", (float)(dw * i - halfHz), spectraABS_shift[i]);
    }
    fclose(file);

    free(spectraABS);
    free(spectraABS_shift);
    free(spectraFile);
}

float getDopplerHz(float* spectraABS, long int size, long double dw) {
    float power = spectraABS[0];
    float dwMax = (float)dw * size;
    float halfHz = (float)dwMax / 2;
    long int index = 0;

    for (long int i = 0; i < size; ++i) {
        if (power < spectraABS[i]) {
            power = spectraABS[i];
            index = i;
        }
    }
    return (float)dw * index - halfHz;
}

int isDopHz(float* spectraABS, long int size, JSON_Tree* jsonParam, JSON_Tree* params, float* hz) {
    float* spectraABS_shift = malloc(size * sizeof(float));
    // if(spectraABS_shift == NULL) ALLOC_ERROR;

    for (long i = 0; i < size / 2; ++i) {
        spectraABS_shift[i] = spectraABS[size / 2 + i];
    }
    for (long i = 0; i < size / 2; ++i) {
        spectraABS_shift[size / 2 + i] = spectraABS[i];
    }

    long discret = JSON_getInt(params->tree, "root:discretization(Hz)");
    double Tu_sec = JSON_getFloat(params->tree, "root:m2_Tu");

    long double dt = (long double)1.0 / discret;
    long int Tu = (long int)round(Tu_sec/dt);
    dt = Tu*dt;

    long double dw = (long double)1/(dt*size);

    float dwMax = (float)dw*size;
    float halfHz = (float)dwMax/2;
    long int index = 0;
    float avg = 0.0;
    for(long i = 0; i < size; ++i) {
        avg += spectraABS[i];
    }
    avg = avg / size;

    float power = spectraABS_shift[0];
    for (long i = 0; i < size; ++i) {
        if (dw * i - halfHz > -10000 && dw * i - halfHz < 10000) {
            if(power < spectraABS_shift[i]){
                power = spectraABS_shift[i];
                index = i;
            }
        }
    }

    hz[0] = (float)dw*index-halfHz;

    free(spectraABS_shift);
    if((power > 2 * avg)) {
        return 1;
    }
    return 0;
}