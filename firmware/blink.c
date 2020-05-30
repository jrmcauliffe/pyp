#include <msp430.h>
#define timeout

unsigned int gamma[5];
unsigned char intensity;
unsigned int button_state;                 // Used to track debounce state of button
unsigned int ticks_remaining;


void main(void)
{
    // Approx 25, 50, 75 and 100% pwm brightness
    gamma[0] = 625;
    gamma[1] = 2500;
    gamma[2] = 5625;
    gamma[3] = 10000;
    gamma[4] = 0;      // Off and mark for sleep

    ticks_remaining = 500;  // Countdown timer

    intensity = 0;    // start with dimmest 'on' value

    WDTCTL = WDTPW + WDTHOLD;     // Stop WDT

    // Initalise all pins to output to reduce power consumption on unused pins
    P1DIR = 0xF7;                  // configure P1.0 and P1.6 as output, P1.3 as input
    P1OUT = 0x08;                  //Select pull-up mode for P1.3
    P1SEL |= BIT6;                  // configure P1.6 as special (timer)
    P1REN |= BIT3;                 // Enable internal pull-up/down resistors
    P2DIR = 0xFF;
    P2OUT = 0x00;

    P1IE |= BIT3;                   // P1.3 IE
    P1IES |= BIT3;                  // P1.3 Hi/lo edge
    P1IFG &= ~BIT3;                 // P1.3 IFG cleared

    // Timer A configuration
	TACTL = 0x210;
    TACCR0 = 10000;
    TACCR1 = gamma[0];
    TACCTL0 = 0x90;
    TACCTL1 = 0x10E0;

    _BIS_SR(CPUOFF + GIE);          // Enter LPM0 w/ interrupt

}

// Timer A0 interrupt service routine
#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A (void)
{


    // Debounce button press
    button_state = ( button_state << 1 ) | (P1IN & BIT3)| 0xE000;
    if(button_state == 0xF000) {
        intensity += 1;
        TACCR1 = gamma[intensity];
    }

//    // Decrement and check count down timer
    ticks_remaining -= 1;
    if(ticks_remaining == 0) {
           TACCR1 = 0 ;   //Turn off LED
          _BIS_SR(LPM3);

   }


}
// Port 1 interrupt service routine
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
{
   P1IFG &= ~BIT3;                      // P1.3 IFG cleared
   if(intensity == 4) intensity = 0;    // Default brightness


}
