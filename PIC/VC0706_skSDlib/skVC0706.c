/*******************************************************************************
*  skVC0706.c - VC0706-chip用関数ライブラリ                                    *
*               (Digital Video Processor VC0706)                               *
*                                                                              *
*    VC070x_GetError       - エラー情報を得る処理                              *
*    VC070x_GetVersion     - ファームウェアのバージョンを得る処理              *
*    VC070x_Reset          - カメラのリセットを行う処理                        *
*    VC070x_PowerCtl       - 省電力モードの制御を行う処理                      *
*    VC070x_GetImageSize   - 画像の大きさを得る処理                            *
*    VC070x_GetColorSts    - COLORのモードを得る処理                           *
*    VC070x_SetColorCtl    - COLORのモードを設定する処理                       *
*    VC070x_GetCompression - 画像の圧縮比を得る処理                            *
*    VC070x_SetCompression - 画像の圧縮比を設定する処理                        *
*    VC070x_MotionCtlEnable- モーション監視を許可する処理                      *
*    VC070x_SetMotionCtl   - 動きのモニターリングを制御する処理                *
*    VC070x_MotionDetect   - モーション監視を行う処理                          *
*    VC070x_TakePicture    - 画像をキャプチャーする処理                        *
*    VC070x_ReadPicture    - 画像データを読み込む処理                          *
*    VC070x_ResumeVideo    - ビデオを再開する処理                              *
*                                                                              *
* ============================================================================ *
*  VERSION DATE        BY                    CHANGE/COMMENT                    *
* ---------------------------------------------------------------------------- *
*  1.01    2017-03-10  きむ茶工房(きむしげ)  Create                            *
* ============================================================================ *
*  PIC 18F2xK22                                                                *
*  MPLAB X(v3.50)                                                              *
*  MPLAB(R) XC8 C Compiler Version 1.40                                        *
*******************************************************************************/
#include <xc.h>
#include "skUARTlib.h"
#include "skVC0706.h"

char ReadReturnBuff[VC070X_READDATA_BUFFSZ] ;     // コマンド応答受信バッファ
union VC070X_ERROR_t Erro ;                       // 最後にエラーが返された時のエラー情報

