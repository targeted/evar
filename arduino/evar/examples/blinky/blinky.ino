extern "C" {
#include <evar.h>
#include <main_task.h>
}

void setup() {
    
    /*
     * Initialization parameters are passed as a part of task data structure.
     * Note that it has to be static, global or allocated, not on stack.
     */
    static main_task_data_t main_task_data = {
        .interval = 1000000 // one blink a second
    };

    evar__setup(main_task, &main_task_data);
}

void loop() {
    evar__loop();
}
