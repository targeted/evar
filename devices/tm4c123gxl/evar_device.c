/*
 * This is the support file for EK-TM4C123GXL board based on TM4C123GH6PM MCU.
 */

#include "evar_device.h"
#include <evar_config.h>

volatile unsigned short hardware_timer_ticks;

void SysTick_Handler(void) {

    evar_device__timer_pin_on();

    hardware_timer_ticks += 1;

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

    // Enable all peripheral modules that we are going to be using.

    // Configure UART 0 to be the serial link.

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA); 
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA)) {}

    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0)) {}

    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);
    UARTStdioConfig(0, 115200, 16000000);

    UARTFIFOEnable(UART0_BASE);

    // Set RGB pins to output.

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF)) {}

    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);

    // Configure timer.

    hardware_timer_ticks = 0;

    SysTickPeriodSet(SysCtlClockGet() / EVAR_TIMER_FREQUENCY);
    SysTickIntEnable();
    SysTickEnable();
}

unsigned short evar_device__get_timer_ticks(void) {
    return hardware_timer_ticks;
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
 * If any of the remaining board-specific things were declared in
 * evar_device_<board>.h as functions, they should be implemented here.
 */
