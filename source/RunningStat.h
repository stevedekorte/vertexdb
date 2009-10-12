#ifndef RunningStat_DEFINED
#define RunningStat_DEFINED 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include "Date.h"

typedef struct
{
	size_t sampleCount;
	int started;
	int index;
	size_t *times;
	double t1;
	size_t runningSum; // in microseconds
	Date *startDate;
} RunningStat;

RunningStat *RunningStat_new(void);
void RunningStat_free(RunningStat *self);
void RunningStat_setSampleCount_(RunningStat *self, size_t count);
size_t RunningStat_sampleCount(RunningStat *self);
void RunningStat_startTimer(RunningStat *self);
void RunningStat_stopTimer(RunningStat *self);
double RunningStat_aveTime(RunningStat *self);

#ifdef __cplusplus
}
#endif
#endif
