/*
 * This is the support file for EK-TM4C123GXL board based on TM4C123GH6PM MCU.
 */

#ifndef EVAR_DEVICE_H
#define EVAR_DEVICE_H

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "driverlib/adc.h"
#include "driverlib/cpu.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"
#include "driverlib/watchdog.h"
#include "utils/uartstdio.h"

/*
 * Any device-specific initialization, executed once at startup.
 * In particular, the clock timer, if it is used, must be started here.
 */
void evar_device__initialize(void);

/*
 * This returns the current timer ticks, provided either by either
 * a dedicated hardware timer or the surrounding environment.
 * It is assumed that this value is updated asynchronously behind
 * the scenes, and it can freely roll over and keep ticking.
 * For the timer frequency of 10KHz it overflows in 6.5 seconds,
 * during which time a scheduler pass must happen.
 */
unsigned short evar_device__get_timer_ticks(void);

/*
 * Display debug message if possible, then shut down.
 */
void evar_device__crash(unsigned short error, char* message);

/*
 * Shut down all hardware, then halt the CPU.
 */
void evar_device__halt(void);

/*
 * The following hardware-specific thunks could be declared as either functions
 * or defines, to inline intrinsics/assembler instructions and save time and stack.
 * From each pair uncomment one option and either implement it in evar_device_<board>.c
 * or inline in evar_device_<board>.h
 */

/*
 * Put the CPU to sleep, keeping the interrupts enabled. Conceptually, this
 * should pause the execution until the next timer tick (or another interrupt).
 */
#define evar_device__cpu_idle() SysCtlSleep()

/*
 * This is only needed in a multi-core multi-threaded environment, where a CPU can be executing code on the side,
 * even after having been put to sleep with evar_device__cpu_idle. This is true for the Windows implementation,
 * where asynchronous execution is emulated by threads, running even when the main scheduler thread is sleeping.
 * In a regular single-core environment, where the CPU will only resume running after an interrupt, and interrupt
 * handlers are the only code that can be executing asynchronously, this is not needed and can be left empty.
 */
#define evar_device__wake_cpu()

/*
 * The type of variable to store the state of interrupts.
 * Can be a single 0/1 byte, or a 32-bit mask for example.
 */
typedef bool evar_interrupts_enabled_t;

/*
 * Store the current state of interrupts into a variable, then disable the interrupts (if they were enabled).
 */
#define evar_device__save_and_disable_interrupts(interrupts_enabled) interrupts_enabled = !IntMasterDisable()

/*
 * Restore the state of interrupts saved in evar_device__save_and_disable_interrupts.
 */
#define evar_device__restore_interrupts(interrupts_enabled) if (interrupts_enabled) IntMasterEnable()

/*
 * Disable all interrupts.
 */
#define evar_device__disable_interrupts() IntMasterDisable()

/*
 * Enable all interrupts.
 */
#define evar_device__enable_interrupts() IntMasterEnable()

// tri-color LED for the application to use
#define evar_device__builtin_r_led_on()  GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_PIN_1)
#define evar_device__builtin_r_led_off() GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0)
#define evar_device__builtin_g_led_on()  GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3)
#define evar_device__builtin_g_led_off() GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0)
#define evar_device__builtin_b_led_on()  GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2)
#define evar_device__builtin_b_led_off() GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0)

// pseudo uni-color LED for the application to use
#define evar_device__builtin_led_on() { \
    evar_device__builtin_r_led_on(); \
    evar_device__builtin_g_led_on(); \
    evar_device__builtin_b_led_on(); \
}

#define evar_device__builtin_led_off() { \
    evar_device__builtin_r_led_off(); \
    evar_device__builtin_g_led_off(); \
    evar_device__builtin_b_led_off(); \
}

/*
 * The following definitions are used internally by the framework to configure and pulse output pins
 * at certain moments, thus allowing timing/latency measurements and/or debugging with scope/LEDs.
 * Each can be a preprocessor define or a void(void) function.
 * The actually used pins should be initialized for output in evar_device__initialize.
 */

// on when the clock timer interrupt handler is executing
#define evar_device__timer_pin_on()
#define evar_device__timer_pin_off()

// on when a task is executing, off when a scheduler is executing (or sleeping)
#define evar_device__running_pin_on()
#define evar_device__running_pin_off()

// on when the scheduler has put the CPU to idle state sleep, off when the scheduler or a task is actively executing
#define evar_device__idle_pin_on()
#define evar_device__idle_pin_off()

// on when either evar__send_message or evar__send_async_message is executing, off otherwise
#define evar_device__sending_pin_on()
#define evar_device__sending_pin_off()

// on when evar__receive_message is executing, off otherwise
#define evar_device__receiving_pin_on()
#define evar_device__receiving_pin_off()

// turns on once when evar_device__crash has been called, never turns off after that
#define evar_device__crashed_pin_on() { \
    evar_device__builtin_r_led_on();  \
    evar_device__builtin_g_led_off(); \
    evar_device__builtin_b_led_off(); \
}

#define evar_device__crashed_pin_off() { \
    evar_device__builtin_r_led_off(); \
    evar_device__builtin_g_led_off(); \
    evar_device__builtin_b_led_off(); \
}

// turns on when evar_device__halt has been called, never turns off after that
#define evar_device__halted_pin_on()
#define evar_device__halted_pin_off()

#endif
