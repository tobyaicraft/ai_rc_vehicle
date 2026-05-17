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
#include "DrvBuzzer.h"
#include "DrvSensorFusion.h"
#include "AppVehicle.h"
#include "AppProtocol.h"
#include "AppObstacle.h"

/******************************************************************************/
/*                           Module Variables                                 */
/******************************************************************************/
#define MOVE_TIMEOUT_MS     200u    /* MOVE 미수신 시 정지까지 시간 */
#define BAT_LOW_THRESHOLD   6800u   /* 저전압 경고 임계값 (mV) */
#define TEST_YAW_TRIGGER    15.0f   /* TEST 모드: Yaw 보정 트리거 임계값 (°) */

/* 저전압 경고 멜로디 (주파수Hz, 0=무음)
 * 100ms 단위로 한 음씩 재생, 끝나면 반복
 * ♪ Super Mario Bros Theme                                    */
static const uint16 s_lowBatMelody[] = {
    /* E5 E5 . E5 . C5 E5 . G5 . . . G4 . . .   */
    659, 659, 0, 659, 0, 523, 659, 0,
    784, 0, 0, 0, 392, 0, 0, 0,
    /* C5 . . G4 . . E4 . . A4 . B4 . Bb4 A4 . */
    523, 0, 0, 392, 0, 0, 330, 0,
    0, 440, 0, 494, 0, 466, 440, 0,
    /* G4 E5 . G5 A5 . F5 G5 . E5 . C5 D5 B4 . */
    392, 659, 0, 784, 880, 0, 698, 784,
    0, 659, 0, 523, 587, 494, 0, 0,
    /* pause 2s */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
#define MELODY_LEN  (sizeof(s_lowBatMelody) / sizeof(s_lowBatMelody[0]))

static uint8 s_melodyIdx = 0u;

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
    /* ── TEST 모드: 직진 + Yaw ±15° 이탈 시 자동 보정 ── */
    if (g_vehicleMode == VEHICLE_MODE_TEST)
    {
        if (!AppVehicle_IsTurning())
        {
            float32 yaw    = DrvMpu9250_GetYaw();
            float32 absYaw = (yaw < 0.0f) ? -yaw : yaw;

            if (absYaw > TEST_YAW_TRIGGER)
                g_vehicleCmd = VEHICLE_YAW_ZERO;   /* 보정 회전 */
            else
                g_vehicleCmd = VEHICLE_FORWARD;     /* 직진 유지 */
        }
    }
    else if (g_vehicleMode == VEHICLE_MODE_MANUAL ||
             g_vehicleMode == VEHICLE_MODE_CALIB)
    {
        /* MOVE 타임아웃: 200ms 동안 MOVE 명령 없으면 정지 (수동 모드만) */
        if ((g_1ms_counter - g_lastMoveTime) > MOVE_TIMEOUT_MS)
        {
            g_vehicleCmd = VEHICLE_STOP;
        }
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

    /* 센서 필터링 업데이트 (이동 평균 + 이상치 제거) */
    DrvSensorFusion_Update();

    uint16 irLeft  = DrvSensorFusion_GetIrLeft();
    uint16 irRight = DrvSensorFusion_GetIrRight();
    uint16 usDist  = DrvSensorFusion_GetUltrasonic();
    uint16 batMv   = DrvAdc_GetBatteryMv();
    uint8  obstacle = (uint8)AppObstacle_Detect();
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
    DrvUart_SendString(",B:");
    Uint16ToStr(batMv, str);
    DrvUart_SendString(str);
    DrvUart_SendString(",O:");
    Uint16ToStr((uint16)obstacle, str);
    DrvUart_SendString(str);
    DrvUart_SendString("\r\n");

    /* 저전압 경고 멜로디 재생 */
    if ((batMv > 0u) && (batMv < BAT_LOW_THRESHOLD))
    {
        uint16 note = s_lowBatMelody[s_melodyIdx];
        if (note > 0u)
            DrvBuzzer_PlayNote(note);
        else
            DrvBuzzer_Off();

        s_melodyIdx++;
        if (s_melodyIdx >= MELODY_LEN)
            s_melodyIdx = 0u;
    }
    else
    {
        DrvBuzzer_Off();
        s_melodyIdx = 0u;
    }

    DrvMpu9250_SendUart();
}
