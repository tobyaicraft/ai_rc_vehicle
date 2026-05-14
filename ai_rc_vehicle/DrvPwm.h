/**********************************************************************************************************************
 * \file DrvPwm.h
 * \brief GTM TOM0 4채널 모터 PWM 드라이버 (1kHz)
 *
 * FL: TOM0_Ch0  -> TOUT26 -> P33.4  (X102 pin 14)
 * FR: TOM0_Ch1  -> TOUT27 -> P33.5  (X102 pin 13)
 * RL: TOM0_Ch6  -> TOUT4  -> P02.4  (X103 pin 17)
 * RR: TOM0_Ch13 -> TOUT5  -> P02.5  (X103 pin 18)
 *********************************************************************************************************************/
#ifndef DRVPWM_H
#define DRVPWM_H

#include "Ifx_Types.h"

void DrvPwm_Init(void);
void DrvPwm_SetDutyFL(float32 dutyPercent);   /* 0.0 ~ 100.0 */
void DrvPwm_SetDutyFR(float32 dutyPercent);
void DrvPwm_SetDutyRL(float32 dutyPercent);
void DrvPwm_SetDutyRR(float32 dutyPercent);

#endif /* DRVPWM_H */
