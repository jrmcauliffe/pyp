#include <msp430.h>
#define timeout

unsigned int gamma[5];
 int intensity;
unsigned int button_state;                 // Used to track debounce state of button
unsigned int ticks;

void main(void)
{
    // Approx 25, 50, 75 and 100% pwm brightness
    gamma[0] = 0;
    gamma[1] = 625;
    gamma[2] = 2500;
    gamma[3] = 5625;
    gamma[4] = 10000;

    ticks = 0;         // Clock tick counter

    intensity = 1;    // start with dimmest 'on' value

    WDTCTL = WDTPW + WDTHOLD;     // Stop WDT

    P1DIR = 0xF7;                  // configure P1.0 and P1.6 as output, P1.3 as input
    P1OUT = BIT3;                  //Select pull-up mode for P1.3
    P1SEL |= BIT6;                 // configure P1.6 as special (timer)
    P1REN |= BIT3;                 // Enable internal pull-up/down resistors
    P2DIR = 0xFF;                  // Set P2 for lowest power draw
    P2OUT = 0x00;

    P1IE |= BIT3;                   // P1.3 IE
    P1IES |= BIT3;                  // P1.3 Hi/lo edge
    P1IFG &= ~BIT3;                 // P1.3 IFG cleared

    // Timer A configuration
	TACTL = 0x210;
    TACCR0 = 10000;
    TACCR1 = gamma[intensity];
    TACCTL0 = 0x90;
    TACCTL1 = 0x10E0;

    _BIS_SR(CPUOFF + GIE);          // Enter LPM0 w/ global interrupts
    // while(1);
}

// Timer A0 interrupt service routine
#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A (void)
{

    // Debounce button press
    button_state = ( button_state << 1 ) | (P1IN & BIT3) | 0xE000;
    if(button_state == 0xF000) {
        ticks = 0;
        intensity += 1;
    }


    if(intensity == 5 || ticks == 500) {
           P1OUT &= ~BIT6;  // Set off
           P1SEL &= ~BIT6;
           intensity = 0;
           ticks = 0;       // Reset tick counter
           //_BIS_SR_IRQ(LPM4);   // Sleep
    } else  TACCR1 = gamma[intensity];

    ticks += 1;
}

// Port 1 interrupt service routine
// Just need this to wake up
// All button code handled by timer ISR
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
{
   P1IFG &= ~BIT3;         // P1.3 IFG cleared
//  P1OUT ^= BIT0;          // Toggle Red LED
   if(!(P1SEL & BIT6)) {
       P1SEL |= BIT6;        // Reenable led for output from timer
      _BIS_SR_IRQ(CPUOFF);   // Put cpu back in LPM0 for timer
   }
}
