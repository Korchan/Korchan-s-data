
#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "skUARTlib.h"
#include "skVC0706.h"
#include "skSPIlib.h"
#include "skSDlib.h"
#include "skI2Clib.h"
#include "skI2CLCDlib.h"

#define _XTAL_FREQ   32000000

#pragma config PRICLKEN=OFF,PLLCFG=OFF,FOSC=INTIO67     // CONFIG1H
#pragma config BOREN=NOSLP,BORV=285,PWRTEN=ON          // CONFIG2L
#pragma config WDTEN=OFF                               // CONFIG2H
#pragma config MCLRE=EXTMCLR,HFOFST=OFF                // CONFIG3H
#pragma config LVP=OFF                                 // CONFIG4L

int FileNo ;
char JPG_Size[3][8] = {"640x480","320x240","160x120"} ;

void interrupt InterFunction( void )
{
     InterI2C() ;
     InterUART() ;
}

void main()
{
     struct SDFILE_OBJECT fp ;
     uint32_t jpgsz ;
     uint16_t adrs ;
     int i , ans , jpgcnt , jpglst ;
     char *p , buf[18] ;

     PLLEN  = 1; 
     OSCCON = 0b01100000;
     INTCON2bits.RBPU = 0 ;
     WPUB   = 0b00100110 ;
     ANSELA = 0b00000000 ;
     ANSELB = 0b00000000 ; 
     ANSELC = 0b00000000 ; 
     TRISB  = 0b00100110 ;
     TRISC  = 0b10010000 ;
     PORTB  = 0b00000000 ; 
     PORTC  = 0b00000000 ; 


     InitUART(0,0,51) ;
     InitI2C_Master(1) ;
     LCD_Init(LCD_NOT_ICON,35,LCD_VDD3V,8) ;
     LCD_Puts(" Camera ") ;
     SPI_Init(SPI_MODE3,SPI_CLOCK_DIV4,0) ;
     ans = SD_Init() ;
     if (ans != 0) {
          LCD_SetCursor(0,1) ;
          LCD_Puts("ErrorSDC") ;
          while(1) ;
     }
     FileNo = 0 ;

     __delay_ms(3000) ;
     UART_Flush() ;

     LCD_SetCursor(0,0) ;
     p = VC070x_GetVersion() ;
     if (p == 0) LCD_Puts("ERR-0x11") ;
     else        LCD_Puts(p) ;
     LCD_SetCursor(0,1) ;
     ans = VC070x_GetImageSize() ;
     if (ans == -1) {
          LCD_Puts("ERR-0x30") ;
          while(1) ;
     } else LCD_Puts(&JPG_Size[ans&0x0F][0]) ;

     while(1) {
          if (PORTBbits.RB5 == 0) {
               LCD_SetCursor(0,0) ;
               LCD_Puts("SwitchON") ;
               LCD_SetCursor(0,1) ;
               LCD_Puts("        ") ;
               jpgsz = VC070x_TakePicture() ;
               if (jpgsz != 0) {
                    sprintf(buf,"IMG%05d.JPG\0",FileNo) ;
                    ans = SD_Open(&fp,buf,O_RDWR) ;
                    if (ans == 0) {
                         jpgcnt = jpgsz / VC070X_READDATA_BUFFSZ ;
                         jpglst = jpgsz % VC070X_READDATA_BUFFSZ ; 
                         adrs = 0 ;
                         for(i=0 ; i<jpgcnt ; i++) {
                              p = VC070x_ReadPicture(adrs,VC070X_READDATA_BUFFSZ) ;
                              SD_Write(&fp,p,VC070X_READDATA_BUFFSZ) ;
                              adrs = adrs + VC070X_READDATA_BUFFSZ ;
                              sprintf(buf,"%d\0",i) ;
                              LCD_SetCursor(0,1) ;
                              LCD_Puts(buf) ;
                         }
                         if (jpglst != 0) {
                              p = VC070x_ReadPicture(adrs,jpglst) ;
                              SD_Write(&fp,p,jpglst) ;
                              sprintf(buf,"%d\0",i) ;
                              LCD_SetCursor(0,1) ;
                              LCD_Puts(buf) ;
                         }
                         SD_Close(&fp) ;
                         LCD_SetCursor(0,0) ;
                         LCD_Puts("Success ") ;
                         FileNo++ ;
                    } else {
                         LCD_SetCursor(0,1) ;
                         LCD_Puts("FailOpen") ;
                    }
                    VC070x_ResumeVideo() ;
               } else {
                    LCD_SetCursor(0,1) ;
                    LCD_Puts("ERR-0x36") ;
               }
          }
     }
}
