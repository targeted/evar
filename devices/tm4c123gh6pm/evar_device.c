/*
 * This is the support file for TM4C123GH6PM MCU.
 */

#include "evar_device.h"
#include <evar_config.h>

volatile unsigned short evar_device__timer_ticks;

void SysTick_Handler(void) {
    evar_device__timer_pin_on();
    evar_device__timer_ticks += 1;
    evar_device__timer_pin_off();
}

void evar_device__initialize(void) {

    // Enable lazy stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.

    FPULazyStackingEnable();

    // Set the clocking to run directly from the crystal, system clock 80MHz.

    SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    // Disable watchdog timer.

    SysCtlPeripheralEnable(SYSCTL_PERIPH_WDOG0);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_WDOG0)) {}
    WatchdogUnlock(WATCHDOG0_BASE);
    WatchdogResetDisable(WATCHDOG0_BASE);

    // Configure clock timer.

    evar_device__timer_ticks = 0;

    SysTickPeriodSet(SysCtlClockGet() / EVAR_TIMER_FREQUENCY);
    SysTickIntEnable();
    SysTickEnable();
}

void evar_device__crash(unsigned short error, char* message) {
    UARTprintf("crash %04x: %s\n", error, message);
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
 * If any of the remaining device-specific things were declared in
 * evar_device.h as functions, they should be implemented here.
 */
