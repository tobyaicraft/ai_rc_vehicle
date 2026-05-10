/**********************************************************************************************************************
 * \file AppTask.h
 * \brief RC Vehicle application tasks (1ms / 10ms / 100ms)
 *********************************************************************************************************************/
#ifndef APPTASK_H
#define APPTASK_H

#include "Ifx_Types.h"

/** \brief 1ms periodic task - HC-12 수신 처리 */
void AppTask_1ms(void);

/** \brief 10ms periodic task - Vehicle 제어 업데이트 */
void AppTask_10ms(void);

/** \brief 100ms periodic task - LED 하트비트 */
void AppTask_100ms(void);

#endif /* APPTASK_H */
