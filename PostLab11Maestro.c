/*
 * File:   PostLab11Maestro.c
 * Author: jorge
 *
 * Created on 13 de mayo de 2022, 11:33 PM
 */
// CONFIG1
#pragma config FOSC = INTRC_NOCLKOUT// Oscillator Selection bits (INTOSC oscillator: CLKOUT function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF      // RE3/MCLR pin function select bit (RE3/MCLR pin function is digital input, MCLR internally tied to VDD)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF      // Brown Out Reset Selection bits (BOR disabled)
#pragma config IESO = OFF       // Internal External Switchover bit (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is disabled)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (RB3 pin has digital I/O, HV on MCLR must be used for programming)
// CONFIG2
#pragma config BOR4V = BOR40V   // Brown-out Reset Selection bit (Brown-out Reset set to 4.0V)
#pragma config WRT = OFF        // Flash Program Memory Self Write Enable bits (Write protection off)
// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>
#include <stdint.h>             // int8_t, unit8_t


#define _XTAL_FREQ 1000000
#define FLAG_SPI 0xFF

uint8_t  val_pot;                  // Variable valor de potenciometro

void __interrupt() isr (void){
    if(PIR1bits.ADIF){                  // Interrupción por ADC
        if(ADCON0bits.CHS == 0){        // Interrupción por AN0
            val_pot = ADRESH;           // Valor del ADRESH al valor del potenciometro
        }
        PIR1bits.ADIF = 0;              // Limpiar la bandera de ADC
    }
    return;
}

void setup(void){
    ANSEL = 0b00000001;         // AN0 entrada analógica        
    ANSELH = 0;                 // I/O digitales
    
    TRISA = 0b00000001;         // RA0 como entradas
    TRISC = 0b00010000;         // SD1 entrada, SCK y SD0 como salida
    PORTCbits.RC4 = 0;
    TRISD = 0;                  // POTD como salida
    PORTA = 0;                  // Se limpia PORTA
    PORTC = 0;                  // Se limpia PORTC
    PORTD = 0;                  // Se limpia PORTD
    // Configuración del oscilador
    OSCCONbits.IRCF = 0b0100;   // 1 MHz
    OSCCONbits.SCS = 1;         // Oscilador interno
    // Configuración de interrupciones
    INTCONbits.GIE = 1;         // Habilitar interrupciones globales
    INTCONbits.PEIE = 1;        // Habilitar interrupciones de periféricos
    PIR1bits.ADIF = 0;          // Limpiar la bandera de ADC
    PIE1bits.ADIE = 1;          // Habilitar interrupciones de ADC
    
    // Configuración ADC
    ADCON0bits.ADCS = 0b01;         // FOSC/8
    ADCON1bits.VCFG0 = 0;           // VDD
    ADCON1bits.VCFG1 = 0;           // VSS
    ADCON0bits.CHS = 0b0000;        // Selecciona el AN0
    ADCON1bits.ADFM = 0;            // Justificador a la izquierda
    ADCON0bits.ADON = 1;            // Habilitar modulo ADC
    __delay_us(40);                // Sample time

    // Configuración de Maestro
    // Configuración de SPI
    SSPCONbits.SSPM = 0b0000;   // SPI Maestro, Reloj -> FOSC/4  (250 KBIS/s)
    SSPCONbits.CKP = 0;         // Reloj inactivo
    SSPCONbits.SSPEN = 1;       // Habilitar pines de SPI
    SSPSTATbits.CKE = 1;        // Dato enviado cada flanco de subida
    SSPSTATbits.SMP = 1;        // Dato al final del pulso de reloj
    SSPBUF = val_pot;           // Valor inicial = 0
    
}

void main(void) {
    setup();
    while(1){
        if(ADCON0bits.GO == 0){ // Si no hay proceso de conversión
            __delay_us(40);
            ADCON0bits.GO = 1;
        }
        PORTAbits.RA7 = 1;      // Deshabilitar el SS del primer esclavo
        SSPBUF = val_pot;
        while(!SSPSTATbits.BF){}// Esperar el envío
        // Cambio en el selector (SS) para generar respuesta del PIC
        PORTAbits.RA7 = 0;      // Se hailita nuevamente el primer esclavo
        PORTAbits.RA6 = 1;      // Deshabilitar el SS del segundo esclavo
        PORTAbits.RA7 = 1;      // Deshabilitar el SS del primer esclavo
        __delay_ms(10);         // Delay para detectar el cambio de pin
        PORTAbits.RA7 = 0;      // Se hailita nuevamente el primer esclavo
        
        // Master inicia la comunicación y prende el clock
        SSPBUF = FLAG_SPI;
        while(!SSPSTATbits.BF){} // Esperar a que reciba un dato
        PORTD = SSPBUF;         // Mostrar e dato recibido en PORTD
        PORTAbits.RA6 = 0;      // Se hailita nuevamente el segundo esclavo
        
    }
    return;
}
