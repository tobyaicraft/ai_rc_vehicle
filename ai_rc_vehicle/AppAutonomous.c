/**********************************************************************************************************************
 * \file AppAutonomous.c
 * \brief Bug Algorithm 기반 자율주행 상태 머신
 *
 * [동작 순서]
 *   1. 직진
 *   2. 전방 장애물 감지 시:
 *      a. 후진 600ms (안전 거리 확보)
 *      b. IR 좌/우 비교 → 여유 있는 쪽으로 90도 회전
 *      c. 1.5초 직진 (장애물 통과)
 *      d. Yaw 0° 복귀 (원래 진행 방향 복원)
 *   3. 다시 직진
 *********************************************************************************************************************/
#include "AppAutonomous.h"
#include "AppVehicle.h"
#include "AppObstacle.h"
#include "DrvSensorFusion.h"
#include "DrvGtmTimer.h"

/******************************************************************************/
/*                           Configuration                                    */
/******************************************************************************/
#define BACKUP_TIME_MS    600u    /* 후진 지속 시간 (ms) */
#define PAUSE_TIME_MS     100u    /* 후진→회전 정지 시간 (ms) */
#define BYPASS_TIME_MS   1000u    /* 장애물 통과 직진 시간 (ms) */

#define FORWARD_SPEED    60.0f    /* 직진 속도 (%) */
#define BACKUP_SPEED     60.0f    /* 후진 속도 (%) */
#define TURN_SPEED      100.0f    /* 회전 속도 (%) */

/******************************************************************************/
/*                           State Machine                                    */
/******************************************************************************/
typedef enum
{
    AUTO_IDLE = 0,
    AUTO_FORWARD,          /* 직진 */
    AUTO_BACKWARD,         /* 후진 — 안전 거리 확보 */
    AUTO_PAUSE,            /* 후진→회전 사이 정지 (모터 안정화) */
    AUTO_TURNING,          /* 90도 회전 */
    AUTO_BYPASS_FORWARD,   /* 직진 — 장애물 통과 */
    AUTO_YAW_RETURN        /* Yaw 0° 복귀 — 원래 방향 복원 */
} AutoState_t;

static AutoState_t s_state        = AUTO_IDLE;
static uint32      s_stateStartMs = 0u;

static void enterState(AutoState_t next)
{
    s_state        = next;
    s_stateStartMs = g_1ms_counter;
}

/******************************************************************************/
/*                           Public API                                       */
/******************************************************************************/
void AppAutonomous_Start(void)
{
    g_vehicleSpeed = FORWARD_SPEED;
    enterState(AUTO_FORWARD);
}

void AppAutonomous_Stop(void)
{
    g_vehicleCmd = VEHICLE_STOP;
    enterState(AUTO_IDLE);
}

void AppAutonomous_Update(void)
{
    uint32         elapsed = g_1ms_counter - s_stateStartMs;
    ObstacleType_t obs     = AppObstacle_Detect();

    switch (s_state)
    {
    /* ── IDLE ─────────────────────────────────────────────────── */
    case AUTO_IDLE:
        g_vehicleCmd = VEHICLE_STOP;
        break;

    /* ── 1. 직진 ──────────────────────────────────────────────── */
    case AUTO_FORWARD:
        g_vehicleSpeed = FORWARD_SPEED;
        g_vehicleCmd   = VEHICLE_FORWARD;

        if (obs == OBSTACLE_FRONT || obs == OBSTACLE_BOTH_SIDES)
        {
            g_vehicleCmd = VEHICLE_STOP;
            enterState(AUTO_BACKWARD);
        }
        break;

    /* ── 2a. 후진 (안전 거리 확보) ───────────────────────────── */
    case AUTO_BACKWARD:
        g_vehicleSpeed = BACKUP_SPEED;
        g_vehicleCmd   = VEHICLE_REVERSE;

        if (elapsed >= BACKUP_TIME_MS)
        {
            g_vehicleCmd = VEHICLE_STOP;
            enterState(AUTO_PAUSE);
        }
        break;

    /* ── 2a'. 정지 (모터 안정화 후 회전) ─────────────────────── */
    case AUTO_PAUSE:
        g_vehicleCmd = VEHICLE_STOP;

        if (elapsed >= PAUSE_TIME_MS)
        {
            /* IR raw: 작을수록 멀다 = 여유 있음 → 여유 있는 쪽으로 회전 */
            uint16 irLeft  = DrvSensorFusion_GetIrLeft();
            uint16 irRight = DrvSensorFusion_GetIrRight();

            g_vehicleSpeed = TURN_SPEED;
            g_vehicleCmd   = (irLeft <= irRight) ? VEHICLE_TURN90_L
                                                  : VEHICLE_TURN90_R;
            enterState(AUTO_TURNING);
        }
        break;

    /* ── 2b. 90도 회전 ────────────────────────────────────────── */
    case AUTO_TURNING:
        /* AppVehicle TURN90 상태 머신이 처리 — 완료까지 대기 */
        if (!AppVehicle_IsTurning())
        {
            g_vehicleSpeed = FORWARD_SPEED;
            g_vehicleCmd   = VEHICLE_FORWARD;
            enterState(AUTO_BYPASS_FORWARD);
        }
        break;

    /* ── 2c. 직진 (장애물 통과) ──────────────────────────────── */
    case AUTO_BYPASS_FORWARD:
        g_vehicleSpeed = FORWARD_SPEED;
        g_vehicleCmd   = VEHICLE_FORWARD;

        /* 새 장애물이 나타나면 즉시 다시 회피 */
        if (obs == OBSTACLE_FRONT || obs == OBSTACLE_BOTH_SIDES)
        {
            g_vehicleCmd = VEHICLE_STOP;
            enterState(AUTO_BACKWARD);
            break;
        }

        if (elapsed >= BYPASS_TIME_MS)
        {
            /* Yaw 0° 복귀 명령 발행 — AppVehicle이 실행 */
            g_vehicleSpeed = TURN_SPEED;
            g_vehicleCmd   = VEHICLE_YAW_ZERO;
            enterState(AUTO_YAW_RETURN);
        }
        break;

    /* ── 2d. Yaw 0° 복귀 ─────────────────────────────────────── */
    case AUTO_YAW_RETURN:
        /* AppVehicle YAW_ZERO 상태 머신이 처리 — 완료까지 대기 */
        if (!AppVehicle_IsTurning())
        {
            g_vehicleSpeed = FORWARD_SPEED;
            enterState(AUTO_FORWARD);
        }
        break;
    }
}
