/**********************************************************************************************************************
 * \file DrvGtmTimer.c
 * \brief GTM TOM0 Ch15 기반 1ms 스케줄러 tick 드라이버
 *
 * Clock chain:
 *   GTM GCLK  = 100 MHz (fSOURCE 200MHz, GTM 모듈 기본 분주)
 *   FXCLK1    = GCLK / 16 = 6,250,000 Hz
 *   period    = 6,250 ticks  →  1ms (오차 없음)
 *
 * TOM0_Ch15를 사용하여 Ch0~Ch13을 모터 PWM에 양보
 *********************************************************************************************************************/
#include "DrvGtmTimer.h"
#include "Gtm/Tom/Pwm/IfxGtm_Tom_Pwm.h"

/******************************************************************************/
/*                           Configuration                                    */
/******************************************************************************/
#define TIMER_PERIOD_TICKS  6250u    /* FXCLK1(6.25MHz) / 6250 = 1ms */

/******************************************************************************/
/*                           Module Variables                                 */
/******************************************************************************/
volatile uint32 g_1ms_counter = 0u;

static IfxGtm_Tom_Pwm_Driver s_timer;

/******************************************************************************/
/*                           ISR (1ms tick)                                   */
/******************************************************************************/
IFX_INTERRUPT(gtmTimer_ISR, 0, DRVGTMTIMER_ISR_PRIORITY)
{
    IfxGtm_Tom_Ch_clearOneNotification(&MODULE_GTM.TOM[IfxGtm_Tom_0], IfxGtm_Tom_Ch_15);
    g_1ms_counter++;
}

/******************************************************************************/
/*                           Functions                                        */
/******************************************************************************/
void DrvGtmTimer_Init(void)
{
    Ifx_GTM *gtm = &MODULE_GTM;

    /* GTM 모듈 Enable + FXCLK 활성화 */
    IfxGtm_enable(gtm);
    IfxGtm_Cmu_setGclkFrequency(gtm, IfxGtm_Cmu_getModuleFrequency(gtm));
    IfxGtm_Cmu_enableClocks(gtm, IFXGTM_CMU_CLKEN_FXCLK);

    IfxGtm_Tom_Pwm_Config cfg;
    IfxGtm_Tom_Pwm_initConfig(&cfg, gtm);

    cfg.tom                      = IfxGtm_Tom_0;
    cfg.tomChannel               = IfxGtm_Tom_Ch_15;
    cfg.clock                    = IfxGtm_Tom_Ch_ClkSrc_cmuFxclk1;   /* GCLK / 16 */
    cfg.period                   = TIMER_PERIOD_TICKS;
    cfg.dutyCycle                = TIMER_PERIOD_TICKS / 2u;
    cfg.synchronousUpdateEnabled = TRUE;
    cfg.pin.outputPin            = NULL_PTR;                          /* 핀 출력 없음 (타이머 전용) */
    cfg.interrupt.ccu0Enabled    = TRUE;
    cfg.interrupt.isrProvider    = IfxSrc_Tos_cpu0;
    cfg.interrupt.isrPriority    = DRVGTMTIMER_ISR_PRIORITY;

    IfxGtm_Tom_Pwm_init(&s_timer, &cfg);
}
