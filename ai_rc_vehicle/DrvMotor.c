/**********************************************************************************************************************
 * \file DrvMotor.c
 * \brief 4WD 모터 드라이버 래퍼
 *
 * 방향 전환 시 슛스루 방지:
 *   이전 방향 != 새 방향이면 → PWM=0 → GPIO 방향 변경 → PWM 설정
 *********************************************************************************************************************/
#include "DrvMotor.h"
#include "DrvPwm.h"
#include "DrvDio.h"

/******************************************************************************/
/*                           Module Variables                                 */
/******************************************************************************/
static MotorDirection s_dir[MOTOR_COUNT];

/******************************************************************************/
/*                           Static Helpers                                   */
/******************************************************************************/
typedef void (*SetDutyFn)(float32);
typedef void (*SetDirFn)(MotorDirection);

static const SetDutyFn s_setDuty[MOTOR_COUNT] = {
    DrvPwm_SetDutyFL, DrvPwm_SetDutyFR, DrvPwm_SetDutyRL, DrvPwm_SetDutyRR
};

static const SetDirFn s_setDir[MOTOR_COUNT] = {
    DrvDio_SetMotorFL, DrvDio_SetMotorFR, DrvDio_SetMotorRL, DrvDio_SetMotorRR
};

/******************************************************************************/
/*                           Functions                                        */
/******************************************************************************/
void DrvMotor_Init(void)
{
    uint8 i;
    for (i = 0; i < MOTOR_COUNT; i++)
        s_dir[i] = MOTOR_STOP;
}

void DrvMotor_SetDuty(MotorId id, sint16 duty)
{
    MotorDirection newDir;
    float32 absDuty;

    if (duty >  100) duty =  100;
    if (duty < -100) duty = -100;

    if      (duty > 0) newDir = MOTOR_FORWARD;
    else if (duty < 0) newDir = MOTOR_REVERSE;
    else               newDir = MOTOR_STOP;

    absDuty = (duty < 0) ? (float32)(-duty) : (float32)duty;

    if (newDir != s_dir[id])
    {
        s_setDuty[id](0.0f);          /* 1. PWM=0 */
        s_setDir[id](newDir);         /* 2. 방향 변경 */
        s_dir[id] = newDir;
    }

    s_setDuty[id](absDuty);           /* 3. PWM 설정 */
}

void DrvMotor_Brake(MotorId id)
{
    s_setDuty[id](0.0f);
    s_setDir[id](MOTOR_BRAKE);
    s_dir[id] = MOTOR_BRAKE;
}

void DrvMotor_Coast(MotorId id)
{
    s_setDuty[id](0.0f);
    s_setDir[id](MOTOR_STOP);
    s_dir[id] = MOTOR_STOP;
}
