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

#include <string.h>
#include <evar.h>
#include <evar_vars.h>

/*
 * The following defines allow for priority tuning, e.g. if a task keeps receiving messages,
 * it will be allowed to execute four times before a task running in background will finally
 * supersede it.
 */
#define EVAR_QUANTA_PER_ROUND (4)
#define EVAR_RECEIVED_QUANTA  (1)
#define EVAR_SLEEPING_QUANTA  (2)
#define EVAR_RUNNING_QUANTA   (4)

/*
 * This check is not strictly mandatory, but it is nice to avoid rounding.
 * The conversion may not necessarily be to a power of 10, with a source clock of 1MHz
 * one nice and fast to calculate option is to have the timer tick 15625 times per second,
 * then EVAR_USEC_TO_TICKS is (usec >> 6), EVAR_TICKS_TO_USEC is (ticks << 6),
 * and the practical minimal application level intervals are around 0.1ms.
 */
//EVAR_ASSERT(EVAR_TICKS_TO_USEC(EVAR_USEC_TO_TICKS(1000000)) == 1000000, whole_usec_to_ticks);

#define VALID(task) ((task) < (EVAR_MAX_TASKS))
EVAR_ASSERT(!VALID(EVAR_INVALID_TASK_ID), invalid_task_id);

#define WAKE_UP_AT(task) (evar_task_entries[task].wake_up_at)
#define NEXT_UP_AT(task) (evar_task_entries[task].next_up_at)

#define P_TASK(task) (evar_task_structs[task].p_task)
#define P_MESSAGE_QUEUE(task) (P_TASK(task)->p_message_queue)
#define P_TASK_INFO(task) (&evar_task_structs[task].task_info)
#define P_MESSAGE_STORE(task) (P_TASK_INFO(task)->p_message_store)

#define PREV(task) (evar_task_entries[task].prev_task)
#define NEXT(task) (evar_task_entries[task].next_task)
#define LIST(task) (evar_task_entries[task].task_list)

#define DETACHED(task) (evar_task_entries[task].detached)
#define ACTIVE(task) (LIST(task) != AVAILABLE)

#define HEAD(task_list) (evar_task_list_heads[task_list])

/*
 * Connects two consecutive elements in a bidirectional linked list.
 */
static void link(evar_task_id_t prev, evar_task_id_t next) {
    evar_assert(VALID(prev));
    evar_assert(VALID(next));
    NEXT(prev) = next;
    PREV(next) = prev;
}

/*
 * Removes the task from the list it is currently on. The task becomes detached.
 */
static void remove_task(evar_task_id_t task) {

    task_list_t task_list;
    evar_task_id_t prev;
    evar_task_id_t next;

    evar_assert(VALID(task));
    evar_assert(!DETACHED(task));

    task_list = LIST(task);
    prev = PREV(task);
    if (prev == task) {
        evar_assert(NEXT(task) == task);
        HEAD(task_list) = EVAR_INVALID_TASK_ID;
    }
    else {
        next = NEXT(task);
        evar_assert(next != task);
        link(prev, next);
        if (task == HEAD(task_list)) {
            HEAD(task_list) = next;
        }
    }
    DETACHED(task) = 1;
}

/*
 * Inserts a detached task before another task in a task list.
 * The list can not be empty as it contains at least the before task.
 */
static void insert_task(evar_task_id_t task, evar_task_id_t before_task) {

    task_list_t task_list;
    evar_task_id_t prev;

    evar_assert(VALID(task));
    evar_assert(DETACHED(task));
    evar_assert(VALID(before_task));
    evar_assert(!DETACHED(before_task));

    prev = PREV(before_task);
    link(prev, task);
    link(task, before_task);
    task_list = LIST(before_task);
    LIST(task) = task_list;
    if (before_task == HEAD(task_list)) {
        HEAD(task_list) = task;
    }
    DETACHED(task) = 0;
}

/*
 * Appends a detached task at the end of a task list. The list can be empty,
 * then it becomes a list of one task.
 */
static void append_task(evar_task_id_t task, task_list_t task_list) {

    evar_task_id_t head;
    evar_task_id_t tail;

    evar_assert(VALID(task));
    evar_assert(DETACHED(task));

    head = HEAD(task_list);
    if (!VALID(head)) {
        PREV(task) = NEXT(task) = task;
        HEAD(task_list) = task;
    }
    else {
        tail = PREV(head);
        link(tail, task);
        link(task, head);
    }
    LIST(task) = task_list;
    DETACHED(task) = 0;
}

/*
 * We want to avoid disabling interrupts when manipulating task lists,
 * therefore when a task receives a message asynchronously from an interrupt
 * handler, we cannot put the task to the received list, instead we register
 * the fact in a separate dedicated queue-like structure. Then the scheduler
 * will check this structure before every pass, and move all the tasks that
 * have received asynchronous messages to the received task list.
 * The following utilities that manage the asynchronously received task
 * queue must be executed with interrupts disabled.
 */

