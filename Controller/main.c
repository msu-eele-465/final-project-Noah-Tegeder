#include "intrinsics.h"
#include <msp430.h>

void Init_GPIO();
char message[] = "Hello World";
unsigned int pos = 0;

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;                 // Stop watchdog timer

    P2DIR |= BIT7;
    P2OUT &= ~BIT7;

    PM5CTL0 &= ~LOCKLPM5;                     // Disable the GPIO power-on default high-impedance mode
                                            // to activate 1previously configured port settings

    // Configure UART pins
    P4SEL1 &= ~BIT3;
    P4SEL1 &= ~BIT2;
    P4SEL0 |= BIT2 | BIT3;                    // set 2-UART pin as second function

    // Configure UART
    UCA1CTLW0 |= UCSWRST;
    UCA1CTLW0 |= UCSSEL__ACLK;                    // set ACLK as BRCLK

    UCA1BRW = 3;                              // INT(32768/4800)
    UCA1MCTLW = 0x9200;

    UCA1CTLW0 &= ~UCSWRST;                    // Initialize eUSCI
    UCA1IE |= UCRXIE;                         // Enable USCI_A1 RX interrupt
    __no_operation();                         // For debugger
    __enable_interrupt();

    while(1)
    {
    }
}


#pragma vector=EUSCI_A1_VECTOR
__interrupt void EUSCI_A1_RX_ISR(void)
{
    if(UCA1RXBUF == 't')
    {

        P2OUT ^= BIT7;
        UCA1TXBUF = message[pos++];
    }
    UCA1IFG &= ~UCRXIFG;
    UCA1IFG &= ~UCTXCPTIFG;
}

