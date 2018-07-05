// VC0706チップのカメラテスト(動きを監視するサンプル)
// skSDlibのライブラリを使用したプログラム
// MPLAB X v3.50/XC8 v1.40 でのコンパイルです。
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

// 定数の定義
#define _XTAL_FREQ   32000000      // delay用に必要(クロック32MHzを指定)

// コンフィギュレーションの設定(ここの記述に無い設定はデフォルト値での動作)
// 外部クロックは使用しない(PRICLKEN_OFF)
// 動作クロックの4倍はソフトで制御(PLLCFG_OFF)：内部ｸﾛｯｸを使用でCLKOUT無(INTIO67)
#pragma config PRICLKEN=OFF,PLLCFG=OFF,FOSC=INTIO67     // CONFIG1H
// 電源電圧降下常時監視機能ON(BOREN_NOSLP)：監視電圧は(2.85V)に設定
// 電源ONから後65.6msにﾌﾟﾛｸﾞﾗﾑを開始する(PWRTEN_ON)
#pragma config BOREN=NOSLP,BORV=285,PWRTEN=ON          // CONFIG2L
// ｳｵｯﾁﾄﾞｯｸﾞﾀｲﾏｰ無し(WDTEN_OFF)
#pragma config WDTEN=OFF                               // CONFIG2H
// MCLRのﾘｾｯﾄﾋﾟﾝとして使用する、ﾃﾞｼﾞﾀﾙ入力不可(MCLRE_EXTMCLR)
// オシレータが安定するのを待ってシステムクロックを供給する(HFOFST_OFF)
#pragma config MCLRE=EXTMCLR,HFOFST=OFF                // CONFIG3H
// 低電圧プログラミング機能使用しない(LVP_OFF)
#pragma config LVP=OFF                                 // CONFIG4L

int FileNo ;     // ファイル番号
char JPG_Size[3][8] = {"640x480","320x240","160x120"} ;

// 割り込みの処理
void interrupt InterFunction( void )
{
     // Ｉ２Ｃ関連の割り込み(LCDで利用)
     InterI2C() ;
     // ＵＳＡＲＴ関連の割り込み(CAMERAで利用)
     InterUART() ;
}
// メインの処理
void main()
{
     int  ans ;
     char *p ;

     PLLEN  = 1;              // 内部クロックを4x倍で利用する
     OSCCON = 0b01100000;     // 内部クロックとする(8MHzx4=32MHz)
     INTCON2bits.RBPU = 0 ;   // 内部プルアップを行う
     WPUB   = 0b00100110 ;    // RB1/RB2/RB5をプルアップする
     ANSELA = 0b00000000 ;    // AN0-4アナログは使用しない、デジタルI/Oに割当
     ANSELB = 0b00000000 ;    // AN8-13アナログは使用しない、デジタルI/Oに割当
     ANSELC = 0b00000000 ;    // AN14-19アナログは使用しない、デジタルI/Oに割当
     TRISA  = 0b00000000 ;    // RA0-RA7全て出力に設定
     TRISB  = 0b00100110 ;    // RB1(SCL2)RB2(SDA2)RB5(SW)は入力、他は全て出力、1で入力 0で出力
     TRISC  = 0b10010000 ;    // RC4(SDI1)RC7(RX1)は入力、他は全て出力、1で入力 0で出力
     PORTA  = 0b00000000 ;    // 出力ピンの初期化(全てLOWにする)
     PORTB  = 0b00000000 ;    // 出力ピンの初期化(全てLOWにする)
     PORTC  = 0b00000000 ;    // 出力ピンの初期化(全てLOWにする)

     // USART通信の初期化を行う(38400bps)
     InitUART(0,0,51) ;
     // I2C通信のマスターモードで初期化を行う、通信速度は400KHz
     InitI2C_Master(1) ;
     // I2C接続小型LCDモジュールの初期化処理
     // ICON OFF,コントラスト(0-63),VDD=3.3Vで使う,LCDは8文字列
     LCD_Init(LCD_NOT_ICON,35,LCD_VDD3V,8) ;
     LCD_Puts(" Camera ") ;
     // SPIの初期化を行う(クロック極性:HIGH クロック位相:0　通信速度:Fosc/4)
     SPI_Init(SPI_MODE3,SPI_CLOCK_DIV4,0) ;
//     // MMC/SDCの初期化を行う
//     ans = SD_Init() ;
//     if (ans != 0) {
//          // SDカード初期化エラー(カード未挿入かも？)
//          LCD_SetCursor(0,1) ;
//          LCD_Puts("ErrorSDC") ;
//          while(1) ;     // 終了
//     }
     FileNo = 0 ;        // 写真番号、写真を撮る度にプラスされる

     __delay_ms(3000) ;  // 3秒後に開始(カメラからのメッセージはこの間に読み捨てる)
     UART_Flush() ;      // UARTバッファのクリア

     // GetVersion処理
     LCD_SetCursor(0,0) ;
     p = VC070x_GetVersion() ;
     if (p == 0) LCD_Puts("ERR-0x11") ;
     else        LCD_Puts(p) ;
     // Motion監視を開始する
     LCD_SetCursor(0,1) ;
     ans = VC070x_MotionCtlEnable() ;
     ans = VC070x_SetMotionCtl(VC070X_MOTION_DETECT_ON) ;
     if (ans == -1) {
          LCD_Puts("ERR-0x37") ;
          while(1) ;     // 終了
     }
     LCD_Puts("StartMon") ;

     while(1) {
          // カメラからのデータ受信を行う(モーション監視)
          ans = VC070x_MotionDetect() ;
          if (ans != -1) {
               // 何か受信したようだ
               if (ans == VC070X_COMM_MOTION_DETECTED) {
                    // 動きを検出したぞぉ
                    LCD_SetCursor(0,0) ;
                    LCD_Puts("Detected") ;
                    LCD_SetCursor(0,1) ;
                    LCD_Puts("        ") ;
                    // ３秒後に表示を消す
                    __delay_ms(3000) ;
                    LCD_SetCursor(0,0) ;
                    LCD_Puts("        ") ;

                    /*  ここで写真を保存するなり警報を出すなりご自由に  */

               } else {
                    // 他のコマンドを受信したようだ
                    LCD_SetCursor(0,1) ;
                    LCD_Puts("etc. cmd") ;
                    UART_Flush() ;      // UARTバッファのクリア
               }
          }
     }
}
