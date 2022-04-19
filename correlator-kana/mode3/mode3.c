#include <stdio.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include "../include/directories.h"
#include "../include/panic.h"
#include "../include/utils.h"
#include "mode3.h"

#define DELTA 2000

void processData(JSON_Tree* params, int u, char* sig1, char* sig2, int ch) {
    char* bufferBeginStr = JSON_getString(params->tree, "root:m1_buffer_begin");
    unsigned long bufferBegin = timeToSecs(bufferBeginStr);

    char* bufferEndStr = JSON_getString(params->tree, "root:m1_buffer_end");
    unsigned long bufferEnd = timeToSecs(bufferEndStr);

    long discretization = JSON_getInt(params->tree, "root:discretization(Hz)");

    unsigned long dataSize = ((bufferEnd - bufferBegin) + 1) * discretization;

    ////////	Read in array hz correcte Signal 1	//////
    char* signalFileName1 = getSignalFileName(sig1, ch);
    char* signalFilePath1 = malloc(strlen(SIGNAL_DIR) + strlen(signalFileName1) + 1);
    sprintf(signalFilePath1, "%s%s", SIGNAL_DIR, signalFileName1);

    FILE* fp = fopen(signalFilePath1, "r");
    if(!fp) {
        panic("cannot open first signal file");
    }

    float* signal1 = malloc(2 * dataSize * sizeof(float));
    // if(signal1 == NULL) ALLOC_ERROR;

    rewind(fp);
    if(fread(signal1, sizeof(float), 2 * dataSize, fp) == 0) {
        panic("cannot read first signal file");
    }
    fclose(fp);
    ///////////////////////////////////////////////////

    ////////	Read in array hz correcte Signal 2	//////
    char* signalFileName2 = getSignalFileName(sig2, ch);
    char* signalFilePath2 = malloc(strlen(SIGNAL_DIR) + strlen(signalFileName2) + 1);
    sprintf(signalFilePath2, "%s%s", SIGNAL_DIR, signalFileName2);

    fp = fopen(signalFilePath2, "r");
    if(!fp) {
        panic("cannot open second signal file");
    }

    float* signal2 = malloc(2 * dataSize * sizeof(float));
    // if(signal2 == NULL) ALLOC_ERROR;

    rewind(fp);
    if(fread(signal2, sizeof(float), 2 * dataSize, fp) == 0) {
        panic("cannot read second signal file");
    }
    fclose(fp);
    ///////////////////////////////////////////////////

    printf("\t%s X %s : delay=%d : ", signalFileName1, signalFileName2, u);
    fflush(stdout);

    long int type = 1;
    if(type == 0) {
        long unsigned WN = 1;
        long unsigned windowSize = dataSize / WN;

        ///////////////////  Signal 1 un Signal 2 spektru reizinzashana  ////////////////
        fftwf_complex* spectra = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * 2 * windowSize );
        // if (spectra == NULL) ALLOC_ERROR;

        fftwf_complex* spectra1 = spectraOfSignal(signal1, dataSize, WN);
        fftwf_complex* spectra2 = spectraOfSignal(signal2, dataSize, WN);

        for(long i = 0; i < 2 * windowSize; ++i){
            float Re2 = spectra1[i][0];
            float Im2 = spectra1[i][1];
            float Re1 = spectra2[i][0];
            float Im1 = spectra2[i][1];
            spectra[i][0] = Re2 * Re1 + Im2 * Im1;
            spectra[i][1] = Re2 * Im1 - Re1 * Im2;
            //Re1*Im2 - Re2*Im1;	//Principa nepareizi sheit, jabūt sādi: Re2*Im1-Re1*Im2, bet tad sanak pīķis otra pusē, kas nesakrīt ar NIRFI datiem.
        }

        fftwf_free(spectra1);
        fftwf_free(spectra2);

        ///////////////////  Inversa FFT no signalu reizinajuma  ////////////////
        fftwf_complex* corelation = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * 2 * windowSize);
        // if (corelation == NULL) ALLOC_ERROR;

        fftwf_plan plan_backward = fftwf_plan_dft_1d(2 * windowSize, spectra, corelation, FFTW_BACKWARD, FFTW_ESTIMATE);
        fftwf_execute(plan_backward);

        char* psResultCor = JSON_getString(params->tree, "root:M3_Corelation");
        char* psName = malloc(strlen(psResultCor) + 100);
        // if(!psName) ALLOC_ERROR;
        sprintf(psName, "%s/corelationOfTwoSignals", psResultCor);

        int plot = 2;
        long double dt = (long double)1.0 / discretization;
        createPSfile(corelation, windowSize, dt, psName, plot);

        fftwf_destroy_plan(plan_backward);
    } else if(type == 1) {
        char* psName = malloc(strlen(M3_SPECTRA_DIR) + 100);
        // if(!psName) ALLOC_ERROR;
        sprintf(psName,"%s/%s-%s.spectra", M3_SPECTRA_DIR, sig1, sig2);
        /*
        k0 == 0.0030675;
        k1 == 0.000000062+k0;
        k2 == 0.000000062+k1;
        */
        FILE* tempfile = fopen(psName, "a+");
        long double dt = (long double)1.0 / discretization;
        long double dw = (long double)1.0 / (dt * dataSize);

        fftwf_complex* SignalsMply = multiplyOfSignals(signal1, signal2, dataSize, u);

        for(long i = dataSize / 2 - 250; i < dataSize / 2 + 250; ++i){
            float p = (float)(sqrt(pow(SignalsMply[i][0], 2) + pow(SignalsMply[i][1], 2)));
            float d = (float)(dw * i - dw * dataSize / 2);
            fprintf(tempfile, "%d %f %lf\n",u, d, p);
        }

        fprintf(tempfile, "\n");
        fclose(tempfile);
        fftwf_free(SignalsMply);
    }
    printf("\033[32mOk!\033[0m\n");
}

