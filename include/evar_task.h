#ifndef EVAR_TASK_H
#define EVAR_TASK_H

/*
 * This header is supposed to be included in every task implementation.
 * It generates the task structures, such as message queue and declares
 * the task methods which are left for the source file to implement.
 */

#ifndef EVAR_TASK_NAME
#error EVAR_TASK_NAME must be defined
#endif

#include <evar.h>
#include <evar_message_queue.h>

/*
 * The following declarations lay out the task interface, the definitions
 * will have to be provided in the task's source file which includes this.
 */

/*
 * Initializes the task before it is about to run for the first time.
 * In the task_info:
 * current_task    = actual, informational
 * parent_task     = actual, informational
 * p_task_data     = initialization parameter, could be reallocated and overwritten, the new value will linger
 * p_message_store = points to the default statically allocated buffer, good for one instance of the task,
 *                   or many instances if they choose to share the same queue as equal readers, could be
 *                   reallocated, the new value will linger
 */
static void EVAR_CONCAT(EVAR_TASK_NAME, __initialize)(evar_task_info_t* p_task_info);

/*
 * One of the following three functions will be invoked by the scheduler,
 * depending on what was the reason for picking that task:
 * 1. When the task had messages in its queue     -> {EVAR_TASK_NAME}__receive()
 * 2. When the sleep timeout expired for the task -> {EVAR_TASK_NAME}__wake_up()
 * 3. Otherwise, when the task was running        -> {EVAR_TASK_NAME}__run()
 */

/*
 * Executed when the task was running.
 * In the task_info:
 * current_task    = actual, informational
 * parent_task     = actual, informational
 * p_task_data     = actual, as returned from __initialize
 * p_message_store = actual, as returned from __initialize
 */
static void EVAR_CONCAT(EVAR_TASK_NAME, __run)(evar_task_info_t* p_task_info);

/*
 * Executed when the sleep interval elapses.
 * In the task_info:
 * current_task    = actual, informational
 * parent_task     = actual, informational
 * p_task_data     = actual, as returned from __initialize
 * p_message_store = actual, as returned from __initialize
 */
static void EVAR_CONCAT(EVAR_TASK_NAME, __wake_up)(evar_task_info_t* p_task_info);

/*
 * Executed when the task had messages in its queue.
 * In the task_info:
 * current_task    = actual, informational
 * parent_task     = actual, informational
 * p_task_data     = actual, as returned from __initialize
 * p_message_store = actual, as returned from __initialize
 */
static void EVAR_CONCAT(EVAR_TASK_NAME, __receive)(evar_task_info_t* p_task_info);

/*
 * Cleans up after the task has exited.
 * In the task_info:
 * current_task    = actual, informational
 * parent_task     = actual, informational
 * p_task_data     = actual, to be cleaned up, if returned from __initialize
 * p_message_store = actual, to be cleaned up, if returned from __initialize
 */
static void EVAR_CONCAT(EVAR_TASK_NAME, __cleanup)(evar_task_info_t* p_task_info);

/*
 * This is a class-like structure with methods and static members.
 */
static evar_task_t EVAR_CONCAT(_, EVAR_TASK_NAME) = {
#ifdef EVAR_TASK_HAS_MESSAGE_QUEUE
    &(EVAR_CONCAT(EVAR_TASK_NAME, _message_queue)),
#else
    NULL,
#endif
    EVAR_CONCAT(EVAR_TASK_NAME, __initialize),
    EVAR_CONCAT(EVAR_TASK_NAME, __run),
    EVAR_CONCAT(EVAR_TASK_NAME, __wake_up),
    EVAR_CONCAT(EVAR_TASK_NAME, __receive),
    EVAR_CONCAT(EVAR_TASK_NAME, __cleanup)
};

/*
 * This is the only non-static data element here that the framework requires
 * to reference this task, it is the same as the task name to be used simply as
 * EVAR_TASK(task_name);
 * evar__create_task(task_name, ...);
 */
evar_task_t* EVAR_TASK_NAME = &EVAR_CONCAT(_, EVAR_TASK_NAME);

/*
 * The following utilities are used when a task is yielding control back
 * to the scheduler, to signal the way it wants to be called again.
 * If the compiler supports that, it's better to call them like this:
 * return evar_task__exit();
 * otherwise:
 * evar_task__exit();
 * return;
 */

/*
 * return evar_task__exit();
 * Clean up and dispose of the task, never execute again.
 */
void evar_task__exit(void);

/*
 * return evar_task__keep_running();
 * Keep the task running, execute again at the earliest convenience.
 */
void evar_task__keep_running(void);

/*
 * return evar_task__sleep();
 * Wait for a message to appear in the task's message queue, execute again
 * when there is one (possibly immediately, if there already was one).
 */
void evar_task__sleep(void);

/*
 * return evar_task__sleep_for(N);
 * Put the task to sleep, execute again in N microseconds from *now*, or
 * when a message appears in the task's message queue (possibly immediately).
*/
void evar_task__sleep_for(evar_interval_t usec);

/*
 * return evar_task__sleep_next(N);
 * Put the task to sleep, execute again in N microseconds since the end
 * of the last such interval. As an example, if at moment 0 the task calls
 * evar_task__sleep_next(100), and then it gets a message at moment 10,
 * processes it, and calls evar_task__sleep_next(100) again, it will still
 * wake up at moment 100. The task can freely run, use other forms of sleep
 * and/or receive messages, the "sleep next" interval will still be respected
 * if a call to evar_task__sleep_next is made and the interval did not elapse.
 * But the moment the interval does elapse (e.g. at moment 101), the same
 * call evar_task__sleep_next(100) will put the task to sleep until moment 200.
 */
void evar_task__sleep_next(evar_interval_t usec);

#endif
