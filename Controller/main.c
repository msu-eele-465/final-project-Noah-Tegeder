#include "intrinsics.h"
#include <msp430.h>
#include <math.h>

void adc_config();
void timer_setup();
void i2c_config();
void uart_config();
void recieve_humidity();
void recieve_temp();
void send_temp();
void humidity_command();
void send_humidity();
void recieve_wind();

const char hString[] = "Humidity: ";
const char tString[] = "Temperature: ";
char i2c_source;
unsigned char data = 0;

char message[] = "Hello World";
unsigned int pos = 0;

int msb_status;
int thousands;
int hundreds;
int tens;
int ones;

unsigned char recieve = 0;

// Variables used in humidity measurement
long humidity_out;
float humidity;
int real_humidity;

// Variables used in temperature measurment
unsigned char msb_bank;
short plant_out;
float plant_temp;
int real_plant;
int avg_plant;
int k = 0;
int plant[];
int temp_status = 0;

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;                 // Stop watchdog timer

    P2DIR |= BIT7;
    P2OUT &= ~BIT7;

    i2c_config();
    timer_setup();
    //adc_config();

    PM5CTL0 &= ~LOCKLPM5;                     // Disable the GPIO power-on default high-impedance mode
                                            // to activate 1previously configured port settings
    uart_config();

    UCA1CTLW0 &= ~UCSWRST;                    // Initialize eUSCI
    UCA1IE |= UCRXIE;                         // Enable USCI_A1 RX interrupt
    __no_operation();                         // For debugger
    __enable_interrupt();

    while(1)
    {
        if(recieve == 'h')
        {
            humidity_command();
            recieve_humidity();
            send_humidity();
            recieve = 0;
        }
        if(recieve == 't')
        {
            send_temp();
            recieve = 0;
        }
        if(recieve == 'w')
        {
            //recieve_wind();
        }
        if(temp_status = 1)
        {
            recieve_temp();
            temp_status = 0;
        }
    }
}


#pragma vector=EUSCI_A1_VECTOR
__interrupt void EUSCI_A1_RX_ISR(void)
{
    recieve = UCA1RXBUF;
    P2OUT ^= BIT7;
    UCA1IFG &= ~UCRXIFG;
    UCA1IFG &= ~UCTXCPTIFG;
}

