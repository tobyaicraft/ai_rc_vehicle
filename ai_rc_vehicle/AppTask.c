/**********************************************************************************************************************
 * \file AppTask.c
 * \brief RC Vehicle application tasks
 *
 * Task_1ms   : UART 수신 → 방향키 ESC 시퀀스 파싱 (HC-12 + HC-10 BLE)
 * Task_10ms  : Vehicle 모터 제어 + 키 타임아웃 처리
 * Task_100ms : LED 하트비트
 *
 * 키보드 방향키 프로토콜 (터미널 ESC 시퀀스):
 *   ↑ = 0x1B 5B 41 → 전진
 *   ↓ = 0x1B 5B 42 → 후진
 *   → = 0x1B 5B 43 → 우스핀
 *   ← = 0x1B 5B 44 → 좌스핀
 *   숫자 '0'~'9'   → 속도 설정
 *   키 미입력 200ms → 자동 정지
 *
 * UART 채널:
 *   ASCLIN0 (P15.2/P15.3)  : HC-12 (433MHz RF)
 *   ASCLIN1 (P20.10/P20.9) : HC-10 (BLE)
 *********************************************************************************************************************/
#include "AppTask.h"
#include "DrvDio.h"
#include "DrvUart.h"
#include "DrvUart1.h"
#include "DrvAdc.h"
#include "DrvUltrasonic.h"
#include "DrvMpu9250.h"
#include "DrvGtmTimer.h"
#include "AppVehicle.h"

/******************************************************************************/
/*                           Module Variables                                 */
/******************************************************************************/
#define CMD_TIMEOUT_MS  200u    /* 키 미입력 시 정지까지 시간 */

static uint32 s_lastCmdTime = 0u;   /* 마지막 명령 수신 시각 (ms) */

/* ESC 시퀀스 파서 상태 (채널별 독립) */
typedef enum
{
    ESC_IDLE = 0,
    ESC_GOT_1B,     /* 0x1B 수신 */
    ESC_GOT_5B      /* 0x1B 0x5B 수신 */
} EscState;

static EscState s_escState[2] = { ESC_IDLE, ESC_IDLE };  /* [0]=HC-12, [1]=HC-10 */

/******************************************************************************/
/*                           Static Functions                                 */
/******************************************************************************/
static void setCommand(uint8 cmd)
{
    g_vehicleCmd = cmd;
    s_lastCmdTime = g_1ms_counter;
}

static void processArrowKey(uint8 code)
{
    switch (code)
    {
    case 0x41: setCommand(VEHICLE_FORWARD);    break;  /* ↑ A */
    case 0x42: setCommand(VEHICLE_REVERSE);    break;  /* ↓ B */
    case 0x43: setCommand(VEHICLE_SPIN_RIGHT); break;  /* → C */
    case 0x44: setCommand(VEHICLE_SPIN_LEFT);  break;  /* ← D */
    default:   break;
    }
}

/******************************************************************************/
/*                           UART Parsing                                     */
/******************************************************************************/
static void parseUartByte(uint8 rxByte, uint8 ch)
{
    switch (s_escState[ch])
    {
    case ESC_IDLE:
        if (rxByte == 0x1Bu)
        {
            s_escState[ch] = ESC_GOT_1B;
        }
        else if (rxByte >= '0' && rxByte <= '9')
        {
            float32 spd = (float32)(rxByte - '0') * 10.0f;
            if (spd < 10.0f) spd = 10.0f;
            g_vehicleSpeed = spd;
            s_lastCmdTime = g_1ms_counter;
        }
        break;

    case ESC_GOT_1B:
        if (rxByte == 0x5Bu)
        {
            s_escState[ch] = ESC_GOT_5B;
        }
        else
        {
            s_escState[ch] = ESC_IDLE;
        }
        break;

    case ESC_GOT_5B:
        processArrowKey(rxByte);
        s_escState[ch] = ESC_IDLE;
        break;

    default:
        s_escState[ch] = ESC_IDLE;
        break;
    }
}

/******************************************************************************/
/*                           1ms Task                                         */
/******************************************************************************/
void AppTask_1ms(void)
{
    uint8 rxByte;

    /* CH0: HC-12 (ASCLIN0) */
    while (DrvUart_ReceiveByte(&rxByte))
    {
        parseUartByte(rxByte, 0);
    }

    /* CH1: HC-10 BLE (ASCLIN1) */
    while (DrvUart1_ReceiveByte(&rxByte))
    {
        parseUartByte(rxByte, 1);
    }
}

/******************************************************************************/
/*                           10ms Task                                        */
/******************************************************************************/
void AppTask_10ms(void)
{
    /* 키 타임아웃: CMD_TIMEOUT_MS 동안 입력 없으면 정지 */
    uint32 elapsed = g_1ms_counter - s_lastCmdTime;
    if (elapsed > CMD_TIMEOUT_MS)
    {
        g_vehicleCmd = VEHICLE_STOP;
    }

    AppVehicle_Update();

    /* IMU: read sensors + update attitude (100Hz) */
    DrvMpu9250_ReadSensors();
}

/******************************************************************************/
/*                           100ms Task                                       */
/******************************************************************************/
/* Simple uint16 to decimal string */
static void Uint16ToStr(uint16 val, char *buf)
{
    char tmp[6];
    int i = 0;

    if (val == 0)
    {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }

    while (val > 0)
    {
        tmp[i++] = '0' + (val % 10);
        val /= 10;
    }

    /* Reverse */
    int j;
    for (j = 0; j < i; j++)
    {
        buf[j] = tmp[i - 1 - j];
    }
    buf[j] = '\0';
}

void AppTask_100ms(void)
{
    DrvDio_ToggleLed0();

    /* Trigger ultrasonic & read previous result */
    DrvUltrasonic_Trigger();

    /* Send sensor data via UART: "L:xxxx,R:xxxx,U:xxx\r\n" */
    {
        uint16 irLeft  = DrvAdc_GetIrLeft();
        uint16 irRight = DrvAdc_GetIrRight();
        uint16 usDist  = (uint16)DrvUltrasonic_GetDistanceCm();
        char str[8];

        DrvUart_SendString("L:");
        Uint16ToStr(irLeft, str);
        DrvUart_SendString(str);
        DrvUart_SendString(",R:");
        Uint16ToStr(irRight, str);
        DrvUart_SendString(str);
        DrvUart_SendString(",U:");
        Uint16ToStr(usDist, str);
        DrvUart_SendString(str);
        DrvUart_SendString("\r\n");
    }

    /* Send IMU attitude data */
    DrvMpu9250_SendUart();
}
