/*
 * This header is supposed to be included in every task implementation.
 * It generates the task structures and declares the task methods which
 * are left for the source file to implement.
 */

#ifndef EVAR_TASK_H
#define EVAR_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <evar_types.h>
#include <evar_device.h>
#include <evar_assert.h>

#ifdef EVAR_TASK_NAME

/*
 * Include the header for the task being implemented,
 * it contains the task data and message structures.
 */
#include EVAR_QUOTE(EVAR_TASK_NAME.h)

/*
 * To start a new task.
 */
evar_task_id_t evar__create_task(evar_task_t* p_task, void* p_task_data);

/*
 * Sends a message from the current running task to another task.
 * Interrupt handlers and possibly other asynchronous sources
 * should use evar__send_async_message.
 */
evar_mq_result_t evar__send_message(evar_task_id_t receiver, void* p_message, evar_message_size_t message_size);

/*
 * For a task to be able to receive messages EVAR_TASK_MESSAGE_COUNT variable
 * must be defined set to the maximum number of messages in its queue.
 */

#if defined(EVAR_TASK_MESSAGE_COUNT) && ((EVAR_TASK_MESSAGE_COUNT) > 0)

/*
 * If the task declares that it has a message queue, it must
 * specify their structure in {task_name}_message_t.
 */
EVAR_ASSERT(sizeof(EVAR_CONCAT(EVAR_TASK_NAME, _message_t)) > 0, sizeof_task_message);

/*
 * Internal structure of the message store is invisible here.
 * The task allocates it as {task_name}_message_store_t.
 */

typedef struct {
    unsigned char opaque[(EVAR_MESSAGE_STORE_SIZE) + (EVAR_TASK_MESSAGE_COUNT) * sizeof(EVAR_CONCAT(EVAR_TASK_NAME, _message_t))];
} EVAR_CONCAT(EVAR_TASK_NAME, _message_store_t);
        
EVAR_ASSERT(
    sizeof(EVAR_CONCAT(EVAR_TASK_NAME, _message_store_t)) == 
    (EVAR_MESSAGE_STORE_SIZE) + (EVAR_TASK_MESSAGE_COUNT) * sizeof(EVAR_CONCAT(EVAR_TASK_NAME, _message_t)), 
    sizeof_task_message_store_t
);

/*
 * Private visibility, initializes the message store in place.
 */
void _evar__initialize_message_store(void* p_message_store);

/*
 * This function is supposed to be called from {task_name}__initialize 
 * to initialize the task instance's message store. The buffer must be
 * static, global or allocated, not on stack.
 */
static void evar__initialize_message_store(evar_message_store_t* p_message_store) {
    _evar__initialize_message_store(p_message_store);
}

void EVAR_CONCAT(evar__initialize_message_store_, EVAR_TASK_NAME)(void) {
    (void)evar__initialize_message_store; // mark as used
}

/*
 * Private visibility, returns the first message from the current task's queue.
 * The received message is removed from the queue.
 */
evar_mq_result_t _evar__receive_message(void* p_message, evar_message_size_t message_size);

/*
 * This task-specific implementation restricts the type of the message to be received.
 */
static evar_mq_result_t evar__receive_message(EVAR_CONCAT(EVAR_TASK_NAME, _message_t)* p_message) {
    return _evar__receive_message(p_message, sizeof(EVAR_CONCAT(EVAR_TASK_NAME, _message_t)));
}

void EVAR_CONCAT(evar__receive_message_, EVAR_TASK_NAME)(void) {
    (void)evar__receive_message; // mark as used
}

/*
 * Private visibility, returns a copy of the first message for the current task,
 * but does not remove it from the queue.
 */
evar_mq_result_t _evar__preview_message(void* p_message, evar_message_size_t message_size);

/*
 * This task-specific implementation restricts the type of the message to be previewed.
 */
static evar_mq_result_t evar__preview_message(EVAR_CONCAT(EVAR_TASK_NAME, _message_t)* p_message) {
    return _evar__preview_message(p_message, sizeof(EVAR_CONCAT(EVAR_TASK_NAME, _message_t)));
}