#pragma vector = EUSCI_B0_VECTOR
__interrupt void EUSCI_B0_I2C_ISR(void){
  switch(__even_in_range(UCB0IV, USCI_I2C_UCBIT9IFG))
  {
    case USCI_NONE: break;                  // Vector 0: No interrupts
    case USCI_I2C_UCALIFG: break;           // Vector 2: ALIFG
    case USCI_I2C_UCNACKIFG:                // Vector 4: NACKIFG
                            UCB0CTL1 |= UCTXSTT;                  // I2C start condition
                            break;
    case USCI_I2C_UCSTTIFG: break;          // Vector 6: STTIFG
    case USCI_I2C_UCSTPIFG: break;          // Vector 8: STPIFG
    case USCI_I2C_UCRXIFG3: break;          // Vector 10: RXIFG3
    case USCI_I2C_UCTXIFG3: break;          // Vector 14: TXIFG3
    case USCI_I2C_UCRXIFG2: break;          // Vector 16: RXIFG2
    case USCI_I2C_UCTXIFG2: break;          // Vector 18: TXIFG2
    case USCI_I2C_UCRXIFG1: break;          // Vector 20: RXIFG1
    case USCI_I2C_UCTXIFG1: break;          // Vector 22: TXIFG1
    case USCI_I2C_UCRXIFG0:                 // Vector 24: RXIFG0
                            if(i2c_source == 'T')
                            {
                                if(msb_status == 1)
                                {
                                    msb_bank = UCB0RXBUF;
                                    msb_status = 0;
                                }
                                else
                                {
                                    plant_out = (msb_bank << 8) | UCB0RXBUF;
                                    plant_out = plant_out >> 3;
                                    plant_temp = plant_out * .0625;
                                    real_plant = 100*plant_temp;
                                }
                            }
                            else if(i2c_source == 'H')
                            {
                                if(msb_status == 3)
                                {
                                    humidity_out = UCB0RXBUF;
                                    msb_status--;
                                }
                                else if(msb_status == 2)
                                {
                                    humidity_out = UCB0RXBUF;
                                    msb_status--;
                                }
                                else if(msb_status == 1)
                                {
                                    humidity_out = (humidity_out << 8) | UCB0RXBUF;
                                    msb_status--;
                                }
                                else if(msb_status == 0)
                                {
                                    humidity_out = (humidity_out << 8) | UCB0RXBUF;
                                    humidity_out = humidity_out >> 4;
                                    humidity = (humidity_out/pow(2,20));
                                    humidity = humidity*100;
                                    real_humidity = humidity*100;
                                }
                            }
                            break;
    case USCI_I2C_UCTXIFG0:                 // Vector 26: TXIFG0
                            if(i2c_source == 'C')
                            {
                                if(msb_status == 3)
                                {
                                    UCB0TXBUF = 0xAC;
                                    msb_status--;
                                }
                                else if(msb_status == 2)
                                {
                                    UCB0TXBUF = 0x33;
                                    msb_status--;
                                }
                                else
                                {
                                    UCB0TXBUF = 0x00;
                                    msb_status = 0;
                                    i2c_source = 0;
                                }
                            }
                            else
                            {
                                UCB0TXBUF = data;
                            }
                            break;
    case USCI_I2C_UCBCNTIFG: break;                // Vector 28: BCNTIFG
    case USCI_I2C_UCCLTOIFG: break;         // Vector 30: clock low timeout
    case USCI_I2C_UCBIT9IFG: break;         // Vector 32: 9th bit
    default: break;
  }
}
/*
// ADC interrupt service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=ADC_VECTOR
__interrupt void ADC_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(ADC_VECTOR))) ADC_ISR (void)
#else
#error Compiler not supported!
#endif
{
    calc = 0;
    ADC_Result = 0;
    switch(__even_in_range(ADCIV,ADCIV_ADCIFG))
    {
        case ADCIV_NONE:
            break;
        case ADCIV_ADCOVIFG:
            break;
        case ADCIV_ADCTOVIFG:
            break;
        case ADCIV_ADCHIIFG:
            break;
        case ADCIV_ADCLOIFG:
            break;
        case ADCIV_ADCINIFG:
            break;
        case ADCIV_ADCIFG:
            ADC_Result = ADCMEM0;
            calc = (ADC_Result*3.3)/4096;
            ambient_temp = (calc-1.8663)/(-0.01169);
            real_temp = 100*ambient_temp;
            if(n != 4)
            {
                ambient[n] = real_temp;
            }
            else if(n == 4){
                n = 0;
                ambient[0] = real_temp;
            }
            n++;
            avg_ambient = (ambient[0] + ambient[1] + ambient[2] + ambient[3])/4;
            __bic_SR_register_on_exit(LPM0_bits);            // Clear CPUOFF bit from LPM0          
            break;
        default:
            break;
    }
}
*/
// Timer B0 interrupt service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = TIMER0_B0_VECTOR
__interrupt void Timer_B (void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(TIMER0_B0_VECTOR))) Timer_B (void)
#else
#error Compiler not supported!
#endif
{
    temp_status = 1;
    //ADCCTL0 |= ADCENC | ADCSC;                                    // Sampling and conversion start
}


