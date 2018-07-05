/* 
 * File:   main.c
 * Author: 渡部　耕平
 *
 * Created on 2018/06/19, 15:47
 */

#include <xc.h>
#include <p30f2012.h>

#define FCY 12000000
#include <libpic30.h>
#define _XTAL_FREQ 20000000

int main(void) {
    TRISB = 0x00;
    PORTB = 0x00;
    
    while(1) {
       PORTBbits.RB1 = 1;
       __delay_ms(1000);
       PORTBbits.RB1 = 0;
       __delay_ms(1000);
    }
}

