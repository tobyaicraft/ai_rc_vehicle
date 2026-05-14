/**********************************************************************************************************************
 * \file DrvDio.c
 * \brief Digital I/O driver - LED + Motor direction control
 *
 * LED0 : P13.0 (active-low)
 *
 * Motor Direction GPIO (H-Bridge IN1/IN2):
 *   FL: IN1=P33.2, IN2=P33.3   (X102 pin 16, 15)
 *   FR: IN1=P33.1, IN2=P33.12  (X102 pin 17, 18)
 *   RL: IN1=P00.0, IN2=P00.1   (X103 pin 22, 23)
 *   RR: IN1=P00.2, IN2=P00.3   (X103 pin 24, 25)
 *
 * Direction logic:  STOP=LOW/LOW, FWD=HIGH/LOW, REV=LOW/HIGH
 *********************************************************************************************************************/
#include "DrvDio.h"
#include "IfxPort.h"

/******************************************************************************/
/*                           Functions                                        */
/******************************************************************************/
void DrvDio_Init(void)
{
    /* LED0: P13.0 (active-low) */
    IfxPort_setPinMode(&MODULE_P13, 0, IfxPort_Mode_outputPushPullGeneral);
    IfxPort_setPinHigh(&MODULE_P13, 0);

    /* Motor FL: P33.2 (IN1), P33.3 (IN2) */
    IfxPort_setPinMode(&MODULE_P33, 2, IfxPort_Mode_outputPushPullGeneral);
    IfxPort_setPinMode(&MODULE_P33, 3, IfxPort_Mode_outputPushPullGeneral);
    IfxPort_setPinLow(&MODULE_P33, 2);
    IfxPort_setPinLow(&MODULE_P33, 3);

    /* Motor FR: P33.1 (IN1), P33.12 (IN2) */
    IfxPort_setPinMode(&MODULE_P33, 1, IfxPort_Mode_outputPushPullGeneral);
    IfxPort_setPinMode(&MODULE_P33, 12, IfxPort_Mode_outputPushPullGeneral);
    IfxPort_setPinLow(&MODULE_P33, 1);
    IfxPort_setPinLow(&MODULE_P33, 12);

    /* Motor RL: P00.0 (IN1), P00.1 (IN2) */
    IfxPort_setPinMode(&MODULE_P00, 0, IfxPort_Mode_outputPushPullGeneral);
    IfxPort_setPinMode(&MODULE_P00, 1, IfxPort_Mode_outputPushPullGeneral);
    IfxPort_setPinLow(&MODULE_P00, 0);
    IfxPort_setPinLow(&MODULE_P00, 1);

    /* Motor RR: P00.2 (IN1), P00.3 (IN2) */
    IfxPort_setPinMode(&MODULE_P00, 2, IfxPort_Mode_outputPushPullGeneral);
    IfxPort_setPinMode(&MODULE_P00, 3, IfxPort_Mode_outputPushPullGeneral);
    IfxPort_setPinLow(&MODULE_P00, 2);
    IfxPort_setPinLow(&MODULE_P00, 3);
}

/* LED */
void DrvDio_ToggleLed0(void)  { IfxPort_togglePin(&MODULE_P13, 0); }
void DrvDio_SetLed0On(void)   { IfxPort_setPinLow(&MODULE_P13, 0); }
void DrvDio_SetLed0Off(void)  { IfxPort_setPinHigh(&MODULE_P13, 0); }

/* Motor FL: IN1=P33.2, IN2=P33.3 */
void DrvDio_SetMotorFL(MotorDirection dir)
{
    switch (dir)
    {
    case MOTOR_FORWARD:
        IfxPort_setPinHigh(&MODULE_P33, 2);
        IfxPort_setPinLow(&MODULE_P33, 3);
        break;
    case MOTOR_REVERSE:
        IfxPort_setPinLow(&MODULE_P33, 2);
        IfxPort_setPinHigh(&MODULE_P33, 3);
        break;
    case MOTOR_BRAKE:
        IfxPort_setPinHigh(&MODULE_P33, 2);
        IfxPort_setPinHigh(&MODULE_P33, 3);
        break;
    default: /* MOTOR_STOP */
        IfxPort_setPinLow(&MODULE_P33, 2);
        IfxPort_setPinLow(&MODULE_P33, 3);
        break;
    }
}

/* Motor FR: IN1=P33.1, IN2=P33.12 (실측: 방향 반전 보정) */
void DrvDio_SetMotorFR(MotorDirection dir)
{
    switch (dir)
    {
    case MOTOR_FORWARD:
        IfxPort_setPinLow(&MODULE_P33, 1);
        IfxPort_setPinHigh(&MODULE_P33, 12);
        break;
    case MOTOR_REVERSE:
        IfxPort_setPinHigh(&MODULE_P33, 1);
        IfxPort_setPinLow(&MODULE_P33, 12);
        break;
    case MOTOR_BRAKE:
        IfxPort_setPinHigh(&MODULE_P33, 1);
        IfxPort_setPinHigh(&MODULE_P33, 12);
        break;
    default: /* MOTOR_STOP */
        IfxPort_setPinLow(&MODULE_P33, 1);
        IfxPort_setPinLow(&MODULE_P33, 12);
        break;
    }
}

/* Motor RL: IN1=P00.0, IN2=P00.1 (실측: 방향 반전 보정) */
void DrvDio_SetMotorRL(MotorDirection dir)
{
    switch (dir)
    {
    case MOTOR_FORWARD:
        IfxPort_setPinLow(&MODULE_P00, 0);
        IfxPort_setPinHigh(&MODULE_P00, 1);
        break;
    case MOTOR_REVERSE:
        IfxPort_setPinHigh(&MODULE_P00, 0);
        IfxPort_setPinLow(&MODULE_P00, 1);
        break;
    case MOTOR_BRAKE:
        IfxPort_setPinHigh(&MODULE_P00, 0);
        IfxPort_setPinHigh(&MODULE_P00, 1);
        break;
    default: /* MOTOR_STOP */
        IfxPort_setPinLow(&MODULE_P00, 0);
        IfxPort_setPinLow(&MODULE_P00, 1);
        break;
    }
}

/* Motor RR: IN1=P00.2, IN2=P00.3 */
void DrvDio_SetMotorRR(MotorDirection dir)
{
    switch (dir)
    {
    case MOTOR_FORWARD:
        IfxPort_setPinHigh(&MODULE_P00, 2);
        IfxPort_setPinLow(&MODULE_P00, 3);
        break;
    case MOTOR_REVERSE:
        IfxPort_setPinLow(&MODULE_P00, 2);
        IfxPort_setPinHigh(&MODULE_P00, 3);
        break;
    case MOTOR_BRAKE:
        IfxPort_setPinHigh(&MODULE_P00, 2);
        IfxPort_setPinHigh(&MODULE_P00, 3);
        break;
    default: /* MOTOR_STOP */
        IfxPort_setPinLow(&MODULE_P00, 2);
        IfxPort_setPinLow(&MODULE_P00, 3);
        break;
    }
}
