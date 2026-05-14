/**********************************************************************************************************************
 * \file AppVehicle.c
 * \brief 4WD RC Vehicle 제어 모듈
 *********************************************************************************************************************/
#include "AppVehicle.h"
#include "DrvMotor.h"

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
    g_vehicleSpeed = 80.0f;
}

void AppVehicle_Update(void)
{
    sint16 spd  = (sint16)g_vehicleSpeed;
    sint16 spdF = (sint16)(g_vehicleSpeed * 0.86f);  /* 전륜 보정 */

    switch ((VehicleCommand)g_vehicleCmd)
    {
    case VEHICLE_FORWARD:
        DrvMotor_SetDuty(MOTOR_FL,  spdF);
        DrvMotor_SetDuty(MOTOR_FR,  spdF);
        DrvMotor_SetDuty(MOTOR_RL,  spd);
        DrvMotor_SetDuty(MOTOR_RR,  spd);
        break;

    case VEHICLE_REVERSE:
        DrvMotor_SetDuty(MOTOR_FL, -spdF);
        DrvMotor_SetDuty(MOTOR_FR, -spdF);
        DrvMotor_SetDuty(MOTOR_RL, -spd);
        DrvMotor_SetDuty(MOTOR_RR, -spd);
        break;

    case VEHICLE_SPIN_LEFT:
        DrvMotor_SetDuty(MOTOR_FL, -spdF);
        DrvMotor_SetDuty(MOTOR_FR,  spdF);
        DrvMotor_SetDuty(MOTOR_RL, -spd);
        DrvMotor_SetDuty(MOTOR_RR,  spd);
        break;

    case VEHICLE_SPIN_RIGHT:
        DrvMotor_SetDuty(MOTOR_FL,  spdF);
        DrvMotor_SetDuty(MOTOR_FR, -spdF);
        DrvMotor_SetDuty(MOTOR_RL,  spd);
        DrvMotor_SetDuty(MOTOR_RR, -spd);
        break;

    default: /* VEHICLE_STOP */
        DrvMotor_Coast(MOTOR_FL);
        DrvMotor_Coast(MOTOR_FR);
        DrvMotor_Coast(MOTOR_RL);
        DrvMotor_Coast(MOTOR_RR);
        break;
    }
}
