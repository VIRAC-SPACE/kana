#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <vdifio.h>
#include <sys/time.h>
#include "../include/directories.h"
#include "../include/panic.h"
#include "../include/utils.h"
#include "../include/socket.h"
#include "mode1.h"




void processDataFolder(JSON_Tree* params, char* station) {
    DIR* directory;
    if ((directory = opendir(DATA_DIR)) == NULL){
        panic("cannot open data dir");
    }

    struct dirent* entry;
    while ((entry = readdir(directory)) != NULL ) {
        if (isKanaFile(entry, DATA_DIR, station)) {
            printf("\t%s ", entry->d_name);
            if (isVDIF(params, station)) {
                printf("(VDIF) : ");
                fflush(stdout);
                processVDIFFile(params, entry, station);
            } else {
                printf("(MARK5) : ");
                fflush(stdout);
                processDataFile(params, entry, station);
            }
            printf("\033[32mOk!\033[0m\n");
        }
    }

    if (closedir(directory) == -1){
        perror("closedir");
    }
}

void processDataFile(JSON_Tree* params, struct dirent* entry, char* station) {
    char* filePath = malloc(strlen(DATA_DIR) + strlen(entry->d_name) + 1);
    sprintf(filePath, "%s%s", DATA_DIR, entry->d_name);

    FILE* input = fopen(filePath, "rb");
    if (input == NULL) {
        panic("couldn't open mark5 file");
    }

    CorrelationData* cdata = getCorrelationData(params, station, entry->d_name);
    setTargetFreq(params, cdata, 1);

    unsigned char* buffer = malloc(cdata->size);
    // if (buffer == NULL) ALLOC_ERROR;

    fseek(input, cdata->begin + 60, SEEK_SET);
    fread(buffer, 1, cdata->size, input);

    Spectra* spectra = fourierTransform(buffer, cdata);

    freeSpectra(spectra);
    fclose(input);
    free(filePath);
    free(buffer);
    free(cdata);
}

void processVDIFFile(JSON_Tree* params, struct dirent* entry, char* station) {
    char* filePath = malloc(strlen(DATA_DIR) + strlen(entry->d_name) + 1);
    sprintf(filePath, "%s%s", DATA_DIR, entry->d_name);

    FILE* input = fopen(filePath, "rb");
    if(input == NULL) {
        panic("couldn't open vdif file");
    }

    CorrelationData* cdata = getCorrelationData(params, station, entry->d_name);

    long chanSize = (cdata->frameSize - cdata->headerSize) / cdata->chanCount;

    for (int chan = 1; chan <= cdata->chanCount; ++chan) {
        setTargetFreq(params, cdata, chan);
        cdata->fileName = addChanToFileName(entry->d_name, chan);

        unsigned char* buffer = malloc(cdata->size);
        // if (buffer == NULL) ALLOC_ERROR;

        // reset to begin frame
        fseek(input, cdata->frameSize * cdata->beginFrame, SEEK_SET);
        for (int i = 0; i < cdata->readFrames; ++i) {
            // seek to channel
            fseek(input, cdata->headerSize + (chan-1)*chanSize, SEEK_CUR);

            char chanBuffer[chanSize];
            fread(chanBuffer, 1, chanSize, input);

            // seek to frame end
            fseek(input, (cdata->chanCount-chan)*chanSize, SEEK_CUR);

            // copy channel data to buffer
            memcpy(buffer + i*chanSize, chanBuffer, chanSize);
        }

        Spectra* spectra = fourierTransform(buffer, cdata);
        freeSpectra(spectra);
        free(buffer);
    }

    fclose(input);
    free(filePath);
    free(cdata);
}

void processRealtimeData(JSON_Tree* params, char* station) {
    char* inAddress = JSON_getString(params->tree, "root:rt_in_address");
    int inSocket = connectToServer(inAddress);
    printf("\tInput connected to %s\n", inAddress);

    char* outAddress = JSON_getString(params->tree, "root:rt_out_address");
    int outSocket = connectToServer(outAddress);
    printf("\tOutput connected to %s\n", outAddress);

    CorrelationData* cdata = getCorrelationData(params, station, "");
    int tmp = 1;
    setTargetFreq(params, cdata, tmp);

    union {
        unsigned char bytes[8];
        long number;
    } longUnion;
    longUnion.number = cdata->window;
    Message* wndMsg = malloc(sizeof(Message));
    wndMsg->data = (unsigned char*)longUnion.bytes;
    wndMsg->size = 8;
    sendToSocket(outSocket, wndMsg);
    free(wndMsg);

    while(1) {
        struct timeval stop, start;
        gettimeofday(&start, NULL);

        Message* inMsg = recieveFromSocket(inSocket);
        if (inMsg == NULL) break;
        touchSocket(inSocket);

        cdata->size = inMsg->size;
        Spectra* spectra = fourierTransform(inMsg->data, cdata);

        Message* outMsg = malloc(sizeof(Message));
        outMsg->data = (unsigned char*)spectra->data;
        outMsg->size = sizeof(float)*spectra->size;
        sendToSocket(outSocket, outMsg);

        gettimeofday(&stop, NULL);
        printf("duration: %lu\n", stop.tv_usec - start.tv_usec);

        freeSpectra(spectra);
        freeMessage(inMsg);
        free(outMsg);
    }

    disconnectFromServer(inSocket);
    disconnectFromServer(outSocket);
    free(cdata);
}

