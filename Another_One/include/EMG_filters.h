#ifndef __EMGFILTERS_H_
#define __EMGFILTERS_H_
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include "cJSON.h"
#include <stdbool.h>


typedef enum { NOTCH_FREQ_50HZ = 50, NOTCH_FREQ_60HZ = 60 } NOTCH_FREQUENCY;
typedef enum { SAMPLE_FREQ_500HZ = 500, SAMPLE_FREQ_1000HZ = 1000 } SAMPLE_FREQUENCY;

void EMG_init(SAMPLE_FREQUENCY sampleFreq, NOTCH_FREQUENCY  notchFreq, bool enableNotchFilter,bool enableLowpassFilter, bool enableHighpassFilter);
int EMG_update(int inputValue);
int getEMGCount(int gforce_envelope);

#endif