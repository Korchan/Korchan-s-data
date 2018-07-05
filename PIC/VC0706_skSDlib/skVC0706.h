/*******************************************************************************
*  skVC0706.h - VC0706-chip用関数ライブラリヘッダファイル                      *
*               (Digital Video Processor VC0706)                               *
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
#ifndef _SKVC0706_H_
#define _SKVC0706_H_

#include <stdint.h>
#include <stdbool.h>

#ifndef _XTAL_FREQ
 // Unless already defined assume 32MHz system frequency
 // This definition is required to calibrate __delay_us() and __delay_ms()
 #define _XTAL_FREQ 32000000 // 使用するPIC等により動作周波数値を設定する
#endif


#define VC070X_READDATA_BUFFSZ        128    // 画像データ受信バッファサイズ
#define VC070X_READDATA_DELAY          10    // 画像データ受信時のInterval timeを指定(x0.01mS単位)

#define VC070X_MOTION_DETECT_ON         1    // 動きの監視を開始する
#define VC070X_MOTION_DETECT_OFF        0    // 動きの監視を停止する
#define VC070X_COMM_MOTION_DETECTED  0x39    // 動きを検出した時に送信されるコマンド
#define VC070X_COLOR_AUTO            0x00    // カラーモードは自動
#define VC070X_COLOR_COLOR           0x01    // カラーモードはCOLOR
#define VC070X_COLOR_MONO            0x02    // カラーモードは白黒
#define VC070X_640X480               0x00    // 画像のサイズは640x480の大きさ
#define VC070X_320X240               0x11    // 画像のサイズは320x240の大きさ
#define VC070X_160X120               0x22    // 画像のサイズは160x120の大きさ


// エラーが返された時の情報構造体
union VC070X_ERROR_t {
     struct {
          uint8_t cmd_no ;
          uint8_t err_no ;
     } ;
     uint16_t info ;
} ;

// 関数のプロトタイプ宣言
uint16_t VC070x_GetError() ;
char *VC070x_GetVersion() ;
int VC070x_Reset() ;
int VC070x_PowerCtl(int info,uint16_t time) ;
int VC070x_MotionCtlEnable() ;
int VC070x_MotionDetect() ;
int VC070x_SetMotionCtl(int val) ;
int VC070x_GetImageSize() ;
int VC070x_SetImageSize(int sz) ;
int VC070x_GetColorSts() ;
int VC070x_SetColorCtl(int val) ;
int VC070x_GetCompression() ;
int VC070x_SetCompression(int val) ;
int VC070x_ResumeVideo() ;
uint32_t VC070x_TakePicture() ;
char *VC070x_ReadPicture(uint16_t adrs,int len) ;



#endif
