/**********************************************************************************************************************
 * \file AppTask.c
 * \brief RC Vehicle application tasks
 *
 * Task_1ms   : HC-12 UART 수신 → 방향키 ESC 시퀀스 파싱
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
 *********************************************************************************************************************/
#include "AppTask.h"
#include "DrvDio.h"
#include "DrvUart.h"
#include "DrvGtmTimer.h"
#include "AppVehicle.h"

/******************************************************************************/
/*                           Module Variables                                 */
/******************************************************************************/
#define CMD_TIMEOUT_MS  200u    /* 키 미입력 시 정지까지 시간 */

static uint32 s_lastCmdTime = 0u;   /* 마지막 명령 수신 시각 (ms) */

/* ESC 시퀀스 파서 상태 */
typedef enum
{
    ESC_IDLE = 0,
    ESC_GOT_1B,     /* 0x1B 수신 */
    ESC_GOT_5B      /* 0x1B 0x5B 수신 */
} EscState;

static EscState s_escState = ESC_IDLE;

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
/*                           1ms Task                                         */
/******************************************************************************/
void AppTask_1ms(void)
{
    uint8 rxByte;
    while (DrvUart_ReceiveByte(&rxByte))
    {
        /* ESC 시퀀스 상태 머신 */
        switch (s_escState)
        {
        case ESC_IDLE:
            if (rxByte == 0x1Bu)
            {
                s_escState = ESC_GOT_1B;
            }
            else if (rxByte >= '0' && rxByte <= '9')
            {
                /* 속도 설정: '1'=10%, '5'=50%, '9'=90% */
                float32 spd = (float32)(rxByte - '0') * 10.0f;
                if (spd < 10.0f) spd = 10.0f;
                g_vehicleSpeed = spd;
                s_lastCmdTime = g_1ms_counter;
            }
            break;

        case ESC_GOT_1B:
            if (rxByte == 0x5Bu)
            {
                s_escState = ESC_GOT_5B;
            }
            else
            {
                s_escState = ESC_IDLE;
            }
            break;

        case ESC_GOT_5B:
            processArrowKey(rxByte);
            s_escState = ESC_IDLE;
            break;

        default:
            s_escState = ESC_IDLE;
            break;
        }
    }
}

/******************************************************************************/
/*                           10ms Task                                        */
/******************************************************************************/
void AppTask_10ms(void)
{
    /* 키 타임아웃: CMD_TIMEOUT_MS 동안 입력 없으면 정지 */
    /* TODO: HC-12 연결 후 활성화 */
#if 0
    uint32 elapsed = g_1ms_counter - s_lastCmdTime;
    if (elapsed > CMD_TIMEOUT_MS)
    {
        g_vehicleCmd = VEHICLE_STOP;
    }
#endif

    AppVehicle_Update();
}

/******************************************************************************/
/*                           100ms Task                                       */
/******************************************************************************/
void AppTask_100ms(void)
{
    DrvDio_ToggleLed0();
}