/*
 * Clears the asynchronously received task queue.
 * Must be executed with interrupts disabled.
 */
static void clear_received_async(void) {
    evar_received_async.count = 0;
    memset(evar_received_async.bitmap, 0, sizeof(evar_received_async.bitmap));
}

/*
 * Removes the task from the asynchronously received task queue.
 * If the task is not there, nothing happens.
 * Must be executed with interrupts disabled.
 */
static void remove_task_from_received_async(evar_task_id_t task) {

    evar_task_id_t i;

    if (!(evar_received_async.bitmap[task >> 3] & (1 << (task & 7)))) { // the task is not registered
        return;
    }

    evar_assert(evar_received_async.count > 0); // there has to be at least one task there

    // find the task in the ordered list

    for (i = 0; i < evar_received_async.count; ++i) {
        if (evar_received_async.tasks[i] == task) {
            break;
        }
    }

    evar_assert(i < evar_received_async.count); // the task must have been found

    // the remaining tasks in the list must be shifted one position to the left to maintain order

    while (++i < evar_received_async.count) {
        evar_received_async.tasks[i - 1] = evar_received_async.tasks[i];
    }

    // finally we unmark the removed task

    evar_received_async.count -= 1;
    evar_received_async.bitmap[task >> 3] &= ~(1 << (task & 7));
}

/*
 * Registers the task as having received an asynchronous message.
 * If the task is already registered, nothing happens.
 * Must be executed with interrupts disabled.
 */
static void register_task_as_received_async(evar_task_id_t task) {

    if (evar_received_async.bitmap[task >> 3] & (1 << (task & 7))) { // the task is already registered
        return;
    }

    evar_assert(evar_received_async.count < (EVAR_MAX_TASKS)); // there must be space for at least one task

    // note that the new task appears at the end of the list,
    // thus making the list ordered by first message received

    evar_received_async.tasks[evar_received_async.count++] = task;
    evar_received_async.bitmap[task >> 3] |= 1 << (task & 7);
}

/*
 * Moves the tasks that have received asynchronous messages between two consequent
 * scheduler passes to the received task list. It is executed in the context of
 * the scheduler, outside any task, therefore all tasks must be on one list or
 * another, none can be detached.
 * Must be executed with interrupts disabled.
 */
static void flush_received_async(void) {

    evar_task_id_t slot;
    evar_task_id_t task;

    if (evar_received_async.count > 0) { // there are tasks that have received asynchronous messages

        // note that this loop keeps the order of tasks being appended
        // the same as they were on the received_async_tasks list
        // i.e. the same order in which they have received messages

        for (slot = 0; slot < evar_received_async.count; ++slot) {
            task = evar_received_async.tasks[slot];
            evar_assert(VALID(task));
            evar_assert(!DETACHED(task));
            if (ACTIVE(task) && LIST(task) != RECEIVED) {
                remove_task(task);
                append_task(task, RECEIVED);
            }
        }

        // after that, the async queue is cleared

        evar_received_async.count = 0;
        memset(evar_received_async.bitmap, 0, sizeof(evar_received_async.bitmap));
    }
}

/*
 * Initializes all the task structures. All the tasks are put on available list,
 * all the other lists are empty. The device is initialized to start the timer.
 */
void evar__initialize(void) {

    evar_task_id_t task;

    // the internal structures are hardware-agnostic and are initialized
    // before the device, in particular before starting the timer

    HEAD(AVAILABLE) = EVAR_INVALID_TASK_ID;
    HEAD(RUNNING)   = EVAR_INVALID_TASK_ID;
    HEAD(SLEEPING)  = EVAR_INVALID_TASK_ID;
    HEAD(RECEIVED)  = EVAR_INVALID_TASK_ID;

    for (task = 0; task < (EVAR_MAX_TASKS); ++task) {
        DETACHED(task) = 1;
        append_task(task, AVAILABLE);
    }
    evar_active_tasks = 0;

    memset(evar_task_quanta, 0, sizeof(evar_task_quanta));

    // here the device is initialized, in particular the timer is started, if it is supported

    evar_device__initialize();

    // time counting starts

    evar_base_timestamp.high = 0;
    evar_base_timestamp.low  = 0;

    evar_prev_timer_ticks = evar_device__get_timer_ticks();
    evar_clock_ticks = 0;

    // the async message registry is initialized after the device, only
    // because it is expected to be accessed with interrupts disabled

    evar_device__disable_interrupts();
    clear_received_async();
    evar_device__enable_interrupts();

    // now that the device is initialized, we pull all the diagnostic pins and LEDs down

    evar_device__builtin_led_off();
    evar_device__timer_pin_off();
    evar_device__running_pin_off();
    evar_device__idle_pin_off();
    evar_device__sending_pin_off();
    evar_device__receiving_pin_off();
    evar_device__crashed_pin_off();
    evar_device__halted_pin_off();

    // no task to execute yet

    evar_current_task = EVAR_INVALID_TASK_ID;
}

