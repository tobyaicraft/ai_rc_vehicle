/**********************************************************************************************************************
 * \file DrvMotor.h
 * \brief 4WD 모터 드라이버 래퍼 — DrvPwm + DrvDio 통합 인터페이스
 *
 * [Driver Layer]
 *   DrvMotor_SetDuty(id, duty)  duty: -100 ~ +100 (음수=역회전)
 *   DrvMotor_Brake(id)          능동 제동 (IN1=H, IN2=H, PWM=0)
 *   DrvMotor_Coast(id)          자유 회전 (IN1=L, IN2=L, PWM=0)
 *
 * 방향 전환 시 슛스루 방지 시퀀스:
 *   PWM=0 → GPIO 방향 변경 → PWM 설정
 *********************************************************************************************************************/
#ifndef DRVMOTOR_H
#define DRVMOTOR_H

#include "Ifx_Types.h"

typedef enum
{
    MOTOR_FL    = 0,
    MOTOR_FR    = 1,
    MOTOR_RL    = 2,
    MOTOR_RR    = 3,
    MOTOR_COUNT = 4
} MotorId;

void DrvMotor_Init(void);
void DrvMotor_SetDuty(MotorId id, sint16 duty);  /* -100 ~ +100 */
void DrvMotor_Brake(MotorId id);
void DrvMotor_Coast(MotorId id);

#endif /* DRVMOTOR_H */
