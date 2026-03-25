#ifndef MAIN_TASK_H
#define MAIN_TASK_H

#include <evar.h>

EVAR_TASK(main_task);

typedef struct {
  
  /*
   * Initialization parameters.
   */
  evar_interval_t interval;

  /*
   * Runtime state.
   */
  unsigned char led;

} main_task_data_t;

typedef struct {

  /*
   * This task does not accept messages.
   */

} main_task_message_t;

#endif
