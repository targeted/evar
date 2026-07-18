/*
 * This is the support file for Infineon/Cypress CY8C5888LTI-LP097.
 */

#ifndef EVAR_DEVICE_H
#define EVAR_DEVICE_H

#include "project.h"
#include <FS.h>
#include <stdio.h>

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
 * This can be a unsigned short (void) function or a preprocessor
 * define pointing to a volatile global unsigned short variable.
 */
extern volatile unsigned short evar_device__timer_ticks;
#define evar_device__get_timer_ticks() (evar_device__timer_ticks)

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
 * From each pair uncomment one option and either implement it in evar_device.c
 * or inline in evar_device.h
 */

/*
 * Put the CPU to sleep, keeping the interrupts enabled. Conceptually, this
 * should pause the execution until the next timer tick (or another interrupt).
 */
#define evar_device__cpu_idle() __WFI()

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
typedef uint8_t evar_interrupts_enabled_t;

/*
 * Store the current state of interrupts into a variable, then disable the interrupts (if they were enabled).
 */
#define evar_device__save_and_disable_interrupts(interrupts_enabled) interrupts_enabled = !CyEnterCriticalSection()

/*
 * Restore the state of interrupts saved in evar_device__save_and_disable_interrupts.
 */
#define evar_device__restore_interrupts(interrupts_enabled) CyExitCriticalSection(!interrupts_enabled)

/*
 * Disable all interrupts.
 */
#define evar_device__disable_interrupts() CyGlobalIntDisable

/*
 * Enable all interrupts.
 */
#define evar_device__enable_interrupts() CyGlobalIntEnable

/*
 * Single-bit debug LED.
 */
#define evar_device__builtin_led_on()
#define evar_device__builtin_led_off()

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

#define evar_device__crashed_pin_on()
#define evar_device__crashed_pin_off()

#define evar_device__halted_pin_on()
#define evar_device__halted_pin_off()

#include <stdarg.h>

#ifdef DEBUG
    
extern char debug_buf[128];

#define DEBUG_PRINT(STR) \
    if (1) { \
        DEBUG_UART_PutString(STR); \
    }

#define DEBUG_PRINT1(FMT, ARG) \
    if (1) { \
        snprintf(debug_buf, sizeof(debug_buf), (FMT), (ARG)); \
        DEBUG_UART_PutString(debug_buf); \
    }

#define DEBUG_PRINT2(FMT, ARG1, ARG2) \
    if (1) { \
        snprintf(debug_buf, sizeof(debug_buf), (FMT), (ARG1), (ARG2)); \
        DEBUG_UART_PutString(debug_buf); \
    }

#endif

#endif
