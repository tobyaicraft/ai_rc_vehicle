/**********************************************************************************************************************
 * \file DrvFlash.h
 * \brief DFLASH 캘리브레이션 데이터 저장/로드 드라이버
 *
 * - DFLASH Sector 0 (0xAF000000) 사용
 * - 8바이트 (1 페이지) 캘리브레이션 데이터 Read/Write
 * - 모터 Duty 보정, 회전 팩터, 매직넘버 — 전부 1페이지에 수납
 * - 구조체 패딩 방지: 전 필드 uint8 only
 *********************************************************************************************************************/
#ifndef DRVFLASH_H
#define DRVFLASH_H

#include "Ifx_Types.h"

/******************************************************************************/
/*                           캘리브레이션 데이터 구조체                       */
/******************************************************************************/
/* 전 필드 uint8 — 컴파일러 패딩 불가, 정확히 8바이트 보장 */
typedef struct
{
    uint8  dutyFL;          /* [0] FL 모터 Duty [%] (0~100)         */
    uint8  dutyFR;          /* [1] FR 모터 Duty [%] (0~100)         */
    uint8  dutyRL;          /* [2] RL 모터 Duty [%] (0~100)         */
    uint8  dutyRR;          /* [3] RR 모터 Duty [%] (0~100)         */
    uint8  turnFactorFront; /* [4] 회전 시 전륜 팩터 [%] (0~100)    */
    uint8  turnFactorRear;  /* [5] 회전 시 후륜 팩터 [%] (0~100)    */
    uint8  magicLo;         /* [6] 매직 하위 0xFE                   */
    uint8  magicHi;         /* [7] 매직 상위 0xCA                   */
} DrvFlash_CalData;         /* 총 8 bytes = 1 DFLASH page */

#define DRVFLASH_MAGIC_LO       0xFEu
#define DRVFLASH_MAGIC_HI       0xCAu

/* 기본값 */
#define DRVFLASH_DUTY_DEFAULT       70u
#define DRVFLASH_TURN_DEFAULT       100u
#define DRVFLASH_BAT_MUL_DEFAULT    2000u   /* 런타임 전용 (Flash 저장 안 함) */

/******************************************************************************/
/*                           Global Calibration Data (RAM)                    */
/******************************************************************************/
extern uint8  g_calDutyFL;
extern uint8  g_calDutyFR;
extern uint8  g_calDutyRL;
extern uint8  g_calDutyRR;
extern uint8  g_calTurnFront;
extern uint8  g_calTurnRear;
extern uint16 g_batMultiplier;

/******************************************************************************/
/*                           Public Functions                                 */
/******************************************************************************/
boolean DrvFlash_LoadCalibration(void);
boolean DrvFlash_SaveCalibration(void);
boolean DrvFlash_EraseSector(void);

#endif /* DRVFLASH_H */
