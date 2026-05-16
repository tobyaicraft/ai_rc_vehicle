/**********************************************************************************************************************
 * \file AppObstacle.c
 * \brief 장애물 감지 — 3개 센서 융합 로직
 *
 * 판단 우선순위:
 *   1. 전방(초음파) 위험 → OBSTACLE_FRONT
 *   2. 좌+우 동시 위험   → OBSTACLE_BOTH_SIDES
 *   3. 좌측만 위험       → OBSTACLE_LEFT
 *   4. 우측만 위험       → OBSTACLE_RIGHT
 *   5. 안전              → OBSTACLE_NONE
 *********************************************************************************************************************/
#include "AppObstacle.h"
#include "DrvSensorFusion.h"

ObstacleType_t AppObstacle_Detect(void)
{
    uint16 front = DrvSensorFusion_GetUltrasonic();
    uint16 left  = DrvSensorFusion_GetIrLeft();
    uint16 right = DrvSensorFusion_GetIrRight();

    /* 초음파: 거리(cm) 기준 — 작을수록 가까움 */
    boolean front_blocked = (front < DANGER_DISTANCE_FRONT);

    /* IR: ADC raw 기준 — 클수록 가까움 (역비례 특성) */
    boolean left_blocked  = (left  > IR_DANGER_THRESHOLD);
    boolean right_blocked = (right > IR_DANGER_THRESHOLD);

    /* 우선순위 기반 판단 */
    if (front_blocked)                       return OBSTACLE_FRONT;
    if (left_blocked && right_blocked)       return OBSTACLE_BOTH_SIDES;
    if (left_blocked)                        return OBSTACLE_LEFT;
    if (right_blocked)                       return OBSTACLE_RIGHT;

    return OBSTACLE_NONE;
}
