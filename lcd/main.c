#include "intrinsics.h"
#include <msp430.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

unsigned char RXData = 0;

char temp_set = 0;
char wind_set = 0;
char humid_set = 0;

#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))

void lcd_init()
{
    P1DIR |= BIT1;  
    P1OUT &= ~BIT1;  
    P1DIR |= BIT0;  
    P1OUT &= ~BIT0;  
    P1DIR |= BIT4;  
    P1OUT &= ~BIT4;  
    P1DIR |= BIT5;  
    P1OUT &= ~BIT5;
    P1DIR |= BIT6;
    P1OUT &= ~BIT6;  
    P1DIR |= BIT7;  
    P1OUT &= ~BIT7;  
    P2DIR |= BIT0;  
    P2OUT &= ~BIT0;
}

// Brings cursor to last position on LCD screen
void position(int steps)
{
    return_home();
    int i = 0;
    for(i = 0; i<steps; i++)
    {
        cursor_right();
    } 
}

void lcd_setup()
{
    lcd_write(0b00100010);
    __delay_cycles(500);
    DB5(0);
    DB7(1);
    DB6(1);
    DB4(0);
    P1OUT |= BIT6;
    __delay_cycles(1000);
    P1OUT &= ~BIT6;
    __delay_cycles(500);
    lcd_write(0b00001111);
    lcd_write(0b00000001);
    __delay_cycles(10000);
    lcd_write(0b00000110);
}

void DB7(int status)
{
    
    if(status == 1)
    {
        P1OUT |= BIT1; 
    }
    else 
    {
        P1OUT &= ~BIT1;
    }
}

/*
Function to change output of DB6 depending on int input.
*/
void DB6(int status)
{
    if(status == 1)
    {
        P1OUT |= BIT0; 
    }
    else 
    {
        P1OUT &= ~BIT0;
    }
}

/*
Function to change output of DB5 depending on int input.
*/
void DB5(int status)
{
    if(status == 1)
    {
        P1OUT |= BIT4; 
    }
    else 
    {
        P1OUT &= ~BIT4;
    }
}

/*
Function to change output of DB4 depending on int input.
*/
void DB4(int status)
{
    if(status == 1)
    {
        P1OUT |= BIT5; 
    }
    else 
    {
        P1OUT &= ~BIT5;
    }
}

void cursor_right()
{
    P2OUT &= ~BIT0;
    lcd_write(0b00010100);
    __delay_cycles(500);
    P2OUT |= BIT0;
}

void clear_cgram()
{
    P2OUT &= ~BIT0;
    lcd_write(0b00000001);
    P2OUT |= BIT0;
}

void return_home()
{
    P2OUT &= ~BIT0;
    lcd_write(0b00000010);
    P2OUT |= BIT0;
}


void lcd_write(int in)
{
        __delay_cycles(500);
        if(CHECK_BIT(in,7) != 0){
            DB7(1);
        } else{
            DB7(0);
        }
        if(CHECK_BIT(in,6) != 0){
            DB6(1);
        } else{
            DB6(0);
        }
        if(CHECK_BIT(in,5) != 0){
            DB5(1);
        } else{
            DB5(0);
        }
        if(CHECK_BIT(in,4) != 0){
            DB4(1);
        } else{
            DB4(0);
        }
        __delay_cycles(500);
        P1OUT |= BIT6;
        __delay_cycles(1000);
        P1OUT &= ~BIT6;
        __delay_cycles(500);
        if(CHECK_BIT(in,3) != 0){
            DB7(1);
        } else{
            DB7(0);
        }
        if(CHECK_BIT(in,2) != 0){
            DB6(1);
        } else{
            DB6(0);
        }
        if(CHECK_BIT(in,1) != 0){
            DB5(1);
        } else{
            DB5(0);
        }
        if(CHECK_BIT(in,0) != 0){
            DB4(1);
        } else{
            DB4(0);
        }   
        __delay_cycles(500);
        P1OUT |= BIT6;
        __delay_cycles(1000);
        P1OUT &= ~BIT6;
        __delay_cycles(500);
}

void lcd_print(const char input[], int length)
{
    int i = 0;
    for(i = 0; i < length; i++){
        lcd_write(input[i]);
    }
}

int main(void) { 
    WDTCTL = WDTPW | WDTHOLD;  // Stop watchdog timer

    // Configure Pins for I2C
    P1SEL0 |= BIT2 | BIT3;                                // I2C pins

    // Disable the GPIO power-on default high-impedance mode
    // to activate previously configured port settings
    PM5CTL0 &= ~LOCKLPM5;

    // Configure USCI_B0 for I2C mode
    UCB0CTLW0 |= UCSWRST;                                 //Software reset enabled
    UCB0CTLW0 |= UCMODE_3;                                //I2C slave mode, SMCLK
    UCB0I2COA0 = 0x0B | UCOAEN;                           //SLAVE0 own address is 0x0A| enable
    UCB0CTLW0 &=~UCSWRST;                                 //clear reset register


    //__bis_SR_register(LPM0_bits | GIE);                   // Enter LPM0 w/ interrupts
    __no_operation();

    PM5CTL0 &= ~LOCKLPM5;  // Enable GPIO


    lcd_init();
    lcd_setup();
    lcd_setup();
    __delay_cycles(500);
    clear_cgram();
    __delay_cycles(1000);
    return_home();
    __enable_interrupt();  // Enable global interrupts
    UCB0IE |=  UCRXIE0;      //receive interrupt enable

    while(1)
    {
    }
}

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = USCI_B0_VECTOR
__interrupt void USCIB0_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCI_B0_VECTOR))) USCIB0_ISR (void)
#else
#error Compiler not supported!
#endif
{
                                       // SLAVE0
        RXData = UCB0RXBUF;                              // Get RX data
        if(RXData == 0xAD)
        {
            clear_cgram();
            position(14);
            lcd_write('%');
            return_home();
            lcd_print("Humidity: ",10);
            humid_set = 3;
        }
        if(humid_set != 0 && RXData != 0xAD && RXData != 0xAC && RXData != 0xAB)
        {
            if(humid_set == 2)
            {
                lcd_write(RXData);
                lcd_write('.');
            }
            else{
                lcd_write(RXData);
            }
            humid_set--;
        }
        if(RXData == 0xAC)
        {
            clear_cgram();
            position(10);
            lcd_write(0b11011111);
            lcd_write(0b01000011);
            return_home();
            lcd_print("Temp: ",6);
            temp_set = 3;
        }
        if(temp_set != 0 && RXData != 0xAC && RXData != 0xAD && RXData != 0xAB)
        {
            if(temp_set == 2)
            {
                lcd_write(RXData);
                lcd_write('.');
            }
            else{
                lcd_write(RXData);
            }
            temp_set--;
        }
        if(RXData == 0xAB)
        {
            clear_cgram();
            position(10);
            lcd_write('m');
            lcd_write('/');
            lcd_write('s');
            return_home();
            lcd_print("Wind: ",6);
            wind_set = 3;
        }
        if(wind_set != 0 && RXData != 0xAC && RXData != 0xAD && RXData != 0xAB)
        {
            if(wind_set == 2)
            {
                lcd_write(RXData);
                lcd_write('.');
            }
            else{
                lcd_write(RXData);
            }
            wind_set--;
        }
        P2OUT ^= BIT6;
        __bic_SR_register_on_exit(LPM0_bits);                       // Vector 24: RXIFG0 break;
    

}