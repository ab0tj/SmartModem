/* 
 * File:   Main.c
 * Author: Alex Swedenburg AB0TJ
 *
 * Software for the SmartModem chip.
 * Portions adapted from Bob Ball's modemless TNC and Gary Dion's WhereAVR project.
 *
 * Created on April 11, 2012, 12:16 PM
 */
#include <htc.h>
#include <stdio.h>
#include <stdlib.h>

__CONFIG(CP_OFF & CCPMX_RB0 & DEBUG_OFF & WRT_OFF & CPD_OFF & LVP_OFF & BOREN_OFF & MCLRE_OFF & PWRTE_OFF & WDTE_OFF & FOSC_HS);
__CONFIG(IESO_OFF & FCMEN_OFF);

bit busy;
char byte;

void Init_Hardware(void) {
    INTCON = 0b11001000;        // Interrupt config
    PIE1 = 0b00000010;          // Peripheral Interrupt config 1
    PIE2 = 0b01000000;          // Peripheral Interrupt config 2
    OPTION_REG = 0b10000100;    // Timer0 internal clock, 1:16 pre
    T1CON = 0b00110000;         // Timer1 internal clock, 1:8 pre, disabled
    T2CON = 0b00000101;         // Timer2 internal clock, 1:4 pre, no post
    CMCON = 0b00000110;         // Comparator config
    ANSEL = 0b00011111;         // Analog pins config
    TRISA = 0b11110111;         // Port A Config
    TRISB = 0b00110100;         // Port B Config
    PORTA = 0b00000000;         // Zero the Port A outputs
    PORTB = 0b00000000;         // Zero the Port B outputs
    RCSTA = 0b10100000;         // Serial port RX Config
    TXSTA = 0b00100100;         // Serial port TX Config
    SPBRG = 129;                // 9600 Baud
    TMR2 = 126;                 // Set ~9600Hz bit timer
}

void interrupt isr(void) {
    static char count;
    static bit last;
    static bit rxtoggled;
    static char last8bits;
    static char bit_count;
    static char dcd;
    static char sample_clock;
    static char next_sample;
    static char ones_count;

    if (RBIF) // Zero-crossing detected
    {
        count = TMR0;
        TMR0 = 0;
        RBIF = 0;
        if (count > 92) // Above 1700Hz?
        {
            if (last == 0)
            {
                rxtoggled = 1;
                dcd = count;
            }
            last = 1;
        }
        else if (count > 49) // Ignore freq. above 3200Hz
        {
            if (last == 1)
            {
                rxtoggled = 1;
                dcd = count;
            }
        }
    }
    else if (TMR2IF) // Bit timer interrupt
    {
        TMR2 = 126; // ~9600 Hz
        TMR2IF = 0;
        if (dcd)
        {
            dcd--;
            busy = 1;
            if (rxtoggled)
            {
                if (ones_count != 5) // If this zero wasn't just for bitstuffing...
                {
                    bit_count++;
                    last8bits >>= 1; // Shift in a zero, from the left
                }

                rxtoggled = 0;
                ones_count = 0;
                sample_clock = 0;
                next_sample = 12; // 1.5 ax.25 bits
            }
            else
            {
                if (++sample_clock == next_sample)
                {
                    ones_count++;
                    bit_count++;
                    last8bits >> = 1; // Shift in a zero, from the left
                    last8bits |= 0x80; // Make that zero a one
                    sample_clock = 0;
                    next_sample = 8; // 1 ax.25 bit
                }
            }
            if (last8bits == 0x7E)
            {
                bit_count = 0; // Sync the bit counter here
                TXREG = 0x1B;
                TXREG = 0x7B;
                RA4 = 1; // Turn on DCD LED
            }
            else
            {
                if (bit_count == 8)
                {
                    bit_count = 0;
                    TXREG = 0x7B;
                }
            }
        }
        else // DCD expired, must be recieving noise
        {
            busy = 0;
            RA4 = 0; // Turn off DCD LED
        }
    }
}

int main() {
    Init_Hardware();
    while (1); // This would be a good place to put serial RX polling
}