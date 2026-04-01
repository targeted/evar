#ifndef EVAR_H
#define EVAR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <evar_types.h>

/*
 * Syntax sugar to reference other tasks by name.
 */
#define EVAR_TASK(task_name) extern evar_task_t* task_name

/*
 * Called once at application startup, creates the first task
 * and keeps running the scheduler until all the tasks have exited.
 * After all tasks have exited the device is halted.
 */
void evar__run(evar_task_t* p_main_task, void* p_main_task_data);

/*
 * Creates the first task and returns, expecting future calls to evar__loop,
 * supposedly called from the Arduino-like setup function.
 */
void evar__setup(evar_task_t* p_main_task, void* p_main_task_data);

/*
 * A single pass of the scheduler, picking and running one task and then returning.
 * This is similar to what evar__run runs in an infinite loop. It is supposed to be
 * called from Arduino-like loop function, returns nothing, and does not halt the
 * device even after all the tasks have exited, just nothing is done after that.
 */
void evar__loop(void);

#ifdef __cplusplus
}
#endif

#endif
