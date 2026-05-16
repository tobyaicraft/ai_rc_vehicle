/**********************************************************************************************************************
 * \file DrvBuzzer.h
 * \brief Passive buzzer driver (PWM, variable frequency)
 *
 * Buzzer : TOM0_Ch10 -> TOUT2 -> P02.2 (X103 pin 15)
 * Duty 50% = ON / 0% = OFF, frequency 변경으로 음계 제어
 *********************************************************************************************************************/
#ifndef DRVBUZZER_H
#define DRVBUZZER_H

#include "Ifx_Types.h"

void DrvBuzzer_Init(void);
void DrvBuzzer_PlayNote(uint16 freqHz);   /* 해당 주파수로 소리 출력 */
void DrvBuzzer_Off(void);

#endif /* DRVBUZZER_H */
