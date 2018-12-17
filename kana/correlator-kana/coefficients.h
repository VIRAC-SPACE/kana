#ifndef COEFFICIENTS_H
#define COEFFICIENTS_H

#include "jsonReader.h"

typedef struct {
	long double	dt;
  long double F;
	long double	F0;
	long double	Fsn;
  long double A0;
  long double A1;
	long double	A2;
	long double	A3;
	long double	sTime;
} Coefficients;

inline void getCoefficients(Coefficients* coeff, JSON_Tree* coeffTree, JSON_Tree* jsonParam, char* signalName) {
	char* str = malloc(sizeof(char) * 255);
	unsigned long i = 0;
	while(1){
		sprintf(str, "root:stations:%s:coefficients:%ld:Start_time", signalName, i);
		double sTime1 = JSON_getFloat(coeffTree->tree, str);

		sprintf(str, "root:stations:%s:coefficients:%ld:End_time", signalName, i);
		double eTime1 = JSON_getFloat(coeffTree->tree, str);

		double sTime2 = JSON_getFloat(jsonParam->tree, "root:start_time");
		double eTime2 = JSON_getFloat(jsonParam->tree, "root:end_time");

		if(sTime1 <= sTime2 && eTime1 >= eTime2 ) {
      sprintf(str, "root:stations:%s:coefficients:%ld:F(kHz)", signalName, i);
			coeff->F = JSON_getFloat(coeffTree->tree, str);

			sprintf(str, "root:stations:%s:coefficients:%ld:F0(MHz)", signalName, i);
			coeff->F0 = JSON_getFloat(coeffTree->tree, str);

			sprintf(str, "root:stations:%s:coefficients:%ld:Fsn", signalName, i);
			coeff->Fsn = JSON_getFloat(coeffTree->tree, str);

      sprintf(str, "root:stations:%s:coefficients:%ld:A0", signalName, i);
			coeff->A0 = JSON_getFloat(coeffTree->tree, str);

      sprintf(str, "root:stations:%s:coefficients:%ld:A1", signalName, i);
			coeff->A1 = JSON_getFloat(coeffTree->tree, str);

			sprintf(str, "root:stations:%s:coefficients:%ld:A2", signalName, i);
			coeff->A2 = JSON_getFloat(coeffTree->tree, str);

			sprintf(str, "root:stations:%s:coefficients:%ld:A3", signalName, i);
			coeff->A3 = JSON_getFloat(coeffTree->tree, str);

			coeff->sTime = sTime2-sTime1;
			break;
		}
		++i;
	}

	free(str);
}

#endif /* COEFFICIENTS_H */
