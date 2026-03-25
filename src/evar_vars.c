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
evar_task_id_t _evar_current_task;

/*
 * This structure contains heads of the task lists.
 * The four lists are AVAILABLE_LIST, RUNNING_LIST, SLEEPING_LAST and RECEIVED_LIST.
 */
evar_task_id_t _evar_task_list_heads[TASK_LIST_COUNT];

/*
 * This array accumulates the quanta, charged to the tasks picked for execution.
 */
unsigned char _evar_task_quanta[EVAR_MAX_TASKS];

/*
 * This is a dedicated interlocked mini-queue that registers tasks that
 * have received messages asynchronously, from the interrupt handlers.
 */
_evar_received_async_queue_t _evar_received_async_queue;

/*
 * This is the internal half of the running task information,
 * beneficial to have faster access to.
 */
_evar_task_entry_t _evar_task_entries[EVAR_MAX_TASKS];

/*
 * This is the external half of the running task information,
 * accessed not so frequently.
 */
_evar_task_struct_t _evar_task_structs[EVAR_MAX_TASKS];

/*
 * This is the current starting point of the global timestamp.
 */
_evar_timestamp_t _evar_base_timestamp;

/*
 * This is the internal clock in ticks, updated once per scheduler pass.
 * On the time scale it appears to the right from _evar_base_timestamp and
 * is the difference between _evar_current_timestamp and _evar_base_timestamp.
 */
_evar_clock_ticks_t _evar_clock_ticks;

/*
 * This is the current timestamp for the pass and is equal to
 * _evar_base_timestamp + usec(_evar_clock_ticks).
 */
_evar_timestamp_t _evar_current_timestamp;

/*
 * This will keep the previous value returned by evar_device__get_timer_ticks()
 */
unsigned short _evar_prev_timer_ticks;

/*
 * Number of currently active tasks.
 */
unsigned char _evar_active_task_count;
