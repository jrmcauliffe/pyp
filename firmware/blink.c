#include <msp430.h>				
#include <stdbool.h>
/**
 * blink.c
 */


const unsigned int ticks = 10000;
const unsigned int intensity_step = 10;

volatile unsigned int intensity;
volatile unsigned int state;


// Simple gamma adjustment
unsigned int gamma(unsigned int percentage) {
    return percentage * percentage; // calculate gamma using basic squared
}

void main(void)
{


    WDTCTL = WDTPW + WDTHOLD;     // Stop WDT

    // Initalise all pins to output to reduce power consumption on unused pins
    P1DIR = 0xFF;
    P1OUT = 0x00;
    P2DIR = 0xFF;
    P2OUT = 0x00;
    // LED output
	P1DIR |= BIT0 + BIT6;			// configure P1.0 and P1.6 as output
    P1SEL |= BIT6;                  // configure P1.6 as special

    // Pushbutton input
    P1DIR &= ~BIT3;                // configure P1.3 as input
    P1REN |= BIT3;                 // Enable internal pull-up/down resistors
    P1OUT |= BIT3;                 //Select pull-up mode for P1.3
    P1IE |= BIT3;                   // Enable P1.3 interrupts
    P1IES |= BIT3;                  // P1.3 Hi/lo edge
    P1IFG &= ~BIT3;                 // P1.3 IFG cleared

    // Timer A configuration
    intensity = 0;
	TACTL = 0x210;
    TACCR0 = ticks;
    TACCR1 = gamma(intensity);
    TACCTL0 = 0x90;
    TACCTL1 = 0x10E0;
    P1OUT &= ~0x01;

    _BIS_SR(CPUOFF + GIE);          // Enter LPM0 w/ interrupt

}

// Timer A0 interrupt service routine
#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A (void)
{
    // Debounce button press
    state = ( state << 1 ) | (P1IN & BIT3)| 0xE000;
    if(state == 0xF000) {
        intensity = (intensity + intensity_step) % (100 + intensity_step);
        TACCR1 = gamma(intensity);
    }


  // P1OUT ^= BIT0;                          // Toggle P1.0
}
// Port 1 interrupt service routine
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
{
   P1IFG &= ~BIT3;                     // P1.3 IFG cleared
}
