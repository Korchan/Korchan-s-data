/*******************************************************************************
*  skUARTlib - ＵＳＡＲＴ関数ライブラリ                                        *
*          このライブラリはUSART(シリアル通信)を行う為の関数集です             *
*                                                                              *
*    InterUART      - ＵＳＡＲＴ関連の割り込み処理                             *
*    InitUART       - ＵＳＡＲＴ通信の初期化を行う処理                         *
*    UART_Send      - 相手に指定した個数のデータを送信する処理                 *
*    UART_Write     - 相手に１バイトのデータを送信する処理                     *
*    UART_Available - 受信したデータの個数を返す処理                           *
*    UART_Read      - 受信したデータを１バイト読み込む処理                     *
*    UART_Flush     - 受信バッファをクリアする処理                             *
*                                                                              *
*    メモ：標準はUART_8N1(8bit,nonParity,1stop)だが、"skUARTlib.h"の記述を、   *
*          #define UART_8O1とすると(8bit,oddParity,1stop)で送信できます。      *
*          だけどぉ、受信時のパリティチェックは行っていません。                *
*                                                                              *
* ============================================================================ *
*  VERSION DATE        BY                    CHANGE/COMMENT                    *
* ---------------------------------------------------------------------------- *
*  1.00    2013-01-20  きむ茶工房(きむしげ)  Create                            *
*  1.10    2013-02-18  きむ茶工房(きむしげ)  UART_Flush関数の追加              *
*  2.00    2015-05-25  きむ茶工房(きむしげ)  16F1825/1829 18F2xK22 対応        *
*  3.01    2017-02-01  きむ茶工房(きむしげ)  パリティ(ODD)対応                 *
*  3.10    2017-02-04  きむ茶工房(きむしげ)  低速通信速度に対応                *
* ============================================================================ *
*  PIC 12F1822 16F1705 16F182x 16F1938/19xx 18F2xK22 1814K50 18F8722           *
*  MPLAB IDE(V8.84) MPLAB X(V3.50)                                             *
*  MPLAB(R) XC8 C Compiler Version 1.00/1.32/1.38/1.40                         *
*******************************************************************************/
#include <xc.h>
#include "skUARTlib.h"


char UART_Buffer[UART_BUFFER_SIZE] ;    // 受信したデータを格納するバッファ
int  UART_Buffer_inptr ;                // 受信バッファの読み込みポインター
int  UART_Buffer_outptr ;               // 受信バッファの書き込みポインター


