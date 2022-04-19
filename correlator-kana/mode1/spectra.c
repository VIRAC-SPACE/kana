#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include "../include/directories.h"
#include "../include/panic.h"
#include "../include/utils.h"
#include "spectra.h"

Spectra* fourierTransform(unsigned char* buffer, CorrelationData* cdata) {
    float* data = bytesToBinary(buffer, cdata->size);

    Spectra* spectra = createSpectra(data, cdata);
    Spectra* cropped = cropSpectra(spectra, cdata);
    Spectra* compressed = compressSpectra(spectra, cdata);

    writeSpectra(compressed, cdata);
    writeSignal(data, cdata);
    writeJson(cdata);

    freeSpectra(spectra);
    freeSpectra(cropped);
    free(data);

    return compressed;
}

Spectra* createSpectra(float* data, CorrelationData* cdata){
    // TODO: move power of 2 allocation to bytesToBinary function
    // find closest power of 2 for the data size
    int pow2;
    if (cdata->size*8 < cdata->discretization*2) {
        pow2 = (int)log2f((float)cdata->discretization*2) + 1;
    } else {
        pow2 = (int)log2f((float)cdata->size*8) + 1;
    }
    unsigned long pow2size = pow(2, pow2);

    unsigned long n = pow2size;
    unsigned long outSize = n/2 + 1;

    // fill data until closest power of 2
    float* pow2data = malloc(sizeof(float)*n);
    for (int i = 0; i < n; ++i) {
        if (i < cdata->size*8) pow2data[i] = data[i];
        else pow2data[i] = 0.0f;
    }

    fftwf_complex* fourier = fftwf_malloc(sizeof(fftwf_complex)*outSize);
    // if (fourier == NULL) ALLOC_ERROR;

    fftwf_plan plan = fftwf_plan_dft_r2c_1d(n, pow2data, fourier, FFTW_ESTIMATE);
    fftwf_execute(plan);

    // convert fourier to real spectra
    Spectra* spectra = newSpectra(outSize);
    for (unsigned long i = 0; i < spectra->size; ++i)
        spectra->data[i] = complexToMagnitude(fourier[i]);

    fftwf_destroy_plan(plan);
    fftwf_free(fourier);
    fftwf_cleanup();
    free(pow2data);

    return spectra;
}

Spectra* cropSpectra(Spectra* spectra, CorrelationData* cdata) {
    long offset = cdata->target - cdata->window / 2;
    if (offset < 0 || offset + cdata->window > spectra->size) {
        panic("window too small for target");
    }

    Spectra* cropped = newSpectra(cdata->window);
    for (unsigned long i = 0; i < cropped->size; ++i) {
        cropped->data[i] = spectra->data[i + offset];
    }
    return cropped;
}

Spectra* compressSpectra(Spectra* spectra, CorrelationData* cdata) {
    unsigned long interval = (unsigned long)round((cdata->fft_compress * cdata->window) + 0.5);

    Spectra* compressed = newSpectra(spectra->size / interval);
    for (unsigned long i = 0; i < compressed->size; ++i){
        float compValue = 0;
        for (unsigned long j = 0; j < interval; ++j) {
            compValue = compValue + spectra->data[i * interval + j];
        }
        compValue = compValue / interval;
        compressed->data[i] = compValue;
    }
    return compressed;
}

float complexToMagnitude(fftwf_complex complex) {
    return sqrt(pow(complex[0], 2) + pow(complex[1], 2));
}

void writeSpectra(Spectra* spectra, CorrelationData* cdata) {
    char* filepath = malloc(strlen(M1_SPECTRA_DIR) + strlen(cdata->fileName) + 20);
    // if(!filepath) ALLOC_ERROR;

    sprintf(filepath,"%s/%s", M1_SPECTRA_DIR, cdata->fileName);
    changeFileExt(filepath, ".spectra");

    FILE* file = fopen(filepath, "w");
    if(!file) {
        char message[32];
        sprintf(message, "cannot open '%s'", filepath);
        panic(message);
    }

    double step = cdata->window / spectra->size;
    long offset = cdata->target - cdata->window / 2;
    for(unsigned long i = 0; i < spectra->size; ++i) {
        fprintf(file, "%f %f\n", (float)(step * i + offset), spectra->data[i]);
    }

    fclose(file);
    free(filepath);
}

