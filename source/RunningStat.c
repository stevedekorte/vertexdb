#include "RunningStat.h"
#include "Log.h"
#include "Date.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

RunningStat *RunningStat_new(void)
{
	RunningStat *self = calloc(1, sizeof(RunningStat));
	RunningStat_setSampleCount_(self, 100);
	self->startDate = Date_new();
	Date_now(self->startDate);
	return self;
}

void RunningStat_free(RunningStat *self)
{
	free(self->times);
	Date_free(self->startDate);
	free(self);
}

void RunningStat_setSamplesTo_(RunningStat *self, size_t dt)
{
	int i;
	
	for (i = 0; i < self->sampleCount; i ++)
	{
		self->times[i] = dt;
	}
	
	self->runningSum = dt * self->sampleCount;
}

void RunningStat_setSampleCount_(RunningStat *self, size_t count)
{
	self->sampleCount = count;
	self->times = realloc(self->times, count * sizeof(size_t));
	RunningStat_setSamplesTo_(self, 0);
	self->started = 0;
}

size_t RunningStat_sampleCount(RunningStat *self)
{
	return self->sampleCount;
}

void RunningStat_startTimer(RunningStat *self)
{
	self->t1 = Date_secondsSinceNow(self->startDate);
}

void RunningStat_stopTimer(RunningStat *self)
{
	size_t dt = (size_t)((Date_secondsSinceNow(self->startDate) - self->t1) * 1000000.0);
	
	if (!self->started)
	{		
		self->started = 1;
		RunningStat_setSamplesTo_(self, dt);
	}
	else
	{
		self->runningSum -= self->times[self->index];
		self->times[self->index] = dt;
		self->runningSum += self->times[self->index];
	}
	
	self->index ++;
	self->index = self->index % self->sampleCount;
}

double RunningStat_aveTime(RunningStat *self)
{
	return ((double)self->runningSum / (double)self->sampleCount)/1000000.0;
}

double RunningStat_lastTime(RunningStat *self)
{
	int i = self->index - 1;
	if (i == -1) i = self->sampleCount - 1;
	return ((double)self->times[i])/1000000.0;
}
