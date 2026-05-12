/**********************************************************************************************************************
 * \file Cpu0_Main.c
 * \brief AI RC Vehicle - GTM Timer 스케줄러 + 4WD 모터 제어
 *
 * 초기화 순서:
 *   DrvIntc → DrvDio → DrvGtmTimer → DrvPwm → DrvUart → AppVehicle
 *   → 인터럽트 Enable → Scheduler loop
 *********************************************************************************************************************/
#include "Ifx_Types.h"
#include "IfxCpu.h"
#include "DrvIntc.h"
#include "DrvDio.h"
#include "DrvGtmTimer.h"
#include "DrvPwm.h"
#include "DrvUart.h"
#include "DrvUart1.h"
#include "DrvAdc.h"
#include "DrvUltrasonic.h"
#include "AppVehicle.h"
#include "Scheduler.h"

/******************************************************************************/
/*                           Main                                             */
/******************************************************************************/
void core0_main(void)
{
    /* Driver 초기화 (인터럽트 비활성 상태에서) */
    DrvIntc_Init();
    DrvDio_Init();
    DrvGtmTimer_Init();     /* GTM Enable + FXCLK + TOM0_Ch15 (1ms tick) */
    DrvPwm_Init();          /* TOM0_Ch0/1/6/13 (모터 PWM 100Hz) */
    DrvUart_Init();         /* ASCLIN0 9600 (HC-12) */
    DrvUart1_Init();        /* ASCLIN1 9600 (HC-10 BLE) */
    DrvAdc_Init();          /* VADC IR sensors (AN1, AN12) */
    DrvUltrasonic_Init();   /* HC-SR04 (Trig=P02.6 GPIO, Echo=P02.7 TIM0_7) */

    AppVehicle_Init();

    /* 글로벌 인터럽트 Enable */
    IfxCpu_enableInterrupts();

    DrvUart_SendString("=== AI RC Vehicle Ready ===\r\n");
    DrvUart1_SendString("=== AI RC Vehicle Ready (BLE) ===\r\n");

    /* 메인 루프: 스케줄러 실행 */
    while (1)
    {
        Scheduler_Run();
    }
}