/*******************************************************************************
*  InterUART( void )                                                           *
*    ＵＳＡＲＴ関連の割り込み処理                                              *
*     この関数は受信を行う場合はメインプログラムの割込み関数で必ず呼びます     *
*******************************************************************************/
void InterUART( void )
{
     int  x ;
     char dt ;

     if (RC0IF == 1) {        // 割込みはＵＳＡＲＴ通信の受信か？
          // レジスタからデータを受信
          dt = RCREG0 ;
          x = (UART_Buffer_outptr + 1) % UART_BUFFER_SIZE ;
          if (x != UART_Buffer_inptr) {
               // バッファが満杯でないならデータを保存する
               UART_Buffer[UART_Buffer_outptr] = dt ;
               // バッファ書き込みポインターを次に進める
               UART_Buffer_outptr = x ;
          }

          // オーバランエラー処理(RCREGバッファが３個目のデータを受信したらオーバフロー)
          if (RCSTA_OERR == 1) {
               RCSTA_CREN = 0 ;
               while( RCSTA_OERR ) ;
               RCSTA_CREN = 1 ;
          }

          // 割込み受信フラグをリセット
          RC0IF = 0 ;
     }
}
/*******************************************************************************
*  InitUART(rx,tx,brg)                                                         *
*    ＵＳＡＲＴ通信の初期化を行う処理                                          *
*    rx  : 受信(RX)するピンの番号を指定する                                    *
*    tx  : 送信(TX)するピンの番号を指定する                                    *
*    brg : ボーレートジェネレータの値を指定します(通信速度になります)          *
*          (ボーレート値が"207"を超えたら、BRGH=0/BRG16=1に設定します)         *
*                    4MHz  8MHz 16MHz 32MHz 40MHz 48MHz 64MHz                  *
*          (SYNC=0 BRGH=1 BRG16=0)                                             *
*           9600bps   25    51   103   207    x     x     x                    *
*          19200bps   12    25    51   103   129   155   207                   *
*          (SYNC=0 BRGH=0 BRG16=1)                                             *
*           4800bps                    416   520   624   832                   *
*           9600bps                          259   311   416                   *
*******************************************************************************/
void InitUART(int rx, int tx, unsigned int brg)
{
#if defined(_12F1822)
     RXDTSEL = 0 ;                 // 6番ピン(RA1)をRX受信ピンとする
     TXCKSEL = 0 ;                 // 7番ピン(RA0)をTX送信ピンとする
     if (rx == 2) RXDTSEL = 1 ;    // 2番ピン(RA5)をRX受信ピンとする
     if (tx == 3) TXCKSEL = 1 ;    // 3番ピン(RA4)をTX受信ピンとする
#endif

#if defined(_16F1823) || defined(_16F1825)
     RXDTSEL = 0 ;                 //  5番ピン(RC5)をRX受信ピンとする
     TXCKSEL = 0 ;                 //  6番ピン(RC4)をTX送信ピンとする
     if (rx == 12) RXDTSEL = 1 ;   // 12番ピン(RA1)をRX受信ピンとする
     if (tx == 13) TXCKSEL = 1 ;   // 13番ピン(RA0)をTX受信ピンとする
#endif

#if defined(_16F1826) || defined(_16F1827)
     RXDTSEL = 0 ;                 //  7番ピン(RB1)をRX受信ピンとする
     TXCKSEL = 0 ;                 //  8番ピン(RB2)をTX送信ピンとする
     if (rx == 8)  RXDTSEL = 1 ;   //  8番ピン(RB2)をRX受信ピンとする
     if (tx == 11) TXCKSEL = 1 ;   // 11番ピン(RB5)をTX受信ピンとする
#endif

#if defined(_16F1829)
     RXDTSEL = 0 ;                 // 12番ピン(RB5)をRX受信ピンとする
     TXCKSEL = 0 ;                 // 10番ピン(RB7)をTX送信ピンとする
     if (rx == 5) RXDTSEL = 1 ;    //  5番ピン(RC5)をRX受信ピンとする
     if (tx == 6) TXCKSEL = 1 ;    //  6番ピン(RC4)をTX受信ピンとする
#endif

#if defined(UART_8O1) || defined(UART_8E1)
     // Data=8bit Parity=0dd/Even Stop=1bit Flow=none
     TXSTA0 = 0b01100100 ;         // 送信情報設定：非同期モード　９ビット・パリティ有り
     RXSTA0 = 0b11010000 ;         // 受信情報設定
#else
     // Data=8bit Parity=none Stop=1bit Flow=none
     TXSTA0 = 0b00100100 ;         // 送信情報設定：非同期モード　８ビット・ノンパリティ
     RXSTA0 = 0b10010000 ;         // 受信情報設定
#endif
     // ボーレートの設定
     if (brg <= 207) {
          BRGH0  = 1 ;
          BRG160 = 0 ;
          SPBRG0 = brg ;
     } else {
          BRGH0  = 0 ;
          BRG160 = 1 ;
          SPBRGH0 = (brg >> 8) ;
          SPBRG0  = brg & 0x00FF ;
     }
     RC0IF  = 0 ;                  // USART割込み受信フラグの初期化
     RC0IE  = 1 ;                  // USART割込み受信を有効にする
     PEIE   = 1 ;                  // 周辺装置割込みを有効にする
     GIE    = 1 ;                  // 全割込み処理を許可する
     UART_Buffer_inptr  = 0 ;      // 受信バッファの読み込みポインターの初期化
     UART_Buffer_outptr = 0 ;      // 受信バッファの書き込みポインターの初期化
}
/*******************************************************************************
*  UART_Send(dt,num)                                                           *
*    相手に指定した個数のデータを送信する処理                                  *
*                                                                              *
*    dt  : 送信するデータを格納した配列を指定します。                          *
*    num : 送信するデータの個数を指定します。                                  *
*******************************************************************************/
void UART_Send(const char *dt,int num)
{
     union pari_t test ;
     int i , x ;

     // 指定した個数分繰り返す
     for (i=0 ; i<num ; i++) {
#ifdef UART_8O1
          // 奇数は1、偶数は０で得られるので反転してセットする(奇数パリティ)
          test.dt = *dt ;
          x = test.b0 + test.b1 +  test.b2 + test.b3 + test.b4 + test.b5 + test.b6 + test.b7 ;
          TX9D0 = (x & 0x01) ^ 0x01 ; // 奇数パリティのセット
#endif
          while(TX0IF==0) ;   // 送信可能になるまで待つ
          TXREG0 = *dt++ ;    // 送信する
     }
}
/*******************************************************************************
*  UART_Write(dt)                                                              *
*    相手に１バイトのデータを送信する処理                                      *
*                                                                              *
*    dt  : 送信するデータを指定します。                                        *
*******************************************************************************/
void UART_Write(char dt)
{
     union pari_t test ;
     int x ;

#ifdef UART_8O1
     // 奇数は1、偶数は０で得られるので反転してセットする(奇数パリティ)
     test.dt = dt ;
     x = test.b0 + test.b1 +  test.b2 + test.b3 + test.b4 + test.b5 + test.b6 + test.b7 ;
     TX9D0 = (x & 0x01) ^ 0x01 ; // 奇数パリティのセット
#endif
     while(TX0IF==0) ;        // 送信可能になるまで待つ
     TXREG0 = dt ;            // 送信する
}
/*******************************************************************************
*  ans = UART_Available()                                                      *
*    受信したデータの個数を返す処理                                            *
*                                                                              *
*    ans : 受信したデータの個数を返します。                                    *
*******************************************************************************/
int UART_Available()
{
     return (UART_Buffer_outptr + UART_BUFFER_SIZE - UART_Buffer_inptr) % UART_BUFFER_SIZE ;
}
/*******************************************************************************
*  ans = UART_Read()                                                           *
*    受信したデータを１バイト読み込む処理                                      *
*                                                                              *
*    ans : 受信したデータを返します  0xffffを返したら受信データは空です。      *
*******************************************************************************/
unsigned int UART_Read()
{
     unsigned int  ans ;

     // バッファは空である
     if (UART_Buffer_inptr == UART_Buffer_outptr) return 0xffff ;
     // バッファから読み出す
     ans = UART_Buffer[UART_Buffer_inptr] ;
     // バッファ読み込みポインターを次に進める
     UART_Buffer_inptr = (UART_Buffer_inptr + 1) % UART_BUFFER_SIZE ;

     return ans ;
}
/*******************************************************************************
*  UART_Flush()                                                                *
*    受信バッファをクリアする処理                                              *
*******************************************************************************/
void UART_Flush()
{
     UART_Buffer_inptr  = 0 ;      // 受信バッファの読み込みポインターの初期化
     UART_Buffer_outptr = 0 ;      // 受信バッファの書き込みポインターの初期化
}
