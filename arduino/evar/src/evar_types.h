#ifndef EVAR_TYPES_H
#define EVAR_TYPES_H

#include <stddef.h>
#include <limits.h>
#include <evar_preproc.h>

/*
 * Publicly visible opaque for the timestamp.
 */
typedef struct {
    unsigned char opaque[8];
} evar_timestamp_t;

/*
 * Time duration (into the future) in microseconds, maximum being about 70 minutes.
 */
typedef unsigned long evar_interval_t;
EVAR_ASSERT(sizeof(evar_interval_t) == 4, sizeof_evar_interval_t);

/*
 * Signed difference between two timestamps in microseconds, maximum being about +-35 minutes.
 */
typedef long evar_time_delta_t;
EVAR_ASSERT(sizeof(evar_time_delta_t) == 4, sizeof_evar_time_delta_t);
#define EVAR_MAX_POSITIVE_TIME_DELTA (LONG_MAX)
#define EVAR_MAX_NEGATIVE_TIME_DELTA (LONG_MIN)

/*
 * Return codes for message queue methods.
 */
typedef unsigned char evar_mq_result_t;

#define EVAR_MQ_SUCCESS            (0)
#define EVAR_MQ_INVALID_QUEUE      (1)
#define EVAR_MQ_QUEUE_FULL         (2)
#define EVAR_MQ_QUEUE_EMPTY        (3)
#define EVAR_MQ_INVALID_PARAMETER  (4)

typedef unsigned short evar_message_size_t;
typedef unsigned short evar_message_count_t;

/*
 * We don't want for the message store structure to be public, only its size.
 */
typedef void evar_message_store_t;

#define _EVAR_MESSAGE_STORE_SIZE ( \
    sizeof(evar_message_count_t) + \
    sizeof(evar_message_size_t)  + \
    sizeof(unsigned char*)       + \
    sizeof(unsigned long)        + \
    sizeof(evar_message_count_t) + \
    sizeof(unsigned long)        + \
    sizeof(unsigned long)          \
)
#define EVAR_MESSAGE_STORE_SIZE ((((_EVAR_MESSAGE_STORE_SIZE) + 3) / 4) * 4)

/*
 * Handle to a task.
 */
typedef unsigned char evar_task_id_t;
#define EVAR_INVALID_TASK_ID (0xFF)

typedef void evar_task_data_t;

/*
 * Desriptor structure for the task, available to the task code.
 */
typedef struct {
    evar_task_id_t        current_task;    // id of this task itself
    evar_task_id_t        parent_task;     // id of the task that created it
    evar_task_data_t*     p_task_data;     // task instance specific data
    evar_message_store_t* p_message_store; // task instance message buffer
} evar_task_info_t;

/*
 * Publicly visible structure for a task. It is only made public
 * because its instance is generated in evar_task.h which has to be
 * included in the task code.
 */
typedef struct {
    void  (*initialize) (evar_task_info_t* p_task_info);
    void  (*run)        (evar_task_info_t* p_task_info);
    void  (*wake_up)    (evar_task_info_t* p_task_info);
    void  (*receive)    (evar_task_info_t* p_task_info);
    void  (*cleanup)    (evar_task_info_t* p_task_info);
} evar_task_t;

#endif