void EVAR_CONCAT(evar__preview_message_, EVAR_TASK_NAME)(void) {
    (void)evar__preview_message; // mark as used
}

#endif

/*
 * The following declarations lay out the task interface, the definitions
 * will have to be provided in the task's source file which includes this.
 */

/*
 * Initializes the task after it has been created but before it runs.
 * In its initialization a task can create other tasks, which is a form
 * of indirect recursion.
 */
static void EVAR_CONCAT(EVAR_TASK_NAME, __initialize)(evar_task_info_t* p_task_info);

/*
 * One of the following three task's functions will be invoked by the scheduler,
 * which one exactly depending on what was the reason for picking that task.
 */

/*
 * Executed when the task was running continously using evar_task__keep_running().
 */
static void EVAR_CONCAT(EVAR_TASK_NAME, __run)(evar_task_info_t* p_task_info);

/*
 * Executed when the task was sleeping using either evar_task__sleep_for()
 * or evar_task__sleep_next() and its sleep interval expired.
 */
static void EVAR_CONCAT(EVAR_TASK_NAME, __wake_up)(evar_task_info_t* p_task_info);

/*
 * Executed when messages had appeared in the task's message queue
 * (even though it was running or sleeping). Receiving a message wakes
 * the task up immediately and unconditionally.
 */
static void EVAR_CONCAT(EVAR_TASK_NAME, __receive)(evar_task_info_t* p_task_info);

/*
 * Is called to clean up after the task returns with evar_task__exit().
 */
static void EVAR_CONCAT(EVAR_TASK_NAME, __cleanup)(evar_task_info_t* p_task_info);

/*
 * This is a class-like structure with methods and static members.
 */
static evar_task_t EVAR_CONCAT(_, EVAR_TASK_NAME) = {
    
#if defined(EVAR_TASK_MESSAGE_COUNT) && ((EVAR_TASK_MESSAGE_COUNT) > 0)
    sizeof(EVAR_CONCAT(EVAR_TASK_NAME, _message_t)),
    (EVAR_TASK_MESSAGE_COUNT),
#else
    0,
    0,
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
 * As with the other cases, if there is a message in the queue, the task will
 * wake immediately.
 */
void evar_task__sleep_next(evar_interval_t usec);

#endif

/*
 * The rest of the functions are not task-specific and can be included
 * in source files that are not implementing tasks themselves.
 */

/*
 * Sends an asynchronous message from an interrupt handler to a task.
 * This function should only be used by interrupt handlers and possibly
 * other asynchronous sources. The tasks should use evar__send_message.
 * This can be executed with or without interrupts disabled. Interrupts
 * will be disabled for the push operation, and the previous state of
 * interrupts will be restored at exit.
 * Note that this is a possible cause of re-entering, if another interrupt
 * happens after execution of this function starts, and another handler
 * also needs to send a message. If re-entrancy is a problem, as with
 * compiled stack, interrupts must be disabled before calling this.
 */
evar_mq_result_t evar__send_async_message(
    evar_task_id_t receiver,
    void* p_message,
    evar_message_size_t message_size
);

/*
 * Returns the current absolute timestamp in microseconds. The returned
 * value is opaque and is only useful with evar__get_time_delta below.
 */
void evar__get_current_timestamp(evar_timestamp_t* p_timestamp);

/*
 * Returns the *signed* time delta between two timestamps in microseconds. The maximum
 * value is about +-35 minutes, after that the maximum signed value will be returned.
 */
evar_time_delta_t evar__get_time_delta(evar_timestamp_t* p_timestamp1, evar_timestamp_t* p_timestamp2);

/*
 * Displays a debugging code/message, then halts.
 */
void evar__crash(unsigned short error, char* message);

/*
 * Immediate device shutdown, hardware halt.
 */
void evar__halt(void);

#ifdef __cplusplus
}
#endif

#endif
