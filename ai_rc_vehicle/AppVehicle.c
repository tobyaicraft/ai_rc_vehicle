/**********************************************************************************************************************
 * \file AppVehicle.c
 * \brief 4WD RC Vehicle 제어 모듈
 *********************************************************************************************************************/
#include "AppVehicle.h"
#include "DrvMotor.h"
#include "DrvAdc.h"
#include "DrvMpu9250.h"
#include "DrvFlash.h"
#include <math.h>

/******************************************************************************/
/*                           Configuration                                    */
/******************************************************************************/
#define BAT_VREF_MV     8400u
#define BAT_VMIN_MV     6400u
#define BAT_COMP_MAX    8

#define TURN90_TARGET   90.0f    /* 목표 회전 각도 */
#define TURN90_MARGIN   3.0f     /* 도달 판정 여유 (±3°) */
#define TURN90_TIMEOUT  300u     /* 타임아웃 (×10ms = 3초) */

#define YAW_ZERO_MARGIN  5.0f   /* 0도 복귀 판정 여유 (±5°) */
#define YAW_ZERO_TIMEOUT 500u   /* 타임아웃 (×10ms = 5초) */

/******************************************************************************/
/*                           Global Variables                                 */
/******************************************************************************/
volatile uint8   g_vehicleCmd   = VEHICLE_STOP;
volatile float32 g_vehicleSpeed = 50.0f;

/******************************************************************************/
/*                           Turn90 State                                     */
/******************************************************************************/
static boolean s_turn90Active  = FALSE;
static sint8   s_turn90Dir     = 0;      /* +1=우회전, -1=좌회전 */
static float32 s_turn90StartYaw = 0.0f;
static uint16  s_turn90Counter = 0u;

static boolean s_yawZeroActive  = FALSE;
static sint8   s_yawZeroDir     = 0;     /* +1=우회전, -1=좌회전 */
static uint16  s_yawZeroCounter = 0u;

/******************************************************************************/
/*                           Static Functions                                 */
/******************************************************************************/

static sint16 getBatCompensation(void)
{
    uint16 batMv = DrvAdc_GetBatteryMv();

    if ((batMv == 0u) || (batMv >= BAT_VREF_MV))
        return 0;
    if (batMv <= BAT_VMIN_MV)
        return BAT_COMP_MAX;

    return (sint16)(((uint32)(BAT_VREF_MV - batMv) * BAT_COMP_MAX)
                    / (BAT_VREF_MV - BAT_VMIN_MV));
}

/* Yaw 차이 계산 (-180 ~ +180 범위 정규화) */
static float32 yawDelta(float32 current, float32 start)
{
    float32 d = current - start;
    if (d >  180.0f) d -= 360.0f;
    if (d < -180.0f) d += 360.0f;
    return d;
}

/******************************************************************************/
/*                           Functions                                        */
/******************************************************************************/
void AppVehicle_Init(void)
{
    g_vehicleCmd    = VEHICLE_STOP;
    g_vehicleSpeed  = 100.0f;
    s_turn90Active  = FALSE;
    s_yawZeroActive = FALSE;
}

boolean AppVehicle_IsTurning(void)
{
    return s_turn90Active || s_yawZeroActive;
}