void evar__crash(unsigned short error, char* message) {
    evar_device__crash(error, message); // should not return
}

void evar__halt(void) {
    evar_device__halt(); // should not return
}

/*
 * Forward declaration.
 */
void evar_task__exit(void);

/*
 * Initializes one available task and moves it to running.
 * Returns the id of the created task.
 */
evar_task_id_t evar__create_task(evar_task_t* p_task, evar_task_data_t* p_task_data) {

    evar_task_id_t task;
    evar_task_info_t* p_task_info;
    evar_task_id_t prev_current_task;

    task = HEAD(AVAILABLE);
    if (VALID(task)) {

        evar_assert(evar_active_tasks < (EVAR_MAX_TASKS));
        evar_active_tasks += 1;

        // the task is removed from the available list and becomes detached

        remove_task(task);

        // fill in the task structures with the default values

        P_TASK(task) = p_task;

        p_task_info = P_TASK_INFO(task);
        p_task_info->current_task    = task;
        p_task_info->parent_task     = evar_current_task;
        p_task_info->p_task_data     = p_task_data; // with the call to initialize it is to be interpreted as initialization parameter
        p_task_info->p_message_store = NULL;

        WAKE_UP_AT(task) = EVAR_INVALID_CLOCK_TICKS;
        NEXT_UP_AT(task) = EVAR_INVALID_CLOCK_TICKS;

        // since the task id could have been reused, we must clear the related shared structures
        // otherwise the new task would have inherited:

        evar_task_quanta[task] = 0; // (1) the charged quanta (not terribly important)

        evar_device__disable_interrupts();     // (2) the fact that the previous task had received async messages,
        remove_task_from_received_async(task); // this is crucial, because the new task might not even be expecting
        evar_device__enable_interrupts();      // any messages at all

        // the task's initialize method is about to be called, and to be able
        // to use evar_task__... functions in it, we pretend for the moment
        // that the task being initialized is currently running

        prev_current_task = evar_current_task;
        evar_current_task = task;
        evar_assert(DETACHED(evar_current_task));

        // in its initialize function the task allocates memory,
        // initializes its state structures and initializes hardware

        evar_device__running_pin_on();
        p_task->initialize(p_task_info);
        evar_device__running_pin_off();

        // if the task did not make any decision on how to proceed with
        // its execution by calling one of the evar_task__... functions,
        // it is as if it has called evar_task__exit

        if (DETACHED(task)) {
            evar_task__exit();
        }

        // restore the actual currently running task

        evar_current_task = prev_current_task;

        if (LIST(task) == AVAILABLE) {
            return EVAR_INVALID_TASK_ID; // the task exited after initialization
        }

    }
    else { // the other reason for this call to fail is that we may have run out of tasks
        evar_assert(evar_active_tasks == (EVAR_MAX_TASKS));
    }

    return task;
}

/*
 * Sends a message from the current running task to another task.
 * Interrupt handlers and possibly other asynchronous sources
 * should use evar__send_async_message.
 */
evar_mq_result_t evar__send_message(
    evar_task_id_t receiver,
    void* p_message,
    evar_message_size_t message_size
) {

    evar_message_queue_t* p_message_queue;
    evar_mq_args_t push_args;
    evar_mq_result_t mq_result;

    evar_device__sending_pin_on();

    // receiver is used only to locate the queue, after that it becomes implicit

    evar_assert(VALID(receiver));
    evar_assert(DETACHED(receiver) || ACTIVE(receiver));

    evar_assert(p_message != NULL);

    p_message_queue = P_MESSAGE_QUEUE(receiver);
    if (p_message_queue == NULL) {
        evar_device__sending_pin_off();
        return EVAR_MQ_INVALID_QUEUE;
    }

    push_args.p_message_store = P_MESSAGE_STORE(receiver);
    push_args.p_message       = p_message;
    push_args.message_size    = message_size;

    evar_device__disable_interrupts();
    mq_result = p_message_queue->push(&push_args);
    evar_device__enable_interrupts();

    // since this call is made from a task, we have freedom
    // to manipulate task lists, and we move the receiver
    // to the received list

    if (mq_result == EVAR_MQ_SUCCESS) {
        if (!DETACHED(receiver)) {
            if (LIST(receiver) != RECEIVED) {
                remove_task(receiver);
                append_task(receiver, RECEIVED);
            }
        }
        else { // an odd case when a task is sending a message to itself
            evar_assert(receiver == evar_current_task);
        }
    }

    evar_device__sending_pin_off();
    return mq_result;
}

/*
 * Sends an asynchronous message from an interrupt handler to a task.
 * This function should only be used by interrupt handlers and possibly
 * other asynchronous sources. The tasks should use evar__send_message.
 * This can be executed with or without interrupts disabled. Interrupts
 * will be disabled for the push operation, and the previous state of
 * interrupts will be restored at exit.
 */
