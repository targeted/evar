#ifndef EVAR_MESSAGE_QUEUE_H
#define EVAR_MESSAGE_QUEUE_H

/*
 * This header generates a minimal in-place implementation of a message queue,
 * controlled by the following compile-time variables:
 *
 * #define EVAR_TASK_MESSAGE_SIZE             (1)
 * #define EVAR_TASK_MESSAGE_COUNT            (10)
 * #define EVAR_TASK_MESSAGES_CAN_BE_DROPPED
 * #include <evar_task.h>
 *
 * Note that in case message size is zero, messages become "ephemeral" and 
 * the message queue is reduced to a fixed-size integer counter. Then the maximum
 * message count can be increased freely without increasing memory consumption.
 */

#include <evar_types.h>

/*
 * Defining any of the related variables before including this header 
 * will cause for a message queue to be instantiated.
 */
#if defined(EVAR_TASK_MESSAGE_SIZE) || defined(EVAR_TASK_MESSAGE_COUNT) || defined(EVAR_TASK_MESSAGES_CAN_BE_DROPPED)

EVAR_ASSERT((EVAR_TASK_MESSAGE_SIZE) >= 0, message_size_may_not_be_negative);
EVAR_ASSERT((EVAR_TASK_MESSAGE_COUNT) > 0, message_count_must_be_positive);

#if (EVAR_TASK_MESSAGE_SIZE) > 0
#include <string.h>
#endif

#define EVAR_TASK_HAS_MESSAGE_QUEUE

/*
 * Minimize the size of the message counter type.
 */
typedef
#if (EVAR_TASK_MESSAGE_COUNT) < 256
    unsigned char
#else
    unsigned short
#endif
_evar_message_count_t;

EVAR_ASSERT((EVAR_TASK_MESSAGE_COUNT) <= ((_evar_message_count_t)-1), max_message_count);
EVAR_ASSERT((EVAR_TASK_MESSAGE_SIZE) <= ((evar_message_size_t)-1), max_message_size);

#if (EVAR_TASK_MESSAGE_SIZE) > 0
typedef struct { unsigned char _[EVAR_TASK_MESSAGE_SIZE]; } _evar_message_t;
EVAR_ASSERT(sizeof(_evar_message_t) == (EVAR_TASK_MESSAGE_SIZE), sizeof__evar_message_t);
#endif

/*
 * A circular buffer structure with opaque fixed-size messages in it.
 * Note that when the messages are empty, this becomes just a counter.
 */
typedef struct {
    _evar_message_count_t count;
#if (EVAR_TASK_MESSAGE_SIZE) > 0
    _evar_message_count_t head;
    _evar_message_count_t tail;
    _evar_message_t messages[EVAR_TASK_MESSAGE_COUNT];
#endif
} _message_store_t;

/*
 * This is an opaque fixed-size structure to be used by this task
 * for allocating space for its message store, knowing just its size.
 * Because it has public visibility, to be used in the task code,
 * it is tagged with the task name.
 */
typedef struct { unsigned char _[sizeof(_message_store_t)]; } EVAR_CONCAT(EVAR_TASK_NAME, _message_store_t);
EVAR_ASSERT(sizeof(EVAR_CONCAT(EVAR_TASK_NAME, _message_store_t)) == sizeof(_message_store_t), sizeof__message_store_t);

/*
 * Puts a copy of the message at the end of the message queue.
 * Returns failure if the queue is full and EVAR_TASK_MESSAGES_CAN_BE_DROPPED is not defined.
 * If EVAR_TASK_MESSAGES_CAN_BE_DROPPED is defined, the oldest message is discarded.
 */
static evar_mq_result_t EVAR_CONCAT(EVAR_TASK_NAME, _mq_push)(evar_mq_args_t* p_push_args) {

    _message_store_t* p_message_store;
#if (EVAR_TASK_MESSAGE_SIZE) > 0
    _evar_message_t* p_tail;
#endif

    evar_assert(p_push_args != NULL);

    p_message_store = (_message_store_t*)p_push_args->p_message_store;
    evar_assert(p_message_store != NULL);
    
#if (EVAR_TASK_MESSAGE_SIZE) > 0
    if (p_push_args->p_message == NULL || p_push_args->message_size != (EVAR_TASK_MESSAGE_SIZE)) {
        return EVAR_MQ_INVALID_MESSAGE;
    }
#else
    if (p_push_args->p_message != NULL || p_push_args->message_size != 0) {
        return EVAR_MQ_INVALID_MESSAGE;
    }
#endif

#ifndef EVAR_TASK_MESSAGES_CAN_BE_DROPPED
    if (p_message_store->count == (EVAR_TASK_MESSAGE_COUNT)) {
        return EVAR_MQ_QUEUE_FULL;
    }
#endif

#if (EVAR_TASK_MESSAGE_SIZE) > 0
    p_tail = p_message_store->messages + p_message_store->tail;
    memcpy(p_tail, p_push_args->p_message, (EVAR_TASK_MESSAGE_SIZE));

    if (++p_message_store->tail == (EVAR_TASK_MESSAGE_COUNT)) {
        p_message_store->tail = 0;
    }
#endif

    if (p_message_store->count < (EVAR_TASK_MESSAGE_COUNT)) {
        p_message_store->count += 1;
    }
    else {
#if (EVAR_TASK_MESSAGE_SIZE) > 0
        if (++p_message_store->head == (EVAR_TASK_MESSAGE_COUNT)) {
            p_message_store->head = 0;
        }
#endif
    }

#if (EVAR_TASK_MESSAGE_SIZE) > 0
    evar_assert(p_message_store->tail == ((p_message_store->head + p_message_store->count) % (EVAR_TASK_MESSAGE_COUNT)));
#endif

    return EVAR_MQ_SUCCESS;
}

