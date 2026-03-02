#ifndef EVAR_H
#define EVAR_H

#include <evar_types.h>
#include <evar_device.h>
#include <evar_assert.h>

/*
 * Initializes the device and prepares the framework structures.
 */
void evar__initialize(void);

/*
 * Called once at application startup, creates the first task
 * and keeps running the scheduler until all the tasks have exited.
 * After that the device is halted.
 */
void evar__run(evar_task_t* p_main_task, void* p_main_task_data);

/*
 * To start a new task.
 */
evar_task_id_t evar__create_task(evar_task_t* p_task, evar_task_data_t* p_task_data);

/*
 * Combines evar__initialize and evar__create_task in one,
 * supposedly called from the Arduino-like setup function.
 */
void evar__setup(evar_task_t* p_main_task, void* p_main_task_data);

/*
 * A single pass of the scheduler, picking and running one task and then returning.
 * This is similar to what evar__run runs in an infinite loop. It can be used directly,
 * instead of evar__run, when the outside environment is Arduino-like and expects
 * application construction around setup/loop pair.
 * Returns:
 * EVAR_LOOP_HALTED : All tasks have exited and there is no more active tasks, this is a halting condition.
 * EVAR_LOOP_ACTIVE : A task was picked and allowed to execute, looping must continue immediately.
 * EVAR_LOOP_IDLE   : There are active tasks, but none was immediately runnable,
 *                    the CPU may be put to idle state to wait for the timer interrupt.
 */

typedef unsigned char evar_loop_result_t;
evar_loop_result_t evar__loop(void);

#define EVAR_LOOP_HALTED (0)
#define EVAR_LOOP_ACTIVE (1)
#define EVAR_LOOP_IDLE   (2)

/*
 * Displays a debugging code/message, then halts.
 */
void evar__crash(unsigned short error, char* message);

/*
 * Immediate device shutdown, hardware halt.
 */
void evar__halt(void);

/*
 * Syntax sugar to declare tasks by name.
 */
#define EVAR_TASK(task_name) extern evar_task_t* task_name

/*
 * To start a new task.
 */
evar_task_id_t evar__create_task(evar_task_t* p_task, evar_task_data_t* p_task_data);

/*
 * Sends a message from the current running task to another task.
 * Interrupt handlers and possibly other asynchronous sources
 * should use evar__send_async_message.
 */
evar_mq_result_t evar__send_message(evar_task_id_t receiver, void* p_message, evar_message_size_t message_size);

/*
 * Sends an asynchronous message from an interrupt handler to a task.
 * This function should only be used by interrupt handlers and possibly
 * other asynchronous sources. The tasks should use evar__send_message.
 * It can be executed with or without interrupts disabled. Interrupts
 * will be disabled for the push operation, and the previous state of
 * interrupts will be restored at exit.
 */
evar_mq_result_t evar__send_async_message(evar_task_id_t receiver, void* p_message, evar_message_size_t message_size);

/*
 * Receives the first message from the current task's queue. The received message is removed from the queue.
 */
evar_mq_result_t evar__receive_message(void* p_message, evar_message_size_t message_size);

/*
 * Returns a copy of the first message for the current task, but does not remove it from the queue.
 */
evar_mq_result_t evar__preview_message(void* p_message, evar_message_size_t message_size);

/*
 * Returns id of a given active task, the first instance of it,
 * if there are multiple instances of the same task.
 */
evar_task_id_t evar__get_task_id(evar_task_t* p_task);

/*
 * This is the global timestamp updated once per scheduler pass.
 */
extern evar_timestamp_t evar_current_timestamp;

/*
 * Returns the *signed* time delta between two timestamps in microseconds. The maximum 
 * value is about +-35 minutes, after that the maximum signed value will be returned.
 */
evar_time_delta_t evar__get_time_delta(evar_timestamp_t* p_timestamp1, evar_timestamp_t* p_timestamp2);

#endif