evar_mq_result_t evar__send_async_message(
    evar_task_id_t receiver,
    void* p_message,
    evar_message_size_t message_size
) {

    evar_message_queue_t* p_message_queue;
    evar_mq_args_t push_args;
    evar_interrupts_enabled_t interrupts_enabled;
    evar_mq_result_t mq_result;

    evar_device__sending_pin_on();

    // receiver is used only to locate the queue, after that it becomes implicit

    evar_assert(VALID(receiver));
    evar_assert(DETACHED(receiver) || ACTIVE(receiver));

    evar_assert(p_message != NULL);

    // from this moment we are using tasks shared state without disabling interrupts,
    // they are only disabled for the duration of pushing the message to the queue

    // this shared state does not change after the receiving task has been initialized,
    // the only race condition there is here, is when the receiving task exits, and at
    // that moment it receives an asynchronous message, therefore you should only make
    // interrupt handlers send messages to tasks that are running forever

    p_message_queue = P_MESSAGE_QUEUE(receiver);
    if (p_message_queue == NULL) {
        evar_device__sending_pin_off();
        return EVAR_MQ_INVALID_QUEUE;
    }

    push_args.p_message_store = P_MESSAGE_STORE(receiver);
    push_args.p_message       = p_message;
    push_args.message_size    = message_size;

#ifdef evar_device__save_and_disable_interrupts
    evar_device__save_and_disable_interrupts(interrupts_enabled);
#else
    evar_device__save_and_disable_interrupts(&interrupts_enabled);
#endif

    mq_result = p_message_queue->push(&push_args);

    if (mq_result == EVAR_MQ_SUCCESS) {

        // register the fact that the task has received an asynchronous message
        // in a dedicated interlocked mini queue, the scheduler will react upon
        // it at its next pass

        register_task_as_received_async(receiver);

        // because this is executed asynchronously to the scheduler, there is a chance
        // that this happens when the scheduler has put the CPU to sleep by a call to
        // evar_device__cpu_idle

        // for a regular interrupt-driven single-core MCU, that this code is even executing
        // would mean that the interrupt had happened, and its handler has called this function,
        // therefore the CPU was already awaken and is running normally, nothing is to be done

        // if, on the other hand, we are running in a multi-threaded multi-core environment,
        // where interrupts are ether simulated (e.g. Windows implementation) or behave
        // differently, we need to kick the sleeping scheduler by the following call

        evar_device__wake_cpu();
    }

#ifdef evar_device__restore_interrupts
    evar_device__restore_interrupts(interrupts_enabled);
#else
    evar_device__restore_interrupts(&interrupts_enabled);
#endif

    evar_device__sending_pin_off();
    return mq_result;
}

/*
 * Receives the first message from the current task's queue. The received message is removed from the queue.
 */
evar_mq_result_t evar__receive_message(
    void* p_message,
    evar_message_size_t message_size
) {

    evar_message_queue_t* p_message_queue;
    evar_mq_args_t pop_args;
    evar_mq_result_t mq_result;

    evar_device__receiving_pin_on();

    evar_assert(VALID(evar_current_task));
    evar_assert(DETACHED(evar_current_task));

    p_message_queue = P_MESSAGE_QUEUE(evar_current_task);
    if (p_message_queue == NULL) {
        evar_device__receiving_pin_off();
        return EVAR_MQ_INVALID_QUEUE;
    }

    pop_args.p_message_store = P_MESSAGE_STORE(evar_current_task);
    pop_args.p_message       = p_message;
    pop_args.message_size    = message_size;

    evar_device__disable_interrupts();
    mq_result = p_message_queue->pop(&pop_args);
    evar_device__enable_interrupts();

    evar_device__receiving_pin_off();
    return mq_result;
}

/*
 * Returns a copy of the first message for the current task, but does not remove it from the queue.
 */
evar_mq_result_t evar__preview_message(
    void* p_message,
    evar_message_size_t message_size
) {

    evar_message_queue_t* p_message_queue;
    evar_mq_args_t peek_args;
    evar_mq_result_t mq_result;

    evar_device__receiving_pin_on();

    evar_assert(VALID(evar_current_task));
    evar_assert(DETACHED(evar_current_task));

    p_message_queue = P_MESSAGE_QUEUE(evar_current_task);
    if (p_message_queue == NULL) {
        evar_device__receiving_pin_off();
        return EVAR_MQ_INVALID_QUEUE;
    }

    peek_args.p_message_store = P_MESSAGE_STORE(evar_current_task);
    peek_args.p_message       = p_message;
    peek_args.message_size    = message_size;

    evar_device__disable_interrupts();
    mq_result = p_message_queue->peek(&peek_args);
    evar_device__enable_interrupts();

    evar_device__receiving_pin_off();
    return mq_result;
}

/*
 * Advances the base timestamp when the internal clock is about to be truncated.
 */