// 送信コマンドに対する返答を読み込む処理
int vc070x_read(char *buff,int len)
{
     uint16_t dt ;
     int i , t , rflg ;

     i = t = 0 ;
     rflg  = 0 ;
     while(1) {
          if (UART_Available() > 0) {
               dt = UART_Read() ;
               if (dt == 0x76) rflg = 1 ;
               if (rflg == 1) {
                    *buff = (char)dt ;
                    buff++ ;
                    i++ ;
                    if (i >= len) return i ; // 受信終了
               }
               t = 0 ;
          }
          __delay_us(10) ;
          t++ ;
          if (t >= 10000) return -1 ;        // 100msでタイムオーバー
     }
     return i ;
}
// コマンドの送信と返答の処理を行う
int vc070x_sendread(char *cmd,int snd_su,int rcv_su)
{
     Erro.cmd_no = *(cmd+2) ;
     Erro.err_no = 0 ;
     UART_Send(cmd,snd_su) ;
     if (vc070x_read(ReadReturnBuff,rcv_su) == rcv_su) {
          if (ReadReturnBuff[2] == Erro.cmd_no) {
               if (ReadReturnBuff[3] == 0x00) {
                    return 0 ;
               } else Erro.err_no = ReadReturnBuff[3] ;// エラーが返された。
          } else Erro.err_no = 10 ;     // 送信コマンドに対する返答でないよ。
     } else Erro.err_no = 11 ;          // 受信タイムオーバー
     return -1 ;
}
/*******************************************************************************
*  ans = VC070x_GetError()                                                     *
*    エラー情報を得る処理                                                      *
*    最後に受信した時のエラー番号を返します。                                  *
*                                                                              *
*    ans  : エラー番号を返す                                                   *
*******************************************************************************/
uint16_t VC070x_GetError()
{
     return Erro.info ;
}
/*******************************************************************************
*  ans = VC070x_Reset()                                                        *
*    カメラのリセットを行う処理                                                *
*    リセットすると電源ON時と同じメッセージを送信して来るので読み捨てる        *
*    電源ON後2-3秒程してコマンド指示を開始する様にデータシートに書いて有るので *
*    リセットしたら2-3秒程待った方が良さげかも。                               *
*                                                                              *
*    ans  : 成功時は０を返す                                                   *
*           失敗時は-1を返す                                                   *
*******************************************************************************/
int VC070x_Reset()
{
     char cmd[4]= {0x56,0x00,0x26,0x00} ;    // SYSTEM_RESET
     int ans ;

     if ((ans=vc070x_sendread(cmd,4,5)) == 0) {
          __delay_ms(1000) ;  // カメラからのメッセージはこの間に読み捨てる
     }
     UART_Flush() ;           // UARTバッファのクリア
     return ans ;
}
/*******************************************************************************
*  ans = VC070x_GetVersion()                                                   *
*    ファームウェアのバージョンを得る処理                                      *
*                                                                              *
*    *ans : 成功時はバージョン情報文字列へのアドレスを返す(例："VC0703 1.00")  *
*           失敗時は０を返す                                                   *
*******************************************************************************/
char *VC070x_GetVersion()
{
     char cmd[4]= {0x56,0x00,0x11,0x00} ;    // GEN_VERSION

     if (vc070x_sendread(cmd,4,16) == 0) {
           ReadReturnBuff[16] = 0x00 ;
           return (char *)&ReadReturnBuff[5] ;
     }
     return 0 ;
}
/*******************************************************************************
*  ans = VC070x_PowerCtl(info,time)                                            *
*    省電力モードの制御を行う処理                                              *
*                                                                              *
*    info : ０=OFF １=ON その他=time時間で省電力モードに入る                   *
*    time : 10ms単位で指定する                                                 *
*    ans  : 成功時は０を返す                                                   *
*           失敗時は-1を返す                                                   *
*******************************************************************************/
int VC070x_PowerCtl(int info,uint16_t time)
{
     char cmd[8]= {0x56,0x00,0x3E,0x03,0x00,0x01,0x00,0x00} ;    // POWER_SAVE_CTRL
     int  su ;

     switch(info) {
        case 0:     // 省電力モードOFF
        case 1:     // 省電力モードON
               cmd[6] = (char)info ;
               su = 7 ;
               break ;
        default :   // 指定時間で省電力モードに入る
               cmd[3] = 0x04 ;
               cmd[4] = 0x01 ;
               cmd[5] = 0x04 ;
               cmd[6] = (char)(time >> 8) ;
               cmd[7] = (char)(time & 0x00FF) ;
               su = 8 ;
               break ;
     }
     return vc070x_sendread(cmd,su,5) ;
}
/*******************************************************************************
*  ans = VC070x_GetCompression()                                               *
*    画像の圧縮比を得る処理                                                    *
*                                                                              *
*    ans : 成功時は0x00～0xFF                                                  *
*          失敗時は-1を返す                                                    *
*******************************************************************************/
int VC070x_GetCompression()
{
     char cmd[8]= {0x56,0x00,0x30,0x04,0x01,0x01,0x12,0x04} ;         // READ_DATA

     if (vc070x_sendread(cmd,8,6) == 0) {
           return (int)ReadReturnBuff[5] ;
     }
     return -1 ;
}
/*******************************************************************************
*  ans = VC070x_SetCompression(val)                                            *
*    画像の圧縮比を設定する処理                                                *
*    リセットすると元に戻ります。                                              *
*                                                                              *
*    val  : 画像圧縮比の指定をする(数値が高いと圧縮比も高い)                   *
*    ans  : 成功時は０を返す                                                   *
*           失敗時は-1を返す                                                   *
*******************************************************************************/
int VC070x_SetCompression(int val)
{
     char cmd[9]= {0x56,0x00,0x31,0x05,0x01,0x01,0x12,0x04,0x00} ;    // WRITE_DATA

     cmd[8] = val ;
     return vc070x_sendread(cmd,9,5) ;
}
/*******************************************************************************
*  ans = VC070x_GetColorSts()                                                  *
*    COLORのモードを得る処理                                                   *
*                                                                              *
*    ans : 成功時は 0=自動設定  1=COLORモード  2=白黒モード                    *
*          失敗時は-1を返す                                                    *
*******************************************************************************/
int VC070x_GetColorSts()
{
     char cmd[4]= {0x56,0x00,0x3D,0x00} ;         // COLOR_STATUS

     if (vc070x_sendread(cmd,4,8) == 0) {
          return (int)ReadReturnBuff[6] ;
     }
     return -1 ;
}
/*******************************************************************************
*  ans = VC070x_SetColorCtl(val)                                               *
*    COLORのモードを設定する処理                                               *
*    白黒モードにして見たがぁ、色落ち？になっただけで白黒にならなかったが？    *
*                                                                              *
*    val  : COLORのモードの指定をする(0=自動設定  1=COLORモード  2=白黒モード) *
*    ans  : 成功時は０を返す                                                   *
*           失敗時は-1を返す                                                   *
*******************************************************************************/
int VC070x_SetColorCtl(int val)
{
     char cmd[6]= {0x56,0x00,0x3C,0x02,0x01,0x00} ;    // COLOR_CTRL

     cmd[5] = val ;
     return vc070x_sendread(cmd,6,5) ;
}
/*******************************************************************************
*  ans = VC070x_GetImageSize()                                                 *
*    画像の大きさを得る処理                                                    *
*                                                                              *
*    ans : 成功時は 0(0x00)=640x480  17(0x11)=320x240  34(0x22)=160x120        *
*          失敗時は-1を返す                                                    *
*******************************************************************************/
int VC070x_GetImageSize()
{
     char cmd[8]= {0x56,0x00,0x30,0x04,0x04,0x01,0x00,0x19} ;         // READ_DATA

     if (vc070x_sendread(cmd,8,6) == 0) {
           return (int)ReadReturnBuff[5] ;
     }
     return -1 ;
}
/*******************************************************************************
*  ans = VC070x_SetImageSize(sz)                                               *
*    画像の大きさを設定する処理                                                *
*    この関数で設定しても、640x480のまま変わらない。                           *
*    上手く動作せず、リセットしてもダメでぇやり方が有るかもだが不明です。      *
*                                                                              *
*    sz   : 画像サイズの指定をする                                             *
*           0(0x00)=640x480  17(0x11)=320x240  34(0x22)=160x120                *
*    ans  : 成功時は０を返す                                                   *
*           失敗時は-1を返す                                                   *
*******************************************************************************/
int VC070x_SetImageSize(int sz)
{
     char cmd[9]= {0x56,0x00,0x31,0x05,0x04,0x01,0x00,0x19,0x00} ;    // WRITE_DATA

     cmd[8] = sz ;
     return vc070x_sendread(cmd,9,5) ;
}
/*******************************************************************************
*  ans = VC070x_MotionCtlEnable()                                              *
*    モーション監視を許可する処理                                              *
*                                                                              *
*    ans  : 成功時は０を返す                                                   *
*           失敗時は-1を返す                                                   *
*******************************************************************************/
int VC070x_MotionCtlEnable()
{
     char cmd[7]= {0x56,0x00,0x42,0x03,0x00,0x01,0x01} ;   // MOTION_CTRL

     // モーション監視を使用出来るように許可する
     return vc070x_sendread(cmd,7,5) ;
}
/*******************************************************************************
*  ans = VC070x_SetMotionCtl(val)                                              *
*    動きのモニターリングを制御する処理                                        *
*                                                                              *
*    val  : 1=モニターリング開始  0=モニターリング停止                         *
*    ans  : 成功時は０を返す                                                   *
*           失敗時は-1を返す                                                   *
*******************************************************************************/
int VC070x_SetMotionCtl(int val)
{
     char cmd[5]= {0x56,0x00,0x37,0x01,0x00} ;             // COMM_MOTION_CTRL

     // モニターリングの設定
     cmd[4] = val ;
     return vc070x_sendread(cmd,5,5) ;
}
/*******************************************************************************
*  ans = VC070x_MotionDetect()                                                 *
*    モーション監視を行う処理                                                  *
*                                                                              *
*    ans  : -1 = 何も受信していないか或いはエラーを受信した                    *
*           それ以外は、受信したコマンド番号を返します。                       *
*******************************************************************************/
int VC070x_MotionDetect()
{
     if (UART_Available() > 0) {
          if (vc070x_read(ReadReturnBuff,4) == 4) {
               if ((ReadReturnBuff[3] == 0x00)) {
                    return ReadReturnBuff[2] ;    // 受信したコマンド番号を返す
               } // else はerrorを返した
          } // else は受信タイムオーバー
     }
     return -1 ;
}
/*******************************************************************************
*  ans = VC070x_ResumeVideo()                                                  *
*    ビデオを再開する処理                                                      *
*                                                                              *
*    ans  : 成功時は０を返す                                                   *
*           失敗時は-1を返す                                                   *
*******************************************************************************/
int VC070x_ResumeVideo()
{
     char cmd[5]= {0x56,0x00,0x36,0x01,0x03} ;   // FBUF_CTRL(resume frame)

     return vc070x_sendread(cmd,5,5) ;
}
/*******************************************************************************
*  ans = VC070x_TakePicture()                                                  *
*    画像をキャプチャーする処理                                                *
*    この関数をコールする場合は、ビデオが起動している事が前提です。            *
*                                                                              *
*    ans : 成功時はキャプチャーした画像のファイルサイズを返す                  *
*          失敗時は０を返す                                                    *
*******************************************************************************/
uint32_t VC070x_TakePicture()
{
     union {
          struct {
               unsigned char b1 ;  // Low byte
               unsigned char b2 ;
               unsigned char b3 ;
               unsigned char b4 ;  // High byte
          } ;
          uint32_t  l ;
     } dt ;
     char cmd1[5]= {0x56,0x00,0x36,0x01,0x00} ;   // FBUF_CTRL(stop current frame)
     char cmd2[5]= {0x56,0x00,0x34,0x01,0x00} ;   // GET_FBUF_LEN(JPEG file size)

     UART_Flush() ;           // UARTバッファのクリア
     // 起動しているビデオを停止させる(キャプチャーする)
     if (vc070x_sendread(cmd1,5,5) == 0) {
          // 画像のファイルサイズを得る
          if (vc070x_sendread(cmd2,5,9) == 0) {
               dt.b4 = ReadReturnBuff[5] ;
               dt.b3 = ReadReturnBuff[6] ;
               dt.b2 = ReadReturnBuff[7] ;
               dt.b1 = ReadReturnBuff[8] ;
               return dt.l ;
          }
     }
     return 0 ;
}
/*******************************************************************************
*  ans = VC070x_ReadPicture(adrs,len)                                          *
*    画像データを読み込む処理                                                  *
*    キャプチャーした画像データをFBUFから読み込みます。                        *
*                                                                              *
*    adrs : 読み込むFBUFのアドレスを指定する                                   *
*    len  : 読み込むデータの長さ指定する(VC070x_READDATA_BUFFSIZ未満とする)    *
*    *ans : 読み込んだデータの格納先を返す                                     *
*******************************************************************************/
char *VC070x_ReadPicture(uint16_t adrs,int len)
{
     uint16_t dt ;
     int i , c ;
     char cmd[16]= {0x56,0x00,0x32,0x0c,0x00,0x0A,
                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0A} ;   // READ_FBUF

     // lenの長さ分のデータを送れ指示
     cmd[8] = adrs >> 8 ;     // FBUFの読み出すアドレスのHIGH側
     cmd[9] = adrs & 0xFF ;   // FBUFの読み出すアドレスのLOW側
     cmd[12]= len >> 8 ;      // 読み出すデータ長さのHIGH側
     cmd[13]= len & 0xFF ;    // 読み出すデータ長さのLOW側
     cmd[14]= VC070X_READDATA_DELAY >> 8 ;      // Interval timeのHIGH側
     cmd[15]= VC070X_READDATA_DELAY & 0xFF ;    // Interval timeのLOW側
     UART_Send(cmd,16) ;
     // データを受信する
     // 先頭と最後には"76 00 32 00 00"が有るので読み捨てる
     i = c = 0 ;
     while(1) {
          if (UART_Available() > 0) {
               dt = UART_Read() ;
               i++ ;
               if (i>5) {
                    // 6バイト目からlenの長さ分読み込む
                    ReadReturnBuff[c] = dt ;
                    c++ ;
                    if (c == len) break ;
               }
          }
     }
     i = 0 ;
     while(1) {
          if (UART_Available() > 0) {
               dt = UART_Read() ;
               i++ ;
               if (i==4) c = dt ;  // 通信状況の取り出し
               if (i>=5) break ;
          }
     }
     return (char *)&ReadReturnBuff ;
}
