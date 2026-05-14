/**********************************************************************************************************************
 * \file DrvDio.h
 * \brief Digital I/O driver - LED + Motor direction control
 *
 * Motor Direction GPIO:
 *   FL: IN1=P33.2, IN2=P33.3   (X102 pin 16, 15)
 *   FR: IN1=P33.1, IN2=P33.12  (X102 pin 17, 18)
 *   RL: IN1=P00.0, IN2=P00.1   (X103 pin 22, 23)
 *   RR: IN1=P00.2, IN2=P00.3   (X103 pin 24, 25)
 *********************************************************************************************************************/
#ifndef DRVDIO_H
#define DRVDIO_H

#include "Ifx_Types.h"

typedef enum
{
    MOTOR_STOP    = 0,   /* IN1=L, IN2=L (coast) */
    MOTOR_FORWARD = 1,   /* IN1=H, IN2=L */
    MOTOR_REVERSE = 2,   /* IN1=L, IN2=H */
    MOTOR_BRAKE   = 3    /* IN1=H, IN2=H (short brake) */
} MotorDirection;

void DrvDio_Init(void);

/* LED */
void DrvDio_ToggleLed0(void);
void DrvDio_SetLed0On(void);
void DrvDio_SetLed0Off(void);

/* Motor direction */
void DrvDio_SetMotorFL(MotorDirection dir);
void DrvDio_SetMotorFR(MotorDirection dir);
void DrvDio_SetMotorRL(MotorDirection dir);
void DrvDio_SetMotorRR(MotorDirection dir);

#endif /* DRVDIO_H */
