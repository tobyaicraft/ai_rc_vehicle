/**********************************************************************************************************************
 * \file AppTask.c
 * \brief RC Vehicle application tasks
 *
 * Task_1ms   : UART 수신 → 패킷 파서에 바이트 공급 (HC-12 + HC-10 BLE)
 * Task_10ms  : Vehicle 모터 제어 + MOVE 타임아웃 처리
 * Task_100ms : LED 하트비트 + 센서 데이터 송신
 *
 * 패킷 프로토콜: AppProtocol.h/c 참조
 *   MOVE 명령 미수신 200ms 경과 시 자동 정지 (안전 기능)
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
#include "AppProtocol.h"

/******************************************************************************/
/*                           Module Variables                                 */
/******************************************************************************/
#define MOVE_TIMEOUT_MS  200u   /* MOVE 미수신 시 정지까지 시간 */

/******************************************************************************/
/*                           1ms Task                                         */
/******************************************************************************/
void AppTask_1ms(void)
{
    uint8 rxByte;

    /* CH0: HC-12 (ASCLIN0) */
    while (DrvUart_ReceiveByte(&rxByte))
    {
        AppProtocol_Feed(rxByte, 0u);
    }

    /* CH1: HC-10 BLE (ASCLIN1) */
    while (DrvUart1_ReceiveByte(&rxByte))
    {
        AppProtocol_Feed(rxByte, 1u);
    }
}

/******************************************************************************/
/*                           10ms Task                                        */
/******************************************************************************/
void AppTask_10ms(void)
{
    /* MOVE 타임아웃: 200ms 동안 MOVE 명령 없으면 정지 */
    if ((g_1ms_counter - g_lastMoveTime) > MOVE_TIMEOUT_MS)
    {
        g_vehicleCmd = VEHICLE_STOP;
    }

    AppVehicle_Update();

    /* IMU: read sensors (100Hz) */
    DrvMpu9250_ReadSensors();
}

/******************************************************************************/
/*                           100ms Task                                       */
/******************************************************************************/
static void Uint16ToStr(uint16 val, char *buf)
{
    char tmp[6];
    int  i = 0;

    if (val == 0u)
    {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }

    while (val > 0u)
    {
        tmp[i++] = '0' + (char)(val % 10u);
        val /= 10u;
    }

    int j;
    for (j = 0; j < i; j++)
        buf[j] = tmp[i - 1 - j];
    buf[j] = '\0';
}

void AppTask_100ms(void)
{
    DrvDio_ToggleLed0();

    DrvUltrasonic_Trigger();

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

    DrvMpu9250_SendUart();
}
