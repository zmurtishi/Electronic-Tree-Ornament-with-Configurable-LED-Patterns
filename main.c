
// PIC16F1823 Configuration Bit Settings

// 'C' source line config statements

// CONFIG1
#pragma config FOSC = INTOSC    // Oscillator Selection (INTOSC oscillator: I/O function on CLKIN pin)
#pragma config WDTE = OFF       // Watchdog Timer Enable (WDT disabled)
#pragma config PWRTE = ON      // Power-up Timer Enable (PWRT disabled)
#pragma config MCLRE = ON       // MCLR Pin Function Select (MCLR/VPP pin function is MCLR)
#pragma config CP = OFF         // Flash Program Memory Code Protection (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Memory Code Protection (Data memory code protection is disabled)
#pragma config BOREN = OFF       // Brown-out Reset Enable (Brown-out Reset enabled)
#pragma config CLKOUTEN = OFF   // Clock Out Enable (CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin)
#pragma config IESO = ON        // Internal/External Switchover (Internal/External Switchover mode is enabled)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor is enabled)

// CONFIG2
#pragma config WRT = OFF        // Flash Memory Self-Write Protection (Write protection off)
#pragma config PLLEN = OFF         // PLL Enable (4x PLL enabled)
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will cause a Reset)
#pragma config BORV = LO        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), low trip point selected.)
#pragma config DEBUG = OFF      // In-Circuit Debugger Mode (In-Circuit Debugger disabled, ICSPCLK and ICSPDAT are general purpose I/O pins)
#pragma config LVP = ON         // Low-Voltage Programming Enable (Low-voltage programming enabled)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>

#define SOLID 0
#define BLINK 1
#define BLINK_FASTER 2
#define TWINKLE 3
#define TWINKLE_STAR 4
#define LIT_STAR 5
#define LOW_POWER 6

#define NUM_STATES 5

#define DEPRESSED 0
#define PRESSED 1
//
#define FOUR_HOURS 115200
#define TWENTY_FOUR_HOURS 691200
//#define FOUR_HOURS 40
//#define TWENTY_HOURS 80
#define STATE_ADDR 0x00
#define NUM_ELEMENTS 256

uint8_t state = 0;
uint8_t previous_state = 0;
uint32_t count = 0;
uint8_t seconds = 0;
uint8_t pressed = 0;
uint8_t index = 0;
uint8_t pwm_on = 0;
uint8_t last_PORTC = 0;
uint8_t last_PSTR1CON = 0;
uint8_t last_CCPR1L = 0;
uint8_t last_CCP1CONbits_DC1B = 0; 
uint8_t lastRC4 = 0;
uint8_t lastRC5 = 0;

const uint16_t rng[256] = {383,877,478,300,92,723,473,129,432,934,915,83,735,878,102,340,378,443,634,350,280,500,840,471,939,780,1016,238,996,913,118,308,507,511,29,672,322,746,988,776,352,787,944,985,941,263,564,192,608,47,464,891,411,143,773,210,369,206,243,210,659,795,219,962,965,494,385,597,496,559,204,269,62,959,193,337,294,980,208,166,936,561,934,203,926,1015,469,929,329,940,334,504,397,956,263,202,624,202,20,196,955,623,510,322,266,6,812,591,226,168,837,544,831,54,712,734,571,288,967,458,57,938,410,723,598,130,353,468,826,910,272,870,889,930,884,455,609,323,939,296,93,851,506,984,788,646,980,35,250,860,257,533,446,627,299,873,282,105,72,873,382,69,156,652,24,941,85,602,653,296,1021,501,980,462,417,364,566,905,580,681,860,239,900,463,943,327,39,376,32,379,567,876,686,24,387,288,68,196,859,158,1006,827,75,619,527,876,200,733,250,654,193,346,760,975,203,745,30,133,1004,731,388,534,462,405,772,100,347,556,122,844,661,773,782,970,623,105,865,151,56,819,946,445,112,861,206,549,465,271,540,455,72,106,1010,66,601,739};

void interruptsEnable(void){
    INTCONbits.PEIE = 1;
    INTCONbits.GIE = 1;
    PIE1bits.TMR1IE = 1;
}

void interruptsEnable2(void){
    INTCONbits.PEIE = 1;
    INTCONbits.GIE = 1;
    PIE1bits.TMR1IE = 1;
}