void AppVehicle_Update(void)
{
    sint16 comp = getBatCompensation();

    sint16 spdFL = (sint16)(((float32)g_calDutyFL + comp) * g_vehicleSpeed / 100.0f);
    sint16 spdFR = (sint16)(((float32)g_calDutyFR + comp) * g_vehicleSpeed / 100.0f);
    sint16 spdRL = (sint16)(((float32)g_calDutyRL + comp) * g_vehicleSpeed / 100.0f);
    sint16 spdRR = (sint16)(((float32)g_calDutyRR + comp) * g_vehicleSpeed / 100.0f);

    sint16 turnFL = (sint16)(((float32)g_calDutyFL + comp) * g_calTurnFront / 100.0f * g_vehicleSpeed / 100.0f);
    sint16 turnFR = (sint16)(((float32)g_calDutyFR + comp) * g_calTurnFront / 100.0f * g_vehicleSpeed / 100.0f);
    sint16 turnRL = (sint16)(((float32)g_calDutyRL + comp) * g_calTurnRear  / 100.0f * g_vehicleSpeed / 100.0f);
    sint16 turnRR = (sint16)(((float32)g_calDutyRR + comp) * g_calTurnRear  / 100.0f * g_vehicleSpeed / 100.0f);

    /* ── 90도 회전 상태 머신 ────────────────────────────── */
    if (s_turn90Active)
    {
        float32 delta = yawDelta(DrvMpu9250_GetYaw(), s_turn90StartYaw);
        float32 absDelta = (delta < 0.0f) ? -delta : delta;

        s_turn90Counter++;

        /* 목표 도달 또는 타임아웃 → 정지 */
        if ((absDelta >= (TURN90_TARGET - TURN90_MARGIN)) ||
            (s_turn90Counter >= TURN90_TIMEOUT))
        {
            s_turn90Active = FALSE;
            g_vehicleCmd   = VEHICLE_STOP;
            DrvMotor_Coast(MOTOR_FL);
            DrvMotor_Coast(MOTOR_FR);
            DrvMotor_Coast(MOTOR_RL);
            DrvMotor_Coast(MOTOR_RR);
            return;
        }

        /* 회전 계속 */
        if (s_turn90Dir > 0)    /* 우회전 */
        {
            DrvMotor_SetDuty(MOTOR_FL,  turnFL);
            DrvMotor_SetDuty(MOTOR_FR, -turnFR);
            DrvMotor_SetDuty(MOTOR_RL,  turnRL);
            DrvMotor_SetDuty(MOTOR_RR, -turnRR);
        }
        else                    /* 좌회전 */
        {
            DrvMotor_SetDuty(MOTOR_FL, -turnFL);
            DrvMotor_SetDuty(MOTOR_FR,  turnFR);
            DrvMotor_SetDuty(MOTOR_RL, -turnRL);
            DrvMotor_SetDuty(MOTOR_RR,  turnRR);
        }
        return;
    }

    /* ── Yaw 0도 복귀 상태 머신 ──────────────────────────── */
    if (s_yawZeroActive)
    {
        float32 curYaw = DrvMpu9250_GetYaw();
        float32 absYaw = (curYaw < 0.0f) ? -curYaw : curYaw;

        s_yawZeroCounter++;

        /* ±5° 이내 도달 또는 타임아웃 → 래치: 정지하고 종료 */
        if ((absYaw <= YAW_ZERO_MARGIN) ||
            (s_yawZeroCounter >= YAW_ZERO_TIMEOUT))
        {
            s_yawZeroActive = FALSE;
            g_vehicleCmd    = VEHICLE_STOP;
            DrvMotor_Coast(MOTOR_FL);
            DrvMotor_Coast(MOTOR_FR);
            DrvMotor_Coast(MOTOR_RL);
            DrvMotor_Coast(MOTOR_RR);
            return;
        }

        /* 회전 계속 */
        if (s_yawZeroDir > 0)   /* 우회전 */
        {
            DrvMotor_SetDuty(MOTOR_FL,  turnFL);
            DrvMotor_SetDuty(MOTOR_FR, -turnFR);
            DrvMotor_SetDuty(MOTOR_RL,  turnRL);
            DrvMotor_SetDuty(MOTOR_RR, -turnRR);
        }
        else                    /* 좌회전 */
        {
            DrvMotor_SetDuty(MOTOR_FL, -turnFL);
            DrvMotor_SetDuty(MOTOR_FR,  turnFR);
            DrvMotor_SetDuty(MOTOR_RL, -turnRL);
            DrvMotor_SetDuty(MOTOR_RR,  turnRR);
        }
        return;
    }

    /* ── 90도 회전 시작 트리거 ───────────────────────────── */
    if (g_vehicleCmd == VEHICLE_TURN90_R || g_vehicleCmd == VEHICLE_TURN90_L)
    {
        s_turn90Active   = TRUE;
        s_turn90Dir      = (g_vehicleCmd == VEHICLE_TURN90_R) ? 1 : -1;
        s_turn90StartYaw = DrvMpu9250_GetYaw();
        s_turn90Counter  = 0u;
        return;
    }

    /* ── Yaw 0도 복귀 시작 트리거 ─────────────────────────── */
    if (g_vehicleCmd == VEHICLE_YAW_ZERO)
    {
        float32 curYaw = DrvMpu9250_GetYaw();
        float32 absYaw = (curYaw < 0.0f) ? -curYaw : curYaw;

        /* 이미 ±5° 이내이면 동작 불필요 */
        if (absYaw <= YAW_ZERO_MARGIN)
        {
            g_vehicleCmd = VEHICLE_STOP;
            return;
        }

        s_yawZeroActive  = TRUE;
        /* yaw > 0 (왼쪽 회전 상태) → 우회전(-), yaw < 0 → 좌회전(+) */
        s_yawZeroDir     = (curYaw > 0.0f) ? 1 : -1;
        s_yawZeroCounter = 0u;
        return;
    }

    /* ── 일반 주행 ──────────────────────────────────────── */
    switch ((VehicleCommand)g_vehicleCmd)
    {
    case VEHICLE_FORWARD:
        DrvMotor_SetDuty(MOTOR_FL,  spdFL);
        DrvMotor_SetDuty(MOTOR_FR,  spdFR);
        DrvMotor_SetDuty(MOTOR_RL,  spdRL);
        DrvMotor_SetDuty(MOTOR_RR,  spdRR);
        break;

    case VEHICLE_REVERSE:
        DrvMotor_SetDuty(MOTOR_FL, -spdFL);
        DrvMotor_SetDuty(MOTOR_FR, -spdFR);
        DrvMotor_SetDuty(MOTOR_RL, -spdRL);
        DrvMotor_SetDuty(MOTOR_RR, -spdRR);
        break;

    case VEHICLE_SPIN_LEFT:
        DrvMotor_SetDuty(MOTOR_FL, -turnFL);
        DrvMotor_SetDuty(MOTOR_FR,  turnFR);
        DrvMotor_SetDuty(MOTOR_RL, -turnRL);
        DrvMotor_SetDuty(MOTOR_RR,  turnRR);
        break;

    case VEHICLE_SPIN_RIGHT:
        DrvMotor_SetDuty(MOTOR_FL,  turnFL);
        DrvMotor_SetDuty(MOTOR_FR, -turnFR);
        DrvMotor_SetDuty(MOTOR_RL,  turnRL);
        DrvMotor_SetDuty(MOTOR_RR, -turnRR);
        break;

    default: /* VEHICLE_STOP */
        DrvMotor_Coast(MOTOR_FL);
        DrvMotor_Coast(MOTOR_FR);
        DrvMotor_Coast(MOTOR_RL);
        DrvMotor_Coast(MOTOR_RR);
        break;
    }
}
