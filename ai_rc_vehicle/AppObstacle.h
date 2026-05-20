/**********************************************************************************************************************
 * \file AppObstacle.h
 * \brief 장애물 감지 — 3개 센서 융합 (IR Left + IR Right + Ultrasonic)
 *
 * 필터링된 센서 값을 기반으로 장애물 방향을 판단
 * AUTO 모드에서 자율 회피에 사용
 *********************************************************************************************************************/
#ifndef APPOBSTACLE_H
#define APPOBSTACLE_H

#include "Ifx_Types.h"

#define DANGER_DISTANCE_FRONT  25u   /* 전방 위험 거리 (cm) — 초음파 */
#define DANGER_DISTANCE_SIDE   15u   /* 측면 위험 거리 (cm) — IR 변환 */

/* IR ADC → 대략적 거리 변환 임계값 (ADC raw)
 * GP2Y0A21: ~2500 ADC ≈ 15cm, ~1500 ADC ≈ 25cm */
#define IR_DANGER_THRESHOLD   2500u  /* 이 값 이상이면 가까움 (위험) */

typedef enum
{
    OBSTACLE_NONE       = 0,
    OBSTACLE_FRONT      = 1,
    OBSTACLE_LEFT       = 2,
    OBSTACLE_RIGHT      = 3,
    OBSTACLE_BOTH_SIDES = 4
} ObstacleType_t;

ObstacleType_t AppObstacle_Detect(void);

#endif /* APPOBSTACLE_H */
