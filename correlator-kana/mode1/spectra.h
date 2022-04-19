#ifndef KANA_SPECTRA_H
#define KANA_SPECTRA_H

#include <fftw3.h>

typedef struct {
    char* fileName; // the name of the data file
    unsigned long size; // the size in bytes of the correlated fragment
    unsigned long begin; // the begin byte of the correlated fragment
    unsigned long window; // the size of the window of frequencies in fft
    unsigned long target; // the center frequency in fft
    long discretization;
    double fft_compress;
    long double	epoch; // the start time of the correlation
    long double time; // the start time in the observation
    long double length; // the length in seconds of the correlation
    long obsTime; // full observation length in seconds
    char* station;

    // VDIF info
    long headerSize;
    long frameSize;
    long beginFrame;
    long readFrames;
    long chanCount;

} CorrelationData;

typedef struct {
    float* data;
    unsigned long size;
} Spectra;
Spectra* newSpectra(long size);
void freeSpectra(Spectra* spectra);

Spectra* fourierTransform(unsigned char* buffer, CorrelationData* cdata);
Spectra* createSpectra(float* data, CorrelationData* cdata);
Spectra* compressSpectra(Spectra* spectra, CorrelationData* cdata);
Spectra* cropSpectra(Spectra* spectra, CorrelationData* cdata);
float complexToMagnitude(fftwf_complex complex);
void writeSpectra(Spectra* spectra, CorrelationData* cdata);
void writeSignal(float* data, CorrelationData* cdata);
void writeJson(CorrelationData* cdata);
float* bytesToBinary(const unsigned char* buffer, unsigned long size);

#endif //KANA_SPECTRA_H