static void advance_base_timestamp(evar_clock_ticks_t advanced_ticks) {

    unsigned long advanced_usec;

    advanced_usec = EVAR_TICKS_TO_USEC(advanced_ticks);

    evar_base_timestamp.low += advanced_usec;
    if (evar_base_timestamp.low < advanced_usec) {
        evar_base_timestamp.high += 1;
    }
}

/*
 * Updates the current global timestamp for the duration of the pass.
 */
static void update_current_timestamp(void) {

    unsigned long clock_usec;
    _evar_timestamp_t* p_current_timestamp;

    clock_usec = EVAR_TICKS_TO_USEC(evar_clock_ticks);
    p_current_timestamp = (_evar_timestamp_t*)&evar_current_timestamp;

    *p_current_timestamp = evar_base_timestamp;
    p_current_timestamp->low += clock_usec;
    if (p_current_timestamp->low < clock_usec) {
        p_current_timestamp->high += 1;
    }
}

/*
 * Returns the *signed* time delta between two timestamps in microseconds. The maximum
 * value is about +-35 minutes, after that the maximum signed value will be returned.
 */
evar_time_delta_t evar__get_time_delta(evar_timestamp_t* p_timestamp1, evar_timestamp_t* p_timestamp2) {

    _evar_timestamp_t* p_ts1;
    _evar_timestamp_t* p_ts2;
    unsigned long diff;

    evar_assert(p_timestamp1 != NULL);
    evar_assert(p_timestamp2 != NULL);

    p_ts1 = (_evar_timestamp_t*)p_timestamp1;
    p_ts2 = (_evar_timestamp_t*)p_timestamp2;

    if (p_ts1->high < p_ts2->high) { // ts1 < ts2

        if ((p_ts2->high - p_ts1->high) > 1) {
            return EVAR_MAX_POSITIVE_TIME_DELTA;
        }
        else if ((p_ts1->low & 0x80000000) == 0 || (p_ts2->low & 0x80000000) != 0) {
            return EVAR_MAX_POSITIVE_TIME_DELTA;
        }

        diff = (0xFFFFFFFF - p_ts1->low) + p_ts2->low + 1;
        if ((diff & 0x80000000) == 0) {
            return diff;
        }
        else {
            return EVAR_MAX_POSITIVE_TIME_DELTA;
        }
    }
    else if (p_ts1->high > p_ts2->high) { // ts1 > ts2

        if ((p_ts1->high - p_ts2->high) > 1) {
            return EVAR_MAX_NEGATIVE_TIME_DELTA;
        }
        else if ((p_ts2->low & 0x80000000) == 0 || (p_ts1->low & 0x80000000) != 0) {
            return EVAR_MAX_NEGATIVE_TIME_DELTA;
        }

        diff = (0xFFFFFFFF - p_ts2->low) + p_ts1->low + 1;
        if ((diff & 0x80000000) == 0) {
            return -diff;
        }
        else {
            return EVAR_MAX_NEGATIVE_TIME_DELTA;
        }
    }
    else if (p_ts1->low < p_ts2->low) { // ts1 < ts2

        diff = p_ts2->low - p_ts1->low;
        if ((diff & 0x80000000) == 0) {
            return diff;
        }
        else {
            return EVAR_MAX_POSITIVE_TIME_DELTA;
        }
    }
    else if (p_ts1->low > p_ts2->low) { // ts1 > ts2

        diff = p_ts1->low - p_ts2->low;
        if ((diff & 0x80000000) == 0) {
            return -diff;
        }
        else {
            return EVAR_MAX_NEGATIVE_TIME_DELTA;
        }
    }
    else {
        return 0;
    }
}

/*
 * Returns id of a given active task, the first instance of it,
 * if there are multiple instances of the same task.
 */
evar_task_id_t evar__get_task_id(evar_task_t* p_task) {

    evar_task_id_t task;

    for (task = 0; task < (EVAR_MAX_TASKS); ++task) {
        if (ACTIVE(task) && P_TASK(task) == p_task) {
            return task;
        }
    }

    return EVAR_INVALID_TASK_ID;
}

/*
 * Returns 1 if the task's message queue has messages in it,
 * 0 if the queue is empty, or is missing altogether.
 */
static unsigned char task_has_messages(evar_task_id_t task) {

    evar_message_queue_t* p_message_queue;
    evar_mq_args_t peek_args;
    evar_mq_result_t mq_result;

    p_message_queue = P_MESSAGE_QUEUE(task);
    if (p_message_queue == NULL) {
        return 0;
    }

    peek_args.p_message_store = P_MESSAGE_STORE(task);
    peek_args.p_message = NULL;
    peek_args.message_size = 0;

    evar_device__disable_interrupts();
    mq_result = p_message_queue->peek(&peek_args);
    evar_device__enable_interrupts();

    evar_assert(mq_result == EVAR_MQ_SUCCESS || mq_result == EVAR_MQ_QUEUE_EMPTY);
    return mq_result == EVAR_MQ_SUCCESS ? 1 : 0;
}

/*
 * Called at return from one of the task execute functions to request task
 * termination. The task is cleaned up and returned to the available list.
 * The same effect would be achieved by simply returning without calling any
 * of the evar_task__... before.
 */
