#ifndef EVAR_VARS_H
#define EVAR_VARS_H

#include <evar_types.h>
#include <evar_internal_types.h>

/*
 * The following global variables declarations are arranged in the order of
 * access optimization ease/importance, i.e. the most benefits are to be obtained
 * if the first variables are put into some kind of device-specific fast memory.
 */

/*
 * Id of the task currently picked for execution.
 */
extern evar_task_id_t _evar_current_task;

/*
 * This structure contains heads of the task lists.
 * The four lists are AVAILABLE_LIST, RUNNING_LIST, SLEEPING_LAST and RECEIVED_LIST.
 */
extern evar_task_id_t _evar_task_list_heads[TASK_LIST_COUNT];

/*
 * This array accumulates the quanta, charged to the tasks picked for execution.
 */
extern unsigned char _evar_task_quanta[EVAR_MAX_TASKS];

/*
 * This is a dedicated interlocked mini-queue that registers messages
 * that interrupt handlers send to the tasks. It must be accessed
 * with interrupts disabled.
 */
extern _evar_received_async_queue_t _evar_received_async_queue;

/*
 * This is the internal half of the running task information,
 * beneficial to have faster access to.
 */
extern _evar_task_entry_t _evar_task_entries[EVAR_MAX_TASKS];

/*
 * This is the external half of the running task information,
 * accessed not so frequently.
 */
extern _evar_task_struct_t _evar_task_structs[EVAR_MAX_TASKS];

/*
 * This is the current starting point of the global timestamp.
 */
extern _evar_timestamp_t _evar_base_timestamp;

/*
 * This is the internal clock in ticks, updated once per scheduler pass.
 * On the time scale it appears to the right from _evar_base_timestamp and
 * is the difference between _evar_current_timestamp and _evar_base_timestamp.
 */
extern _evar_clock_ticks_t _evar_clock_ticks;

/*
 * This is the current timestamp for the pass and is equal to
 * _evar_base_timestamp + usec(_evar_clock_ticks).
 */
extern _evar_timestamp_t _evar_current_timestamp;

/*
 * This will keep the previous value returned by evar_device__get_timer_ticks()
 */
extern unsigned short _evar_prev_timer_ticks;

/*
 * Number of currently active tasks.
 */
extern unsigned char _evar_active_task_count;

#endif
