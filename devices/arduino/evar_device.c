/*
 * This is the support file for Arduino environment.
 */

#include <evar_device.h>
#include <evar_config.h>

void evar_device__initialize(void) {
    pinMode(LED_BUILTIN, OUTPUT);
}

unsigned short evar_device__get_timer_ticks(void) {
    return EVAR_USEC_TO_TICKS(micros()) & 0xFFFF;
}

void evar_device__crash(unsigned short error, char* message) {
    EVAR_UNUSED(error);
    EVAR_UNUSED(message);
    evar_device__crashed_pin_on();
    evar_device__halt();
}

void evar_device__halt(void) {
    evar_device__halted_pin_on();
    while (1) {
        evar_device__disable_interrupts();
        evar_device__cpu_idle();
    }
}

/*
 * If any of the remaining board-specific things were declared in
 * evar_device_<board>.h as functions, they should be implemented here.
 */