CorrelationData* getCorrelationData(JSON_Tree* params, char* st, char* nm) {
    char* bufferBeginStr = JSON_getString(params->tree, "root:m1_buffer_begin");
    double bufferBegin = timeToSecs(bufferBeginStr);

    char* bufferEndStr = JSON_getString(params->tree, "root:m1_buffer_end");
    double bufferEnd = timeToSecs(bufferEndStr);

    double bufferLength = bufferEnd - bufferBegin;

    char* rtLenStr = JSON_getString(params->tree, "root:rt_batch_length");
    double rtLen = timeToSecs(rtLenStr);

    // bits per second
    long discretization = JSON_getInt(params->tree, "root:discretization(Hz)");
    double fftCompress = JSON_getFloat(params->tree, "root:m1_fft_compress");

    long spectraWindow = discretization;
    if (JSON_hasKey(params->tree, "root:m1_spectra_window(Hz)") == 1) {
        spectraWindow = JSON_getInt(params->tree, "root:m1_spectra_window(Hz)");
    }

    unsigned long bufferSize = floor(bufferLength * discretization) / 8;
    unsigned long bufferBeginByte = floor(bufferBegin * discretization) / 8;
    unsigned long rtSize = floor(rtLen * discretization) / 8;

    long rt = JSON_getInt(params->tree, "root:realtime");
    long obsLength = JSON_getInt(params->tree, "root:observation_length");

    CorrelationData* cdata = malloc(sizeof(CorrelationData));
    // if (cdata == NULL) ALLOC_ERROR;
    cdata->size = bufferSize;
    cdata->begin = bufferBeginByte;
    cdata->fft_compress = fftCompress;
    cdata->discretization = discretization;
    cdata->window = spectraWindow;
    cdata->time = bufferBegin;
    cdata->length = bufferLength;
    cdata->chanCount = 1;
    cdata->station = strdup(st);
    cdata->obsTime = obsLength;

    if (rt == 1) {
        cdata->size = rtSize;
        cdata->begin = 0;
        cdata->epoch = time(NULL);
        cdata->fileName = "rt.spectra";
    } else if (isVDIF(params, st)) {
        addVDIFData(cdata, nm);
    } else {
        char* filepath = malloc(64);
        sprintf(filepath, "%s%s", DATA_DIR, nm);
        cdata->epoch = readUnixTime(filepath);
        cdata->fileName = addChanToFileName(nm, 1);

        if (cdata->length < 0) {
            panic("'m1_buffer_end' must be bigger than 'm1_buffer_begin'");
        }

        if (getFileSize(filepath) < cdata->begin + cdata->size) {
            panic("file too small for buffer");
        }
    }

    return cdata;
}

void addVDIFData(CorrelationData* cdata, char* nm) {
    char* filePath = malloc(64);
    sprintf(filePath, "%s%s", DATA_DIR, nm);

    FILE* input = fopen(filePath, "rb");
    if (input == NULL) {
        panic("couldn't open vdif file");
    }

    // read first header for info
    vdif_header* header = malloc(VDIF_HEADER_BYTES);
    fread(header, 1, VDIF_HEADER_BYTES, input);

    long fileSize = getFileSize(filePath);
    int frameSize = getVDIFFrameBytes(header);
    long frameCount = fileSize / frameSize;
    long fps = frameCount / cdata->obsTime;

    long arraySize = frameSize - VDIF_HEADER_BYTES;
    long chanCount = getVDIFNumChannels(header);
    long chanSize = arraySize / chanCount;

    long beginFrame = fps * cdata->time;
    long readFrames = fps * cdata->length;

    cdata->epoch = (long double)getVDIFEpoch(header);
    cdata->size = readFrames * chanSize;

    cdata->headerSize = VDIF_HEADER_BYTES;
    cdata->frameSize = frameSize;
    cdata->beginFrame = beginFrame;
    cdata->readFrames = readFrames;
    cdata->chanCount = chanCount;

    if (beginFrame > fileSize / frameSize) {
        panic("'m1_buffer_begin' too big");
    }
    if (beginFrame + readFrames > fileSize / frameSize) {
        panic("'m1_buffer_end' too big");
    }
}

void setTargetFreq(JSON_Tree* params, CorrelationData* cdata, int ch) {
    char* targetLoc = malloc(strlen("root:stations:AA:target(Hz):00") + 1);
    sprintf(targetLoc, "root:stations:%s:target(Hz):%d", cdata->station, 1);

    if (JSON_hasKey(params->tree, targetLoc) == 0) {
        cdata->target = cdata->discretization / 2;
    } else {
        cdata->target = JSON_getInt(params->tree, targetLoc);
    }
}

char* addChanToFileName(char* dataFile, int chan) {
    char chanStr[12];
    sprintf(chanStr, "_%d", chan);
    char* chanFileName = malloc(strlen(dataFile) + strlen(chanStr) + 1);
    int x = (int)(strrchr(dataFile, '.') - dataFile);
    strncpy(chanFileName, dataFile, x);
    chanFileName[x] = '\0';
    strcat(chanFileName, chanStr);
    strcat(chanFileName, dataFile + x);
    return chanFileName;
}

int isVDIF(JSON_Tree* params, char* station) {
    char* str = malloc(64);
    sprintf(str, "root:stations:%s:format", station);
    char* format = JSON_getString(params->tree, str);

    if (strcmp(format, "VDIF") == 0) {
        return 1;
    } else {
        return 0;
    }
}
