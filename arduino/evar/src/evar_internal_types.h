#ifndef EVAR_INTERNAL_TYPES_H
#define EVAR_INTERNAL_TYPES_H

#include <evar_types.h>
#include <evar_config.h>

EVAR_ASSERT((EVAR_MAX_TASKS) > 0, min_evar_max_tasks);
EVAR_ASSERT((EVAR_MAX_TASKS) <= EVAR_INVALID_TASK_ID, max_evar_max_tasks);

/*
 * Perpetual timestamp in microseconds, internal structure.
 */
typedef struct {
    unsigned long high;
    unsigned long low;
} _evar_timestamp_t;
EVAR_ASSERT(sizeof(_evar_timestamp_t) == sizeof(evar_timestamp_t), sizeof_evar_timestamp_t);

/*
 * Internal clock in hardware-specific ticks, based entirely on the timer frequency.
 * One unit here is exactly one timer tick, assumption being that timer frequency
 * is significantly less than 1MHz, practically in tens KHz at most.
 */
typedef unsigned long _evar_clock_ticks_t;
EVAR_ASSERT(sizeof(_evar_clock_ticks_t) == 4, sizeof__evar_clock_ticks_t);
#define EVAR_INVALID_CLOCK_TICKS (ULONG_MAX)
#ifndef EVAR_DEBUG
// the maximum number of ticks that can still be converted to milliseconds
#define EVAR_MAX_CLOCK_TICKS (EVAR_USEC_TO_TICKS(ULONG_MAX) / 2)
#else
// in debugging, this is reduced to 10 seconds to cause frequent clock truncation
#define EVAR_MAX_CLOCK_TICKS (EVAR_USEC_TO_TICKS(10000000))
#endif
// this check implies that a full interval of microseconds could still be accounted for
EVAR_ASSERT(EVAR_MAX_CLOCK_TICKS <= (EVAR_USEC_TO_TICKS(ULONG_MAX) / 2), max_clock_ticks);

/*
 * The task list identifier.
 */
typedef unsigned char _evar_task_list_t;

#define AVAILABLE_LIST  (0)
#define RUNNING_LIST    (1)
#define SLEEPING_LIST   (2)
#define RECEIVED_LIST   (3)
#define TASK_LIST_COUNT (4)

/*
 * This structure conceptually belongs within evar_task_t, and here extracted are
 * the fields that are most frequently accessed, so that the _evar_task_entries array
 * containing them can be placed in some kind of device-specific near memory.
 */
typedef struct {
    evar_task_id_t      prev_task;     // link to the previous element in the list the task is on
    evar_task_id_t      next_task;     // link to the next element in the list the task is on
    _evar_clock_ticks_t wake_up_at;    // the moment in the future, when a sleeping task expects to wake up
    _evar_clock_ticks_t next_up_at;    // the end of the last interval set by evar_task__sleep_next
    _evar_task_list_t   task_list;     // enum value in range 0-3, the list the task is on, or was last on
    unsigned char       detached;      // single bit, non-zero if the task is not on any list (currently executing)
} _evar_task_entry_t;

/*
 * Information specific to *an instance* of a task.
 */
typedef struct {
    evar_task_t*     p_task;    // pointer to the task "class" structure
    evar_task_info_t task_info; // instance-specific data for the task
} _evar_task_struct_t;

/*
 * The following structure registers tasks that have received asynchronous
 * messages from interrupt handlers. It is effectively a dedicated mini-queue,
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
} _evar_received_async_queue_t;

/*
 * Abstract message store in a form of a fixed-size circular buffer.
 */
typedef struct {
    evar_message_count_t capacity;            // maximum number of messages
    evar_message_size_t  message_size;        // fixed size of each message
    unsigned char*       p_message_buffer;    // opaque array of messages, to be re-cast for each task individually
    unsigned long        message_buffer_size; // capacity multiplied by message_size, byte size of the buffer pointed by p_messages
    evar_message_count_t count;               // current number of messages
    unsigned long        head;                // offset to the first actual message in the array
    unsigned long        tail;                // offset to the first empty slot in the array
} _evar_message_store_t;

EVAR_ASSERT(sizeof(_evar_message_store_t) <= (EVAR_MESSAGE_STORE_SIZE), sizeof_evar_message_store_t);

#endif
