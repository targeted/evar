#ifndef EVAR_VARS_H
#define EVAR_VARS_H

#include <evar_types.h>

/*
 * The following global variables declarations are arranged in the order of
 * access optimization ease/importance, i.e. the most benefits are to be obtained
 * if the first variables are put into some kind of device-specific fast memory.
 */

/*
 * Id of the task currently picked for execution.
 */
extern evar_task_id_t evar_current_task;

/*
 * This structure contains heads of the task lists.
 * The four lists are AVAILABLE, RUNNING, SLEEPING and RECEIVED
 */
extern evar_task_id_t evar_task_list_heads[4];

/*
 * This array accumulates the quanta, charged to the tasks picked for execution.
 */
extern unsigned char evar_task_quanta[EVAR_MAX_TASKS];

/*
 * This is a dedicated interlocked mini-queue that registers messages
 * that interrupt handlers send to the tasks. It must be accessed
 * with interrupts disabled.
 */
extern evar_received_async_t evar_received_async;

/*
 * This is the internal half of the running task information,
 * beneficial to have faster access to.
 */
extern evar_task_entry_t evar_task_entries[EVAR_MAX_TASKS];

/*
 * This is the external half of the running task information,
 * accessed not so frequently.
 */
extern evar_task_struct_t evar_task_structs[EVAR_MAX_TASKS];

/*
 * This is the current starting point of the global timestamp.
 */
extern _evar_timestamp_t evar_base_timestamp;

/*
 * This is the current global timestamp, updated once per scheduler pass.
 * Here it is the opaque value presented to the application code.
 */
extern evar_timestamp_t evar_current_timestamp;

/*
 * This will keep the previous value returned by evar_device__get_timer_ticks()
 */
extern evar_timer_ticks_t evar_prev_timer_ticks;

/*
 * This is the internal clock in ticks, updated once per scheduler pass,
 * after which it becomes the "now" for the duration of the pass.
 * On the time scale it appears to the right from evar_base_timestamp and
 * is the difference between evar_current_timestamp and evar_base_timestamp.
 */
extern evar_clock_ticks_t evar_clock_ticks;

/*
 * Number of currently active tasks.
 */
extern evar_task_id_t evar_active_tasks;

#endif