void createPSfile(fftwf_complex *spectra, long unsigned dataSize, long double dw, char *psName, int plot){
    if(plot == 1){

        FILE *pipe = popen("gnuplot","w");
        fprintf(pipe, "set terminal postscript\n");
        fprintf(pipe, "set output '%s'\n", psName);
        fprintf(pipe, "set title 'spektrs'\n");
        fprintf(pipe, "set xlabel 'hz'\n");
        fprintf(pipe, "set ylabel 'Power'\n");
        fprintf(pipe, "plot '-' with lines\n");
        for(unsigned long i = 8000000; i < 24000000; ++i) {
            fprintf(pipe, "%g %g\n", (float)(dw*i), (float)(sqrt(pow( spectra[i][0], 2) + pow(spectra[i][1], 2))) );
        }

        fprintf(pipe, "e");
        fflush(pipe);
        fclose(pipe);

    }else if(plot == 2){
        long double dt = dw;
        float *cor;
        cor = (float*)malloc(dataSize*2 * sizeof(float));
        // if(cor == NULL) ALLOC_ERROR;

        for(unsigned long i = 0; i < dataSize; ++i) {
            cor[i] =
                    (
                            sqrt (pow( spectra[dataSize+i][0], 2) +
                                  pow(spectra[dataSize+i][1], 2) )
                    );
        }
        for(unsigned long i = dataSize; i < 2 * dataSize; ++i) {
            cor[i] =
                    (
                            sqrt (pow( spectra[i-dataSize][0], 2) +
                                  pow(spectra[i-dataSize][1], 2) )
                    );
        }

        fftwf_free(spectra);


        int mean;
        if (dataSize > 0.5 * 16000000) {
            mean = 1;
        } else {
            mean = 0;
        }

        if(mean == 1) {
            meanOfCorrGraph(cor, dataSize, dt, psName);
        } else if (mean == 0) {
            FILE* pipe = popen("gnuplot","w");
            fprintf(pipe, "set terminal postscript\n");
            fprintf(pipe, "set output '%s.ps'\n", psName);
            fprintf(pipe, "set title 'korelacijas grafiks'\n");
            fprintf(pipe, "set xlabel 'aizture (mu sec)'\n");
            fprintf(pipe, "set ylabel 'Power'\n");

            fprintf(pipe, "plot '-' with lines\n");

            for(unsigned long i = 0; i < 2 * dataSize; ++i) {
                fprintf(pipe, "%f %lf\n", (float)((float)i*dt-(float)dataSize*dt), cor[i]);
            }

            fprintf(pipe, "e");
            fflush(pipe);
            fclose(pipe);
        }
    }
}

void meanOfCorrGraph(float* cor, long unsigned dataSize, long double dt, char* psName) {
    long unsigned h = (long unsigned)dataSize / (0.5 * 16000000);
    long unsigned mean = h / 2;
    long unsigned interval = h;
    long unsigned newDataSize = 2 * (dataSize / mean) - 1;

    float* newCor = malloc(newDataSize * sizeof(float));
    // if(newCor == NULL) ALLOC_ERROR;

    for(long i = 0; i < newDataSize; ++i){
        float avg = 0;
        for(long j = 0; j < interval; ++j) {
            avg = avg + cor[mean * i + j];
        }
        newCor[i] = (avg / interval);
    }
    fftwf_free(cor);
    dt = dt*mean;

    FILE *pipe = popen("gnuplot","w");

    fprintf(pipe, "set terminal postscript\n");
    fprintf(pipe, "set output '%s.ps'\n", psName);
    fprintf(pipe, "set title 'korelacijas grafiks'\n");
    fprintf(pipe, "set xlabel 'aizture (mu sec)'\n");
    fprintf(pipe, "set ylabel 'Power'\n");
    fprintf(pipe, "plot '-' u (($0*%g-%g)*1e6 ):1 with lines\n", (double) (dt), (double)(DELTA * dt));

    for(long i = newDataSize / 2 - DELTA; i < (newDataSize / 2 + DELTA); ++i){
        fprintf(pipe,"%f\n", newCor[i]);
    }

    fftwf_free(newCor);
    fprintf(pipe, "e");
    fflush(pipe);
    fclose(pipe);
}

