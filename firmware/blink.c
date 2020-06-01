#include <msp430.h>
#define TIMEOUT 3600        // Approx 1 hr (~1Mhz clock)

unsigned char intensity;    // Current intensity setting
unsigned int gamma[4];      // Convert intensity to gamma corrected clock
unsigned int button_state;  // De-bounce state of button
unsigned int seconds;       // Track on time
unsigned int ms;            // Splitting avoids long values

void stop() {
    TACTL = 0x0000;         // Stop TimerA
    P1SEL &= ~BIT6;         // Disable special function for P1.6 (led)
    P1OUT &= ~BIT6;         // Set output low for P1.6 (led)
}

void start() {
    seconds = TIMEOUT;          // Reset timeout variables
    ms = 1000;
    TACCR1 = gamma[intensity];
    P1SEL |= BIT6;              // Set P1.6 to special function timer output
    TACTL =  TASSEL_2 + MC_1;   // Start TimerA
}


void main(void)
{
    // Pre-calculated timer values for brightness using simple squared gamma curve
    gamma[0] = 70;             // 25% with gamma
    gamma[1] = 280;            // 50% with gamma
    gamma[2] = 629;            // 75% with gamma
    gamma[3] = 1118;           // 100%, gives 1khz clock on device tested

    WDTCTL = WDTPW + WDTHOLD;  // Stop WDT

    // Port configuration
    P1DIR = 0xF7;              // configure P1.6 as output, P1.3 as input
    P1OUT = BIT3;              // Select pull-up mode for P1.3
    P1REN |= BIT3;             // Enable internal pull-up/down resistors
    P2DIR = 0xFF;              // Set P2 for lowest power draw
    P2OUT = 0x00;

    // Enable and configure button interrupt
    P1IE |= BIT3;              // P1.3 IE
    P1IES |= BIT3;             // P1.3 Hi/lo edge
    P1IFG &= ~BIT3;            // P1.3 IFG cleared

    // Timer A configuration
    TACCR0 = gamma[3];         // Gamma [3] will be 100% duty cycle
    TACCTL0 = OUTMOD_4 + CCIE; // Toggle + IE
    TACCTL1 = CCIS0 + OUTMOD_7;

    intensity = 0;             // Default to 25% brightness
    start();                   // Enable and configure timer and defaults
    _BIS_SR(CPUOFF + GIE);     // Enter LPM0 w/ global interrupts
}

// Timer A0 interrupt service routine
#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A (void)
{
    // Decrement timeout counters
    ms -= 1;
    if(ms == 0) {
        seconds -= 1;
        ms = 1000;
    }

    // Simple linear fade out near end of timeout
    if(seconds < gamma[intensity]) TACCR1 = seconds;

    // Debounce button press
    button_state = ( button_state << 1 ) | (P1IN & BIT3) | 0xE000;
    if(button_state == 0xF000) {
        intensity += 1;
        start();
    }

    // Turn off led and sleep if timeout or button cycle
    if(intensity == 4 || seconds == 0) {
        stop();
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
    if(TACTL == 0x00){
        // Switch back to LMP0
        __bic_SR_register_on_exit(LPM4_bits);
        __bis_SR_register_on_exit(LPM0_bits);
        intensity = 0;
        start();
    }
}
