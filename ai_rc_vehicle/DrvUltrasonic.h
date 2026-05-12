/**********************************************************************************************************************
 * \file DrvUltrasonic.h
 * \brief HC-SR04 Ultrasonic sensor driver
 *
 * TRIG: P02.6 (GPIO output, 10us pulse)
 * ECHO: P02.7 (GTM TIM0_7, PWM measurement)
 *********************************************************************************************************************/
#ifndef DRVULTRASONIC_H
#define DRVULTRASONIC_H

#include "Ifx_Types.h"

void DrvUltrasonic_Init(void);
void DrvUltrasonic_Trigger(void);
float32 DrvUltrasonic_GetDistanceCm(void);

#endif /* DRVULTRASONIC_H */
