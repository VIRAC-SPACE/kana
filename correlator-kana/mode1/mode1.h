#ifndef KANA_MODE_H
#define KANA_MODE_H

#include <dirent.h>
#include "../include/jsonReader.h"
#include "spectra.h"

void processDataFolder(JSON_Tree* params, char* signalName);
void processDataFile(JSON_Tree* params, struct dirent* dit, char* station);
void processVDIFFile(JSON_Tree* params, struct dirent* dit, char* station);
void processRealtimeData(JSON_Tree* params, char* station);

CorrelationData* getCorrelationData(JSON_Tree* params, char* st, char* nm);
void addVDIFData(CorrelationData* cdata, char* nm, JSON_Tree* params);
void setTargetFreq(JSON_Tree* params, CorrelationData* cdata, int ch);

char* addChanToFileName(char* dataFile, int chan);

int isVDIF(JSON_Tree* params, char* signalName);


#endif //KANA_MODE_H