void interruptsDisable(void){
    INTCONbits.PEIE = 0;
    INTCONbits.GIE = 0;
    PIE1bits.TMR1IE = 0;
}

void writeEEPROM(uint8_t data, uint8_t address){
    interruptsDisable();
    EEDATA = data;
    EEADR = address;
    EECON1bits.EEPGD = 0;
    EECON1bits.WREN = 1;
    EECON1bits.CFGS = 0;
    EECON1bits.EEPGD = 0;
    EECON2 = 0x55;
    EECON2 = 0x0AA;
    EECON1bits.WR = 1;
    while(EECON1bits.WR);
    EECON1bits.WREN = 0;
    interruptsEnable();
}

uint8_t readEEPROM(uint8_t address){
    uint8_t data;
    EECON1bits.EEPGD = 0;
    EECON1bits.WREN = 0; 
    EECON1bits.CFGS = 0;
    EECON1bits.EEPGD = 0;
    EEADR = address;
    EECON1bits.RD = 1;
    while(EECON1bits.RD);
    data = EEDATA;
    return data;
}

void pwmInit(void){
    index = 0;    
    PIR1bits.TMR2IF = 0;
    TMR2 = 0;
    CCP1CONbits.P1M = 0;
    CCP1CONbits.CCP1M = 0b1100;
    PR2 = 255;
    CCPR1L = 0b11111111;
    CCP1CONbits.DC1B = (uint8_t)0b11; 
    T2CONbits.T2CKPS = 0b00;
    PIE1bits.TMR2IE = 1;
    T2CONbits.TMR2ON = 1;
}

void pwmInit2(void){
    index = 0;    
    PIR1bits.TMR2IF = 0;
    TMR2 = 0;
    CCP1CONbits.P1M = 0;
    CCP1CONbits.CCP1M = 0b1100;
    PR2 = 255;
    CCPR1L = 0b11111111;
    CCP1CONbits.DC1B = (uint8_t)0b11; 
    T2CONbits.T2CKPS = 0b00;
    PIE1bits.TMR2IE = 1;
    T2CONbits.TMR2ON = 1;    
}

void disablePWM(void){
    CCP1CONbits.CCP1M = 0;
    CCPR1L = 0;
    CCP1CONbits.DC1B = 0;
    T2CONbits.TMR2ON = 0;
    PIE1bits.TMR2IE = 0;
    PIR1bits.TMR2IF = 0;
}

void enterLowPowerMode(void){   
    if(pwm_on){ 
        last_PSTR1CON = PSTR1CON;
        last_CCPR1L = CCPR1L;
        last_CCP1CONbits_DC1B = CCP1CONbits.DC1B;
        disablePWM();      
    }
    previous_state = state;
    state = LOW_POWER;
    lastRC4 = PORTCbits.RC4;
    lastRC5 = PORTCbits.RC5;    
    PORTCbits.RC4 = 0;
    PORTCbits.RC5 = 0;
}

void exitLowPowerMode(void){
    if(pwm_on){
        pwmInit();
        PSTR1CON = last_PSTR1CON;
        CCPR1L = last_CCPR1L;
        CCP1CONbits.DC1B = last_CCP1CONbits_DC1B;   
    }   
    PORTCbits.RC4 = lastRC4;
    PORTCbits.RC5 = lastRC5;
    state = previous_state;
    count = 0;
}

