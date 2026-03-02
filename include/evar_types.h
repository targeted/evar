#ifndef EVAR_TYPES_H
#define EVAR_TYPES_H

#include <stddef.h>
#include <evar_preproc.h>
#include <evar_config.h>
#include <evar_device_types.h>

// handle to a task
typedef unsigned char evar_task_id_t;
#define EVAR_INVALID_TASK_ID ((evar_task_id_t)-1)
EVAR_ASSERT((EVAR_MAX_TASKS) > 0, min_evar_max_tasks);
EVAR_ASSERT((EVAR_MAX_TASKS) <= EVAR_INVALID_TASK_ID, max_evar_max_tasks);

/*
 * Types related to time accounting.
 */

typedef struct { // perpetual timestamp in microseconds, internal structure
    unsigned long high;
    unsigned long low;
} _evar_timestamp_t;
EVAR_ASSERT(sizeof(_evar_timestamp_t) == 8, sizeof__evar_timestamp_t);

typedef struct { // same timestamp, reinterpreted as publicly visible opaque structure
    unsigned char opaque[sizeof(_evar_timestamp_t)];
} evar_timestamp_t;
EVAR_ASSERT(sizeof(evar_timestamp_t) == sizeof(_evar_timestamp_t), sizeof_evar_timestamp_t);

typedef unsigned long evar_interval_t; // *unsigned* time duration in microseconds, maximum being about 70 minutes
EVAR_ASSERT(sizeof(evar_interval_t) == 4, sizeof_evar_interval_t);

typedef long evar_time_delta_t; // *signed* difference between two timestamps in microseconds, maximum being about +-35 minutes
EVAR_ASSERT(sizeof(evar_time_delta_t) == 4, sizeof_evar_time_delta_t);
#define EVAR_MAX_POSITIVE_TIME_DELTA (2147483647L)
#define EVAR_MAX_NEGATIVE_TIME_DELTA (-2147483648L)

typedef unsigned long evar_clock_ticks_t; // internal clock in hardware-specific ticks
EVAR_ASSERT(sizeof(evar_clock_ticks_t) == 4, sizeof_evar_clock_ticks_t);
#define EVAR_INVALID_CLOCK_TICKS ((evar_clock_ticks_t)-1)
#ifndef EVAR_DEBUG
// the maximum number of ticks that can still be converted to milliseconds
#define EVAR_MAX_CLOCK_TICKS (EVAR_USEC_TO_TICKS((evar_clock_ticks_t)-1) / 2)
#else
// in debugging, this is reduced to 10 seconds to cause frequent timestamp truncation
#define EVAR_MAX_CLOCK_TICKS (EVAR_USEC_TO_TICKS(10000000))
#endif
// this check implies that a full interval of microseconds could still be accounted for
EVAR_ASSERT(EVAR_MAX_CLOCK_TICKS <= (EVAR_USEC_TO_TICKS((evar_clock_ticks_t)-1) / 2), max_clock_ticks);

/*
 * Types related to the message queues.
 */

// 64KB ought to be enough for anyone
typedef unsigned short evar_message_size_t;
EVAR_ASSERT(sizeof(evar_message_size_t) == 2, sizeof_evar_message_size_t);

// opaque anonymous type, to be re-cast for every task individually
typedef void evar_message_store_t;

// return codes for message queue methods
typedef unsigned char evar_mq_result_t;
#define EVAR_MQ_SUCCESS         (0)
#define EVAR_MQ_INVALID_QUEUE   (1)
#define EVAR_MQ_QUEUE_FULL      (2)
#define EVAR_MQ_QUEUE_EMPTY     (3)
#define EVAR_MQ_INVALID_MESSAGE (4)

/* 
 * Arg struct is a workaround for platforms where it is not
 * possible to pass multiple arguments to an indirect call.
 */
typedef struct {
    evar_message_store_t* p_message_store;
    void*                 p_message;
    evar_message_size_t   message_size;
} evar_mq_args_t;

// method table for the message queue interface
typedef struct {
    evar_mq_result_t (*push) (evar_mq_args_t* p_args);
    evar_mq_result_t (*pop)  (evar_mq_args_t* p_args);
    evar_mq_result_t (*peek) (evar_mq_args_t* p_args);
} evar_message_queue_t;

/*
 * Types related to task management.
 */

// opaque anonymous type, to be re-cast for every task individually
typedef void evar_task_data_t;

/*
 * This is what the task application code has access to.
 */
typedef struct {
    evar_task_id_t        current_task;     // id of this task itself
    evar_task_id_t        parent_task;      // id of the task that created it
    evar_task_data_t*     p_task_data;      // privately managed context for dynamically allocated data
    evar_message_store_t* p_message_store;  // the task's private buffer to hold its input messages
} evar_task_info_t;

/*
 * "Class" structure for the task.
 */
typedef struct {
    evar_message_queue_t* p_message_queue; // pointer to the message queue interface
    void (*initialize)    (evar_task_info_t* p_task_info);
    void (*run)           (evar_task_info_t* p_task_info);
    void (*wake_up)       (evar_task_info_t* p_task_info);
    void (*receive)       (evar_task_info_t* p_task_info);
    void (*cleanup)       (evar_task_info_t* p_task_info);
} evar_task_t;

/*
 * Information specific to *an instance* of a task.
 */
typedef struct {
    evar_task_t*     p_task;    // pointer to the task "class" structure
    evar_task_info_t task_info; // instance-specific data for the task
} evar_task_struct_t;

/*
 * The four task lists.
 */
typedef unsigned char task_list_t;
#define AVAILABLE (0)
#define RUNNING   (1)
#define SLEEPING  (2)
#define RECEIVED  (3)

/*
 * This structure conceptually belongs within evar_task_t above, and here extracted are
 * the fields that are most frequently accessed, so that the evar_task_entries array
 * containing them can be placed in some kind of device-specific near memory.
 */
typedef struct {
    evar_task_id_t     prev_task;      // link to the previous element in the list the task is on
    evar_task_id_t     next_task;      // link to the next element in the list the task is on
    evar_clock_ticks_t wake_up_at;     // the moment in the future, when a sleeping task expects to wake up
    evar_clock_ticks_t next_up_at;     // the end of the last interval set by evar_task__sleep_next
    unsigned char      task_list : 2;  // range 0-3, id of the list the task is on, or was last on
    unsigned char      detached  : 1;  // single bit, set if the task is not on any list (currently executing)
} evar_task_entry_t;

/*
 * The following structure registers tasks that have received asynchronous
 * messages from interrupt handlers. It is effectively a dedicated mini queue,
 * with a single entry per task. At every scheduler pass the tasks that are
 * listed here are moved to the received list. The structure is interlocked,
 * must be accessed with interrupts disabled.
 */
typedef struct {
    // id's of the tasks in order of having received their messages,
    // each task can only be mentioned here once, hence the fixed size
    evar_task_id_t tasks[EVAR_MAX_TASKS];
    // number of active elements in the above list, 0 to EVAR_MAX_TASKS inclusive
    evar_task_id_t count;
    // single bit per task to register the fact that a task is in the list
    unsigned char bitmap[((EVAR_MAX_TASKS) + 7) / 8];
} evar_received_async_t;

#endif