void evar_task__exit(void) {

    evar_assert(VALID(evar_current_task));
    evar_assert(DETACHED(evar_current_task));

    // this catches the situation when a task has not been properly initialized and
    // is being put back to the available list, there is no need to call the cleanup

    if (LIST(evar_current_task) != AVAILABLE) {
        P_TASK(evar_current_task)->cleanup(P_TASK_INFO(evar_current_task));
    }

    append_task(evar_current_task, AVAILABLE);

    evar_assert(evar_active_tasks > 0);
    evar_active_tasks -= 1;
}

/*
 * Called at return from the task execute function to indicate that
 * the task should be kept on running list.
 * The task will be put on either running list, or received list,
 * if there are messages currently in its queue.
 */
void evar_task__keep_running(void) {

    evar_assert(VALID(evar_current_task));
    evar_assert(DETACHED(evar_current_task));

    append_task(evar_current_task, task_has_messages(evar_current_task) ? RECEIVED : RUNNING);
}

/*
 * Called at return from the task execute function to request being put
 * to sleep until a message becomes available in the queue, which the task
 * should better have, otherwise it will end up sleeping forever.
 * The task will be put on either sleeping list, or received list,
 * if there are messages currently in its queue.
 */
void evar_task__sleep(void) {

    evar_assert(VALID(evar_current_task));
    evar_assert(DETACHED(evar_current_task));

    WAKE_UP_AT(evar_current_task) = EVAR_INVALID_CLOCK_TICKS;

    append_task(evar_current_task, task_has_messages(evar_current_task) ? RECEIVED : SLEEPING);
}

/*
 * Inserts the current task at the correct position in the sleeping list,
 * so that the list maintains its order by time to wake up.
 * The task will be put on either sleeping list, or received list,
 * if there already are currently messages in its queue.
 */
static void task_sleep_until(evar_clock_ticks_t wake_up_at) {

    evar_task_id_t sleeping_head;
    evar_task_id_t sleeping_task;

    evar_assert(wake_up_at != EVAR_INVALID_CLOCK_TICKS);

    evar_assert(VALID(evar_current_task));
    evar_assert(DETACHED(evar_current_task));

    WAKE_UP_AT(evar_current_task) = wake_up_at;

    // if the task has a message queue and there are messages in it,
    // the task will get onto the received list and wake up immediately

    if (task_has_messages(evar_current_task)) {
        append_task(evar_current_task, RECEIVED);
        return;
    }

    // otherwise we insert it in its proper place in the list of sleeping tasks

    sleeping_head = HEAD(SLEEPING);
    if (!VALID(sleeping_head)) {
        append_task(evar_current_task, SLEEPING);
        return;
    }

    sleeping_task = sleeping_head;
    do {
        if (wake_up_at < WAKE_UP_AT(sleeping_task)) {
            insert_task(evar_current_task, sleeping_task);
            return;
        }
        sleeping_task = NEXT(sleeping_task);
    }
    while (sleeping_task != sleeping_head);

    append_task(evar_current_task, SLEEPING);
}

/*
 * Puts the running task to sleep until the given number of microseconds
 * elapses starting now.
 */
void evar_task__sleep_for(evar_interval_t usec) {

    evar_assert(VALID(evar_current_task));
    evar_assert(DETACHED(evar_current_task));

    task_sleep_until(evar_clock_ticks + EVAR_USEC_TO_TICKS(usec));
}

/*
 * Puts the running task to sleep until the given number of microseconds elapses
 * since the last time it was put to sleep using this function. The way it works
 * is that the task may issue multiple calls to this function and before the requested
 * interval elapses, the sleeping time will not be extended, but the moment the next
 * interval begins, it will be. Note that in between these calls, the task might have
 * already been running and sleeping, does not affect its capacity to use it again.
 */
void evar_task__sleep_next(evar_interval_t usec) {

    evar_clock_ticks_t next_up_at;

    evar_assert(VALID(evar_current_task));
    evar_assert(DETACHED(evar_current_task));

    next_up_at = NEXT_UP_AT(evar_current_task);

    if (next_up_at == EVAR_INVALID_CLOCK_TICKS) { // the first time the task calls this function
        NEXT_UP_AT(evar_current_task) = evar_clock_ticks + EVAR_USEC_TO_TICKS(usec);
    }
    else if (next_up_at <= evar_clock_ticks) { // the interval has ended
        NEXT_UP_AT(evar_current_task) = next_up_at + EVAR_USEC_TO_TICKS(usec);
    }
    // else we are within the previous interval and are still waiting for it to end

    task_sleep_until(NEXT_UP_AT(evar_current_task));
}

/*
 * Picks the task that should run now, removes it from the list it is
 * currently on and returns the id of the task. The task becomes detached
 * and must be put on the appropriate list after execution.
 */