void __interrupt() isr(void){
    //PORTCbits.RC1 = 1;
    //PORTCbits.RC1 = 0;
    if(PIR1bits.TMR1IF == 1)
    {
        TMR1 = TMR1 + 61445; //5 cycles needed to execute this instructions   
        count++;
        //PORTCbits.RC2 = 1;
        PIR1bits.TMR1IF = 0;
        if(count == FOUR_HOURS){
            enterLowPowerMode();
        }
        else if(count == TWENTY_FOUR_HOURS){
            exitLowPowerMode();
        }
        if(PORTCbits.RC3 == 0){
            if(pressed == DEPRESSED){
                pressed = PRESSED;
                if(state == SOLID){
                    PORTCbits.RC4 = 1;
                    PORTCbits.RC5 = 1;
                    state = BLINK;
                }
                else if(state == BLINK){
                    PORTCbits.RC4 = 1;
                    PORTCbits.RC5 = 0;                    
                    state = BLINK_FASTER;
                }
                else if(state == BLINK_FASTER){
                    pwmInit();
                    pwm_on = 1;
                    PSTR1CON = 0b00000011;
                    state = TWINKLE;
                }            
                else if(state == TWINKLE){
                    PSTR1CON = 0b00000010;
                    PORTCbits.RC5 = 0;
                    state = TWINKLE_STAR;
                }   
                else if(state == TWINKLE_STAR){
                    PSTR1CON = 0b00000001;
                    CCPR1L = 0b00000111;
                    CCP1CONbits.DC1B = (uint8_t)0b11;                     
                    PORTCbits.RC4 = 1;
                    state = LIT_STAR;
                }
                else if(state == LIT_STAR){
                    disablePWM();
                    pwm_on = 0;
                    state = SOLID;
                }             
                if(state == LOW_POWER){
                    exitLowPowerMode();
                }                
                else{
                    writeEEPROM(state, STATE_ADDR);
                }
            }
        }
        else{
            pressed = DEPRESSED;
            if(state == SOLID){
                    PORTCbits.RC4 = 1;
                    PORTCbits.RC5 = 1;
            }
            else if(state == BLINK){
                if((count % 8) == 0){
                    PORTCbits.RC4 = PORTCbits.RC4 ^ 1;
                    PORTCbits.RC5 = PORTCbits.RC5 ^ 1;
                }
            }    
            else if(state == BLINK_FASTER){
                 if((count % 4) == 0){
                    PORTCbits.RC4 = PORTCbits.RC4 ^ 1;
                    PORTCbits.RC5 = PORTCbits.RC5 ^ 1;
                }                                    
            }
            else if((state == TWINKLE) || (state == TWINKLE_STAR)){
                index++;
                CCPR1L = (uint8_t)(rng[index%NUM_ELEMENTS]>>2);
                CCP1CONbits.DC1B = (uint8_t)(rng[index%NUM_ELEMENTS]&0b11);   
            }   
        }
        //PORTCbits.RC2 = 0;
    }
}

void timer1Enable(void){
    T1CONbits.TMR1CS = 0b10;
    T1CONbits.T1CKPS = 0b00;
    while(OSCSTATbits.T1OSCR == 0);
    T1CONbits.T1OSCEN = 1;
    TMR1 = 61440;
    T1CONbits.T1CKPS = 0b00;
    T1CONbits.TMR1ON = 1;
    PIR1bits.TMR1IF = 0;
    T1CONbits.nT1SYNC = 1;
}

void mcuInit(void){
    OSCCONbits.IRCF = 0b0110;
    TRISA = 0xFF;
    TRISC = 0xFF;
    TRISCbits.TRISC5 = 0;
    TRISCbits.TRISC4 = 0;
    //TRISCbits.TRISC2 = 0;
    //TRISCbits.TRISC1 = 0;
    ANSELC = 0;    
    ANSELA = 0;
    count = 0;
    seconds = 0;
    PORTCbits.RC2 = 0;
}

void main(void){
    mcuInit();
    state = readEEPROM(STATE_ADDR);
    if(state > NUM_STATES)
    {
        state = SOLID;
    }
    switch(state){
        case SOLID:
            PORTCbits.RC4 = 1;
            PORTCbits.RC5 = 1;
            pwm_on = 0;
            break;
        case BLINK:
            PORTCbits.RC4 = 1;
            PORTCbits.RC5 = 1;
            pwm_on = 0;            
            break;                
        case BLINK_FASTER:
            PORTCbits.RC4 = 1;
            PORTCbits.RC5 = 0;  
            pwm_on = 0;            
            break;
        case TWINKLE:
            PORTCbits.RC4 = 1;
            PORTCbits.RC5 = 1;                
            pwmInit2();
            pwm_on = 1;
            PSTR1CON = 0b00000011;
            break;
        case TWINKLE_STAR:
            PORTCbits.RC5 = 0;
            PORTCbits.RC4 = 1;                
            pwmInit2();
            pwm_on = 1;    
            PSTR1CON = 0b00000010;
            break;
        case LIT_STAR:
            PORTCbits.RC4 = 1; 
            PORTCbits.RC5 = 1;    
            pwmInit2();
            pwm_on = 1;
            PSTR1CON = 0b00000001;
            CCPR1L = 0b00000111;    
            CCP1CONbits.DC1B = (uint8_t)0b11;                              
            break;
    }  
    timer1Enable();    
    interruptsEnable2();
    while(1){
        asm("sleep");
    }
}
