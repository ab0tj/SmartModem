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

__CONFIG(CP_OFF & CCPMX_RB0 & WRT_OFF & CPD_OFF & LVP_OFF & BOREN_OFF & MCLRE_OFF & PWRTE_OFF & WDTE_OFF & FOSC_HS);
__CONFIG(IESO_OFF & FCMEN_OFF);

bit last;
bit rxtoggled;
bit dcd;
bit flag;
unsigned char count;
unsigned char last8bits;
unsigned char bit_count;
unsigned char sample_clock;
unsigned char next_sample = 8;
unsigned char ones_count;


void Init_Hardware(void) {
    OPTION_REG = 0b10000011;    // Timer0 internal clock, 1:16 pre
    T1CON = 0b00110000;         // Timer1 internal clock, 1:8 pre, disabled
    T2CON = 0b00000101;         // Timer2 internal clock, 1:4 pre, no post
    ANSEL = 0b00011111;         // Analog pins config
    TRISA = 0b11110111;         // Port A Config
    TRISB = 0b00110100;         // Port B Config
    RCSTA = 0b10100000;         // Serial port RX Config
    TXSTA = 0b00100100;         // Serial port TX Config
    SPBRG = 129;                 // 9600 Baud
    CMCON = 0b00000110;         // Comparator config
    INTCON = 0b11001000;        // Interrupt config
    PIE1 = 0b00000010;          // Peripheral Interrupt config 1
    PR2 = 130;                  // Set ~9600Hz bit timer
}

void interrupt isr(void) {
    count = TMR0;
    if (RBIF) // Zero-crossing detected
    {
        //TMR0 = 0;
        if (count > 91) // Under 1700Hz?
        {
            TMR0 = 0;
            if (!last)
            {
                rxtoggled = 1;
                dcd = 1;
            }
            last = 1;
        }
        else if (count > 48) // Ignore freq. above 3200Hz
        {
            TMR0 = 0;
            if (last)
            {
                rxtoggled = 1;
                dcd = 1;
            }
            last = 0;
        }
        RB4 = 0;
        RBIF = 0;
    }

    if (TMR2IF) // Bit timer interrupt
    {
        if (dcd)									// If we are actively monitoring a signal
	{

            if (rxtoggled)						// See if a tone toggle was recognized
            {
                if(ones_count != 5)			// Only process if NOT a bit stuff toggle
                {
                    bit_count++;				// Increment bit counter
                    last8bits >>= 1;			// Shift in a zero from the left
                }
                if(ones_count == 6) flag = 1;
                rxtoggled = 0;			// Clear toggle flag
                ones_count = 0;				// Clear number of sequential ones
                sample_clock = 0;				// Sync clock for bit sampling
                next_sample = 12;				// Grab next bit after 12 clicks
            }
            else
            {
                if (++sample_clock == next_sample)	// Time to grab next bit?
		{
                    ones_count++;				// Increment ones counter since no toggle
                    bit_count++;				// Increment bit counter
                    last8bits >>= 1;			// Shift the bits to the right
                    last8bits |= 0x80;		//  shift in a one from the left
                    sample_clock = 0;			// Clear the clock for bit sampling
                    next_sample = 8;			// Grab next bit 8 clicks from now
                }

            }	// end else for 'if (rxtoggled)'

            if (flag)			// If the last 8 bits match the ax25 flag
            {
                bit_count = 0;                      // Sync bit_count for an 8-bit boundary
                flag = 0;
                TXREG = 0x1B; // TODO: change to 1B in final source
                TXREG = 0x7E;
                RB0 = 1;    // Turn on DCD light
            }
            else
            {
                if (bit_count == 8)			// Just grabbed 8'th bit for a full byte
		{
                    bit_count = 0;				// Reset bit counter
                    TXREG = last8bits;

		}		// end 'if (bit_count == 8)'
            }		// end else for 'if (last8bits == 0x7E)'
            if (ones_count == 7)
            {
                dcd = 0;
                RB0 = 0;
            }
        }		// end 'if (dcd)'
        
        TMR2IF = 0;
    }
}

int main() {
    Init_Hardware();
    while (1)
    {// This would be a good place to put serial RX polling
        
    }
}