/*
 * This is the support file for MPLAB Express PIC18F46K42.
 */

#include "evar_device.h"
#include <evar_config.h>

volatile unsigned short timer_ticks;

/*
 * This is executed 15625 times per second.
 */
void __interrupt(irq(IRQ_TMR6)) clock_timer_interrupt(void) {

    evar_device__timer_pin_on();

    if (TMR6IF) {
        timer_ticks += 1;
        TMR6IF = 0;
    }

    evar_device__timer_pin_off();
}

void evar_device__initialize(void) {

    // SLEEP instruction will put the CPU to IDLE mode where the clocks keep ticking

    IDLEN = 1;

    // set CPU clock to the highest 64 MHz (this can be changed without affecting the clock timer below)

    OSCFRQbits.FRQ = 0b1000;      // switch HFINTOSC frequency to 64 MHz
    while (!OSCSTATbits.HFOR) {}  // wait for HFINTOSC to settle

    OSCCON1 = 0b01100000;         // clock uses HFINTOSC with 1:1 prescaler, i.e. 64 MHz
    while (!OSCCON3bits.ORDY) {}  // wait for the clock to become ready

    // make timer 6 to maintain the clock

    T6CONbits.ON = 0;             // disable the timer

    T6CLKbits.CS = 0b00110;       // timer source = MFINTOSC calibrated 31.25 KHz before prescaling
    T6CONbits.CKPS = 0b001;       // prescaling 1:2, effective 15625 Hz
    T6CONbits.OUTPS = 0b0000;     // postscaling 1:1

    TMR6IF = 0;
    TMR6IE = 1;

    T6TMR = 0;                    // this will cause an interrupt every tick without reloading
    T6PR = 0;

    T6CONbits.ON = 1;             // enable the timer

    GIE = 1;                      // enable global interrupts
}

unsigned short evar_device__get_timer_ticks(void) {
    return timer_ticks;
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
