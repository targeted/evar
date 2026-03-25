/*
 * The following three lines demonstrate the task implementation pattern.
 */
#include "main_task.h"
#define EVAR_TASK_NAME main_task
#include <evar_task.h>

/*
 * Notice that all the objects here - functions, types and structures
 * are all conventionally named {EVAR_TASK_NAME}_..., this is mandatory.
 */

/*
 * This function is called once when the task is created.
 * It should initialize all the hardware it needs and possibly start other tasks.
 */
void main_task__initialize(evar_task_info_t* p_task_info) {

    /*
     * In the task_info structure you have task_data plus some other runtime info
     * like current task id. But typically all you need is just task_data.
     */
    main_task_data_t* p_task_data = (main_task_data_t*)p_task_info->p_task_data;

    /*
     * Having task runtime data in a structure like that allows to have multiple
     * instances of the same task, like one task per hardware block or something.
     */

    p_task_data->led = 0;
    evar_device__builtin_led_off();

    /*
     * Every task function must return with a call one evar_task__... or another.
     * That tells the scheduler what to do with it.
     */
    return evar_task__sleep_for(p_task_data->interval / 2);
}

void main_task__run(evar_task_info_t* p_task_info) {
    EVAR_UNUSED(p_task_info);
    /*
     * This function is not supposed to be called, because initialize sends 
     * the task into sleep/wake_up cycle, but if it is called and just exits
     * without calling any evar_task__..., it is as if it did call
     * evar_task__exit() and is going to be discarded.
     */
}

void main_task__wake_up(evar_task_info_t* p_task_info) {
 
    /*
     * We get here every time the sleep timeout expires.
     */

    main_task_data_t* p_task_data = (main_task_data_t*)p_task_info->p_task_data;
 
    if (p_task_data->led ^= 1) {
        evar_device__builtin_led_on();
    }
    else {
        evar_device__builtin_led_off();
    }

    /*
     * And now we go to sleep again.
     */

    return evar_task__sleep_for(p_task_data->interval / 2);
}

void main_task__receive(evar_task_info_t* p_task_info) {
    EVAR_UNUSED(p_task_info);
    /*
     * This function is not supposed to be called in this example,
     * because nobody sends it messages, and the message store is
     * not even allocated in initialize.
     */
}

void main_task__cleanup(evar_task_info_t* p_task_info) {
    EVAR_UNUSED(p_task_info);
    /*
     * Likewise, since this task never exits, this function is not going 
     * to be called. Conceptually, it should undo what initialize did.
     */
}