fftwf_complex* spectraOfSignal(float* sig, long unsigned dataSize, long unsigned WN) {

    fftwf_complex* inputData = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * 2 * dataSize);
    // if (inputData == NULL) ALLOC_ERROR;

    long unsigned windowSize = dataSize / WN;

    for(long i = 0; i < windowSize; ++i){
        inputData[i][0] = sig[i*2 ];
        inputData[i][1] = sig[i*2 + 1];
    }

    for(long i = windowSize; i < 2 * windowSize; ++i){
        inputData[i][0] = 0;
        inputData[i][1] = 0;
    }

    fftwf_complex* spectra = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * 2 * dataSize);
    // if (spectra == NULL) ALLOC_ERROR;

    fftwf_plan plan_forward = fftwf_plan_dft_1d(2 * dataSize, inputData, spectra, FFTW_FORWARD, FFTW_ESTIMATE);
    fftwf_execute(plan_forward);

    fftwf_free(inputData);
    free(sig);
    return spectra;
}

fftwf_complex* multiplyOfSignals(float* sig1, float* sig2, long unsigned dataSize, int u){
    fftwf_complex* s1 = (fftwf_complex*)sig1;
    fftwf_complex* s2 = (fftwf_complex*)sig2;

    for(long j = 0; j < dataSize; ++j){
        float Re1 = s1[j][0];
        float Im1 = s1[j][1];
        float Re2 = 0.0f;
        float Im2 = 0.0f;

        if(u < 0){
            int delay = -1 * u;
            if(delay + j <= dataSize){
                Re2 = s2[delay+j][0];
                Im2 = s2[delay+j][1];
            }
        }else {
            if( j - u >= 0){
                Re2 = s2[j-u][0];
                Im2 = s2[j-u][1];
            }
        }
        sig1[j * 2] = Re1 * Re2 - Im1 * Im2;
        sig1[j * 2 + 1] = Re1 * Im2 + Re2 * Im1;
    }

    fftwf_complex* spectra = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * dataSize);
    // if (spectra == NULL) ALLOC_ERROR;

    fftwf_plan plan_forward = fftwf_plan_dft_1d(dataSize, s1, spectra, FFTW_FORWARD, FFTW_ESTIMATE);
    fftwf_execute(plan_forward);

    for(long j = 0; j < dataSize / 2; ++j){
        s1[j][0] = spectra[dataSize / 2 + j][0];
        s1[j][1] = spectra[dataSize / 2 + j][1];
    }
    for(long j = 0; j < dataSize / 2; ++j){
        s1[dataSize / 2 + j][0] = spectra[j][0];
        s1[dataSize / 2 + j][1] = spectra[j][1];
    }

    fftwf_free(spectra);
    return s1;
}

char* getSignalFileName(char* signalName, int channel) {
    DIR* directory = NULL;
    if ((directory = opendir(SIGNAL_DIR)) == NULL) {
        panic("cannot open freq dir");
    }

    char* channelPostfix = malloc(4);
    sprintf(channelPostfix, "_%d", channel);

    char* filename = NULL;
    struct dirent* entry;
    while ((entry = readdir(directory)) != NULL) {
        int isCorrectFile = 0;
        char** split = splitString(entry->d_name, '_');
        for (int i = 0; split[i]; ++i) {
            char* tok = strToUpper(strdup(split[i]));
            char* sig = strToUpper(strdup(signalName));
            if (strcmp(tok, sig) == 0) {
                isCorrectFile = 1;
            }
            free(tok);
            free(sig);
            free(split[i]);
        }
        free(split);

        char* chpos = strrchr(entry->d_name, '_');
        if (chpos != NULL && isCorrectFile &&
            strncmp(chpos, channelPostfix, strlen(channelPostfix)) == 0) {
            filename = entry->d_name;
            break;
        }
    }

    if (filename == NULL) {
        panic("cannot find signal file for channel");
    }

    free(channelPostfix);
    return filename;
}