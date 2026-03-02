/*
 * Evar, cooperative message-passing task scheduler.
 * https://github.com/targeted/evar
 *
 * Copyright (c) 2026 Dmitry Dvoinikov <dmitry@targeted.org>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <evar_vars.h>

/*
 * The following global variables definitions are arranged in the order of
 * access optimization ease/importance, i.e. the most benefits are to be obtained
 * if the first variables are put into some kind of device-specific fast memory.
 */

/*
 * Id of the task currently picked for execution.
 */
evar_task_id_t evar_current_task;

/*
 * This structure contains heads of the task lists.
 * The four lists are AVAILABLE, RUNNING, SLEEPING and RECEIVED
 */
evar_task_id_t evar_task_list_heads[4];

/*
 * This array accumulates the quanta, charged to the tasks picked for execution.
 */
unsigned char evar_task_quanta[EVAR_MAX_TASKS];

/*
 * This is a dedicated interlocked mini-queue that registers messages
 * that interrupt handlers send to the tasks. It must be accessed
 * with interrupts disabled.
 */
evar_received_async_t evar_received_async;

/*
 * This is the internal half of the running task information,
 * beneficial to have faster access to.
 */
evar_task_entry_t evar_task_entries[EVAR_MAX_TASKS];

/*
 * This is the external half of the running task information,
 * accessed not so frequently.
 */
evar_task_struct_t evar_task_structs[EVAR_MAX_TASKS];

/*
 * This is the current starting point of the global timestamp.
 */
_evar_timestamp_t evar_base_timestamp;

/*
 * This is the current global timestamp, updated once per scheduler pass.
 * Here it is the opaque value presented to the application code.
 */
evar_timestamp_t evar_current_timestamp;

/*
 * This will keep the previous value returned by evar_device__get_timer_ticks()
 */
evar_timer_ticks_t evar_prev_timer_ticks;

/*
 * This is the internal clock in ticks, updated once per scheduler pass,
 * after which it becomes the "now" for the duration of the pass.
 * On the time scale it appears to the right from evar_base_timestamp and
 * is the difference between evar_current_timestamp and evar_base_timestamp.
 */
evar_clock_ticks_t evar_clock_ticks;

/*
 * Number of currently active tasks.
 */
evar_task_id_t evar_active_tasks;
