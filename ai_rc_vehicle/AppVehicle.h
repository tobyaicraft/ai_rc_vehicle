/**********************************************************************************************************************
 * \file AppVehicle.h
 * \brief 4WD RC Vehicle 제어 모듈
 *
 * 명령: 정지, 전진, 후진, 좌스핀, 우스핀, 90도 회전
 * 키 미입력 시 자동 정지 (타임아웃)
 *********************************************************************************************************************/
#ifndef APPVEHICLE_H
#define APPVEHICLE_H

#include "Ifx_Types.h"

typedef enum
{
    VEHICLE_STOP       = 0,
    VEHICLE_FORWARD    = 1,
    VEHICLE_REVERSE    = 2,
    VEHICLE_SPIN_LEFT  = 3,
    VEHICLE_SPIN_RIGHT = 4,
    VEHICLE_TURN90_L   = 5,
    VEHICLE_TURN90_R   = 6,
    VEHICLE_YAW_ZERO   = 7
} VehicleCommand;

extern volatile uint8   g_vehicleCmd;      /* VehicleCommand */
extern volatile float32 g_vehicleSpeed;    /* 기본 속도 [%] (0~100) */

void AppVehicle_Init(void);
void AppVehicle_Update(void);              /* 10ms 주기 호출 */
boolean AppVehicle_IsTurning(void);        /* 90도 회전 진행 중? */

#endif /* APPVEHICLE_H */