static evar_task_id_t pick_task_to_execute(void) {

    unsigned char  round;
    evar_task_id_t received_head;
    evar_task_id_t received_task;
    evar_task_id_t sleeping_head;
    evar_task_id_t sleeping_task;
    evar_task_id_t running_head;
    evar_task_id_t running_task;

    // we need to pick one task to execute from the three lists that
    // we have - running, sleeping or received
    // the tasks on different lists have different priorities, accounted in the form of quanta,
    // charged to each task every time it is picked for execution off this or that list

#define PICK_TASK(task, quanta) ((evar_task_quanta[task] + quanta <= EVAR_QUANTA_PER_ROUND) ? (evar_task_quanta[task] += quanta) : 0)

    // this will make at most two passes, first with the current quanta values,
    // and if no task could be picked, the quanta will be zeroed and we try again

    for (round = 0; round < 2; ++round) {

        // the highest priority have tasks that have received messages,
        // note that the actual messages may have already been consumed,
        // for example if a task was executing, receiving messages
        // in a loop, and received an asynchronous message, it will be
        // put on the received list by flush_received_async but
        // have no actual messages in the queue when it is awakened

        received_head = HEAD(RECEIVED);

        if (VALID(received_head)) {
            received_task = received_head;
            do {
                if (PICK_TASK(received_task, EVAR_RECEIVED_QUANTA)) {
                    remove_task(received_task);
                    return received_task;
                }
                received_task = NEXT(received_task);
            }
            while (received_task != received_head);
        }

        // the next priority goes to sleeping tasks ready to wake up

        sleeping_head = HEAD(SLEEPING);
        if (VALID(sleeping_head)) {
            sleeping_task = sleeping_head;
            do {
                if (WAKE_UP_AT(sleeping_task) > evar_clock_ticks) {
                    break;
                }
                if (PICK_TASK(sleeping_task, EVAR_SLEEPING_QUANTA)) {
                    remove_task(sleeping_task);
                    return sleeping_task;
                }
                sleeping_task = NEXT(sleeping_task);
            }
            while (sleeping_task != sleeping_head);
        }

        // finally, we pick a running task

        running_head = HEAD(RUNNING);
        if (VALID(running_head)) {
            running_task = running_head;
            do {
                if (PICK_TASK(running_task, EVAR_RUNNING_QUANTA)) {
                    remove_task(running_task);
                    return running_task;
                }
                running_task = NEXT(running_task);
            }
            while (running_task != running_head);
        }

        // if we did not find a task to execute, clear the charged quanta and make another pass

        memset(evar_task_quanta, 0, sizeof(evar_task_quanta));
    }

#undef PICK_TASK

    return EVAR_INVALID_TASK_ID;
}

/*
 * Goes through the tasks on a given list and subtracts truncated ticks from their time-related fields.
 */
static void truncate_task_list_clocks(task_list_t task_list, evar_clock_ticks_t truncated_ticks) {

    evar_task_id_t head;
    evar_task_id_t task;
    evar_clock_ticks_t wake_up_at;
    evar_clock_ticks_t next_up_at;

    head = HEAD(task_list);
    if (VALID(head)) {
        task = head;
        do {

            // the next_up_at field contains the time in the future, when a task
            // that at some point in the past was put to sleep using evar_task__sleep_next,
            // expects to wake up next, and that value remains relevant across task's
            // changes of state, so the task may be on any list now

            next_up_at = NEXT_UP_AT(task);
            if (next_up_at != EVAR_INVALID_CLOCK_TICKS) {
                if (next_up_at >= truncated_ticks) {
                    NEXT_UP_AT(task) = next_up_at - truncated_ticks;
                } else {
                    NEXT_UP_AT(task) = 0;
                }
            }

            // wake_up_at is similarly containing a moment in the future when
            // a task is supposed to wake up, but it is only relevant for tasks
            // that are currently on sleeping list

            if (task_list == SLEEPING) {
                wake_up_at = WAKE_UP_AT(task);
                if (wake_up_at != EVAR_INVALID_CLOCK_TICKS) {
                    if (wake_up_at >= truncated_ticks) {
                        WAKE_UP_AT(task) = wake_up_at - truncated_ticks;
                    } else {
                        WAKE_UP_AT(task) = 0;
                    }
                }
            }

            task = NEXT(task);
        }
        while (task != head);
    }
}

/*
 * Goes through the all active lists of tasks and subtracts
 * truncated ticks from their clock fields.
 */
static void truncate_stored_clocks(evar_clock_ticks_t truncated_ticks) {
    truncate_task_list_clocks(RUNNING,  truncated_ticks);
    truncate_task_list_clocks(SLEEPING, truncated_ticks);
    truncate_task_list_clocks(RECEIVED, truncated_ticks);
}

/*
 * This function will use timer ticks elapsed since its previous invocation
 * to update the internal clock. It must be called at least once in the time
 * it takes the timer tick counter to overflow, otherwise the clock will become
 * inconsistent.
 * If there is a clock overflow happening "soon", all the references
 * to absolute moments in time that we have stored are corrected,
 * the global timestamp is updated and the internal clock is reset.
 */
