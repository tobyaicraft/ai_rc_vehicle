/**********************************************************************************************************************
 * \file AppTask.c
 * \brief RC Vehicle application tasks
 *
 * Task_1ms   : HC-12 UART 수신 → 명령 파싱
 * Task_10ms  : Vehicle 모터 제어 업데이트
 * Task_100ms : LED 하트비트
 *********************************************************************************************************************/
#include "AppTask.h"
#include "DrvDio.h"
#include "DrvUart.h"
#include "AppVehicle.h"

/******************************************************************************/
/*                           HC-12 Command Parser                             */
/******************************************************************************/
/*
 * HC-12 프로토콜 (1바이트 명령):
 *   'S' = Stop
 *   'F' = Forward
 *   'B' = Reverse (Back)
 *   'L' = Turn Left
 *   'R' = Turn Right
 *   'Q' = Spin Left
 *   'E' = Spin Right
 *   '0'~'9' = 속도 설정 (0=0%, 1=10%, ... 9=90%, 기본=50%)
 */
static void parseCommand(uint8 cmd)
{
    switch (cmd)
    {
    case 'S': g_vehicleCmd = VEHICLE_STOP;       break;
    case 'F': g_vehicleCmd = VEHICLE_FORWARD;    break;
    case 'B': g_vehicleCmd = VEHICLE_REVERSE;    break;
    case 'L': g_vehicleCmd = VEHICLE_TURN_LEFT;  break;
    case 'R': g_vehicleCmd = VEHICLE_TURN_RIGHT; break;
    case 'Q': g_vehicleCmd = VEHICLE_SPIN_LEFT;  break;
    case 'E': g_vehicleCmd = VEHICLE_SPIN_RIGHT; break;
    default:
        if (cmd >= '0' && cmd <= '9')
        {
            g_vehicleSpeed = (float32)(cmd - '0') * 10.0f;
            if (g_vehicleSpeed < 10.0f) g_vehicleSpeed = 10.0f;
        }
        break;
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
        parseCommand(rxByte);
    }
}

/******************************************************************************/
/*                           10ms Task                                        */
/******************************************************************************/
void AppTask_10ms(void)
{
    AppVehicle_Update();
}

/******************************************************************************/
/*                           100ms Task                                       */
/******************************************************************************/
void AppTask_100ms(void)
{
    DrvDio_ToggleLed0();
}
