/*
 * This is the support file for MPLAB Express PIC18F46K42.
 */

#ifndef EVAR_DEVICE_H
#define EVAR_DEVICE_H


// PIC18F46K42 Configuration Bit Settings

// 'C' source line config statements

// CONFIG1L
#pragma config FEXTOSC = ECH    // External Oscillator Selection (EC (external clock) above 8 MHz; PFM set to high power)
#pragma config RSTOSC = EXTOSC  // Reset Oscillator Selection (EXTOSC operating per FEXTOSC bits (device manufacturing default))

// CONFIG1H
#pragma config CLKOUTEN = OFF   // Clock out Enable bit (CLKOUT function is disabled)
#pragma config PR1WAY = ON      // PRLOCKED One-Way Set Enable bit (PRLOCK bit can be cleared and set only once)
#pragma config CSWEN = ON       // Clock Switch Enable bit (Writing to NOSC and NDIV is allowed)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enable bit (Fail-Safe Clock Monitor enabled)

// CONFIG2L
#pragma config MCLRE = EXTMCLR  // MCLR Enable bit (If LVP = 0, MCLR pin is MCLR; If LVP = 1, RE3 pin function is MCLR )
#pragma config PWRTS = PWRT_OFF // Power-up timer selection bits (PWRT is disabled)
#pragma config MVECEN = ON      // Multi-vector enable bit (Multi-vector enabled, Vector table used for interrupts)
#pragma config IVT1WAY = ON     // IVTLOCK bit One-way set enable bit (IVTLOCK bit can be cleared and set only once)
#pragma config LPBOREN = OFF    // Low Power BOR Enable bit (ULPBOR disabled)
#pragma config BOREN = SBORDIS  // Brown-out Reset Enable bits (Brown-out Reset enabled , SBOREN bit is ignored)

// CONFIG2H
#pragma config BORV = VBOR_2P45 // Brown-out Reset Voltage Selection bits (Brown-out Reset Voltage (VBOR) set to 2.45V)
#pragma config ZCD = OFF        // ZCD Disable bit (ZCD disabled. ZCD can be enabled by setting the ZCDSEN bit of ZCDCON)
#pragma config PPS1WAY = ON     // PPSLOCK bit One-Way Set Enable bit (PPSLOCK bit can be cleared and set only once; PPS registers remain locked after one clear/set cycle)
#pragma config STVREN = ON      // Stack Full/Underflow Reset Enable bit (Stack full/underflow will cause Reset)
#pragma config DEBUG = OFF      // Debugger Enable bit (Background debugger disabled)
#pragma config XINST = OFF      // Extended Instruction Set Enable bit (Extended Instruction Set and Indexed Addressing Mode disabled)

// CONFIG3L
#pragma config WDTCPS = WDTCPS_31// WDT Period selection bits (Divider ratio 1:65536; software control of WDTPS)
#pragma config WDTE = ON        // WDT operating mode (WDT enabled regardless of sleep)

// CONFIG3H
#pragma config WDTCWS = WDTCWS_7// WDT Window Select bits (window always open (100%); software control; keyed access not required)
#pragma config WDTCCS = SC      // WDT input clock selector (Software Control)

// CONFIG4L
#pragma config BBSIZE = BBSIZE_512// Boot Block Size selection bits (Boot Block size is 512 words)
#pragma config BBEN = OFF       // Boot Block enable bit (Boot block disabled)
#pragma config SAFEN = OFF      // Storage Area Flash enable bit (SAF disabled)
#pragma config WRTAPP = OFF     // Application Block write protection bit (Application Block not write protected)

// CONFIG4H
#pragma config WRTB = OFF       // Boot Block Write Protection bit (Boot Block not write-protected)
#pragma config WRTC = OFF       // Configuration Register Write Protection bit (Configuration registers not write-protected)
#pragma config WRTD = OFF       // Data EEPROM Write Protection bit (Data EEPROM not write-protected)
#pragma config WRTSAF = OFF     // SAF Write protection bit (SAF not Write Protected)
#pragma config LVP = ON         // Low Voltage Programming Enable bit (Low voltage programming enabled. MCLR/VPP pin function is MCLR. MCLRE configuration bit is ignored)

// CONFIG5L
#pragma config CP = OFF         // PFM and Data EEPROM Code Protection bit (PFM and Data EEPROM code protection disabled)

// CONFIG5H

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>

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
#define evar_device__cpu_idle() SLEEP()

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
typedef unsigned char evar_interrupts_enabled_t;

/*
 * Store the current state of interrupts into a variable, then disable the interrupts (if they were enabled).
 */
#define evar_device__save_and_disable_interrupts(interrupts_enabled) interrupts_enabled = GIE; GIE = 0

/*
 * Restore the state of interrupts saved in evar_device__save_and_disable_interrupts.
 */
#define evar_device__restore_interrupts(interrupts_enabled) GIE = (interrupts_enabled & 0x01)

/*
 * Disable all interrupts.
 */
#define evar_device__disable_interrupts() GIE = 0

/*
 * Enable all interrupts.
 */
#define evar_device__enable_interrupts() GIE = 1

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

#endif
