#ifndef EVAR_CONFIG_H
#define EVAR_CONFIG_H

#include <evar_preproc.h>

// uncomment to enable internal asserts
//#define EVAR_DEBUG

#define EVAR_MAX_TASKS (8)
#define EVAR_TIMER_FREQUENCY (1000)

EVAR_ASSERT((EVAR_TIMER_FREQUENCY) <= 1000000, max_timer_frequency_1mhz);

#ifndef EVAR_USEC_TO_TICKS
#define EVAR_USEC_TO_TICKS(usec) ((usec) / (1000000 / (EVAR_TIMER_FREQUENCY)))
#endif

#ifndef EVAR_TICKS_TO_USEC
#define EVAR_TICKS_TO_USEC(ticks) ((ticks) * (1000000 / (EVAR_TIMER_FREQUENCY)))
#endif

/*
 * This check is not strictly mandatory, but it is nice to avoid rounding (and division).
 * The conversion may not necessarily be to a power of 10, with a source clock of 1MHz
 * one nice and fast to calculate option is to have the timer tick 15625 times per second,
 * then EVAR_USEC_TO_TICKS is (usec >> 6), EVAR_TICKS_TO_USEC is (ticks << 6),
 * and the practical minimal application level intervals are around 0.1ms.
 */
EVAR_ASSERT(EVAR_TICKS_TO_USEC(EVAR_USEC_TO_TICKS(1000000)) == 1000000, whole_usec_to_ticks);

#endif
