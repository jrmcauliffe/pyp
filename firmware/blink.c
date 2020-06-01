#include <msp430.h>
#define TICKSPERSEC 1000    // Approx 1ms/tick
#define TIMEOUT 3600        // Approx 1 hr at 1ms/tick (1Mhz clock)

unsigned char intensity;    // Current intensity setting
unsigned int gamma[4];      // Convert intensity to gamma corrected clock
unsigned int button_state;  // De-bounce state of button
unsigned int ticks;         // Track on time
unsigned int seconds;       // Splitting avoids long values
void main(void)
{
    // Approx 25, 50, 75 and 100% pwm brightness with simple gamma calc
    gamma[0] = 70;
    gamma[1] = 280;
    gamma[2] = 629;
    gamma[3] = 1118;      // Will give 1khz clock on device tested

    ticks = TICKSPERSEC;  // Clock tick counter
    seconds = TIMEOUT;
    intensity = 0;    // start with dimmest 'on' value

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
    TACTL =  TASSEL_2 + MC_1;
    TACCR0 = gamma[3];              // Gamma [3] will be 100% duty cycle
    TACCR1 = gamma[intensity];      // Initial brightness value
    TACCTL0 = OUTMOD_4 + CCIE;     // Toggle + IE
    TACCTL1 = CCIS0 + OUTMOD_7;

    _BIS_SR(CPUOFF + GIE);          // Enter LPM0 w/ global interrupts
}

// Timer A0 interrupt service routine
#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A (void)
{

    // Decrement timeout counters
    ticks -= 1;
    if(ticks == 0) {
        ticks = TICKSPERSEC;
        seconds -= 1;
    }

    // Simple fade out near end
    if(seconds < gamma[intensity]) TACCR1 = seconds;

    // Debounce button press
    button_state = ( button_state << 1 ) | (P1IN & BIT3) | 0xE000;
    if(button_state == 0xF000) {
        seconds = TIMEOUT;
        intensity += 1;
        TACCR1 = gamma[intensity];
    }

    // Turn off led and sleep if timeout or button cycle
    if(intensity == 4 || seconds == 0) {
        ticks = TICKSPERSEC;          // Reset tick counter
        seconds = TIMEOUT;
        TACTL =  0x0000;           // Stop TimerA
        P1SEL &= ~BIT6;            // Disable special function for P1.6 (led)
        P1OUT &= ~BIT6;            // Set output low for P1.6 (led)
       intensity = 0;
       TACCR1 = gamma[intensity];
        __bis_SR_register_on_exit(LPM4_bits);  // Sleep
    }
}

// Port 1 interrupt service routine
// Just need this to wake up and re-enable led pin
// All button code handled by timer ISR
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
{
    P1IFG &= ~BIT3;          // P1.3 IFG cleared
    // Switch back to LMP0
    __bic_SR_register_on_exit(LPM4_bits);
    __bis_SR_register_on_exit(LPM0_bits);
    P1SEL |= BIT6;            // Set P1.6 back to special function timer output
    TACTL =  TASSEL_2 + MC_1; // Restart timer
}
