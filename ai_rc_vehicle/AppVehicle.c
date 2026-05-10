/**********************************************************************************************************************
 * \file AppVehicle.c
 * \brief 4WD RC Vehicle 제어 모듈
 *
 * VehicleCommand에 따라 4개 모터의 PWM duty + 방향을 설정
 * - 전진/후진: 4륜 동일 속도
 * - 좌/우 회전: 안쪽 바퀴 감속 (turnRatio)
 * - 좌/우 스핀: 양쪽 반대 방향 (제자리 회전)
 *********************************************************************************************************************/
#include "AppVehicle.h"
#include "DrvPwm.h"
#include "DrvDio.h"

/******************************************************************************/
/*                           Global Variables                                 */
/******************************************************************************/
volatile uint8   g_vehicleCmd   = VEHICLE_STOP;
volatile float32 g_vehicleSpeed = 50.0f;       /* 기본 속도 [%] */

/******************************************************************************/
/*                           Module Variables                                 */
/******************************************************************************/
static float32 s_spinSpeed = 80.0f;    /* 스핀 속도 [%] */
static float32 s_turnRatio = 0.6f;     /* 회전 시 안쪽 바퀴 비율 */

/******************************************************************************/
/*                           Functions                                        */
/******************************************************************************/
void AppVehicle_Init(void)
{
    g_vehicleCmd   = VEHICLE_STOP;
    g_vehicleSpeed = 50.0f;
}

void AppVehicle_Update(void)
{
    float32 speed = g_vehicleSpeed;
    float32 innerSpeed = speed * s_turnRatio;
    MotorDirection dirFL, dirFR, dirRL, dirRR;
    float32 dutyFL, dutyFR, dutyRL, dutyRR;

    switch ((VehicleCommand)g_vehicleCmd)
    {
    case VEHICLE_FORWARD:
        dirFL = MOTOR_FORWARD;  dirFR = MOTOR_FORWARD;
        dirRL = MOTOR_FORWARD;  dirRR = MOTOR_FORWARD;
        dutyFL = speed;  dutyFR = speed;
        dutyRL = speed;  dutyRR = speed;
        break;

    case VEHICLE_REVERSE:
        dirFL = MOTOR_REVERSE;  dirFR = MOTOR_REVERSE;
        dirRL = MOTOR_REVERSE;  dirRR = MOTOR_REVERSE;
        dutyFL = speed;  dutyFR = speed;
        dutyRL = speed;  dutyRR = speed;
        break;

    case VEHICLE_TURN_LEFT:
        dirFL = MOTOR_FORWARD;  dirFR = MOTOR_FORWARD;
        dirRL = MOTOR_FORWARD;  dirRR = MOTOR_FORWARD;
        dutyFL = innerSpeed;  dutyFR = speed;
        dutyRL = innerSpeed;  dutyRR = speed;
        break;

    case VEHICLE_TURN_RIGHT:
        dirFL = MOTOR_FORWARD;  dirFR = MOTOR_FORWARD;
        dirRL = MOTOR_FORWARD;  dirRR = MOTOR_FORWARD;
        dutyFL = speed;  dutyFR = innerSpeed;
        dutyRL = speed;  dutyRR = innerSpeed;
        break;

    case VEHICLE_SPIN_LEFT:
        dirFL = MOTOR_REVERSE;  dirFR = MOTOR_FORWARD;
        dirRL = MOTOR_REVERSE;  dirRR = MOTOR_FORWARD;
        dutyFL = s_spinSpeed;  dutyFR = s_spinSpeed;
        dutyRL = s_spinSpeed;  dutyRR = s_spinSpeed;
        break;

    case VEHICLE_SPIN_RIGHT:
        dirFL = MOTOR_FORWARD;  dirFR = MOTOR_REVERSE;
        dirRL = MOTOR_FORWARD;  dirRR = MOTOR_REVERSE;
        dutyFL = s_spinSpeed;  dutyFR = s_spinSpeed;
        dutyRL = s_spinSpeed;  dutyRR = s_spinSpeed;
        break;

    default: /* VEHICLE_STOP */
        dirFL = MOTOR_STOP;  dirFR = MOTOR_STOP;
        dirRL = MOTOR_STOP;  dirRR = MOTOR_STOP;
        dutyFL = 0.0f;  dutyFR = 0.0f;
        dutyRL = 0.0f;  dutyRR = 0.0f;
        break;
    }

    /* PWM duty 적용 */
    DrvPwm_SetDutyFL(dutyFL);
    DrvPwm_SetDutyFR(dutyFR);
    DrvPwm_SetDutyRL(dutyRL);
    DrvPwm_SetDutyRR(dutyRR);

    /* 방향 적용 */
    DrvDio_SetMotorFL(dirFL);
    DrvDio_SetMotorFR(dirFR);
    DrvDio_SetMotorRL(dirRL);
    DrvDio_SetMotorRR(dirRR);
}