void i2c_config()
{
    // Configure USCI_B0 for I2C mode
    
    UCB0CTLW0 |= UCSWRST;                   // Software reset enabled

    UCB0CTLW0 |= UCSSEL__SMCLK;
    UCB0BRW = 10;
    
    UCB0CTLW0 |= UCMODE_3 | UCMST | UCTR;   // I2C mode, Master mode, TX
    UCB0CTLW1 |= UCASTP_2;                  // Automatic stop generated
                                            // after UCB0TBCNT is reached

    UCB0TBCNT = 0x0001;                     // number of bytes to be sent
    UCB0I2CSA = 0x0B;                       // Slave address
                                            // Three slaves are being used, both ar 0x0B
                                            // When one of those keys is pressed, update slave address, send data, set back to 0x00  
    // I2C pins, 1.2 SDA, 1.3 SCL
    P1SEL1 &= ~(BIT2 & BIT3);
    P1SEL0 |= BIT2 | BIT3;     

    // Disable reset mode
    UCB0CTLW0 &= ~UCSWRST;

    // I2C interrupt
    UCB0IE |= UCTXIE0 | UCRXIE0;
}

void timer_setup()
{
    // Setup Timer B0
    TB0CTL |= TBCLR;  // Clear timer and dividers
    TB0CTL |= TBSSEL__ACLK;  // Use ACLK
    TB0CTL |= MC__UP;  // Up counting mode
    TB0CCR0 = 16384;    // Compare value
    //TB0CCR1 = 16384;    // CCR1 value

    // Set up timer compare IRQs
    TB0CCTL0 &= ~CCIFG;  // Clear CCR0 flag
    TB0CCTL0 |= CCIE;  // Enable flag

    // Set up timer compare IRQs
    //TB0CCTL1 &= ~CCIFG;  // Clear CCR1 flag
    //TB0CCTL1 |= CCIE;  // Enable flag
}

void adc_config()
{
    // Configure ADC12
    P1SEL0 |= BIT1;
    P1SEL1 |= BIT1;
    ADCCTL0 |= ADCSHT_2 | ADCON;                             // ADCON, S&H=16 ADC clks
    ADCCTL1 |= ADCSHP;                                       // ADCCLK = MODOSC; sampling timer
    ADCCTL2 &= ~ADCRES;                                      // clear ADCRES in ADCCTL
    ADCCTL2 |= ADCRES_2;                                     // 12-bit conversion results
    ADCMCTL0 |= ADCINCH_1;                                     // A1 ADC input select
    ADCIE |= ADCIE0; 
}

void uart_config()
{
    // Configure UART pins
    P4SEL1 &= ~BIT3;
    P4SEL1 &= ~BIT2;
    P4SEL0 |= BIT2 | BIT3;      // set 2-UART pin as second function

    // Configure UART
    UCA1CTLW0 |= UCSWRST;
    UCA1CTLW0 |= UCSSEL__ACLK;  // set ACLK as BRCLK

    UCA1BRW = 3;                // Baud Rate of 9600
    UCA1MCTLW = 0x9200;
}

void recieve_temp()
{
    msb_status = 1;
    i2c_source = 'T';
    UCB0CTLW0 &= ~UCTR;
    UCB0I2CSA = 0x48;
    UCB0TBCNT = 0x02;
    while (UCB0CTL1 & UCTXSTP);
    UCB0CTL1 |= UCTXSTT;
    __delay_cycles(1000);
    UCB0I2CSA = 0x0B;
    UCB0TBCNT = 0x01;
    UCB0CTLW0 |= UCTR;
}

void recieve_humidity()
{
    msb_status = 3;
    i2c_source = 'H';
    UCB0CTLW0 &= ~UCTR;
    UCB0I2CSA = 0x38;
    UCB0TBCNT = 0x04;
    while (UCB0CTL1 & UCTXSTP);
    UCB0CTL1 |= UCTXSTT;
    __delay_cycles(1000);
    UCB0I2CSA = 0x0B;
    UCB0TBCNT = 0x01;
    UCB0CTLW0 |= UCTR;
}

