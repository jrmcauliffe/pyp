#include <msp430.h>				

/**
 * blink.c
 */


// Simple gamma adjustment
int gamma(int percentage) {
    return (181 / 100 ) * percentage * (181/100) * percentage; // calculate gamma using basic squared
}

void main(void)
{
	WDTCTL = WDTPW | WDTHOLD;		// stop watchdog timer
	P1DIR |= 0x40;					// configure P1.6 as output
    P1SEL |= 0x40;                  // configure P1.6 as special
	volatile unsigned int i;		// volatile to prevent optimization
	volatile unsigned int p;

	TACTL = 0x210;
    TACCR0 = 8000;
    TACCR1 = 0;
    TACCTL0 = 0x90;
    TACCTL1 = 0x10E0;



	while(1)
	{
	    TACCR1 = (TACCR1 % 8000) + 200;
	    for(p = 0; p<=100; p=p+20){
            TACCR1 = gamma(p);
	        for(i=100000; i>0; i--); //delay

	    }
	}
}
