#ifndef ANAIN_PRJ_H_INCLUDED
#define ANAIN_PRJ_H_INCLUDED

#include "hwdefs.h"

#define NUM_SAMPLES 64
#define SAMPLE_TIME ADC_SMPR_SMP_71DOT5CYC

#define ANA_IN_LIST \
   ANA_IN_ENTRY(curneg,       GPIOA, 0) \
   ANA_IN_ENTRY(curpos,       GPIOA, 1) \
   ANA_IN_ENTRY(uaux,         GPIOA, 4)

#endif // ANAIN_PRJ_H_INCLUDED