void humidity_command()
{
    UCB0I2CSA = 0x38;
    UCB0TBCNT = 0x03;
    i2c_source = 'C';
    msb_status = 3;
    while (UCB0CTL1 & UCTXSTP);
    UCB0CTL1 |= UCTXSTT;
    __delay_cycles(1000);
    UCB0I2CSA = 0x0B;
    UCB0TBCNT = 0x01;
    UCB0CTLW0 |= UCTR;
}

void send_humidity()
{
    int i = 0;
    for(i = 0; i<sizeof(hString);i++)
    {
        UCA1TXBUF = hString[i];
        __delay_cycles(1000);
    }
    thousands = (real_humidity/1000) + 48;
    real_humidity %= 1000;
    hundreds = (real_humidity/100) + 48;
    real_humidity %= 100;
    tens = (real_humidity/10) + 48;
    ones = (real_humidity%10) + 48;
    UCA1TXBUF = thousands;
    __delay_cycles(1000);
    UCA1TXBUF = hundreds;
    __delay_cycles(1000);
    UCA1TXBUF = '.';
    __delay_cycles(1000);
    UCA1TXBUF = tens;
    __delay_cycles(1000);
    UCA1TXBUF = ones;
    __delay_cycles(1000);
    UCA1TXBUF = '%';
    __delay_cycles(1000);
    UCA1TXBUF = '\n';
    __delay_cycles(1000);

    data = 0xAD;
    UCB0CTLW0 |= UCTXSTT;
    while (UCB0CTL1 & UCTXSTP);
    __delay_cycles(2000);
    data = thousands;
    UCB0CTLW0 |= UCTXSTT;
    while (UCB0CTL1 & UCTXSTP);
    __delay_cycles(2000);
    data = hundreds;
    UCB0CTLW0 |= UCTXSTT;
    while (UCB0CTL1 & UCTXSTP);
    __delay_cycles(2000);
    data = 0b00101110;
    UCB0CTLW0 |= UCTXSTT;
    while (UCB0CTL1 & UCTXSTP);
    __delay_cycles(2000);
    data = tens;
    UCB0CTLW0 |= UCTXSTT;
    while (UCB0CTL1 & UCTXSTP);
    __delay_cycles(2000);
}

void send_temp()
{   
    UCB0I2CSA = 0x0B;
    real_plant = 100*plant_temp;
    thousands = (real_plant/1000) + 48;
    real_plant %= 1000;
    hundreds = (real_plant/100) + 48;
    real_plant %= 100;
    tens = (real_plant/10) + 48; 
    ones = (real_plant%10) + 48;

    int i = 0;
    for(i = 0; i<sizeof(tString);i++)
    {
        UCA1TXBUF = tString[i];
        __delay_cycles(1000);
    }
    UCA1TXBUF = thousands;
    __delay_cycles(10000);
    UCA1TXBUF = hundreds;
    __delay_cycles(10000);
    UCA1TXBUF = '.';
    __delay_cycles(10000);
    UCA1TXBUF = tens;
    __delay_cycles(10000);
    UCA1TXBUF = ones;
    __delay_cycles(10000);
    UCA1TXBUF = 'C';
    __delay_cycles(10000);
    UCA1TXBUF = '\n';
    __delay_cycles(10000);

    data = 0xAC;
    UCB0CTLW0 |= UCTXSTT;
    while (UCB0CTL1 & UCTXSTP);
    __delay_cycles(2000);
    data = thousands;
    UCB0CTLW0 |= UCTXSTT;
    while (UCB0CTL1 & UCTXSTP);
    __delay_cycles(2000);
    data = hundreds;
    UCB0CTLW0 |= UCTXSTT;
    while (UCB0CTL1 & UCTXSTP);
    __delay_cycles(2000);
    data = 0b00101110;
    UCB0CTLW0 |= UCTXSTT;
    while (UCB0CTL1 & UCTXSTP);
    __delay_cycles(2000);
    data = tens;
    UCB0CTLW0 |= UCTXSTT;
    while (UCB0CTL1 & UCTXSTP);
    __delay_cycles(2000);
}