#ifndef EVAR_DEVICE_TYPES_H
#define EVAR_DEVICE_TYPES_H

#include <evar_preproc.h>
#include <evar_config.h>

EVAR_ASSERT((EVAR_TIMER_FREQUENCY) <= 1000000, max_timer_frequency_1mhz);

#ifndef EVAR_USEC_TO_TICKS
#define EVAR_USEC_TO_TICKS(usec) ((usec) / (1000000 / (EVAR_TIMER_FREQUENCY)))
#endif

#ifndef EVAR_TICKS_TO_USEC
#define EVAR_TICKS_TO_USEC(ticks) ((ticks) * (1000000 / (EVAR_TIMER_FREQUENCY)))
#endif

typedef unsigned short evar_timer_ticks_t; // hardware timer ticks, when timer ticks every 100us = 0.1ms, overflows in about 6.5 seconds

#endif
