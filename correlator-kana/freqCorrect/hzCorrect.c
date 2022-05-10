#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <math.h>
#include "../include/directories.h"
#include "../include/panic.h"
#include "../include/utils.h"
#include "../include/coefficients.h"
#include "hzCorrect.h"

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
            dataCorrection(params, entry->d_name, station);
            printf("\033[32mOk!\033[0m\n");
        }
    }
}

void dataCorrection(JSON_Tree* params, char* filename, char* station) {
    char* jsonFile = malloc(strlen(JSON_DIR) + strlen(filename) + 4);
    // if(!jsonFile) ALLOC_ERROR;
    sprintf(jsonFile,"%s%s", JSON_DIR, filename);
    changeFileExt(jsonFile, ".json");

    JSON_Tree* jsonParam = malloc(sizeof(JSON_Tree));
    // if (jsonParam == NULL) ALLOC_ERROR;
    if(JSON_read(jsonParam, jsonFile) == 0) {
        panic("cannot read json file");
    }

    long floatCount = JSON_getInt(jsonParam->tree, "root:float_count");
    char* corr = JSON_getString(jsonParam->tree, "root:corrections");

    if ((strlen(corr) == 1 && corr[0] == 'f') || (strlen(corr) == 2 && corr[1] == 'f')) {
        panic("signal is already frequency-corrected");
    }

    float* signalRe = malloc(floatCount * sizeof(float));
    // if(signalRe == NULL) ALLOC_ERROR;

    char* signalPath = malloc(strlen(SIGNAL_DIR) + strlen(filename) + 1);
    sprintf(signalPath, "%s%s", SIGNAL_DIR, filename);

    FILE* file = fopen(signalPath, "r");
    if(!file) {
        char message[32];
        sprintf(message, "cannot open '%s'", signalPath);
        panic(message);
    }
    if(fread(signalRe, sizeof(float), floatCount, file) != floatCount){
        panic("cannot read file");
    }
    fclose(file);

    float* signal = malloc(2 * floatCount * sizeof(float));
    // if(signal == NULL) ALLOC_ERROR;

    for(long j = 0; j < floatCount; ++j){
        signal[2 * j] = signalRe[j];
        signal[2 * j + 1] = signalRe[j];
    }
    free(signalRe);
    ///////////////////////////////////////////////////

    long discret = JSON_getInt(params->tree, "root:discretization(Hz)");

    Coefficients* coeff = malloc(sizeof(Coefficients));
    getCoefficients(coeff, params, jsonParam, station);
    coeff->F0 = coeff->F0 * pow(10.0, 6); //from MHz to Hz
    coeff->dt = (long double)(1.0 / discret);

    for (long j = 0; j < floatCount; ++j) {
        double T = coeff->sTime + j * coeff->dt;

        // fazu funkcija:
        double radian = HzFunction(coeff, T);
        float Re1 = (float)cos(radian);
        float Im1 = (float)sin(radian);

        // uztvertais signals, kuram veic korekciju:
        float Re2 = signal[2*j];
        float Im2 = 0;

        float Re3 = Re1*Re2 - Im1*Im2;
        float Im3 = Re1*Im2 + Re2*Im1;

        signal[2 * j] = Re3;
        signal[2 * j + 1] = Im3;
    }

    updateJson(jsonParam, jsonFile);
    writeSignal(signal, jsonParam, filename);

    free(signal);
    free(signalPath);
    free(jsonFile);
    free(coeff);
    free(jsonParam->tree);
}

double HzFunction(Coefficients* coeff, double T) {
    return (-2) * M_PI * T * (coeff->F0*(2*coeff->A2*T + 3*coeff->A3*pow(T, 2)) + coeff->Fsn);
}

void writeSignal(float *signal, JSON_Tree *jsonParam, char* fName){
    long floatCount = JSON_getInt(jsonParam->tree, "root:float_count");

    char* datFile = malloc(strlen(SIGNAL_DIR) + strlen(fName) + 10);
    // if(!datFile) ALLOC_ERROR;
    sprintf(datFile,"%s%s", strdup(SIGNAL_DIR), strdup(fName));

    FILE* file = fopen(datFile, "w");
    if(!file) {
        char message[32];
        sprintf(message, "cannot open '%s'", datFile);
        panic(message);
    }
    if(fwrite(signal, sizeof(float), 2 * floatCount, file) != 2 * floatCount) {
        panic("cannot write to file");
    }
    fclose(file);
    free(datFile);
}

void updateJson(JSON_Tree* jsonParam, char* jsonPath) {
    char* filestr = JSON_getString(jsonParam->tree, "root:file");
    long floatCount = JSON_getInt(jsonParam->tree, "root:float_count");
    char* corr = JSON_getString(jsonParam->tree, "root:corrections");
    double startTime = JSON_getFloat(jsonParam->tree, "root:start_time");
    double endTime = JSON_getFloat(jsonParam->tree, "root:end_time");

    FILE* file = fopen(jsonPath, "w");
    if(!file) {
        char message[32];
        sprintf(message, "cannot open '%s'", jsonPath);
        panic(message);
    }

    char newcorr[3];
    newcorr[0] = 'f';
    newcorr[1] = '\0';
    if (strlen(corr) == 1) {
        newcorr[0] = 't';
        newcorr[1] = 'f';
        newcorr[2] = '\0';
    }

    fprintf(file, "{\n");
    fprintf(file, "\t\"file\": \"%s\",\n", filestr);
    fprintf(file, "\t\"float_count\": %ld,\n", floatCount);
    fprintf(file, "\t\"corrections\": \"%s\",\n", newcorr);
    fprintf(file, "\t\"start_time\": %.15f,\n", startTime);
    fprintf(file, "\t\"end_time\": %.15f\n", endTime);
    fprintf(file, "}\n");
    fclose(file);

    free(filestr);
    free(corr);
}