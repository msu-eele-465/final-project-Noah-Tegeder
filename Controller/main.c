#include "intrinsics.h"
#include "msp430fr2355.h"
#include <msp430.h>

int main(void)
{
    WDTCTL= WDTPW | WDTHOLD;

    UCA0CTLW0 |= UCSWRST;
    UCA0CTLW0 |= UCSSEL__ACLK;

    UCA0BRW = 3;
    UCA0MCTLW |= 0x9200;

    P1SEL1 &= ~BIT6;
    P1SEL0 |= BIT6;
    P1SEL1 &= ~BIT7;
    P1SEL0 |= BIT7;

    PM5CTL0 &= ~LOCKLPM5;

    UCA0CTLW0 &= ~UCSWRST;

    while(1)
    {
        UCA0TXBUF = 'A';
        __delay_cycles(10000);
    }

}