static void update_internal_clock(evar_timer_ticks_t timer_ticks) {

    evar_timer_ticks_t ticks_elapsed;

    // get ticks elapsed since the last call to this function

    if (evar_prev_timer_ticks <= timer_ticks) {
        ticks_elapsed = timer_ticks - evar_prev_timer_ticks;
    }
    else { // timer tick counter rolled over and passed zero
        ticks_elapsed = ((evar_timer_ticks_t)-1) - (evar_prev_timer_ticks - timer_ticks - 1);
    }
    evar_prev_timer_ticks = timer_ticks;

    if (EVAR_MAX_CLOCK_TICKS - evar_clock_ticks <= ticks_elapsed) { // we have reached the end of the internal clock range
        truncate_stored_clocks(evar_clock_ticks);
        advance_base_timestamp(evar_clock_ticks);
        evar_clock_ticks = 0;
    }

    evar_clock_ticks += ticks_elapsed;
    evar_assert(evar_clock_ticks < EVAR_MAX_CLOCK_TICKS);

    update_current_timestamp();
}

/*
 * A single pass of the scheduler, picking and running one task and then returning.
 */
static evar_loop_result_t schedule_task(evar_timer_ticks_t timer_ticks) {

    evar_task_t* p_task;
    evar_task_info_t* p_task_info;

    if (evar_active_tasks == 0) {
        return EVAR_LOOP_HALTED;
    }

    // every scheduler pass begins with some housekeeping

    // update the current clock from the provided timer ticks, this is done once per scheduler pass

    update_internal_clock(timer_ticks);

    // move the tasks that have received asynchronous messages to the received list

    evar_device__disable_interrupts();
    flush_received_async();
    evar_device__enable_interrupts();

    // here the decision is made which task to execute next

    evar_current_task = pick_task_to_execute();
    if (!VALID(evar_current_task)) {
        return EVAR_LOOP_IDLE;
    }
    evar_assert(DETACHED(evar_current_task));

    p_task = P_TASK(evar_current_task);
    p_task_info = P_TASK_INFO(evar_current_task);

    // the task has been detached from the list it was picked from,
    // but the reference to that list still remains and is used
    // to switch between the different task code entries

    evar_device__running_pin_on();

    switch (LIST(evar_current_task)) {
        case RECEIVED:
            p_task->receive(p_task_info);
            break;
        case SLEEPING:
            p_task->wake_up(p_task_info);
            break;
        case RUNNING:
            p_task->run(p_task_info);
            break;
        default: // should not happen
            evar_assert(0);
            break;
    }

    evar_device__running_pin_off();

    // the task should have used one of the evar_task__... calls to indicate
    // how it wishes to continue its execution, and each of those calls would
    // have put the task onto one list or another, and if at this moment
    // the task is still detached, means that it simply returned, we treat
    // this as an indication that it needs to terminate

    if (DETACHED(evar_current_task)) {
        evar_task__exit();
    }

    return EVAR_LOOP_ACTIVE;
}

/*
 * The pair of functions evar__setup/evar__loop exist to support the Arduino-like environments
 * where the SDK provides the ticking time source and expects the setup/loop sequence.
 * Note that the device-specific function evar_device__get_timer_ticks must be implemented
 * to return the SDK's get_millis or something like it.
 */
void evar__setup(evar_task_t* p_main_task, void* p_main_task_data) {
    evar__initialize();
    evar__create_task(p_main_task, p_main_task_data);
}

/*
 * What this implementation lacks compared to evar__run, where a hardware timer
 * is dedicated to maintaining the scheduler, is the ability to put the CPU to sleep.
 */
evar_loop_result_t evar__loop(void) {
    unsigned char loop_result;
    loop_result = schedule_task(evar_device__get_timer_ticks());
    switch (loop_result) {
        case EVAR_LOOP_IDLE:
            // this is where we could potentially put the CPU to sleep,
            // but the outside environment would be surprised by that
        case EVAR_LOOP_ACTIVE:
            break;
        default:
            evar__halt();
    }
    return loop_result; // the caller still might do it from this result
}

/*
 * This function combines the setup/loop into a single never returning call.
 * It creates the main task, keeps running the scheduler until all the tasks
 * have exited, after which the device is halted. It requires a dedicated
 * interrupt-based timer to tick behind the scenes.
 */
void evar__run(evar_task_t* p_main_task, void* p_main_task_data) {
    evar__setup(p_main_task, p_main_task_data);
    while (1) {
        switch (schedule_task(evar_device__get_timer_ticks())) {
            case EVAR_LOOP_IDLE:
                evar_device__idle_pin_on();
                evar_device__cpu_idle(); // put the CPU to sleep until the next interrupt
                evar_device__idle_pin_off();
            case EVAR_LOOP_ACTIVE:
                continue; // for
            default:
                evar__halt();
        }
    }
}
