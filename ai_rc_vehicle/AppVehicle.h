/**********************************************************************************************************************
 * \file AppVehicle.h
 * \brief 4WD RC Vehicle 제어 모듈
 *
 * 명령: 정지, 전진, 후진, 좌회전, 우회전, 좌스핀, 우스핀
 *********************************************************************************************************************/
#ifndef APPVEHICLE_H
#define APPVEHICLE_H

#include "Ifx_Types.h"

typedef enum
{
    VEHICLE_STOP       = 0,
    VEHICLE_FORWARD    = 1,
    VEHICLE_REVERSE    = 2,
    VEHICLE_TURN_LEFT  = 3,
    VEHICLE_TURN_RIGHT = 4,
    VEHICLE_SPIN_LEFT  = 5,
    VEHICLE_SPIN_RIGHT = 6
} VehicleCommand;

extern volatile uint8   g_vehicleCmd;      /* VehicleCommand */
extern volatile float32 g_vehicleSpeed;    /* 기본 속도 [%] (0~100) */

void AppVehicle_Init(void);
void AppVehicle_Update(void);              /* 10ms 주기 호출 */

#endif /* APPVEHICLE_H */
