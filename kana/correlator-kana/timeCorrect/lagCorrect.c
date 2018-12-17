#include <stdio.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include "../directories.h"
#include "../panic.h"
#include "../utils.h"
#include "lagCorrect.h"

void processDataFolder(JSON_Tree* params, char* station) {
	DIR* dip;
	if ((dip = opendir(SIGNAL_DIR)) == NULL) {
		panic("cannot open signal dir");
	}

	struct dirent* entry;
	while ((entry = readdir(dip)) != NULL) {
		if (isKanaFile(entry, SIGNAL_DIR, station)) {
			printf("\t%s : ", entry->d_name);
			fflush(stdout);
			dataCorrection(params, entry->d_name, station);
			printf("\033[32mOk!\033[0m\n");
		}
	}
}

void dataCorrection(JSON_Tree* params, char* filename, char* station){
	char* jsonFile = malloc(strlen(JSON_DIR) + strlen(filename) + 4);
	if(!jsonFile) ALLOC_ERROR;
	sprintf(jsonFile,"%s%s", JSON_DIR, filename);
	changeFileExt(jsonFile, ".json");

	JSON_Tree* jsonParam = malloc(sizeof(JSON_Tree));
	if (jsonParam == NULL) ALLOC_ERROR;
	if(JSON_read(jsonParam, jsonFile) == 0) {
		panic("cannot read json file");
	}

	long floatCount = JSON_getInt(jsonParam->tree, "root:float_count");
	char* corr = JSON_getString(jsonParam->tree, "root:corrections");

	if (strlen(corr) != 0) {
		panic("time correction must be the first correction");
	}

	float* signalRe = malloc(floatCount * sizeof(float));
	if(signalRe == NULL) ALLOC_ERROR;

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

	///////////////////////////////////////////////////

	long discret = JSON_getInt(params->tree, "root:discretization(Hz)");

	Coefficients* coeff = malloc(sizeof(Coefficients));
	getCoefficients(coeff, params, jsonParam, station);

	long double F0 = 0.0;
	coeff->F0 = F0 * pow(10.0, 6);	//convert from MHz to Hz
	coeff->dt = (long double)(1.0 / discret);

	double startLag = lagFunction(coeff, coeff->sTime + 0 * coeff->dt);
	double endLag = lagFunction(coeff, coeff->sTime + floatCount * coeff->dt);
	double diffLag = fabs(endLag - startLag);
	long int firstDelay = round(fabs(startLag / coeff->dt));
	int Nlags = round(diffLag / coeff->dt);

	struct lagCorrect* lc = malloc(Nlags * sizeof(struct lagCorrect));

	int indexLC = 0;
	long double prevStep;
	double T;
	firstDelay = 0;
	// laika aiztures korekcija visa signala garumaa:
	for (long j = 0; j < floatCount - firstDelay; ++j) {
		T = coeff->sTime + j*coeff->dt;
		long double lag = lagFunction(coeff, T);

		if(j == 0){
			prevStep = lag;
		}

		if(fabs(lag-prevStep) >= coeff->dt / 2){
			if(lag-prevStep >= 0){
				prevStep = prevStep + coeff->dt;
				lc[indexLC].index = j + firstDelay;
				lc[indexLC].remove = 1;
				indexLC++;
			}else{
				prevStep = prevStep - coeff->dt;
				lc[indexLC].index = j + firstDelay;
				lc[indexLC].remove = 0;
				indexLC++;
			}
		}
	}

	LagCorect(signalRe, lc, floatCount, indexLC);

	updateJson(jsonParam, jsonFile, startLag, endLag);
	writeSignal(signalRe, jsonParam, filename);

	free(signalRe);
	free(jsonFile);
	free(coeff);
	free(jsonParam->tree);
	free(lc);
}

void LagCorect(float* signal, struct lagCorrect* lc, long unsigned dataSize, int indexLC){
	for (long j = 0; j < indexLC; ++j) {
		long index = lc[j].index;

		if(lc[j].remove == 1) { // remove value from signal
			while(index < dataSize-1) {
				signal[index] = signal[index+1];
				++index;
			}
			signal[index] = 0.0;
			for (long k = j + 1; k < indexLC; ++k) {
				lc[k].index = lc[k].index-1;
			}
		} else if(lc[j].remove == 0) { // add value in signal
			float nextVal = signal[index + 1];
			float prevVal = signal[index];
			while( index < dataSize - 1) {
				signal[index + 1] = prevVal;
				prevVal = nextVal;
				if(index + 2 > dataSize - 1) {
					break;
				}
				nextVal = signal[index+2];
				++index;
			}
			for (long k = j + 1; k < indexLC; ++k) {
				lc[k].index = lc[k].index + 1;
			}
		} else {
			panic("cannot lag correct");
		}

	}
}

long double lagFunction(Coefficients* coeff, double T){
	long double result, dt;

	dt = 0.0;
	//dt = 0.180*pow(10.0, -6);
	result = coeff->A0 + coeff->A1*T + coeff->A2*pow(T,2) + coeff->A3*pow(T,3) + dt;
	//printf("%.16Lf\n", result);
	if (isnan(result)) {
		return 0.0;
	}
	return result;
}

void writeSignal(float* signal, JSON_Tree* jsonParam, char* fName){
	long floatCount = JSON_getInt(jsonParam->tree, "root:float_count");

	char* datFile = malloc(strlen(SIGNAL_DIR) + strlen(fName) + 10);
	if(!datFile) {
		ALLOC_ERROR;
	}
	sprintf(datFile,"%s%s", strdup(SIGNAL_DIR), strdup(fName));

	FILE* file = fopen(datFile, "w");
	if(!file) {
		char message[32];
		sprintf(message, "cannot open '%s'", datFile);
		panic(message);
	}
	if(fwrite(signal, sizeof(float), floatCount, file) != floatCount){
		panic("cannot write to file");
	}
	fclose(file);
	free(datFile);
}

void updateJson(JSON_Tree* jsonParam, char* jsonPath, double sL, double eL){
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

	fprintf(file, "{\n");
	fprintf(file, "\t\"file\": \"%s\",\n", filestr);
	fprintf(file, "\t\"float_count\": %ld,\n", floatCount);
	fprintf(file, "\t\"corrections\": \"t\",\n");
	fprintf(file, "\t\"start_time\": %.15f,\n", sL + startTime);
	fprintf(file, "\t\"end_time\": %.15f\n", eL + endTime);
	fprintf(file, "}\n");
	fclose(file);

	free(filestr);
	free(corr);
}
