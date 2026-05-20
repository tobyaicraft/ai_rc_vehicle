/**********************************************************************************************************************
 * \file DrvSensorFusion.h
 * \brief 센서 노이즈 필터링 (이동 평균 + 이상치 제거)
 *
 * - MovingAvg: 이동 평균 필터 (FILTER_SIZE 샘플)
 * - 적외선(IR), 초음파(US) 센서에 적용
 * - 필터링된 값을 AppObstacle에서 사용
 *********************************************************************************************************************/
#ifndef DRVSENSORFUSION_H
#define DRVSENSORFUSION_H

#include "Ifx_Types.h"

#define SENSOR_FILTER_SIZE  3u

typedef struct
{
    uint16  buffer[SENSOR_FILTER_SIZE];
    uint8   index;
    boolean filled;
} MovingAvg_t;

void   MovingAvg_Init(MovingAvg_t *f);
uint16 MovingAvg_Update(MovingAvg_t *f, uint16 newValue);

/* 필터링된 센서 값 (100ms 주기로 갱신) */
void   DrvSensorFusion_Update(void);
uint16 DrvSensorFusion_GetIrLeft(void);
uint16 DrvSensorFusion_GetIrRight(void);
uint16 DrvSensorFusion_GetUltrasonic(void);

#endif /* DRVSENSORFUSION_H */
