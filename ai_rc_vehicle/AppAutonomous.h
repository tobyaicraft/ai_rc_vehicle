/**********************************************************************************************************************
 * \file AppAutonomous.h
 * \brief Bug Algorithm 기반 자율주행 상태 머신
 *
 * FORWARD → BACKWARD       : 전방 장애물 감지
 * BACKWARD → TURNING       : 후진 600ms 후, IR 비교해 넓은 쪽 90도 회전
 * TURNING → BYPASS_FORWARD : 90도 회전 완료 후 1.5초 직진
 * BYPASS_FORWARD → YAW_RETURN : 직진 완료 후 Yaw 0° 복귀
 * YAW_RETURN → FORWARD     : Yaw 복귀 완료 후 직진 재개
 *********************************************************************************************************************/
#ifndef APPAUTONOMOUS_H
#define APPAUTONOMOUS_H

#include "Ifx_Types.h"

void AppAutonomous_Start(void);
void AppAutonomous_Stop(void);
void AppAutonomous_Update(void);   /* AppTask_10ms() 에서 호출 */

#endif /* APPAUTONOMOUS_H */
