/**********************************************************************************************************************
 * \file AppVehicle.c
 * \brief 4WD RC Vehicle 제어 모듈
 *********************************************************************************************************************/
#include "AppVehicle.h"
#include "DrvMotor.h"
#include "DrvFlash.h"

/******************************************************************************/
/*                           Global Variables                                 */
/******************************************************************************/
volatile uint8   g_vehicleCmd   = VEHICLE_STOP;
volatile float32 g_vehicleSpeed = 50.0f;

/******************************************************************************/
/*                           Functions                                        */
/******************************************************************************/
void AppVehicle_Init(void)
{
    g_vehicleCmd   = VEHICLE_STOP;
    g_vehicleSpeed = 100.0f;    /* MOVE 명령 스케일 (캘리브레이션 Duty와 곱해짐) */
}

void AppVehicle_Update(void)
{
    /* 각 모터별 캘리브레이션 Duty × 명령 속도 / 100 */
    sint16 spdFL = (sint16)((float32)g_calDutyFL * g_vehicleSpeed / 100.0f);
    sint16 spdFR = (sint16)((float32)g_calDutyFR * g_vehicleSpeed / 100.0f);
    sint16 spdRL = (sint16)((float32)g_calDutyRL * g_vehicleSpeed / 100.0f);
    sint16 spdRR = (sint16)((float32)g_calDutyRR * g_vehicleSpeed / 100.0f);

    /* 회전용: 캘리브레이션 Duty × 턴팩터 / 100 × 명령 속도 / 100 */
    sint16 turnFL = (sint16)((float32)g_calDutyFL * g_calTurnFront / 100.0f * g_vehicleSpeed / 100.0f);
    sint16 turnFR = (sint16)((float32)g_calDutyFR * g_calTurnFront / 100.0f * g_vehicleSpeed / 100.0f);
    sint16 turnRL = (sint16)((float32)g_calDutyRL * g_calTurnRear  / 100.0f * g_vehicleSpeed / 100.0f);
    sint16 turnRR = (sint16)((float32)g_calDutyRR * g_calTurnRear  / 100.0f * g_vehicleSpeed / 100.0f);

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