void writeSignal(float* data, CorrelationData* cdata){
    char* dataFile = malloc(strlen(SIGNAL_DIR) + strlen(cdata->fileName) + 40);
    // if(!dataFile) ALLOC_ERROR;

    sprintf(dataFile,"%s%s", SIGNAL_DIR, cdata->fileName);
    changeFileExt(dataFile, ".signal");

    FILE* fp = fopen(dataFile, "w");
    if(!fp) {
        char message[32];
        sprintf(message, "cannot open '%s'", dataFile);
        panic(message);
    }
    if(fwrite(data, sizeof(float), cdata->size * 8, fp) != cdata->size * 8) {
        panic("cannot write to file");
    }
    fclose(fp);
    free(dataFile);
}

void writeJson(CorrelationData* cdata){
    char* signalName = malloc(strlen(SIGNAL_DIR) + strlen(cdata->fileName) + 40);
    // if(!signalName) ALLOC_ERROR;
    sprintf(signalName,"%s%s", SIGNAL_DIR, cdata->fileName);
    changeFileExt(signalName, ".signal");

    char* jsonName = malloc(strlen(JSON_DIR) + strlen(cdata->fileName) + 40);
    // if(!jsonName) ALLOC_ERROR;
    sprintf(jsonName, "%s%s", JSON_DIR, cdata->fileName);
    changeFileExt(jsonName, ".json");

    FILE* file = fopen(jsonName, "w");
    if(!file) {
        char message[32];
        sprintf(message, "cannot open '%s'", jsonName);
        panic(message);
    }

    fprintf(file, "{\n");
    fprintf(file, "\t\"file\": \"%s\",\n", signalName);
    fprintf(file, "\t\"float_count\": %ld,\n", cdata->size * 8);
    fprintf(file, "\t\"corrections\": \"\",\n");
    fprintf(file, "\t\"start_time\": %.15Lf,\n", cdata->epoch + cdata->time);
    fprintf(file, "\t\"end_time\": %.15Lf\n", cdata->epoch + cdata->time + cdata->length);
    fprintf(file, "}\n");
    fclose(file);

    free(jsonName);
}

Spectra* newSpectra(long size) {
    Spectra* spectra = malloc(sizeof(Spectra));
    // if (spectra == NULL) ALLOC_ERROR;

    spectra->size = size;
    spectra->data = malloc(sizeof(float) * spectra->size);
    // if (spectra->data == NULL) ALLOC_ERROR;

    return spectra;
}

void freeSpectra(Spectra* spectra) {
    free(spectra->data);
    free(spectra);
}

float* bytesToBinary(const unsigned char* buffer, unsigned long byteCount){
    // calculate closest power of 2 for size

    float* data = malloc(sizeof(float) * 8 * byteCount);
    // if(data == NULL) ALLOC_ERROR;

    /* Create table of all possible bit possitions in byte */
    char table[256][8];
    for(int i = 0; i < 256; ++i) {
        for(int j = 0; j < 8; ++j)
            table[i][j] = 0;
    }

    for (int i = 0; i < 256; ++i) {
        table[i][0] = ((i & 1) == 1)        ?   1 : -1;
        table[i][1] = ((i & 2) == 2)        ?   1 : -1;
        table[i][2] = ((i & 4) == 4)        ?   1 : -1;
        table[i][3] = ((i & 8) == 8)        ?   1 : -1;
        table[i][4] = ((i & 16) == 16)      ?   1 : -1;
        table[i][5] = ((i & 32) == 32)      ?   1 : -1;
        table[i][6] = ((i & 64) == 64)      ?   1 : -1;
        table[i][7] = ((i & 128) == 128)    ?   1 : -1;
    }

    /* Set bits value(0/1) from bytes */
    for (unsigned long i = 0; i < byteCount; ++i){
        for (unsigned long j = 0; j < 8; ++j){
            data[8 * i + j] = (float)table[buffer[i]][j];
        }
    }
    return data;
}
