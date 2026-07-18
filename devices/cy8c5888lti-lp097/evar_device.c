/*
 * This is the support file for Infineon/Cypress CY8C5888LTI-LP097.
 */

#include "evar_device.h"
#include <evar_config.h>

volatile unsigned short evar_device__timer_ticks;

void EVAR_CLOCK_interrupt_handler(void) {
    evar_device__timer_ticks += 1;
}

void evar_device__initialize(void) {
    EVAR_CLOCK_IRQ_StartEx(EVAR_CLOCK_interrupt_handler);
    evar_device__enable_interrupts();
}

void evar_device__crash(unsigned short error, char* message) {
    evar_device__crashed_pin_on();
    DEBUG_PRINT2("error %04X: %s\r\n", error, message);
    evar_device__halt();
}

void evar_device__halt(void) {
    DEBUG_UART_PutString("halted\r\n");
    evar_device__halted_pin_on();
    while (1) {
        evar_device__disable_interrupts();
        evar_device__cpu_idle();
    }
}

/*
 * If any of the remaining board-specific things were declared in
 * evar_device.h as functions, they should be implemented here.
 */

#ifdef DEBUG
    
char debug_buf[128];

#endif
