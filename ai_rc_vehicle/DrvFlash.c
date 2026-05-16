/**********************************************************************************************************************
 * \file DrvFlash.c
 * \brief DFLASH 캘리브레이션 데이터 저장/로드 드라이버
 *
 * 메모리 맵:
 *   DFLASH Sector 0 : 0xAF000000 ~ 0xAF001FFF (8 KB)
 *   캘리브레이션 데이터 : 0xAF000000 (첫 번째 페이지, 8 bytes)
 *
 * 1페이지(8바이트) 구조 — 레퍼런스 프로젝트와 동일한 Flash 시퀀스
 * 구조체 전 필드 uint8 — 패딩 없음 보장
 *********************************************************************************************************************/
#include "DrvFlash.h"
#include "Flash/Std/IfxFlash.h"
#include "IfxScuWdt.h"

/******************************************************************************/
/*                           Configuration                                    */
/******************************************************************************/
#define CAL_FLASH_ADDR      IFXFLASH_DFLASH_START   /* 0xAF000000 */

/******************************************************************************/
/*                           Global Variables                                 */
/******************************************************************************/
uint8  g_calDutyFL     = DRVFLASH_DUTY_DEFAULT;
uint8  g_calDutyFR     = DRVFLASH_DUTY_DEFAULT;
uint8  g_calDutyRL     = DRVFLASH_DUTY_DEFAULT;
uint8  g_calDutyRR     = DRVFLASH_DUTY_DEFAULT;
uint8  g_calTurnFront  = DRVFLASH_TURN_DEFAULT;
uint8  g_calTurnRear   = DRVFLASH_TURN_DEFAULT;
uint16 g_batMultiplier = DRVFLASH_BAT_MUL_DEFAULT;

/******************************************************************************/
/*                           Functions                                        */
/******************************************************************************/

boolean DrvFlash_LoadCalibration(void)
{
    const DrvFlash_CalData *pFlash = (const DrvFlash_CalData *)CAL_FLASH_ADDR;

    if ((pFlash->magicLo != DRVFLASH_MAGIC_LO) ||
        (pFlash->magicHi != DRVFLASH_MAGIC_HI))
    {
        return FALSE;
    }

    if (pFlash->dutyFL <= 100u) g_calDutyFL = pFlash->dutyFL;
    if (pFlash->dutyFR <= 100u) g_calDutyFR = pFlash->dutyFR;
    if (pFlash->dutyRL <= 100u) g_calDutyRL = pFlash->dutyRL;
    if (pFlash->dutyRR <= 100u) g_calDutyRR = pFlash->dutyRR;

    if (pFlash->turnFactorFront <= 100u) g_calTurnFront = pFlash->turnFactorFront;
    if (pFlash->turnFactorRear  <= 100u) g_calTurnRear  = pFlash->turnFactorRear;

    return TRUE;
}


boolean DrvFlash_SaveCalibration(void)
{
    /* 바이트 배열로 직접 구성 — 패딩/정렬 문제 원천 차단 */
    uint8 data[8];
    data[0] = g_calDutyFL;
    data[1] = g_calDutyFR;
    data[2] = g_calDutyRL;
    data[3] = g_calDutyRR;
    data[4] = g_calTurnFront;
    data[5] = g_calTurnRear;
    data[6] = DRVFLASH_MAGIC_LO;
    data[7] = DRVFLASH_MAGIC_HI;

    uint32 wordL = ((uint32)data[3] << 24) | ((uint32)data[2] << 16)
                 | ((uint32)data[1] << 8)  |  (uint32)data[0];
    uint32 wordU = ((uint32)data[7] << 24) | ((uint32)data[6] << 16)
                 | ((uint32)data[5] << 8)  |  (uint32)data[4];

    /* Endinit 해제 */
    uint16 endinitPw = IfxScuWdt_getCpuWatchdogPassword();
    IfxScuWdt_clearCpuEndinit(endinitPw);

    /* 1) Erase (최대 3회 재시도) */
    {
        uint8 retry;
        for (retry = 0u; retry < 3u; retry++)
        {
            IfxFlash_clearStatus(0);
            IfxFlash_eraseSector(CAL_FLASH_ADDR);
            IfxFlash_waitUnbusy(0, IfxFlash_FlashType_D0);

            /* 지워졌는지 검증 (첫 4바이트가 0이면 OK) */
            if (*((volatile uint32 *)CAL_FLASH_ADDR) == 0x00000000u)
            {
                break;
            }
        }
    }

    /* 2) Page Mode 진입 */
    IfxFlash_enterPageMode(CAL_FLASH_ADDR);

    /* 4) 8바이트 데이터 로드 */
    IfxFlash_loadPage2X32(CAL_FLASH_ADDR, wordL, wordU);

    /* 5) Page Write */
    IfxFlash_writePage(CAL_FLASH_ADDR);
    IfxFlash_waitUnbusy(0, IfxFlash_FlashType_D0);

    /* Endinit 복원 */
    IfxScuWdt_setCpuEndinit(endinitPw);

    /* 6) 검증 */
    const DrvFlash_CalData *pVerify = (const DrvFlash_CalData *)CAL_FLASH_ADDR;
    if ((pVerify->magicLo != DRVFLASH_MAGIC_LO) ||
        (pVerify->magicHi != DRVFLASH_MAGIC_HI))
    {
        return FALSE;
    }

    return TRUE;
}


boolean DrvFlash_EraseSector(void)
{
    uint16 endinitPw = IfxScuWdt_getCpuWatchdogPassword();
    IfxScuWdt_clearCpuEndinit(endinitPw);

    uint8 retry;
    for (retry = 0u; retry < 3u; retry++)
    {
        IfxFlash_clearStatus(0);
        IfxFlash_eraseSector(CAL_FLASH_ADDR);
        IfxFlash_waitUnbusy(0, IfxFlash_FlashType_D0);

        if (*((volatile uint32 *)CAL_FLASH_ADDR) == 0x00000000u)
        {
            break;
        }
    }

    IfxScuWdt_setCpuEndinit(endinitPw);

    return (*((volatile uint32 *)CAL_FLASH_ADDR) == 0x00000000u) ? TRUE : FALSE;
}
