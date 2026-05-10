/**********************************************************************************************************************
 * \file Cpu0_Main.c
 * \brief AI RC Vehicle - GTM Timer 기반 협력형 스케줄러
 *
 * 초기화 순서:
 *   DrvIntc (WDT disable) → DrvDio (LED) → DrvGtmTimer (1ms tick) → DrvUart (HC-12)
 *   → 인터럽트 Enable → Scheduler loop
 *********************************************************************************************************************/
#include "Ifx_Types.h"
#include "IfxCpu.h"
#include "DrvIntc.h"
#include "DrvDio.h"
#include "DrvGtmTimer.h"
#include "DrvUart.h"
#include "Scheduler.h"

/******************************************************************************/
/*                           Main                                             */
/******************************************************************************/
void core0_main(void)
{
    /* Driver 초기화 (인터럽트 비활성 상태에서) */
    DrvIntc_Init();
    DrvDio_Init();
    DrvGtmTimer_Init();
    DrvUart_Init();

    /* 글로벌 인터럽트 Enable */
    IfxCpu_enableInterrupts();

    DrvUart_SendString("=== AI RC Vehicle Ready ===\r\n");

    /* 메인 루프: 스케줄러 실행 */
    while (1)
    {
        Scheduler_Run();
    }
}
