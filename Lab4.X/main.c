/*--------------- main.c ---------------
by: Mr. John A Nicholson 
    Mr. Ryan D Aldrich
    Mr. Taric S Alani
EECE.5520-201 Microprocessor Systems II & Embedded Systems
ECE Dept.
UMASS Lowell
Based on specification by Dr. Yan Luo:
lab2 bus pic-galileo.pdf 2016-09-21 18:08:32
https://piazza.com/class_profile/get_resource/irdp4redng6fb/itdgocs3n7f1si

PURPOSE
A PICkit 3 communicates with an analog to digital converter to operate
a digital night light. The PICkit communicates with an Intel Galileo
to print the ADC value to the screen. This program is primarily for the PICkit.
*/

/*
 * INTEL...............GP_7..GP_6..GP_5..GP_4
 * PIC.................RC5...RC4...RC3...RC2.
 * INTEL_WRITE:
 * MSG_GET             X.....X.....1.....1...
 * MSG_PING            X.....X.....0.....1...
 * MSG_RESET           X.....X.....1.....0...
 * PIC_WRITE:
 * ACK                 1.....1.....1.....1...
 * Data_part1/3        D3....D2....D1....D0..
 * Data_part2/3        D7....D6....D5....D4..
 * Data_part3/3        X.....X.....D9....D8..
 * 
 * Where X is unused, 1 = high, 0 = low, D9 through D0 are the upper most bits 
 * to the lower most bits of the ADC conversion results
 * 
 * 
 */


#include "mcc_generated_files/mcc.h"

/*
                         Main application
 */
#define MSG_GET 0x2;
#define MSG_PING 0x1;
#define MSG_RESET 0x0;
unsigned char operation = 0;
unsigned char strobe_last = 0; //initially the strobe is 0, the pic waits for
                               //the strobe to change

unsigned char debouncecheck(void)
{
    __delay_ms(1);
    return (PORTAbits.RA2);
}
void inputmode(void)
{
    LATC2 = 0;  // Close latches, may be redundant ATM
    LATC3 = 0;  
    LATC4 = 0;
    LATC5 = 0;
    TRISC2 = 1; // gpio digital input, high impedence mode
    TRISC3 = 1; // 
    TRISC4 = 1; // 
    TRISC5 = 1; // 
}

void outputmode(void)
{
    TRISC2 = 0; // gpio digital output
    TRISC3 = 0; // 
    TRISC4 = 0; // 
    TRISC5 = 0; // 
}
void main(void)
{
    // initialize the device
    SYSTEM_Initialize();
    inputmode();
    ADCC_Initialize();
    int value = 0; /// buffer to store ADC conversion result
    
    
    while (1)
    {   
        if ( ((PORTAbits.RA2)!=strobe_last) && (debouncecheck()!=strobe_last) ) //Check if the strobe has flipped to high
        {
            if ((PORTAbits.RA2)&&(operation!=0)) // Write mode
            {
                outputmode();
                switch(operation) /// write mode operations
                {
                    case 1: // ACK, general acknowledgement
                        LATC2 = 1;
                        LATC3 = 1;
                        LATC4 = 1;
                        LATC5 = 1;
                        operation = 0;
                        break;
                    case 2:  // Acknowledgement, begins msg_get chain
                        LATC2 = 1;
                        LATC3 = 1;
                        LATC4 = 1;
                        LATC5 = 1;
                        operation = 3;
                        break;
                    case 3:  // msg_get d3-d0
                        LATC2 = (ADRESL&1);
                        LATC3 = ((ADRESL>>1)&1);
                        LATC4 = ((ADRESL>>2)&1);
                        LATC5 = ((ADRESL>>3)&1);
                        operation = 4;
                        break;
                    case 4:  // msg_get d7-d4
                        LATC2 = ((ADRESL>>4)&1);
                        LATC3 = ((ADRESL>>5)&1);
                        LATC4 = ((ADRESL>>6)&1);
                        LATC5 = ((ADRESL>>7)&1);
                        operation = 5;
                        break;
                    case 5:  // msg_get d9-d8
                        LATC2 = (ADRESH&1);
                        LATC3 = ((ADRESH>>1)&1);
                        LATC4 = 0;
                        LATC5 = 0; 
                        operation = 0;
                        break;
                    default:
                        break;
                        
                }
            }
            else if ((PORTAbits.RA2)&&(operation==0)) // Read mode
            {
                __delay_ms(2);
                if((PORTCbits.RC3)&&(PORTCbits.RC2==0)) // gets MSG_RESET
                {
                    ADCC_StartConversion(channel_ANA0);     // open input ANA0 for ADC conversion


                    value = ADCC_GetConversionResult();

                    if(value < 460)    // if analog voltage is below 1.14V 
                    {                                       // turn off LED
                        IO_RA1_SetLow();
                    }

                    if(value > 490)    // if analog voltage is above 1.32V
                    {                                       // turn on LED
                        IO_RA1_SetHigh();
                    }
                    operation = 1;
                }
                else if((PORTCbits.RC3==0)&&(PORTCbits.RC2)) // gets MSG_PING
                {
                    operation = 1;
                }
                else if((PORTCbits.RC3)&&(PORTCbits.RC2)) // gets MSG_GET
                {
                    operation = 2;
                }
                else
                {
                    operation = 0; // you got an error, wait for the next high flip and read again
                }
            }
            
            else // Flip low, switch to input
            {
            inputmode();    
            }
            strobe_last=PORTAbits.RA2; // set the last strobe to be the current strobe value
        }
      if(PORTAbits.RA2==0)
      {
        inputmode();
      }     
        
    }  
}
/**
 End of File
*/