static evar_mq_result_t EVAR_CONCAT(EVAR_TASK_NAME, _mq_pop)(evar_mq_args_t* p_pop_args) {

    _message_store_t* p_message_store;
#if (EVAR_TASK_MESSAGE_SIZE) > 0
    _evar_message_t* p_head;
#endif

    evar_assert(p_pop_args != NULL);

    p_message_store = (_message_store_t*)p_pop_args->p_message_store;
    evar_assert(p_message_store != NULL);

#if (EVAR_TASK_MESSAGE_SIZE) > 0
    if (p_pop_args->p_message == NULL || p_pop_args->message_size != (EVAR_TASK_MESSAGE_SIZE)) {
        return EVAR_MQ_INVALID_MESSAGE;
    }
#else
    if (p_pop_args->p_message != NULL || p_pop_args->message_size != 0) {
        return EVAR_MQ_INVALID_MESSAGE;
    }
#endif

    if (p_message_store->count == 0) {
        return EVAR_MQ_QUEUE_EMPTY;
    }

#if (EVAR_TASK_MESSAGE_SIZE) > 0
    p_head = p_message_store->messages + p_message_store->head;
    memcpy(p_pop_args->p_message, p_head, (EVAR_TASK_MESSAGE_SIZE));
    
    if (++p_message_store->head == (EVAR_TASK_MESSAGE_COUNT)) {
        p_message_store->head = 0;
    }
#endif
    
    p_message_store->count -= 1;
    
#if (EVAR_TASK_MESSAGE_SIZE) > 0
    evar_assert(p_message_store->tail == ((p_message_store->head + p_message_store->count) % (EVAR_TASK_MESSAGE_COUNT)));
#endif

    return EVAR_MQ_SUCCESS;
}

static evar_mq_result_t EVAR_CONCAT(EVAR_TASK_NAME, _mq_peek)(evar_mq_args_t* p_peek_args) {

    _message_store_t* p_message_store;
#if (EVAR_TASK_MESSAGE_SIZE) > 0
    _evar_message_t* p_head;
#endif

    evar_assert(p_peek_args != NULL);

    p_message_store = (_message_store_t*)p_peek_args->p_message_store;
    evar_assert(p_message_store != NULL);

#if (EVAR_TASK_MESSAGE_SIZE) > 0
    if (
        (p_peek_args->p_message != NULL && p_peek_args->message_size != (EVAR_TASK_MESSAGE_SIZE)) ||
        (p_peek_args->p_message == NULL && p_peek_args->message_size != 0)
    ) {
        return EVAR_MQ_INVALID_MESSAGE;
    }
#else
    if (p_peek_args->p_message != NULL || p_peek_args->message_size != 0) {
        return EVAR_MQ_INVALID_MESSAGE;
    }
#endif

    if (p_message_store->count == 0) {
        return EVAR_MQ_QUEUE_EMPTY;
    }

#if (EVAR_TASK_MESSAGE_SIZE) > 0
    if (p_peek_args->p_message != NULL) {
        p_head = p_message_store->messages + p_message_store->head;
        memcpy(p_peek_args->p_message, p_head, (EVAR_TASK_MESSAGE_SIZE));
    }
#endif
    
    return EVAR_MQ_SUCCESS;
}

/*
 * This is a method table for the generated message queue. By this name 
 * {EVAR_TASK_NAME}_message_queue it will be referenced from the task structure.
 */
static evar_message_queue_t EVAR_CONCAT(EVAR_TASK_NAME, _message_queue) = {
    EVAR_CONCAT(EVAR_TASK_NAME, _mq_push),
    EVAR_CONCAT(EVAR_TASK_NAME, _mq_pop),
    EVAR_CONCAT(EVAR_TASK_NAME, _mq_peek)
};

#endif

#endif
